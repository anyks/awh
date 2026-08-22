/**
 * @file value.cpp
 * @date 2026-08-20
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
 * @brief Исходный файл владеющего значения INI
 *
 * \~english
 * @brief Source file of the owning value of INI
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем стандартные заголовочные файлы
 */
#include <atomic>
#include <limits>
#include <fstream>

/**
 * Подключаем заголовочные файлы модуля
 */
#include <codec/ini/value.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён контейнера INI
 */
using namespace awh::codec::ini;

/**
 * @brief Внутренние помощники владеющего значения
 *
 */
namespace {
	/**
	 * @brief Предел роста вместилища обращением по номеру
	 *
	 * @details Номер, пришедший извне, обращается требованием памяти по нему, и предел
	 * этот рост стережёт. Нуль снимает его вовсе
	 *
	 */
	static ::std::atomic <size_t> LIMIT(0x10000);
	/**
	 * @brief Предел глубины заведения по пути
	 *
	 * @details Заведение по пути идёт последовательно и стека не берёт, зато работы по
	 * заведённому дереву - размножение его и снятие - идут возвратно, и дерево небывалой
	 * глубины срывает на них стек
	 *
	 * @note Берётся предел самого наречия, а не свой: глубину вложенности подразделов
	 *       оно уже объявляет, и заводить рядом второй предел значило бы развести их при
	 *       первой же правке одного из двух
	 *
	 * @note Предел этот щедрее того, что записать возможно: наречие глубже подраздела не
	 *       пишет вовсе, а владеющее значение есть дерево общего вида, и запрещать ему
	 *       глубину, какую потребитель держит у себя в памяти, незачем
	 *
	 */
	constexpr size_t DEPTH = static_cast <size_t> (awh::codec::ini::MAX_DEPTH);
	/**
	 * @brief Функция проверки записи на числовую
	 *
	 * @param text  проверяемая запись
	 * @param index разбираемый порядковый номер
	 * @return      признак числовой записи
	 *
	 */
	static bool numbering(const string & text, size_t & index) noexcept {
		/**
		 * Если запись пуста вовсе
		 */
		if(text.empty())
			// Выводим признак того, что запись номером не является
			return false;
		// Выполняем сброс разбираемого номера значения
		index = 0;
		/**
		 * Выполняем перебор всех знаков записи
		 */
		for(const char letter : text){
			/**
			 * Если знак цифрой не является
			 */
			if((letter < '0') || (letter > '9'))
				// Выводим признак того, что запись номером не является
				return false;
			/**
			 * Если добавление разряда выведет номер за предел разрядности
			 */
			if(index > ((numeric_limits <size_t>::max() - static_cast <size_t> (letter - '0')) / 10))
				// Выводим признак того, что запись номером не является
				return false;
			// Добавляем разряд к номеру значения вместилища
			index = ((index * 10) + static_cast <size_t> (letter - '0'));
		}
		// Выводим признак того, что запись является номером значения
		return true;
	}
	/**
	 * @brief Функция получения ссылки на неопределённое значение
	 *
	 * @return ссылка на неопределённое значение
	 *
	 */
	static const awh::codec::ini::Value & missing() noexcept {
		// Неопределённое значение, обращением неудачным выдаваемое
		static const awh::codec::ini::Value result;
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
 * @brief Метод извлечения значения мусорного
 *
 * @return значение мусорное
 *
 */
awh::codec::ini::Value & awh::codec::ini::Value::scrap() noexcept {
	// Значение мусорное, обращением изменяемым неудачным выдаваемое
	static thread_local Value result;
	// Выполняем сброс мусорного значения от прежней записи в него
	result.clear();
	// Выводим ссылку на значение мусорное
	return result;
}
/**
 * @brief Метод извлечения предела роста вместилища по номеру
 *
 * @return предел роста вместилища по номеру
 *
 */
size_t awh::codec::ini::Value::limit() noexcept {
	// Выводим предел роста вместилища по номеру
	return ::LIMIT.load(::std::memory_order_relaxed);
}
/**
 * @brief Метод установки предела роста вместилища по номеру
 *
 * @param value устанавливаемый предел роста
 *
 */
void awh::codec::ini::Value::limit(const size_t value) noexcept {
	// Выполняем установку предела роста вместилища по номеру
	::LIMIT.store(value, ::std::memory_order_relaxed);
}
/**
 * @brief Метод проверки определённости значения
 *
 * @return признак определённости значения
 *
 */
bool awh::codec::ini::Value::valid() const noexcept {
	// Выводим признак определённости значения
	return (this->_type != type_t::NONE);
}
/**
 * @brief Метод извлечения типа значения
 *
 * @return тип хранимого значения
 *
 */
awh::codec::ini::type_t awh::codec::ini::Value::type() const noexcept {
	// Выводим тип хранимого значения
	return this->_type;
}
/**
 * @brief Метод проверки соответствия значения затребованному типу
 *
 * @param type сличаемый тип значения
 * @return     признак соответствия значения затребованному типу
 *
 */
bool awh::codec::ini::Value::is(const type_t type) const noexcept {
	// Выводим признак соответствия значения затребованному типу
	return (this->_type == type);
}
/**
 * @brief Метод извлечения количества значений вместилища
 *
 * @return количество значений вместилища
 *
 */
size_t awh::codec::ini::Value::size() const noexcept {
	// Выводим количество значений вместилища
	return this->_items.size();
}
/**
 * @brief Метод проверки пустоты вместилища
 *
 * @return признак пустоты вместилища
 *
 */
bool awh::codec::ini::Value::empty() const noexcept {
	// Выводим признак пустоты вместилища
	return this->_items.empty();
}
/**
 * @brief Метод сброса значения в исходное состояние
 *
 */
void awh::codec::ini::Value::clear() noexcept {
	// Выполняем снос указателя поиска
	this->unindex();
	// Выполняем сброс типа хранимого значения
	this->_type = type_t::NONE;
	// Выполняем сброс признака значения, записанного в кавычках
	this->_quoted = false;
	// Выполняем очистку содержимого простого значения
	this->_text.clear();
	// Выполняем очистку имён пар вместилища
	this->_names.clear();
	// Выполняем очистку значений вместилища
	this->_items.clear();
}
/**
 * @brief Метод извлечения содержимого простого значения
 *
 * @return содержимое простого значения
 *
 */
const string & awh::codec::ini::Value::text() const noexcept {
	// Выводим содержимое простого значения
	return this->_text;
}
/**
 * @brief Метод извлечения имени пары вместилища по номеру
 *
 * @param index порядковый номер пары вместилища
 * @return      имя пары вместилища
 *
 */
const string & awh::codec::ini::Value::key(const size_t index) const noexcept {
	/**
	 * Если номер за пределы перечня имён выходит
	 */
	if(index >= this->_names.size())
		// Выводим ссылку на пустую запись
		return ::nothing();
	// Выводим имя пары вместилища
	return this->_names.at(index);
}
/**
 * @brief Метод извлечения признака значения, записанного в кавычках
 *
 * @return признак значения, записанного в кавычках
 *
 */
bool awh::codec::ini::Value::quoted() const noexcept {
	// Выводим признак значения, записанного в кавычках
	return this->_quoted;
}
/**
 * @brief Метод установки признака значения, записанного в кавычках
 *
 * @param quoted устанавливаемый признак
 *
 */
void awh::codec::ini::Value::quoted(const bool quoted) noexcept {
	// Выполняем установку признака значения, записанного в кавычках
	this->_quoted = quoted;
}
/**
 * @brief Метод проверки наличия пары вместилища по имени
 *
 * @param name имя искомой пары вместилища
 * @return     признак наличия пары вместилища
 *
 */
bool awh::codec::ini::Value::contains(const string & name) const noexcept {
	/**
	 * Если значение вместилищем пар не является
	 */
	if(this->_type != type_t::TABLE)
		// Выводим отсутствие пары вместилища
		return false;
	/**
	 * Выполняем перебор имён пар вместилища
	 */
	// Выводим признак того, что пары вместилища разыскана
	return (this->locate(name) < this->_names.size());
}
/**
 * @brief Метод разыскания номера пары по имени её
 *
 * @param name имя разыскиваемой пары
 * @return     номер пары, размер вместилища при отсутствии
 *
 */
size_t awh::codec::ini::Value::search(const string & name) const noexcept {
	// Выводим номер разысканной пары
	return this->locate(name);
}
/**
 * @brief Метод разыскания пары по имени
 *
 * @param name имя разыскиваемой пары
 * @return     номер пары, размер вместилища при отсутствии
 *
 */
size_t awh::codec::ini::Value::locate(const string & name) const noexcept {
	/**
	 * Если пар меньше порога заведения указателя
	 *
	 * @note Перебор при малом числе пар дешевле всякого указателя: сличение имён идёт по
	 *       памяти подряд, тогда как заведение указателя стоит выделения памяти и счёта
	 *       отпечатка на всякое имя
	 */
	if(this->_names.size() < static_cast <size_t> (INDEX_THRESHOLD)){
		/**
		 * Выполняем перебор имён пар вместилища
		 */
		for(size_t i = 0; i < this->_names.size(); i++){
			// Если имя пары совпадает с разыскиваемым
			if(this->_names.at(i).compare(name) == 0)
				// Выводим номер разысканной пары
				return i;
		}
		// Выводим признак отсутствия пары
		return this->_names.size();
	}
	/**
	 * Если указатель поиска ещё не заведён
	 */
	if(!this->_index){
		// Выполняем заведение указателя поиска
		this->_index.reset(new unordered_map <string, size_t>());
		// Резервируем место под имена пар вместилища
		this->_index->reserve(this->_names.size());
		/**
		 * Выполняем перебор имён пар вместилища
		 */
		for(size_t i = 0; i < this->_names.size(); i++)
			// Выполняем добавление имени пары в указатель поиска
			this->_index->emplace(this->_names.at(i), i);
	}
	// Выполняем поиск затребованного имени пары
	auto i = this->_index->find(name);
	// Выводим номер разысканной пары
	return ((i != this->_index->end()) ? i->second : this->_names.size());
}
/**
 * @brief Метод сноса указателя поиска
 *
 */
void awh::codec::ini::Value::unindex() noexcept {
	// Выполняем снос заведённого указателя поиска
	this->_index.reset(nullptr);
}
/**
 * @brief Оператор обращения к паре вместилища по имени
 *
 * @param name имя искомой пары вместилища
 * @return     найденное значение
 *
 */
const awh::codec::ini::Value & awh::codec::ini::Value::operator [] (const string & name) const noexcept {
	/**
	 * Если значение вместилищем пар не является
	 */
	if(this->_type != type_t::TABLE)
		// Выводим неопределённое значение
		return ::missing();
	/**
	 * Выполняем перебор имён пар вместилища
	 */
	// Выполняем разыскание затребованной пары вместилища
	const size_t found = this->locate(name);
	// Если пары вместилища разыскана
	if(found < this->_items.size())
		// Выводим значение разысканной пары вместилища
		return this->_items.at(found);
	// Выводим неопределённое значение
	return ::missing();
}
/**
 * @brief Оператор обращения к паре вместилища по имени с заведением
 *
 * @param name имя искомой пары вместилища
 * @return     найденное либо заведённое значение
 *
 */
awh::codec::ini::Value & awh::codec::ini::Value::operator [] (const string & name) noexcept {
	/**
	 * Если значение вместилищем пар не является
	 *
	 * @note Значение перерождается вместилищем, а простое содержимое его теряется:
	 *       обращение изменяемое есть заявление о том, что здесь стоит вместилище
	 */
	if(this->_type != type_t::TABLE){
		// Выполняем очистку прежнего значения
		this->clear();
		// Назначаем значению тип вместилища пар
		this->_type = type_t::TABLE;
	}
	/**
	 * Выполняем перебор имён пар вместилища
	 */
	// Выполняем разыскание затребованной пары вместилища
	const size_t found = this->locate(name);
	// Если пары вместилища разыскана
	if(found < this->_items.size())
		// Выводим значение разысканной пары вместилища
		return this->_items.at(found);
	/**
	 * Если указатель поиска заведён, ведём его приращением
	 *
	 * @note Приращение обязательно: перестроение указателя на всякой правке вернуло бы ту
	 *       самую квадратичность, от какой указатель и заводится
	 */
	if(this->_index)
		// Выполняем добавление заводимого имени в указатель поиска
		this->_index->emplace(name, this->_names.size());
	// Выполняем добавление имени заводимой пары вместилища
	this->_names.push_back(name);
	// Выполняем добавление значения заводимой пары вместилища
	this->_items.emplace_back();
	// Выводим значение заведённой пары вместилища
	return this->_items.back();
}
/**
 * @brief Оператор обращения к значению перечня по номеру
 *
 * @param index порядковый номер значения перечня
 * @return      найденное значение
 *
 */
const awh::codec::ini::Value & awh::codec::ini::Value::operator [] (const size_t index) const noexcept {
	/**
	 * Если номер за пределы вместилища выходит
	 */
	if(index >= this->_items.size())
		// Выводим неопределённое значение
		return ::missing();
	// Выводим значение вместилища по номеру
	return this->_items.at(index);
}
/**
 * @brief Оператор обращения к значению перечня по номеру с заведением
 *
 * @param index порядковый номер значения перечня
 * @return      найденное либо заведённое значение
 *
 */
awh::codec::ini::Value & awh::codec::ini::Value::operator [] (const size_t index) noexcept {
	/**
	 * Если значение вместилищем не является вовсе
	 *
	 * @note Обращение по номеру заводит перечень, а не вместилище пар: имени у номера
	 *       нет, и положить значение в пару было бы некуда
	 */
	if((this->_type != type_t::ARRAY) && (this->_type != type_t::TABLE)){
		// Выполняем очистку прежнего значения
		this->clear();
		// Назначаем значению тип перечня значений
		this->_type = type_t::ARRAY;
	}
	/**
	 * Если номер за пределы вместилища выходит
	 */
	if(index >= this->_items.size()){
		// Получаем предел роста вместилища по номеру
		const size_t limit = ::LIMIT.load(::std::memory_order_relaxed);
		/**
		 * Если рост вместилища предел превышает
		 */
		if((limit > 0) && ((index + 1) > limit))
			// Выводим значение мусорное
			return Value::scrap();
		/**
		 * Если значение является вместилищем пар
		 */
		if(this->_type == type_t::TABLE)
			// Выполняем рост перечня имён пар вместилища
			this->_names.resize(index + 1);
		// Выполняем рост вместилища до затребованного номера
		this->_items.resize(index + 1);
	}
	// Выводим значение вместилища по номеру
	return this->_items.at(index);
}
/**
 * @brief Метод обращения к вложенному значению по пути
 *
 * @param path путь до искомого значения
 * @return     найденное значение
 *
 */
const awh::codec::ini::Value & awh::codec::ini::Value::at(const string & path) const noexcept {
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
		// Номер значения вместилища, частью пути заданный
		size_t index = 0;
		/**
		 * Если вместилище является перечнем значений, а часть пути номером
		 */
		if((result->_type == type_t::ARRAY) && ::numbering(part, index))
			// Выполняем переход к значению перечня по номеру его
			result = &((* result)[index]);
		// Выполняем переход к паре вместилища по имени её
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
 * @brief Метод заведения вложенного значения по пути
 *
 * @param path путь до заводимого значения
 * @return     заведённое значение
 *
 */
awh::codec::ini::Value & awh::codec::ini::Value::place(const string & path) noexcept {
	// Получаем путь к значению без ведущего разделителя частей
	const string route((!path.empty() && (path.front() == '/')) ? path.substr(1) : path);
	/**
	 * Если путь к значению пуст вовсе
	 */
	if(route.empty())
		// Выводим текущее значение
		return (* this);
	/**
	 * Если глубина пути предел вложенности превышает
	 */
	{
		// Количество звеньев пути к значению
		size_t links = 1;
		/**
		 * Выполняем перебор разделителей звеньев пути
		 */
		for(size_t position = route.find('/'); position != string::npos; position = route.find('/', (position + 1)))
			// Увеличиваем количество звеньев пути
			links++;
		/**
		 * Если звеньев пути больше предела допустимого
		 */
		if(links > ::DEPTH)
			// Выводим значение мусорное
			return Value::scrap();
	}
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
		// Номер значения вместилища, частью пути заданный
		size_t index = 0;
		// Получаем признак того, что часть пути является номером
		const bool numeric = ::numbering(part, index);
		/**
		 * Если значение вместилищем не является вовсе
		 */
		if((result->_type != type_t::ARRAY) && (result->_type != type_t::TABLE)){
			// Выполняем очистку прежнего значения
			result->clear();
			// Назначаем значению тип заводимого вместилища
			result->_type = (numeric ? type_t::ARRAY : type_t::TABLE);
		}
		/**
		 * Если вместилище является перечнем значений, а часть пути числом
		 */
		if((result->_type == type_t::ARRAY) && numeric)
			// Выполняем переход к значению перечня по номеру его
			result = &((* result)[index]);
		// Выполняем переход к паре вместилища по имени её
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
 * @brief Метод добавления значения в конец перечня
 *
 * @param value добавляемое значение
 * @return      признак успешности добавления
 *
 */
bool awh::codec::ini::Value::push(const Value & value) noexcept {
	/**
	 * Если значение вместилищем не является вовсе
	 */
	if((this->_type != type_t::ARRAY) && (this->_type != type_t::TABLE)){
		// Выполняем очистку прежнего значения
		this->clear();
		// Назначаем значению тип перечня значений
		this->_type = type_t::ARRAY;
	}
	/**
	 * Если значение является вместилищем пар
	 *
	 * @note Отказ здесь намеренный: добавление без имени во вместилище пар положить
	 *       некуда, а подставлять имя пустое значило бы завести свойство, записи не
	 *       подлежащее
	 */
	if(this->_type == type_t::TABLE)
		// Выводим признак неудачного добавления
		return false;
	// Выполняем добавление значения в конец перечня
	this->_items.push_back(value);
	// Выводим признак успешного добавления
	return true;
}
/**
 * @brief Метод установки пары вместилища
 *
 * @param name  имя устанавливаемой пары
 * @param value устанавливаемое значение
 * @return      признак успешности установки
 *
 */
bool awh::codec::ini::Value::insert(const string & name, const Value & value) noexcept {
	/**
	 * Если имя пары вместилища пусто вовсе
	 */
	if(name.empty())
		// Выводим признак неудачной установки
		return false;
	/**
	 * Если значение вместилищем пар не является
	 */
	if(this->_type != type_t::TABLE){
		// Выполняем очистку прежнего значения
		this->clear();
		// Назначаем значению тип вместилища пар
		this->_type = type_t::TABLE;
	}
	/**
	 * Выполняем перебор имён пар вместилища
	 */
	// Выполняем разыскание устанавливаемой пары вместилища
	const size_t found = this->locate(name);
	/**
	 * Если пары вместилища разыскана
	 *
	 * @note Перезапись ведётся на прежнем месте: порядок пар задан потребителем, и
	 *       перестановка их меняла бы вид записанного текста без его на то воли
	 */
	if(found < this->_items.size()){
		// Выполняем перезапись значения пары вместилища
		this->_items.at(found) = value;
		// Выводим признак успешной установки
		return true;
	}
	// Если указатель поиска заведён, ведём его приращением
	if(this->_index)
		// Выполняем добавление заводимого имени в указатель поиска
		this->_index->emplace(name, this->_names.size());
	// Выполняем добавление имени заводимой пары вместилища
	this->_names.push_back(name);
	// Выполняем добавление значения заводимой пары вместилища
	this->_items.push_back(value);
	// Выводим признак успешной установки
	return true;
}
/**
 * @brief Метод добавления пары вместилища без перезаписи
 *
 * @param name  имя добавляемой пары
 * @param value добавляемое значение
 * @return      признак успешности добавления
 *
 */
bool awh::codec::ini::Value::append(const string & name, const Value & value) noexcept {
	/**
	 * Если имя пары вместилища пусто вовсе либо занято уже
	 */
	if(name.empty() || this->contains(name))
		// Выводим признак неудачного добавления
		return false;
	// Выполняем установку пары вместилища
	return this->insert(name, value);
}
/**
 * @brief Метод удаления пары вместилища по имени
 *
 * @param name имя удаляемой пары
 * @return     признак успешности удаления
 *
 */
bool awh::codec::ini::Value::erase(const string & name) noexcept {
	/**
	 * Если значение вместилищем пар не является
	 */
	if(this->_type != type_t::TABLE)
		// Выводим признак неудачного удаления
		return false;
	/**
	 * Выполняем перебор имён пар вместилища
	 */
	for(size_t i = 0; i < this->_names.size(); i++){
		/**
		 * Если имя пары вместилища совпадает с удаляемым
		 */
		if(this->_names.at(i).compare(name) == 0){
			// Выполняем удаление имени пары вместилища
			this->_names.erase(this->_names.begin() + static_cast <ptrdiff_t> (i));
			// Выполняем удаление значения пары вместилища
			this->_items.erase(this->_items.begin() + static_cast <ptrdiff_t> (i));
			// Выполняем снос указателя поиска
			this->unindex();
			// Выводим признак успешного удаления
			return true;
		}
	}
	// Выводим признак неудачного удаления
	return false;
}
/**
 * @brief Метод удаления значения вместилища по номеру
 *
 * @param index порядковый номер удаляемого значения
 * @return      признак успешности удаления
 *
 */
bool awh::codec::ini::Value::erase(const size_t index) noexcept {
	/**
	 * Если номер за пределы вместилища выходит
	 */
	if(index >= this->_items.size())
		// Выводим признак неудачного удаления
		return false;
	/**
	 * Если значение является вместилищем пар
	 */
	if((this->_type == type_t::TABLE) && (index < this->_names.size()))
		// Выполняем удаление имени пары вместилища
		this->_names.erase(this->_names.begin() + static_cast <ptrdiff_t> (index));
	// Выполняем удаление значения вместилища
	this->_items.erase(this->_items.begin() + static_cast <ptrdiff_t> (index));
	/**
	 * Выполняем снос указателя поиска
	 *
	 * @note Сносится он целиком, а не чинится: удаление сдвигает номера всех пар после
	 *       удалённой, и починка обошлась бы дороже заведения заново
	 */
	this->unindex();
	// Выводим признак успешного удаления
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
bool awh::codec::ini::Value::extract(T & result) const noexcept {
	/**
	 * Если значение простым не является
	 */
	if(this->_type != type_t::STRING)
		// Выводим признак неудачного извлечения
		return false;
	/**
	 * Выводим признак успешности разбора числа из записи значения
	 *
	 * @note Разбор ведётся тем же способом, каким ведёт его дерево настроек: договор
	 *       извлечения общий у них, и расхождение всплыло бы у потребителя, читающего
	 *       то через дерево, то через владеющее значение
	 */
	return numeric(this->_text, result);
}
/**
 * @brief Метод извлечения логического значения
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::ini::Value::value(bool & result) const noexcept {
	/**
	 * Если значение простым не является
	 */
	if(this->_type != type_t::STRING)
		// Выводим признак неудачного извлечения
		return false;
	// Выводим признак успешности разбора логического значения из записи
	return numeric(this->_text, result);
}
/**
 * @brief Метод извлечения содержимого простого значения
 *
 * @param result переменная, куда помещается извлечённое содержимое
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::ini::Value::value(string & result) const noexcept {
	/**
	 * Если значение простым не является
	 */
	if(this->_type != type_t::STRING)
		// Выводим признак неудачного извлечения
		return false;
	// Устанавливаем извлечённое содержимое
	result = this->_text;
	// Выводим признак успешного извлечения
	return true;
}
/**
 * @brief Метод извлечения числа видом `int8_t`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::ini::Value::value(int8_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract <int8_t> (result);
}
/**
 * @copydoc awh::codec::ini::Value::value(int8_t &) const
 */
bool awh::codec::ini::Value::value(int16_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract <int16_t> (result);
}
/**
 * @copydoc awh::codec::ini::Value::value(int8_t &) const
 */
bool awh::codec::ini::Value::value(int32_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract <int32_t> (result);
}
/**
 * @copydoc awh::codec::ini::Value::value(int8_t &) const
 */
bool awh::codec::ini::Value::value(int64_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract <int64_t> (result);
}
/**
 * @copydoc awh::codec::ini::Value::value(int8_t &) const
 */
bool awh::codec::ini::Value::value(uint8_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract <uint8_t> (result);
}
/**
 * @copydoc awh::codec::ini::Value::value(int8_t &) const
 */
bool awh::codec::ini::Value::value(uint16_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract <uint16_t> (result);
}
/**
 * @copydoc awh::codec::ini::Value::value(int8_t &) const
 */
bool awh::codec::ini::Value::value(uint32_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract <uint32_t> (result);
}
/**
 * @copydoc awh::codec::ini::Value::value(int8_t &) const
 */
bool awh::codec::ini::Value::value(uint64_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract <uint64_t> (result);
}
/**
 * @copydoc awh::codec::ini::Value::value(int8_t &) const
 */
bool awh::codec::ini::Value::value(float & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract <float> (result);
}
/**
 * @copydoc awh::codec::ini::Value::value(int8_t &) const
 */
bool awh::codec::ini::Value::value(double & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract <double> (result);
}
/**
 * @brief Оператор сличения значений
 *
 * @param value сличаемое значение
 * @return      признак совпадения значений
 *
 */
bool awh::codec::ini::Value::operator == (const Value & value) const noexcept {
	/**
	 * Если типы значений разнятся
	 */
	if(this->_type != value._type)
		// Выводим признак расхождения значений
		return false;
	/**
	 * Определяем тип сличаемых значений
	 */
	switch(static_cast <uint8_t> (this->_type)){
		// Если значения не определены вовсе
		case static_cast <uint8_t> (type_t::NONE):
			// Выводим признак совпадения значений
			return true;
		// Если значения являются простыми
		case static_cast <uint8_t> (type_t::STRING):
			/**
			 * Выводим признак совпадения содержимого
			 *
			 * @note Признак записи в кавычках сличению не подлежит: кавычки суть
			 *       оформление записи, а не содержимое её
			 */
			return (this->_text.compare(value._text) == 0);
		// Если значения являются перечнями
		case static_cast <uint8_t> (type_t::ARRAY): {
			/**
			 * Если количества значений перечней разнятся
			 */
			if(this->_items.size() != value._items.size())
				// Выводим признак расхождения значений
				return false;
			/**
			 * Выполняем перебор значений перечня
			 *
			 * @note Порядок значений перечня сличению подлежит: перечень одноимённых
			 *       значений есть последовательность, и порядок объявления её значим
			 */
			for(size_t i = 0; i < this->_items.size(); i++){
				/**
				 * Если очередные значения перечней разнятся
				 */
				if(!(this->_items.at(i) == value._items.at(i)))
					// Выводим признак расхождения значений
					return false;
			}
			// Выводим признак совпадения значений
			return true;
		}
	}
	/**
	 * Если количества пар вместилищ разнятся
	 */
	if(this->_names.size() != value._names.size())
		// Выводим признак расхождения значений
		return false;
	/**
	 * Выполняем перебор пар вместилища
	 *
	 * @note Порядок пар вместилища сличению НЕ подлежит: вместилище есть отображение
	 *       имён на значения, и порядок записи его значением не является
	 */
	for(size_t i = 0; i < this->_names.size(); i++){
		// Получаем значение одноимённой пары сличаемого вместилища
		const Value & item = value[this->_names.at(i)];
		/**
		 * Если одноимённой пары в сличаемом вместилище нет вовсе
		 */
		if(!item.valid() && this->_items.at(i).valid())
			// Выводим признак расхождения значений
			return false;
		/**
		 * Если значения одноимённых пар разнятся
		 */
		if(!(this->_items.at(i) == item))
			// Выводим признак расхождения значений
			return false;
	}
	// Выводим признак совпадения значений
	return true;
}
/**
 * @brief Оператор сличения значений на расхождение
 *
 * @param value сличаемое значение
 * @return      признак расхождения значений
 *
 */
bool awh::codec::ini::Value::operator != (const Value & value) const noexcept {
	// Выводим признак расхождения значений
	return !((* this) == value);
}
/**
 * @brief Оператор присваивания значения
 *
 * @param value присваиваемое значение
 * @return      ссылка на текущее значение
 *
 */
awh::codec::ini::Value & awh::codec::ini::Value::operator = (const Value & value) noexcept {
	/**
	 * Если присваивается значение самому себе
	 */
	if(this == &value)
		// Выводим ссылку на текущее значение
		return (* this);
	// Выполняем копирование типа хранимого значения
	this->_type = value._type;
	// Выполняем копирование признака значения, записанного в кавычках
	this->_quoted = value._quoted;
	// Выполняем копирование содержимого простого значения
	this->_text = value._text;
	// Выполняем копирование имён пар вместилища
	this->_names = value._names;
	// Выполняем снос указателя поиска: заведётся он заново при первом же поиске
	this->unindex();
	// Выполняем копирование значений вместилища
	this->_items = value._items;
	// Выводим ссылку на текущее значение
	return (* this);
}
/**
 * @brief Оператор присваивания значения переносом
 *
 * @param value переносимое значение
 * @return      ссылка на текущее значение
 *
 */
awh::codec::ini::Value & awh::codec::ini::Value::operator = (Value && value) noexcept {
	/**
	 * Если присваивается значение самому себе
	 */
	if(this == &value)
		// Выводим ссылку на текущее значение
		return (* this);
	// Выполняем перенос типа хранимого значения
	this->_type = value._type;
	// Выполняем перенос признака значения, записанного в кавычках
	this->_quoted = value._quoted;
	// Выполняем перенос содержимого простого значения
	this->_text = ::std::move(value._text);
	// Выполняем перенос имён пар вместилища
	this->_names = ::std::move(value._names);
	// Выполняем перенесение указателя поиска вместе с именами
	this->_index = ::std::move(value._index);
	// Выполняем перенос значений вместилища
	this->_items = ::std::move(value._items);
	/**
	 * Выполняем сброс перенесённого значения
	 *
	 * @note Сброс обязателен: перенесённое значение остаётся годным к употреблению, и
	 *       тип его, при переносе не тронутый, изображал бы содержимое, уже ушедшее
	 */
	value._type = type_t::NONE;
	// Выводим ссылку на текущее значение
	return (* this);
}
/**
 * @brief Конструктор
 *
 */
awh::codec::ini::Value::Value() noexcept : _type(type_t::NONE), _quoted(false) {}
/**
 * @brief Конструктор вместилища затребованного типа
 *
 * @param type тип заводимого значения
 *
 */
awh::codec::ini::Value::Value(const type_t type) noexcept : _type(type), _quoted(false) {}
/**
 * @brief Конструктор простого значения
 *
 * @param value  устанавливаемое содержимое
 * @param quoted признак значения, записанного в кавычках
 *
 */
awh::codec::ini::Value::Value(const string & value, const bool quoted) noexcept :
 _type(type_t::STRING), _quoted(quoted), _text(value) {}
/**
 * @brief Конструктор простого значения из строки языка
 *
 * @param value  устанавливаемое содержимое
 * @param quoted признак значения, записанного в кавычках
 *
 */
awh::codec::ini::Value::Value(const char * value, const bool quoted) noexcept :
 _type(type_t::STRING), _quoted(quoted), _text((value != nullptr) ? value : "") {}
/**
 * @brief Конструктор копирования
 *
 * @param value копируемое значение
 *
 */
awh::codec::ini::Value::Value(const Value & value) noexcept :
 _type(value._type), _quoted(value._quoted), _text(value._text),
 _names(value._names), _items(value._items) {}
/**
 * @brief Конструктор переноса
 *
 * @param value переносимое значение
 *
 */
awh::codec::ini::Value::Value(Value && value) noexcept :
 _type(value._type), _quoted(value._quoted), _text(::std::move(value._text)),
 _names(::std::move(value._names)), _items(::std::move(value._items)),
 _index(::std::move(value._index)) {
	// Выполняем сброс перенесённого значения
	value._type = type_t::NONE;
}
/**
 * @brief Метод снятия значения с дерева настроек
 *
 * @param document дерево настроек, откуда снимается значение
 *
 */
void awh::codec::ini::Value::absorb(const Document & document) noexcept {
	// Выполняем очистку значения от прежнего содержимого
	this->clear();
	// Назначаем корню тип вместилища пар
	this->_type = type_t::TABLE;
	/**
	 * @brief Функция снятия свойств раздела во вместилище пар
	 *
	 * @param holder     вместилище, куда снимаются свойства
	 * @param section    имя раздела
	 * @param subsection имя подраздела
	 *
	 */
	const auto gather = [&document](Value & holder, const string & section, const string & subsection) noexcept -> void {
		/**
		 * Имена свойств раздела, снятые своими копиями
		 *
		 * @note Копии обязательны: дерево выдаёт имена видами в своё хранилище знаков, а
		 *       всякое обращение к дереву вправе пересчитать подстановку обращений и
		 *       хранилище то переместить. Вид, взятый до пересчёта, повисал бы, и разбор
		 *       брал бы содержимое из освобождённой памяти. Нашёл это ворошитель под
		 *       санитайзером, круговым переносом значения
		 */
		vector <string> names;
		/**
		 * Выполняем перебор всех имён свойств раздела
		 */
		for(auto & name : document.keys(section, subsection))
			// Выполняем снятие копии очередного имени свойства
			names.emplace_back(name);
		/**
		 * Выполняем перебор всех снятых имён свойств раздела
		 */
		for(auto & key : names){
			// Получаем перечень значений одноимённого свойства
			const vector <string_view> values = document.values(key, section, subsection);
			/**
			 * Если значений одноимённого свойства нет вовсе
			 */
			if(values.empty())
				// Выполняем переход к следующему свойству раздела
				continue;
			/**
			 * Если значение одноимённого свойства единственно
			 */
			if(values.size() == 1){
				// Выполняем установку простого значения свойства
				holder.insert(string(key), Value(string(values.front())));
				// Выполняем переход к следующему свойству раздела
				continue;
			}
			/**
			 * Собираем перечень значений одноимённого свойства
			 *
			 * @note Перечень заводится лишь при повторе имени: свойство, объявленное
			 *       однажды, есть значение простое, и обёртка перечнем заставляла бы
			 *       потребителя различать эти два случая при всяком обращении
			 */
			Value listed(type_t::ARRAY);
			/**
			 * Выполняем перебор всех значений одноимённого свойства
			 */
			for(auto & value : values)
				// Выполняем добавление очередного значения в перечень
				listed.push(Value(string(value)));
			// Выполняем установку перечня значений свойства
			holder.insert(string(key), listed);
		}
	};
	// Выполняем снятие свойств верхнего уровня текста настроек
	gather(* this, string(), string());
	/**
	 * Имена объявленных разделов, снятые своими копиями
	 *
	 * @note Копии обязательны по той же причине, что и у имён свойств: дерево выдаёт
	 *       имена видами в своё хранилище знаков, а снятие свойств вправе его переместить
	 */
	vector <pair <string, string>> sections;
	/**
	 * Выполняем перебор всех объявленных разделов текста настроек
	 */
	for(auto & item : document.sections())
		// Выполняем снятие копии имени очередного раздела
		sections.emplace_back(string(item.section), string(item.subsection));
	/**
	 * Выполняем перебор всех снятых имён разделов текста настроек
	 */
	for(auto & name : sections){
		/**
		 * Если имя раздела пусто вовсе
		 *
		 * @note Раздел с пустым именем есть верхний уровень, и свойства его сняты уже
		 */
		if(name.first.empty())
			// Выполняем переход к следующему разделу
			continue;
		// Получаем вместилище раздела, заводя его при надобности
		Value & section = (* this)[name.first];
		/**
		 * Если имя подраздела пусто вовсе
		 */
		if(name.second.empty()){
			// Выполняем снятие свойств раздела
			gather(section, name.first, string());
			// Выполняем переход к следующему разделу
			continue;
		}
		// Получаем вместилище подраздела, заводя его при надобности
		Value & subsection = section[name.second];
		// Выполняем снятие свойств подраздела
		gather(subsection, name.first, name.second);
	}
}
/**
 * @brief Конструктор снятия значения с дерева настроек
 *
 * @param document дерево настроек, откуда снимается значение
 *
 */
awh::codec::ini::Value::Value(const Document & document) noexcept : _type(type_t::NONE), _quoted(false) {
	// Выполняем снятие значения с дерева настроек
	this->absorb(document);
}
/**
 * @brief Метод разбора текста настроек во владеющее значение
 *
 * @param text разбираемый текст настроек
 * @return     признак успешности разбора
 *
 */
bool awh::codec::ini::Value::parse(const string & text) noexcept {
	// Дерево настроек, разбором собираемое
	Document document;
	/**
	 * Если разбор текста настроек завершился отказом
	 */
	if(!document.parse(text))
		// Выводим признак неудачного разбора
		return false;
	// Выполняем снятие значения с дерева настроек
	this->absorb(document);
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод разбора текста настроек во владеющее значение с настройками
 *
 * @param text     разбираемый текст настроек
 * @param settings настройки разбора
 * @return         признак успешности разбора
 *
 */
bool awh::codec::ini::Value::parse(const string & text, const Document::settings_t & settings) noexcept {
	// Дерево настроек, разбором собираемое
	Document document;
	/**
	 * Если разбор текста настроек завершился отказом
	 */
	if(!document.parse(text, settings))
		// Выводим признак неудачного разбора
		return false;
	// Выполняем снятие значения с дерева настроек
	this->absorb(document);
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод чтения текста настроек из файла во владеющее значение
 *
 * @param filename имя читаемого файла
 * @return         признак успешности чтения
 *
 */
bool awh::codec::ini::Value::load(const string & filename) noexcept {
	// Поток чтения файла настроек
	ifstream file(filename, ios::binary);
	/**
	 * Если файл настроек открыть не удалось
	 */
	if(!file.is_open())
		// Выводим признак неудачного чтения
		return false;
	// Считываем содержимое файла настроек целиком
	const string text((istreambuf_iterator <char> (file)), istreambuf_iterator <char> ());
	// Выполняем разбор считанного текста настроек
	return this->parse(text);
}
/**
 * @brief Метод чтения текста настроек из файла с настройками
 *
 * @param filename имя читаемого файла
 * @param settings настройки разбора
 * @return         признак успешности чтения
 *
 */
bool awh::codec::ini::Value::load(const string & filename, const Document::settings_t & settings) noexcept {
	// Поток чтения файла настроек
	ifstream file(filename, ios::binary);
	/**
	 * Если файл настроек открыть не удалось
	 */
	if(!file.is_open())
		// Выводим признак неудачного чтения
		return false;
	// Считываем содержимое файла настроек целиком
	const string text((istreambuf_iterator <char> (file)), istreambuf_iterator <char> ());
	// Выполняем разбор считанного текста настроек
	return this->parse(text, settings);
}
/**
 * @brief Метод записи владеющего значения текстом настроек с настройками
 *
 * @param settings настройки записи
 * @return         записанный текст настроек
 *
 */
string awh::codec::ini::Value::dump(const writer_t::settings_t & settings) const noexcept {
	/**
	 * Если корень вместилищем пар не является
	 *
	 * @note Корнем текста настроек может быть одно лишь вместилище пар: текст настроек
	 *       есть перечень свойств и разделов, и простое значение корнем его не бывает
	 */
	if(this->_type != type_t::TABLE)
		// Выводим пустой текст настроек
		return string();
	// Объект записи текста настроек
	writer_t writer(settings);
	/**
	 * @brief Функция записи свойств вместилища
	 *
	 * @param holder вместилище, чьи свойства записываются
	 * @return       признак успешности записи
	 *
	 */
	const auto compose = [&writer](const Value & holder) noexcept -> bool {
		/**
		 * Выполняем перебор всех пар вместилища
		 */
		for(size_t i = 0; i < holder._items.size(); i++){
			/**
			 * Если имя очередной пары пусто вовсе
			 *
			 * @note Пара такая заводится ростом вместилища по номеру, и записана быть не
			 *       может: имя есть у всякого свойства наречия
			 */
			if((i >= holder._names.size()) || holder._names.at(i).empty())
				// Выполняем переход к следующей паре вместилища
				continue;
			// Получаем значение очередной пары вместилища
			const Value & item = holder._items.at(i);
			/**
			 * Если значение пары является вместилищем пар
			 *
			 * @note Вместилище на этом уровне есть раздел либо подраздел, и записывается
			 *       оно не здесь, а заходом объявления разделов
			 */
			if(item._type == type_t::TABLE)
				// Выполняем переход к следующей паре вместилища
				continue;
			/**
			 * Если значение пары является перечнем одноимённых значений
			 */
			if(item._type == type_t::ARRAY){
				/**
				 * Выполняем перебор всех значений перечня
				 */
				for(size_t j = 0; j < item._items.size(); j++){
					/**
					 * Если очередное значение перечня простым не является
					 *
					 * @note Перечень перечней наречие записать не может: свойство несёт
					 *       одну последовательность знаков, а перечень есть повтор имени
					 */
					if(item._items.at(j)._type != type_t::STRING)
						// Выводим признак неудачной записи
						return false;
					/**
					 * Если записать очередное значение перечня не удалось
					 *
					 * @note Перечень пишется простым повторением имени, а не записью
					 *       «имя[] = значение»: последняя есть наречие PHP, и читающий без
					 *       настройки `arrays` отвергнет её как имя с недопустимым знаком.
					 *       Повтор же имени читается настройкой `duplicates` в перечень -
					 *       ровно тем способом, каким этот перечень и был снят
					 */
					if(!writer.property(holder._names.at(i), item._items.at(j)._text))
						// Выводим признак неудачной записи
						return false;
				}
				// Выполняем переход к следующей паре вместилища
				continue;
			}
			/**
			 * Если значение пары простым не является вовсе
			 */
			if(item._type != type_t::STRING)
				// Выводим признак неудачной записи
				return false;
			/**
			 * Если записать простое свойство не удалось
			 */
			if(!writer.property(holder._names.at(i), item._text))
				// Выводим признак неудачной записи
				return false;
		}
		// Выводим признак успешной записи
		return true;
	};
	/**
	 * Если записать свойства верхнего уровня не удалось
	 *
	 * @note Свойства верхнего уровня пишутся прежде объявления разделов намеренно:
	 *       свойство, за объявлением раздела записанное, принадлежало бы уже ему
	 */
	if(!compose(* this))
		// Выводим пустой текст настроек
		return string();
	/**
	 * Выполняем перебор всех пар корневого вместилища
	 */
	for(size_t i = 0; i < this->_items.size(); i++){
		/**
		 * Если имя очередной пары пусто вовсе
		 */
		if((i >= this->_names.size()) || this->_names.at(i).empty())
			// Выполняем переход к следующей паре вместилища
			continue;
		// Получаем значение очередной пары корневого вместилища
		const Value & section = this->_items.at(i);
		/**
		 * Если значение пары вместилищем пар не является
		 */
		if(section._type != type_t::TABLE)
			// Выполняем переход к следующей паре вместилища
			continue;
		/**
		 * Если объявить раздел не удалось
		 */
		if(!writer.section(this->_names.at(i)))
			// Выводим пустой текст настроек
			return string();
		/**
		 * Если записать свойства раздела не удалось
		 */
		if(!compose(section))
			// Выводим пустой текст настроек
			return string();
		/**
		 * Выполняем перебор всех пар раздела
		 */
		for(size_t j = 0; j < section._items.size(); j++){
			/**
			 * Если имя очередной пары пусто вовсе
			 */
			if((j >= section._names.size()) || section._names.at(j).empty())
				// Выполняем переход к следующей паре раздела
				continue;
			// Получаем значение очередной пары раздела
			const Value & subsection = section._items.at(j);
			/**
			 * Если значение пары вместилищем пар не является
			 */
			if(subsection._type != type_t::TABLE)
				// Выполняем переход к следующей паре раздела
				continue;
			/**
			 * Если объявить подраздел не удалось
			 */
			if(!writer.section(this->_names.at(i), section._names.at(j)))
				// Выводим пустой текст настроек
				return string();
			/**
			 * Если записать свойства подраздела не удалось
			 */
			if(!compose(subsection))
				// Выводим пустой текст настроек
				return string();
			/**
			 * Выполняем перебор всех пар подраздела
			 */
			for(auto & item : subsection._items){
				/**
				 * Если значение пары вместилищем пар является
				 *
				 * @note Глубже подраздела наречие не пишет вовсе, и записать такое дерево
				 *       частью значило бы потерять остаток молча
				 */
				if(item._type == type_t::TABLE)
					// Выводим пустой текст настроек
					return string();
			}
		}
	}
	// Выводим записанный текст настроек
	return writer.text();
}
/**
 * @brief Метод записи владеющего значения текстом настроек
 *
 * @return записанный текст настроек
 *
 */
string awh::codec::ini::Value::dump() const noexcept {
	// Выводим записанный текст настроек с настройками записи по умолчанию
	return this->dump(writer_t::settings_t());
}
/**
 * @brief Метод записи владеющего значения в файл
 *
 * @param filename имя записываемого файла
 * @return         признак успешности записи
 *
 */
bool awh::codec::ini::Value::save(const string & filename) const noexcept {
	// Получаем записанный текст настроек
	const string text = this->dump();
	/**
	 * Если записанный текст настроек пуст вовсе при непустом значении
	 */
	if(text.empty() && !this->_items.empty())
		// Выводим признак неудачной записи
		return false;
	// Поток записи файла настроек
	ofstream file(filename, ios::binary | ios::trunc);
	/**
	 * Если файл настроек открыть не удалось
	 */
	if(!file.is_open())
		// Выводим признак неудачной записи
		return false;
	// Выполняем запись текста настроек в файл
	file.write(text.data(), static_cast <streamsize> (text.size()));
	// Выводим признак успешности записи
	return file.good();
}
/**
 * @brief Метод переноса владеющего значения в дерево настроек
 *
 * @param document дерево настроек, куда переносится значение
 * @return         признак успешности переноса
 *
 */
bool awh::codec::ini::Value::graft(Document & document) const noexcept {
	/**
	 * Если корень вместилищем пар не является
	 */
	if(this->_type != type_t::TABLE)
		// Выводим признак неудачного переноса
		return false;
	/**
	 * Знак, которым начинается обращение к значению
	 *
	 * @note Знак этот берётся у дерева, куда перенос ведётся: подстановка обращений
	 *       настройкою задаётся, и дерево, её не ведущее, оград не требует вовсе
	 */
	const char letter = ((document.settings().references == reference_t::SHELL) ? '$' :
	 ((document.settings().references == reference_t::PYTHON) ? '%' : '\0'));
	/**
	 * @brief Функция ограждения знака обращения удвоением его
	 *
	 * @details Владеющее значение несёт значения РАЗРЕШЁННЫЕ: обращение, в тексте
	 * стоявшее, подстановкою уже заменено, а знак, ограждённый удвоением, разбором
	 * уже сведён к одинарному. Уложи такое значение в дерево дословно - и знак, данными
	 * бывший, обратился бы обращением, а перенос выдал бы значение иное
	 *
	 * @note Нашёл это ворошитель круговым переносом на записи `$${цель}`
	 *
	 * @param value ограждаемое значение
	 * @return      значение с ограждённым знаком обращения
	 *
	 */
	const auto guarded = [letter](const string & value) noexcept -> string {
		/**
		 * Если ограждать нечего
		 */
		if((letter == '\0') || (value.find(letter) == string::npos))
			// Выводим значение неизменным
			return value;
		// Собираемое значение с ограждённым знаком обращения
		string result;
		// Выполняем выделение места под собираемое значение
		result.reserve(value.length() + 8);
		/**
		 * Выполняем перебор всех знаков значения
		 */
		for(size_t i = 0; i < value.length(); i++){
			// Выполняем добавление очередного знака к собираемому значению
			result.push_back(value.at(i));
			/**
			 * Если очередной знак знаком обращения является
			 */
			if(value.at(i) == letter)
				// Выполняем удвоение знака обращения
				result.push_back(letter);
		}
		// Выводим собранное значение
		return result;
	};
	/**
	 * @brief Функция переноса свойств вместилища в дерево настроек
	 *
	 * @param holder     вместилище, чьи свойства переносятся
	 * @param section    имя раздела
	 * @param subsection имя подраздела
	 * @return           признак успешности переноса
	 *
	 */
	const auto transfer = [&document, &guarded](const Value & holder, const string_view section, const string_view subsection) noexcept -> bool {
		/**
		 * Выполняем перебор всех пар вместилища
		 */
		for(size_t i = 0; i < holder._items.size(); i++){
			/**
			 * Если имя очередной пары пусто вовсе
			 */
			if((i >= holder._names.size()) || holder._names.at(i).empty())
				// Выполняем переход к следующей паре вместилища
				continue;
			// Получаем значение очередной пары вместилища
			const Value & item = holder._items.at(i);
			/**
			 * Если значение пары является вместилищем пар
			 */
			if(item._type == type_t::TABLE)
				// Выполняем переход к следующей паре вместилища
				continue;
			/**
			 * Если значение пары является перечнем одноимённых значений
			 *
			 * @note Перечень переносится повтором имени: объявления, в разделе уже
			 *       имеющиеся, сносятся, а значения перечня доливаются заново.
			 *       Установкою первого значения обойтись нельзя: она правит одно
			 *       объявление из перечня, и прочие прежние остались бы в дереве -
			 *       перенос поверх готового дерева наращивал бы перечень всякий раз
			 *
			 * @note Сносом перечень теряет своё место в разделе и уходит в конец его:
			 *       правки объявлений поодиночке договор дерева не имеет, а перенос
			 *       обязан выдать перечень тот, какой в значении, а не сросшийся с
			 *       прежним
			 */
			if(item._type == type_t::ARRAY){
				/**
				 * Выполняем снос прежних объявлений свойства
				 *
				 * @note Итог сноса не проверяется: отказом он отвечает и тогда, когда
				 *       свойства в разделе не было вовсе, а это и есть заход переноса
				 *       в дерево пустое
				 */
				document.erase(holder._names.at(i), section, subsection);
				/**
				 * Выполняем перебор всех значений перечня
				 */
				for(size_t j = 0; j < item._items.size(); j++){
					// Получаем очередное значение перечня
					const Value & entry = item._items.at(j);
					/**
					 * Если значение перечня простым не является вовсе
					 *
					 * @note Перечень записи INI несёт лишь простые значения: вложенности
					 *       запись не имеет, и взяться вложенному значению неоткуда
					 */
					if(entry._type != type_t::STRING)
						// Выводим признак неудачного переноса
						return false;
					/**
					 * Если перенести очередное значение перечня не удалось
					 */
					if(!document.push(holder._names.at(i), guarded(entry._text), section, subsection))
						// Выводим признак неудачного переноса
						return false;
				}
				// Выполняем переход к следующей паре вместилища
				continue;
			}
			/**
			 * Если значение пары простым не является вовсе
			 */
			if(item._type != type_t::STRING)
				// Выводим признак неудачного переноса
				return false;
			/**
			 * Если перенести простое свойство не удалось
			 */
			if(!document.set(holder._names.at(i), guarded(item._text), section, subsection))
				// Выводим признак неудачного переноса
				return false;
		}
		// Выводим признак успешного переноса
		return true;
	};
	/**
	 * Если перенести свойства верхнего уровня не удалось
	 */
	if(!transfer(* this, "", ""))
		// Выводим признак неудачного переноса
		return false;
	/**
	 * Выполняем перебор всех пар корневого вместилища
	 */
	for(size_t i = 0; i < this->_items.size(); i++){
		/**
		 * Если имя очередной пары пусто вовсе
		 */
		if((i >= this->_names.size()) || this->_names.at(i).empty())
			// Выполняем переход к следующей паре вместилища
			continue;
		// Получаем значение очередной пары корневого вместилища
		const Value & section = this->_items.at(i);
		/**
		 * Если значение пары вместилищем пар не является
		 */
		if(section._type != type_t::TABLE)
			// Выполняем переход к следующей паре вместилища
			continue;
		/**
		 * Если объявить раздел не удалось
		 */
		if(!document.create(this->_names.at(i)))
			// Выводим признак неудачного переноса
			return false;
		/**
		 * Если перенести свойства раздела не удалось
		 */
		if(!transfer(section, this->_names.at(i), ""))
			// Выводим признак неудачного переноса
			return false;
		/**
		 * Выполняем перебор всех пар раздела
		 */
		for(size_t j = 0; j < section._items.size(); j++){
			/**
			 * Если имя очередной пары пусто вовсе
			 */
			if((j >= section._names.size()) || section._names.at(j).empty())
				// Выполняем переход к следующей паре раздела
				continue;
			// Получаем значение очередной пары раздела
			const Value & subsection = section._items.at(j);
			/**
			 * Если значение пары вместилищем пар не является
			 */
			if(subsection._type != type_t::TABLE)
				// Выполняем переход к следующей паре раздела
				continue;
			/**
			 * Если объявить подраздел не удалось
			 */
			if(!document.create(this->_names.at(i), section._names.at(j)))
				// Выводим признак неудачного переноса
				return false;
			/**
			 * Если перенести свойства подраздела не удалось
			 */
			if(!transfer(subsection, this->_names.at(i), section._names.at(j)))
				// Выводим признак неудачного переноса
				return false;
			/**
			 * Выполняем перебор всех пар подраздела
			 */
			for(auto & item : subsection._items){
				/**
				 * Если значение пары вместилищем пар является
				 *
				 * @note Глубже подраздела наречие не строит вовсе
				 */
				if(item._type == type_t::TABLE)
					// Выводим признак неудачного переноса
					return false;
			}
		}
	}
	// Выводим признак успешного переноса
	return true;
}

/**
 * @brief Метод получения вместилища, сборкой открытого
 *
 * @return ссылка на открытое вместилище
 *
 */
awh::codec::ini::Value & awh::codec::ini::Builder::opened() noexcept {
	// Получаем ссылку на собираемое значение
	Value * result = &this->_root;
	/**
	 * Выполняем перебор пути к открытому вместилищу
	 */
	for(auto & index : this->_path)
		// Выполняем переход к вложенному вместилищу по номеру его
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
size_t awh::codec::ini::Builder::deposit(Value && value) noexcept {
	// Получаем ссылку на открытое вместилище
	Value & holder = this->opened();
	/**
	 * Если имя свойства назначено
	 */
	if(this->_keyed){
		// Получаем имя заносимого поля вместилища
		const string name(this->_key);
		// Выполняем сброс признака назначенного имени
		this->_keyed = false;
		// Выполняем очистку имени поля вместилища
		this->_key.clear();
		// Выполняем установку поля вместилища
		holder.insert(name, value);
		/**
		 * Выполняем розыск номера занесённого поля вместилища
		 *
		 * @note Концом вместилища номер брать нельзя: имя, вторично поданное сборкою,
		 *       перезаписывает поле НА СВОЁМ МЕСТЕ, и концом оказалось бы поле чужое -
		 *       вместилище, следом открытое, собиралось бы в него. На скалярах ловушка
		 *       эта не видна вовсе: вылезает она лишь тогда, когда за повтором имени
		 *       открывается вместилище
		 */
		const size_t found = holder.search(name);
		// Выводим номер занесённого поля вместилища
		return ((found < holder.size()) ? found : (holder.size() > 0 ? (holder.size() - 1) : 0));
	}
	// Выполняем добавление значения в конец перечня
	holder.push(value);
	// Выводим номер занесённого значения во вместилище
	return (holder.size() - 1);
}
/**
 * @brief Метод открытия вместилища затребованного типа
 *
 * @param value открываемое вместилище
 * @return      признак успешности открытия
 *
 */
bool awh::codec::ini::Builder::expand(Value && value) noexcept {
	/**
	 * Если сборка завершена уже
	 */
	if(this->_done)
		// Выводим признак неудачного открытия
		return false;
	/**
	 * Если глубина открытых вместилищ предел вложенности превышает
	 */
	if(this->_path.size() >= ::DEPTH)
		// Выводим признак неудачного открытия
		return false;
	/**
	 * Если вместилище корневое ещё не открыто
	 */
	if(this->_path.empty() && !this->_root.valid()){
		/**
		 * Если имя свойства назначено
		 */
		if(this->_keyed)
			// Выводим признак неудачного открытия
			return false;
		// Выполняем установку корневого вместилища
		this->_root = ::std::move(value);
		// Выводим признак успешного открытия
		return true;
	}
	// Выполняем занесение открываемого вместилища
	const size_t index = this->deposit(::std::move(value));
	// Выполняем добавление номера открытого вместилища в путь
	this->_path.push_back(index);
	// Выводим признак успешного открытия
	return true;
}
/**
 * @brief Метод открытия раздела
 *
 * @return признак успешности открытия
 *
 */
bool awh::codec::ini::Builder::section() noexcept {
	// Выводим признак успешности открытия раздела
	return this->expand(Value(type_t::TABLE));
}
/**
 * @brief Метод открытия перечня значений одноимённого свойства
 *
 * @return признак успешности открытия
 *
 */
bool awh::codec::ini::Builder::list() noexcept {
	// Выводим признак успешности открытия перечня значений
	return this->expand(Value(type_t::ARRAY));
}
/**
 * @brief Метод закрытия открытого вместилища
 *
 * @return признак успешности закрытия
 *
 */
bool awh::codec::ini::Builder::close() noexcept {
	/**
	 * Если сборка завершена уже
	 */
	if(this->_done)
		// Выводим признак неудачного закрытия
		return false;
	/**
	 * Если имя свойства назначено, а значения ему не дано
	 */
	if(this->_keyed)
		// Выводим признак неудачного закрытия
		return false;
	/**
	 * Если открытых вложенных вместилищ нет вовсе
	 */
	if(this->_path.empty()){
		/**
		 * Если вместилище корневое не открыто вовсе
		 */
		if(!this->_root.valid())
			// Выводим признак неудачного закрытия
			return false;
		// Запоминаем, что сборка завершена
		this->_done = true;
		// Выводим признак успешного закрытия
		return true;
	}
	// Выполняем снятие номера закрываемого вместилища с пути
	this->_path.pop_back();
	// Выводим признак успешного закрытия
	return true;
}
/**
 * @brief Метод назначения имени свойства
 *
 * @param name назначаемое имя свойства
 * @return     признак успешности назначения
 *
 */
bool awh::codec::ini::Builder::key(const string & name) noexcept {
	/**
	 * Если сборка завершена уже, имя пусто вовсе либо назначено уже
	 */
	if(this->_done || name.empty() || this->_keyed)
		// Выводим признак неудачного назначения
		return false;
	/**
	 * Если открытое вместилище вместилищем пар не является
	 */
	if(!this->opened().is(type_t::TABLE))
		// Выводим признак неудачного назначения
		return false;
	// Выполняем установку имени свойства
	this->_key = name;
	// Запоминаем, что имя свойства назначено
	this->_keyed = true;
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
bool awh::codec::ini::Builder::value(const Value & value) noexcept {
	/**
	 * Если сборка завершена уже либо вместилище корневое не открыто вовсе
	 */
	if(this->_done || (this->_path.empty() && !this->_root.valid()))
		// Выводим признак неудачной записи
		return false;
	/**
	 * Если открытое вместилище вместилищем пар является, а имя свойства не назначено
	 */
	if(this->opened().is(type_t::TABLE) && !this->_keyed)
		// Выводим признак неудачной записи
		return false;
	// Выполняем занесение записываемого значения
	this->deposit(Value(value));
	// Выводим признак успешной записи
	return true;
}
/**
 * @brief Метод записи простого значения
 *
 * @param value  записываемое содержимое
 * @param quoted признак значения, записанного в кавычках
 * @return       признак успешности записи
 *
 */
bool awh::codec::ini::Builder::value(const string & value, const bool quoted) noexcept {
	// Выводим признак успешности записи простого значения
	return this->value(Value(value, quoted));
}
/**
 * @copydoc awh::codec::ini::Builder::value(const string &, const bool)
 */
bool awh::codec::ini::Builder::value(const char * value, const bool quoted) noexcept {
	// Выводим признак успешности записи простого значения
	return this->value(Value(value, quoted));
}
/**
 * @brief Метод извлечения глубины открытых вместилищ
 *
 * @return глубина открытых вместилищ
 *
 */
size_t awh::codec::ini::Builder::depth() const noexcept {
	/**
	 * Если сборка завершена уже либо вместилище корневое не открыто вовсе
	 */
	if(this->_done || !this->_root.valid())
		// Выводим нулевую глубину открытых вместилищ
		return 0;
	// Выводим глубину открытых вместилищ вместе с корневым
	return (this->_path.size() + 1);
}
/**
 * @brief Метод сброса сборки в исходное состояние
 *
 */
void awh::codec::ini::Builder::reset() noexcept {
	// Выполняем сброс собираемого значения
	this->_root.clear();
	// Выполняем очистку пути к открытому вместилищу
	this->_path.clear();
	// Выполняем очистку имени свойства
	this->_key.clear();
	// Выполняем сброс признака назначенного имени
	this->_keyed = false;
	// Выполняем сброс признака завершённости сборки
	this->_done = false;
}
/**
 * @brief Метод изъятия собранного значения
 *
 * @return собранное значение
 *
 */
awh::codec::ini::Value awh::codec::ini::Builder::finish() noexcept {
	// Выполняем изъятие собранного значения переносом
	Value result(::std::move(this->_root));
	// Выполняем сброс сборки в исходное состояние
	this->reset();
	// Выводим собранное значение
	return result;
}
/**
 * @brief Конструктор
 *
 */
awh::codec::ini::Builder::Builder() noexcept : _keyed(false), _done(false) {}
