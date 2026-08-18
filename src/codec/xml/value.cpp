/**
 * @file value.cpp
 * @date 2026-08-18
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
 * @brief Реализация владеющего значения XML — заведение поддерева из значений языка,
 *        снятие его с дерева разметки, правка, извлечение значений и перезапись в текст
 *
 * \~english
 * @brief Implementation of an owning value of XML — the creation of a subtree from the values of the language,
 *        the taking of it off a markup tree, the editing, the extraction of the values and the rewriting into a text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <limits>
#include <atomic>
#include <fstream>
#include <type_traits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <num/lexical/lexical.hpp>
#include <codec/xml/value.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Безымянное пространство имён вспомогательных объявлений значения
 *
 */
namespace {
	/**
	 * Размер куска, каким читается файл разметки
	 */
	static constexpr size_t CHUNK = 0x10000;
	/**
	 * @brief Предел роста перечня вложенных узлов обращением по номеру
	 *
	 * @details Значение по умолчанию взято с запасом над всяким разумным обращением по
	 * номеру и стоит около шести мегабайтов памяти. Соперники предела такого не знают
	 * вовсе - и `nlohmann/json`, и указатели `RapidJSON` растят перечень до затребованного
	 * номера, ничем того не стерегая, - зато обычай предохранителя с настройкою у разборщиков
	 * разметки заведён давно: `libxml2` держит предел текста в 10 000 000 знаков со снятием
	 * его признаком `XML_PARSE_HUGE`, `Xerces` - предел развёртывания сущностей в 50 000,
	 * `simdjson` - предел вложенности в 1024
	 *
	 * @note Хранится оно беззнаковым числом с упорядоченным доступом: ставится предел единожды
	 *       при заведении приложения, а читается всяким потоком, обращающимся по номеру
	 *
	 */
	static ::std::atomic <size_t> LIMIT(0x10000);
	/**
	 * @brief Наибольший допустимый объём хранилища знаков дерева разметки
	 *
	 * @details Узлы дерева указывают на содержимое положением в общем хранилище, а
	 *          положение хранится четырьмя байтами: выход за этот объём усёк бы
	 *          положение молча, оставив дерево с отрезками, указывающими не туда
	 *
	 * @note Значение это повторяет предел чтения дерева из текста, заданный там же,
	 *       где чтение: пределы эти обязаны совпадать, ибо хранилище у них одно
	 *
	 */
	static constexpr size_t STORAGE_LIMIT = 0xFFFFFFFF;
	/**
	 * @brief Шаблонная функция приведения дробного числа к затребованному виду
	 *
	 * @details Приведение отвечает языку: дробная часть отбрасывается усечением к нулю.
	 * Разница с `static_cast` одна - дробное, чья целая часть лежит за пределами
	 * затребованного целого вида, выдаётся пределом этого вида
	 *
	 * @note Тело это повторяет тело такой же функции кодека JSON знак в знак, и
	 *       расходиться им нельзя: договор извлечения общий у всех кодеков рамки, и
	 *       расхождение здесь стало бы расхождением поведения кодеков между собой
	 *
	 * @tparam T     затребованный вид числа
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
		 * @note Приведение `NaN` к целому есть неопределённое поведение при любом пределе
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
	 * @brief Шаблонная функция извлечения записи числа дробным
	 *
	 * @details Отказ разбора по нехватке разрядности отказом извлечения не считается:
	 * запись, в родной вид не вместимая, числом быть не перестаёт, и `strtod` отвечает
	 * ей бесконечностью либо нулём. Договор этот общий у кодеков рамки - у JSON и YAML
	 * запись такая извлекается тем же самым порядком, - и расхождение здесь стало бы
	 * расхождением поведения кодеков между собой
	 *
	 * @note Различать отказ по разрядности и отказ по составу записи обязательно:
	 *       первый значит «число, да велико», второй - «не число вовсе»
	 *
	 * @tparam T      затребованный вид числа
	 * @param  begin  начало записи числа
	 * @param  end    конец записи числа
	 * @param  result переменная, куда помещается извлечённое значение
	 * @return        признак успешности извлечения
	 *
	 */
	template <typename T>
	static bool extend(const char * begin, const char * end, T & result) noexcept {
		// Разбираемое дробное число
		double number = 0.;
		// Выполняем разбор записи числа
		const awh::lexical_t::result_t <char> res = awh::lexical_t::fromChars(begin, end, number);
		/**
		 * Если запись дробным числом не разбирается вовсе
		 */
		if(res.ptr != end)
			// Выводим признак неудачного извлечения
			return false;
		/**
		 * Если разбор отказал не нехваткой разрядности
		 */
		if(!static_cast <bool> (res) && (res.error != awh::lexical_t::error_t::OVERFLOW_RANGE))
			// Выводим признак неудачного извлечения
			return false;
		// Устанавливаем извлечённое значение приведением дробного
		result = ::convert <T> (number);
		// Выводим признак успешного извлечения
		return true;
	}
	/**
	 * @brief Функция извлечения пустой строки
	 *
	 * @return пустая строка
	 *
	 */
	static const string & empty() noexcept {
		// Пустая строка, выдаваемая при отсутствии содержимого
		static const string result;
		// Выводим пустую строку
		return result;
	}
	/**
	 * @brief Функция разбора пути на звенья
	 *
	 * @details Путь записывается частями, разделёнными косой чертой: `/Envelope/Body/0`.
	 * Отменяющие записи `~1` и `~0` снимаются, ровно как их снимает указатель JSON:
	 * местное имя узла косой черты содержать не может, а отмена заведена ради единства
	 * договора у кодеков
	 *
	 * @param path   разбираемый путь
	 * @param result перечень, куда помещаются разобранные звенья пути
	 * @return       признак успешности разбора
	 *
	 */
	static bool tokens(const string & path, vector <string> & result) noexcept {
		// Выполняем очистку перечня звеньев пути
		result.clear();
		/**
		 * Если путь пуст
		 */
		if(path.empty())
			// Выводим признак успешного разбора
			return true;
		/**
		 * Если путь не начинается с косой черты
		 */
		if(path.front() != '/')
			// Выводим признак неудачного разбора
			return false;
		// Положение разбираемого знака пути
		size_t offset = 1;
		/**
		 * Выполняем разбор пути звено за звеном
		 */
		while(offset <= path.size()){
			// Выполняем поиск конца очередного звена пути
			const size_t end = path.find('/', offset);
			// Получаем содержимое очередного звена пути
			string token = path.substr(offset, ((end == string::npos) ? string::npos : (end - offset)));
			// Выполняем переход к следующему звену пути
			offset = ((end == string::npos) ? (path.size() + 1) : (end + 1));
			/**
			 * Выполняем снятие отменяющих записей звена пути
			 */
			for(size_t i = token.find('~'); i != string::npos; i = token.find('~', i + 1)){
				/**
				 * Если за знаком отмены не осталось знаков
				 */
				if((i + 1) >= token.size())
					// Выводим признак неудачного разбора
					return false;
				/**
				 * Определяем отменяющую запись звена пути
				 */
				switch(token[i + 1]){
					// Если записана косая черта
					case '1':
						// Выполняем подмену отменяющей записи косой чертой
						token.replace(i, 2, "/");
					break;
					// Если записан сам знак отмены
					case '0':
						// Выполняем подмену отменяющей записи знаком отмены
						token.replace(i, 2, "~");
					break;
					// Если отменяющая запись не опознана
					default:
						// Выводим признак неудачного разбора
						return false;
				}
			}
			// Добавляем разобранное звено в перечень звеньев пути
			result.push_back(::std::move(token));
		}
		// Выводим признак успешного разбора
		return true;
	}
	/**
	 * @brief Функция разбора звена пути номером вложенного узла
	 *
	 * @param token  разбираемое звено пути
	 * @param result переменная, куда помещается разобранный номер узла
	 * @return       признак того, что звено является номером узла
	 *
	 */
	static bool numbered(const string & token, size_t & result) noexcept {
		/**
		 * Если содержимое звена номером узла не является
		 */
		if(token.empty() || ((token.size() > 1) && (token.front() == '0')))
			// Выводим признак того, что звено номером узла не является
			return false;
		// Выполняем сброс разбираемого номера узла
		result = 0;
		/**
		 * Выполняем перебор всех знаков звена пути
		 */
		for(const char letter : token){
			/**
			 * Если знак цифрой не является
			 */
			if((letter < '0') || (letter > '9'))
				// Выводим признак того, что звено номером узла не является
				return false;
			/**
			 * Если добавление разряда выведет номер за предел разрядности
			 *
			 * @details Номер разбирается беззнаковым, и переполнение его языком определено
			 * заворотом: `18446744073709551617` обращалось бы единицей, и обращение по
			 * такому пути отдавало бы соседнее значение молча. Вместилища такой длины не
			 * бывает вовсе - номер этот номером не является
			 *
			 */
			if(result > ((std::numeric_limits <size_t>::max() - static_cast <size_t> (letter - '0')) / 10))
				// Выводим признак того, что звено номером не является
				return false;
			// Добавляем разряд к номеру вложенного узла
			result = ((result * 10) + static_cast <size_t> (letter - '0'));
		}
		// Выводим признак того, что звено является номером узла
		return true;
	}
}

/**
 * @brief Метод проверки определённости значения
 *
 * @return признак определённости значения
 *
 */
bool awh::codec::xml::Value::valid() const noexcept {
	// Выводим признак определённости значения
	return (this->_kind != kind_t::NONE);
}
/**
 * @brief Метод извлечения вида узла
 *
 * @return вид хранимого узла
 *
 */
awh::codec::xml::kind_t awh::codec::xml::Value::kind() const noexcept {
	// Выводим вид хранимого узла
	return this->_kind;
}
/**
 * @brief Метод проверки вида узла
 *
 * @param kind сличаемый вид узла
 * @return     признак совпадения вида
 *
 */
bool awh::codec::xml::Value::is(const kind_t kind) const noexcept {
	// Выводим признак совпадения вида узла
	return (this->_kind == kind);
}
/**
 * @brief Метод извлечения количества вложенных узлов
 *
 * @return количество вложенных узлов
 *
 */
size_t awh::codec::xml::Value::size() const noexcept {
	// Выводим количество вложенных узлов
	return this->_items.size();
}
/**
 * @brief Метод проверки узла на отсутствие вложенных узлов
 *
 * @return признак отсутствия вложенных узлов
 *
 */
bool awh::codec::xml::Value::empty() const noexcept {
	// Выводим признак отсутствия вложенных узлов
	return this->_items.empty();
}
/**
 * @brief Метод очистки значения
 *
 */
void awh::codec::xml::Value::clear() noexcept {
	// Выполняем сброс вида хранимого узла
	this->_kind = kind_t::NONE;
	// Выполняем очистку префикса пространства имён
	this->_prefix.clear();
	// Выполняем очистку местного имени узла
	this->_local.clear();
	// Выполняем очистку обозначения пространства имён
	this->_uri.clear();
	// Выполняем очистку собственного содержимого узла
	this->_text.clear();
	// Выполняем очистку свойств узла
	this->_attributes.clear();
	// Выполняем очистку связываний префиксов
	this->_bindings.clear();
	// Выполняем очистку вложенных узлов
	this->_items.clear();
}
/**
 * @brief Метод извлечения имени узла
 *
 * @return имя узла с учётом пространства имён
 *
 */
awh::codec::xml::name_t awh::codec::xml::Value::name() const noexcept {
	// Собираемое имя узла
	name_t result;
	// Устанавливаем префикс пространства имён имени узла
	result.prefix = string_view(this->_prefix);
	// Устанавливаем местное имя узла
	result.local = string_view(this->_local);
	// Устанавливаем обозначение пространства имён узла
	result.uri = string_view(this->_uri);
	// Выводим собранное имя узла
	return result;
}
/**
 * @brief Метод извлечения местного имени узла
 *
 * @return местное имя узла, а у указания обработчику - цель его
 *
 */
const string & awh::codec::xml::Value::local() const noexcept {
	// Выводим местное имя узла
	return this->_local;
}
/**
 * @brief Метод извлечения обозначения пространства имён узла
 *
 * @return обозначение пространства имён узла
 *
 */
const string & awh::codec::xml::Value::uri() const noexcept {
	// Выводим обозначение пространства имён узла
	return this->_uri;
}
/**
 * @brief Метод извлечения префикса пространства имён узла
 *
 * @return префикс пространства имён узла
 *
 */
const string & awh::codec::xml::Value::prefix() const noexcept {
	// Выводим префикс пространства имён узла
	return this->_prefix;
}
/**
 * @brief Метод установки имени узла
 *
 * @param local  устанавливаемое местное имя узла
 * @param uri    устанавливаемое обозначение пространства имён
 * @param prefix устанавливаемый префикс пространства имён
 *
 */
void awh::codec::xml::Value::name(const string & local, const string & uri, const string & prefix) noexcept {
	// Устанавливаем местное имя узла
	this->_local = local;
	// Устанавливаем обозначение пространства имён узла
	this->_uri = uri;
	// Устанавливаем префикс пространства имён узла
	this->_prefix = prefix;
}
/**
 * @brief Метод сбора содержимого вложенных текстовых узлов
 *
 * @param result строка, куда собирается содержимое
 *
 */
void awh::codec::xml::Value::gather(string & result) const noexcept {
	/**
	 * Выполняем перебор всех вложенных узлов
	 */
	for(auto & item : this->_items){
		/**
		 * Определяем вид вложенного узла
		 */
		switch(static_cast <uint8_t> (item._kind)){
			// Если узел является текстовым содержимым либо дословным разделом
			case static_cast <uint8_t> (kind_t::TEXT):
			case static_cast <uint8_t> (kind_t::CDATA):
			case static_cast <uint8_t> (kind_t::SPACE):
				// Добавляем содержимое узла к собираемому содержимому
				result.append(item._text);
			break;
			/**
			 * Если узел является узлом разметки
			 *
			 * @note Содержимое вложенной разметки собирается наравне со своим: ровно так
			 *       его собирает и узел дерева
			 */
			case static_cast <uint8_t> (kind_t::ELEMENT):
				// Выполняем сбор содержимого вложенного узла разметки
				item.gather(result);
			break;
		}
	}
}
/**
 * @brief Метод извлечения содержимого узла
 *
 * @return содержимое узла
 *
 */
string awh::codec::xml::Value::text() const noexcept {
	/**
	 * Определяем вид хранимого узла
	 */
	switch(static_cast <uint8_t> (this->_kind)){
		// Если узел владеет содержимым своим
		case static_cast <uint8_t> (kind_t::TEXT):
		case static_cast <uint8_t> (kind_t::CDATA):
		case static_cast <uint8_t> (kind_t::SPACE):
		case static_cast <uint8_t> (kind_t::COMMENT):
		case static_cast <uint8_t> (kind_t::PROCESSING):
		case static_cast <uint8_t> (kind_t::DOCTYPE):
			// Выводим собственное содержимое узла
			return this->_text;
	}
	// Собираемое содержимое вложенных текстовых узлов
	string result;
	// Выполняем сбор содержимого вложенных текстовых узлов
	this->gather(result);
	// Выводим собранное содержимое узла
	return result;
}
/**
 * @brief Метод установки собственного содержимого узла
 *
 * @param text устанавливаемое содержимое узла
 * @return     признак успешности установки
 *
 */
bool awh::codec::xml::Value::text(const string & text) noexcept {
	/**
	 * Определяем вид хранимого узла
	 */
	switch(static_cast <uint8_t> (this->_kind)){
		/**
		 * Если узел ещё не определён
		 *
		 * @note Значение неопределённое перерождается узлом текстовым: содержимое без
		 *       имени есть текст, и иного вида, годного ему, у разметки нет
		 */
		case static_cast <uint8_t> (kind_t::NONE):
			// Устанавливаем вид узла текстовым содержимым
			this->_kind = kind_t::TEXT;
		// Если узел владеет содержимым своим
		case static_cast <uint8_t> (kind_t::TEXT):
		case static_cast <uint8_t> (kind_t::CDATA):
		case static_cast <uint8_t> (kind_t::SPACE):
		case static_cast <uint8_t> (kind_t::COMMENT):
		case static_cast <uint8_t> (kind_t::PROCESSING):
		case static_cast <uint8_t> (kind_t::DOCTYPE):
			// Устанавливаем собственное содержимое узла
			this->_text = text;
			// Выводим признак успешной установки
			return true;
		/**
		 * Если узел является узлом разметки
		 *
		 * @note Собственного содержимого у разметки нет вовсе: установка заменяет все
		 *       вложенные узлы одним узлом текстовым
		 */
		case static_cast <uint8_t> (kind_t::ELEMENT): {
			// Выполняем очистку вложенных узлов
			this->_items.clear();
			// Добавляем во вложенные узлы узел текстового содержимого
			this->_items.push_back(Value(kind_t::TEXT, text));
			// Выводим признак успешной установки
			return true;
		}
	}
	// Выводим признак неудачной установки
	return false;
}
/**
 * @brief Метод извлечения свойств узла разметки
 *
 * @return перечень свойств узла в порядке их следования
 *
 */
const vector <awh::codec::xml::Value::property_t> & awh::codec::xml::Value::attributes() const noexcept {
	// Выводим перечень свойств узла
	return this->_attributes;
}
/**
 * @brief Метод извлечения значения свойства узла разметки
 *
 * @param local местное имя разыскиваемого свойства
 * @param uri   обозначение пространства имён свойства
 * @return      значение свойства, пустое - свойства такого нет
 *
 */
const string & awh::codec::xml::Value::attribute(const string & local, const string & uri) const noexcept {
	/**
	 * Выполняем перебор всех свойств узла
	 */
	for(auto & item : this->_attributes){
		/**
		 * Если имя свойства совпадает с разыскиваемым
		 *
		 * @note Сличение идёт по паре обозначения пространства имён и местного имени, а
		 *       не по записи с префиксом: отвечающие по UPnP ставят префиксы всякий
		 *       по-своему, и сличение записью на них разваливается
		 */
		if((item.local.compare(local) == 0) && (item.uri.compare(uri) == 0))
			// Выводим значение разысканного свойства
			return item.value;
	}
	// Выводим отсутствие значения свойства
	return ::empty();
}
/**
 * @brief Метод установки свойства узла разметки
 *
 * @param local  местное имя свойства
 * @param value  устанавливаемое значение свойства
 * @param uri    обозначение пространства имён свойства
 * @param prefix префикс пространства имён свойства
 * @return       признак успешности установки
 *
 */
bool awh::codec::xml::Value::attribute(const string & local, const string & value, const string & uri, const string & prefix) noexcept {
	/**
	 * Если узел ещё не определён
	 */
	if(this->_kind == kind_t::NONE)
		// Устанавливаем вид узла разметкой
		this->_kind = kind_t::ELEMENT;
	/**
	 * Если узел узлом разметки не является
	 *
	 * @note Свойства принадлежат разметке и никому иному: у текстового содержимого,
	 *       примечания и дословного раздела свойств не бывает вовсе
	 */
	if(this->_kind != kind_t::ELEMENT)
		// Выводим признак неудачной установки
		return false;
	/**
	 * Выполняем перебор всех свойств узла
	 */
	for(auto & item : this->_attributes){
		/**
		 * Если имя свойства совпадает с устанавливаемым
		 */
		if((item.local.compare(local) == 0) && (item.uri.compare(uri) == 0)){
			// Выполняем перезапись значения свойства на своём месте
			item.value = value;
			// Выполняем перезапись префикса пространства имён свойства
			item.prefix = prefix;
			// Выводим признак успешной установки
			return true;
		}
	}
	// Заводим свойство узла разметки
	this->_attributes.emplace_back();
	// Устанавливаем префикс пространства имён свойства
	this->_attributes.back().prefix = prefix;
	// Устанавливаем местное имя свойства
	this->_attributes.back().local = local;
	// Устанавливаем обозначение пространства имён свойства
	this->_attributes.back().uri = uri;
	// Устанавливаем значение свойства
	this->_attributes.back().value = value;
	// Выводим признак успешной установки
	return true;
}
/**
 * @brief Метод проверки наличия свойства у узла разметки
 *
 * @param local местное имя разыскиваемого свойства
 * @param uri   обозначение пространства имён свойства
 * @return      признак наличия свойства
 *
 */
bool awh::codec::xml::Value::has(const string & local, const string & uri) const noexcept {
	/**
	 * Выполняем перебор всех свойств узла
	 */
	for(auto & item : this->_attributes){
		/**
		 * Если имя свойства совпадает с разыскиваемым
		 */
		if((item.local.compare(local) == 0) && (item.uri.compare(uri) == 0))
			// Выводим признак наличия свойства
			return true;
	}
	// Выводим отсутствие свойства
	return false;
}
/**
 * @brief Метод снятия свойства узла разметки
 *
 * @param local местное имя снимаемого свойства
 * @param uri   обозначение пространства имён свойства
 * @return      признак успешности снятия
 *
 */
bool awh::codec::xml::Value::detach(const string & local, const string & uri) noexcept {
	/**
	 * Выполняем перебор всех свойств узла
	 */
	for(size_t i = 0; i < this->_attributes.size(); i++){
		/**
		 * Если имя свойства совпадает со снимаемым
		 */
		if((this->_attributes.at(i).local.compare(local) == 0) && (this->_attributes.at(i).uri.compare(uri) == 0)){
			// Выполняем снятие свойства узла
			this->_attributes.erase(this->_attributes.begin() + static_cast <ptrdiff_t> (i));
			// Выводим признак успешного снятия
			return true;
		}
	}
	// Выводим признак неудачного снятия
	return false;
}
/**
 * @brief Метод извлечения связываний префиксов, объявленных узлом
 *
 * @return перечень связываний префиксов
 *
 */
const vector <awh::codec::xml::Value::namespace_t> & awh::codec::xml::Value::bindings() const noexcept {
	// Выводим перечень связываний префиксов
	return this->_bindings;
}
/**
 * @brief Метод объявления связывания префикса с пространством имён
 *
 * @param prefix объявляемый префикс, пустой - объявление по умолчанию
 * @param uri    обозначение пространства имён, пустое - отмена связывания
 * @return       признак успешности объявления
 *
 */
bool awh::codec::xml::Value::binding(const string & prefix, const string & uri) noexcept {
	/**
	 * Если узел ещё не определён
	 */
	if(this->_kind == kind_t::NONE)
		// Устанавливаем вид узла разметкой
		this->_kind = kind_t::ELEMENT;
	/**
	 * Если узел узлом разметки не является
	 */
	if(this->_kind != kind_t::ELEMENT)
		// Выводим признак неудачного объявления
		return false;
	/**
	 * Выполняем перебор всех связываний префиксов узла
	 */
	for(auto & item : this->_bindings){
		/**
		 * Если префикс связывания совпадает с объявляемым
		 */
		if(item.prefix.compare(prefix) == 0){
			// Выполняем перезапись обозначения пространства имён на своём месте
			item.uri = uri;
			// Выводим признак успешного объявления
			return true;
		}
	}
	// Заводим связывание префикса
	this->_bindings.emplace_back();
	// Устанавливаем объявляемый префикс
	this->_bindings.back().prefix = prefix;
	// Устанавливаем обозначение пространства имён
	this->_bindings.back().uri = uri;
	// Выводим признак успешного объявления
	return true;
}
/**
 * @brief Метод извлечения предела роста вместилища по номеру
 *
 * @return предел роста вместилища, нуль - предела нет
 *
 */
size_t awh::codec::xml::Value::limit() noexcept {
	// Выводим установленный предел роста вместилища
	return ::LIMIT.load(::std::memory_order_relaxed);
}
/**
 * @brief Метод установки предела роста вместилища по номеру
 *
 * @param value устанавливаемый предел, нуль снимает предел вовсе
 *
 */
void awh::codec::xml::Value::limit(const size_t value) noexcept {
	// Выполняем установку предела роста вместилища
	::LIMIT.store(value, ::std::memory_order_relaxed);
}
/**
 * @brief Метод извлечения значения неопределённого
 *
 * @return значение неопределённое
 *
 */
const awh::codec::xml::Value & awh::codec::xml::Value::undefined() noexcept {
	// Значение неопределённое, выдаваемое при неудачном обращении
	static const Value result;
	// Выводим значение неопределённое
	return result;
}
/**
 * @brief Метод извлечения значения мусорного
 *
 * @return значение мусорное
 *
 */
awh::codec::xml::Value & awh::codec::xml::Value::scrap() noexcept {
	// Значение мусорное, принимающее на себя запись при неудачном обращении
	static thread_local Value result;
	// Выполняем очистку значения мусорного от записанного прежде
	result.clear();
	// Выводим значение мусорное
	return result;
}
/**
 * @brief Метод проверки наличия вложенного узла разметки с указанным именем
 *
 * @param local местное имя разыскиваемого узла
 * @param uri   обозначение пространства имён узла
 * @return      признак наличия вложенного узла
 *
 */
bool awh::codec::xml::Value::contains(const string & local, const string & uri) const noexcept {
	/**
	 * Выполняем перебор всех вложенных узлов
	 */
	for(auto & item : this->_items){
		/**
		 * Если вложенный узел является узлом разметки с разыскиваемым именем
		 */
		if((item._kind == kind_t::ELEMENT) && (item._local.compare(local) == 0) && (item._uri.compare(uri) == 0))
			// Выводим признак наличия вложенного узла
			return true;
	}
	// Выводим отсутствие вложенного узла
	return false;
}
/**
 * @brief Метод извлечения всех вложенных узлов разметки с указанным именем
 *
 * @param local местное имя разыскиваемых узлов
 * @param uri   обозначение пространства имён узлов
 * @return      перечень указаний на разысканные узлы
 *
 */
vector <const awh::codec::xml::Value *> awh::codec::xml::Value::children(const string & local, const string & uri) const noexcept {
	// Собираемый перечень указаний на разысканные узлы
	vector <const Value *> result;
	/**
	 * Выполняем перебор всех вложенных узлов
	 */
	for(auto & item : this->_items){
		/**
		 * Если вложенный узел является узлом разметки с разыскиваемым именем
		 */
		if((item._kind == kind_t::ELEMENT) && (item._local.compare(local) == 0) && (item._uri.compare(uri) == 0))
			// Добавляем указание на разысканный узел в перечень
			result.push_back(&item);
	}
	// Выводим собранный перечень указаний на разысканные узлы
	return result;
}
/**
 * @brief Метод обращения к узлу по пути
 *
 * @param path путь к разыскиваемому узлу
 * @return     ссылка на разысканный узел
 *
 */
const awh::codec::xml::Value & awh::codec::xml::Value::at(const string & path) const noexcept {
	// Перечень звеньев пути к разыскиваемому узлу
	vector <string> parts;
	/**
	 * Если разбор пути на звенья завершился отказом
	 */
	if(!::tokens(path, parts))
		// Выводим значение неопределённое
		return Value::undefined();
	// Ссылка на разыскиваемый узел
	const Value * result = this;
	/**
	 * Выполняем разбор пути звено за звеном
	 */
	for(auto & token : parts){
		// Номер вложенного узла, разыскиваемый звеном пути
		size_t index = 0;
		/**
		 * Если звено пути обращается к вложенному узлу по номеру
		 */
		if(::numbered(token, index)){
			/**
			 * Если вложенного узла с таким номером нет
			 */
			if(index >= result->_items.size())
				// Выводим значение неопределённое
				return Value::undefined();
			// Выполняем переход к вложенному узлу
			result = &result->_items.at(index);
			// Выполняем переход к следующему звену пути
			continue;
		}
		// Номер разыскиваемого узла разметки
		size_t found = result->_items.size();
		/**
		 * Выполняем перебор всех вложенных узлов
		 */
		for(size_t i = 0; i < result->_items.size(); i++){
			/**
			 * Если вложенный узел является узлом разметки с разыскиваемым именем
			 */
			if((result->_items.at(i)._kind == kind_t::ELEMENT) && (result->_items.at(i)._local.compare(token) == 0)){
				// Запоминаем номер разысканного узла разметки
				found = i;
				// Прекращаем перебор вложенных узлов
				break;
			}
		}
		/**
		 * Если узел разметки с таким именем не разыскан
		 */
		if(found >= result->_items.size())
			// Выводим значение неопределённое
			return Value::undefined();
		// Выполняем переход к разысканному узлу разметки
		result = &result->_items.at(found);
	}
	// Выводим ссылку на разысканный узел
	return (* result);
}
/**
 * @brief Метод обращения к узлу по пути с заведением недостающего
 *
 * @param path путь к разыскиваемому узлу
 * @return     ссылка на разысканный либо заведённый узел
 *
 */
awh::codec::xml::Value & awh::codec::xml::Value::place(const string & path) noexcept {
	// Перечень звеньев пути к разыскиваемому узлу
	vector <string> parts;
	/**
	 * Если разбор пути на звенья завершился отказом
	 */
	if(!::tokens(path, parts))
		// Выводим значение мусорное
		return Value::scrap();
	// Ссылка на разыскиваемый узел
	Value * result = this;
	/**
	 * Выполняем разбор пути звено за звеном
	 */
	for(auto & token : parts){
		// Номер вложенного узла, разыскиваемый звеном пути
		size_t index = 0;
		/**
		 * Если звено пути обращается к вложенному узлу по номеру
		 *
		 * @note Звено числовое недостающего не заводит: имени у заводимого узла взять
		 *       неоткуда, а узел разметки без имени разметкой не является вовсе
		 */
		if(::numbered(token, index)){
			/**
			 * Если вложенного узла с таким номером нет
			 */
			if(index >= result->_items.size())
				// Выводим значение мусорное
				return Value::scrap();
			// Выполняем переход к вложенному узлу
			result = &result->_items.at(index);
			// Выполняем переход к следующему звену пути
			continue;
		}
		/**
		 * Если узел ещё не определён
		 */
		if(result->_kind == kind_t::NONE)
			// Устанавливаем вид узла разметкой
			result->_kind = kind_t::ELEMENT;
		/**
		 * Если узел вложенных узлов иметь не может
		 */
		if((result->_kind != kind_t::ELEMENT) && (result->_kind != kind_t::DOCUMENT))
			// Выводим значение мусорное
			return Value::scrap();
		// Номер разыскиваемого узла разметки
		size_t found = result->_items.size();
		/**
		 * Выполняем перебор всех вложенных узлов
		 */
		for(size_t i = 0; i < result->_items.size(); i++){
			/**
			 * Если вложенный узел является узлом разметки с разыскиваемым именем
			 */
			if((result->_items.at(i)._kind == kind_t::ELEMENT) && (result->_items.at(i)._local.compare(token) == 0)){
				// Запоминаем номер разысканного узла разметки
				found = i;
				// Прекращаем перебор вложенных узлов
				break;
			}
		}
		/**
		 * Если узел разметки с таким именем ещё не заведён
		 */
		if(found >= result->_items.size()){
			// Заводим вложенный узел разметки с именем звена пути
			result->_items.push_back(Value(token));
			// Запоминаем номер заведённого узла разметки
			found = (result->_items.size() - 1);
		}
		// Выполняем переход к разысканному либо заведённому узлу разметки
		result = &result->_items.at(found);
	}
	// Выводим ссылку на разысканный либо заведённый узел
	return (* result);
}
/**
 * @brief Метод обращения к вложенному узлу разметки по местному имени
 *
 * @param local местное имя вложенного узла
 * @return      ссылка на первый вложенный узел с таким именем
 *
 */
const awh::codec::xml::Value & awh::codec::xml::Value::operator [] (const string & local) const noexcept {
	/**
	 * Выполняем перебор всех вложенных узлов
	 */
	for(auto & item : this->_items){
		/**
		 * Если вложенный узел является узлом разметки с разыскиваемым именем
		 */
		if((item._kind == kind_t::ELEMENT) && (item._local.compare(local) == 0))
			// Выводим ссылку на разысканный узел разметки
			return item;
	}
	// Выводим значение неопределённое
	return Value::undefined();
}
/**
 * @brief Метод обращения к вложенному узлу разметки по местному имени с заведением недостающего
 *
 * @param local местное имя вложенного узла
 * @return      ссылка на первый вложенный узел с таким именем
 *
 */
awh::codec::xml::Value & awh::codec::xml::Value::operator [] (const string & local) noexcept {
	/**
	 * Если узел ещё не определён
	 */
	if(this->_kind == kind_t::NONE)
		// Устанавливаем вид узла разметкой
		this->_kind = kind_t::ELEMENT;
	/**
	 * Если узел вложенных узлов иметь не может
	 */
	if((this->_kind != kind_t::ELEMENT) && (this->_kind != kind_t::DOCUMENT))
		// Выводим значение мусорное
		return Value::scrap();
	/**
	 * Выполняем перебор всех вложенных узлов
	 */
	for(auto & item : this->_items){
		/**
		 * Если вложенный узел является узлом разметки с разыскиваемым именем
		 */
		if((item._kind == kind_t::ELEMENT) && (item._local.compare(local) == 0))
			// Выводим ссылку на разысканный узел разметки
			return item;
	}
	// Заводим вложенный узел разметки с затребованным именем
	this->_items.push_back(Value(local));
	// Выводим ссылку на заведённый узел разметки
	return this->_items.back();
}
/**
 * @brief Метод обращения к вложенному узлу по номеру
 *
 * @param index номер вложенного узла
 * @return      ссылка на вложенный узел
 *
 */
const awh::codec::xml::Value & awh::codec::xml::Value::operator [] (const size_t index) const noexcept {
	// Выводим ссылку на вложенный узел, если узел с таким номером заведён
	return ((index < this->_items.size()) ? this->_items.at(index) : Value::undefined());
}
/**
 * @brief Метод обращения к вложенному узлу по номеру с заведением недостающего
 *
 * @param index номер вложенного узла
 * @return      ссылка на вложенный узел
 *
 */
awh::codec::xml::Value & awh::codec::xml::Value::operator [] (const size_t index) noexcept {
	/**
	 * Если узел ещё не определён
	 */
	if(this->_kind == kind_t::NONE)
		// Устанавливаем вид узла разметкой
		this->_kind = kind_t::ELEMENT;
	/**
	 * Если вложенный узел с таким номером уже заведён
	 */
	if(index < this->_items.size())
		// Выводим ссылку на вложенный узел
		return this->_items.at(index);
	/**
	 * Если узел вложенных узлов иметь не может
	 */
	if((this->_kind != kind_t::ELEMENT) && (this->_kind != kind_t::DOCUMENT))
		// Выводим значение мусорное
		return Value::scrap();
			/**
		 * Если рост вместилища выйдет за поставленный предел
		 *
		 * @note Номер, пришедший извне, обращается требованием памяти по нему: предел
		 *       этот рост и стережёт. Ставится он пользователем рамки, а нуль снимает
		 *       его вовсе - см. `Value::limit`
		 */
		{
			// Получаем поставленный предел роста вместилища
			const size_t limit = ::LIMIT.load(::std::memory_order_relaxed);
			/**
			 * Если предел поставлен и затребованный номер за него выходит
			 */
			if((limit > 0) && (index >= limit))
			// Выводим значение мусорное
			return Value::scrap();
		}
	/**
	 * Выполняем рост перечня вложенных узлов до затребованного номера
	 */
	while(this->_items.size() <= index)
		// Добавляем в перечень вложенных узлов узел неопределённый
		this->_items.push_back(Value());
	// Выводим ссылку на вложенный узел
	return this->_items.at(index);
}
/**
 * @brief Метод добавления узла в конец перечня вложенных
 *
 * @param value добавляемый узел
 * @return      признак успешности добавления
 *
 */
bool awh::codec::xml::Value::push(const Value & value) noexcept {
	/**
	 * Если узел ещё не определён
	 */
	if(this->_kind == kind_t::NONE)
		// Устанавливаем вид узла разметкой
		this->_kind = kind_t::ELEMENT;
	/**
	 * Если узел вложенных узлов иметь не может
	 */
	if((this->_kind != kind_t::ELEMENT) && (this->_kind != kind_t::DOCUMENT))
		// Выводим признак неудачного добавления
		return false;
	// Добавляем узел в конец перечня вложенных узлов
	this->_items.push_back(value);
	// Выводим признак успешного добавления
	return true;
}
/**
 * @brief Метод установки вложенного узла разметки по местному имени
 *
 * @param local местное имя вложенного узла
 * @param value устанавливаемый узел
 * @return      признак успешности установки
 *
 */
bool awh::codec::xml::Value::insert(const string & local, const Value & value) noexcept {
	/**
	 * Если узел ещё не определён
	 */
	if(this->_kind == kind_t::NONE)
		// Устанавливаем вид узла разметкой
		this->_kind = kind_t::ELEMENT;
	/**
	 * Если узел вложенных узлов иметь не может
	 */
	if((this->_kind != kind_t::ELEMENT) && (this->_kind != kind_t::DOCUMENT))
		// Выводим признак неудачной установки
		return false;
	/**
	 * Выполняем перебор всех вложенных узлов
	 */
	for(size_t i = 0; i < this->_items.size(); i++){
		/**
		 * Если вложенный узел является узлом разметки с устанавливаемым именем
		 */
		if((this->_items.at(i)._kind == kind_t::ELEMENT) && (this->_items.at(i)._local.compare(local) == 0)){
			// Выполняем перезапись вложенного узла на своём месте
			this->_items.at(i) = value;
			// Устанавливаем вид перезаписанного узла разметкой
			this->_items.at(i)._kind = kind_t::ELEMENT;
			// Устанавливаем имя перезаписанного узла затребованным
			this->_items.at(i)._local = local;
			// Выводим признак успешной установки
			return true;
		}
	}
	// Добавляем узел в конец перечня вложенных узлов
	this->_items.push_back(value);
	// Устанавливаем вид добавленного узла разметкой
	this->_items.back()._kind = kind_t::ELEMENT;
	// Устанавливаем имя добавленного узла затребованным
	this->_items.back()._local = local;
	// Выводим признак успешной установки
	return true;
}
/**
 * @brief Метод снятия вложенного узла разметки по местному имени
 *
 * @param local местное имя снимаемого узла
 * @param uri   обозначение пространства имён узла
 * @return      признак успешности снятия
 *
 */
bool awh::codec::xml::Value::erase(const string & local, const string & uri) noexcept {
	/**
	 * Выполняем перебор всех вложенных узлов
	 */
	for(size_t i = 0; i < this->_items.size(); i++){
		/**
		 * Если вложенный узел является узлом разметки со снимаемым именем
		 */
		if((this->_items.at(i)._kind == kind_t::ELEMENT) &&
		   (this->_items.at(i)._local.compare(local) == 0) && (this->_items.at(i)._uri.compare(uri) == 0)){
			// Выполняем снятие вложенного узла
			this->_items.erase(this->_items.begin() + static_cast <ptrdiff_t> (i));
			// Выводим признак успешного снятия
			return true;
		}
	}
	// Выводим признак неудачного снятия
	return false;
}
/**
 * @brief Метод снятия вложенного узла по номеру
 *
 * @param index номер снимаемого узла
 * @return      признак успешности снятия
 *
 */
bool awh::codec::xml::Value::erase(const size_t index) noexcept {
	/**
	 * Если вложенного узла с таким номером нет
	 */
	if(index >= this->_items.size())
		// Выводим признак неудачного снятия
		return false;
	// Выполняем снятие вложенного узла
	this->_items.erase(this->_items.begin() + static_cast <ptrdiff_t> (index));
	// Выводим признак успешного снятия
	return true;
}
/**
 * @brief Метод извлечения логического значения
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(bool & result) const noexcept {
	// Получаем содержимое узла
	const string text = this->text();
	/**
	 * Если содержимое узла записано истиной
	 */
	if((text.compare("true") == 0) || (text.compare("1") == 0)){
		// Устанавливаем извлечённое логическое значение
		result = true;
		// Выводим признак успешного извлечения
		return true;
	}
	/**
	 * Если содержимое узла записано ложью
	 */
	if((text.compare("false") == 0) || (text.compare("0") == 0)){
		// Устанавливаем извлечённое логическое значение
		result = false;
		// Выводим признак успешного извлечения
		return true;
	}
	// Выводим признак неудачного извлечения
	return false;
}
/**
 * @brief Шаблонный метод извлечения числа затребованным видом
 *
 * @details Число разбирается из содержимого узла: своих видов у разметки нет вовсе,
 * всякое содержимое её есть текст. Знаковость и дробность решаются записью, ровно как
 * их решает разбор числа у кодека JSON
 *
 * @tparam T      затребованный вид числа
 * @param  result переменная, куда помещается извлечённое значение
 * @return        признак успешности извлечения
 *
 */
template <typename T>
bool awh::codec::xml::Value::extract(T & result) const noexcept {
	// Получаем содержимое узла
	const string text = this->text();
	/**
	 * Если содержимое узла пусто
	 */
	if(text.empty())
		// Выводим признак неудачного извлечения
		return false;
	// Получаем указатель на конец записи числа
	const char * end = (text.data() + text.size());
	/**
	 * Выполняем поиск знаков, отличающих дробное число от целого
	 *
	 * @note Поиск идёт по записи целиком, а не по одной лишь точке: число `1e3` точки
	 *       не имеет вовсе, а целым тем не менее не является
	 */
	bool real = false;
	/**
	 * Выполняем перебор всех знаков записи числа
	 */
	for(const char letter : text){
		/**
		 * Если знак отличает дробное число от целого
		 */
		if((letter == '.') || (letter == 'e') || (letter == 'E')){
			// Запоминаем принадлежность числа к дробным
			real = true;
			// Прекращаем перебор знаков записи числа
			break;
		}
	}
	/**
	 * Если число является целым
	 */
	if(!real){
		/**
		 * Если число записано со знаком минуса
		 */
		if(text.front() == '-'){
			// Разбираемое целое число со знаком
			int64_t number = 0;
			// Выполняем разбор записи числа
			const lexical_t::result_t <char> res = lexical_t::fromChars(text.data(), end, number);
			/**
			 * Если запись числа целым со знаком не разбирается
			 */
			if(!static_cast <bool> (res) || (res.ptr != end)){
				/**
				 * Если запись числом не является вовсе
				 *
				 * @note Отказ по нехватке разрядности отличим от отказа по составу записи
				 *       кодом причины, и различать их обязательно: запись, целым не
				 *       вместимая, числом быть не перестаёт
				 */
				if((res.error != lexical_t::error_t::OVERFLOW_RANGE) || (res.ptr != end))
					// Выводим признак неудачного извлечения
					return false;
				// Выводим признак успешности извлечения записи числом дробным
				return ::extend <T> (text.data(), end, result);
			}
			// Устанавливаем извлечённое значение приведением языка
			result = static_cast <T> (number);
			// Выводим признак успешного извлечения
			return true;
		}
		// Разбираемое целое число без знака
		uint64_t number = 0;
		// Выполняем разбор записи числа
		const lexical_t::result_t <char> res = lexical_t::fromChars(text.data(), end, number);
		/**
		 * Если запись числа целым без знака не разбирается
		 */
		if(!static_cast <bool> (res) || (res.ptr != end)){
			/**
			 * Если запись числом не является вовсе
			 */
			if((res.error != lexical_t::error_t::OVERFLOW_RANGE) || (res.ptr != end))
				// Выводим признак неудачного извлечения
				return false;
			// Выводим признак успешности извлечения записи числом дробным
			return ::extend <T> (text.data(), end, result);
		}
		// Устанавливаем извлечённое значение приведением языка
		result = static_cast <T> (number);
		// Выводим признак успешного извлечения
		return true;
	}
	// Выводим признак успешности извлечения записи числом дробным
	return ::extend <T> (text.data(), end, result);
}
/**
 * @brief Метод извлечения числа видом `int8_t`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(int8_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения числа видом `int16_t`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(int16_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения числа видом `int32_t`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(int32_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения числа видом `int64_t`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(int64_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения числа видом `uint8_t`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(uint8_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения числа видом `uint16_t`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(uint16_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения числа видом `uint32_t`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(uint32_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения числа видом `uint64_t`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(uint64_t & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения числа видом `float`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(float & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения числа видом `double`
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(double & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения строкового значения
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::xml::Value::value(string & result) const noexcept {
	/**
	 * Если узел ещё не определён
	 */
	if(this->_kind == kind_t::NONE)
		// Выводим признак неудачного извлечения
		return false;
	// Устанавливаем извлечённое строковое значение
	result = this->text();
	// Выводим признак успешного извлечения
	return true;
}
/**
 * @brief Метод записи значения в поток записи
 *
 * @param writer поток записи, куда ложится значение
 * @return       признак успешности записи
 *
 */
bool awh::codec::xml::Value::compose(writer_t & writer) const noexcept {
	/**
	 * Определяем вид хранимого узла
	 */
	switch(static_cast <uint8_t> (this->_kind)){
		/**
		 * Если узел является корнем дерева
		 */
		case static_cast <uint8_t> (kind_t::DOCUMENT): {
			/**
			 * Выполняем перебор всех вложенных узлов
			 */
			for(auto & item : this->_items){
				/**
				 * Если запись очередного вложенного узла завершилась отказом
				 */
				if(!item.compose(writer))
					// Выводим признак неуспешности записи
					return false;
			}
			// Выводим признак успешности записи
			return true;
		}
		/**
		 * Если узел является узлом разметки
		 */
		case static_cast <uint8_t> (kind_t::ELEMENT): {
			/**
			 * Собираемые связывания префиксов, объявляемые самим узлом
			 *
			 * @note Отрезки указывают на память самого значения и живут дольше записи:
			 *       копии здесь были бы платой ни за что
			 */
			vector <binding_t> declares;
			// Выполняем выделение памяти под связывания префиксов
			declares.reserve(this->_bindings.size());
			/**
			 * Выполняем перебор всех связываний префиксов узла
			 */
			for(auto & item : this->_bindings){
				// Заводим связывание префикса
				declares.emplace_back();
				// Устанавливаем объявляемый префикс
				declares.back().prefix = string_view(item.prefix);
				// Устанавливаем обозначение пространства имён
				declares.back().uri = string_view(item.uri);
			}
			/**
			 * Если открытие узла разметки завершилось отказом
			 *
			 * @note Объявления и желаемый префикс подаются вместе с именем намеренно:
			 *       метка узла собирается разом, и объявить связывание после открытия
			 *       значит опоздать - поток записи к тому времени уже назначил бы префикс
			 *       сам, а поданное следом объявление легло бы в метку вторым
			 */
			if(!writer.open(this->_local, this->_uri, declares, this->_prefix))
				// Выводим признак неуспешности записи
				return false;
			/**
			 * Выполняем перебор всех свойств узла
			 */
			for(auto & item : this->_attributes){
				/**
				 * Если запись свойства узла завершилась отказом
				 */
				if(!writer.attribute(item.local, item.value, item.uri))
					// Выводим признак неуспешности записи
					return false;
			}
			/**
			 * Выполняем перебор всех вложенных узлов
			 */
			for(auto & item : this->_items){
				/**
				 * Если запись очередного вложенного узла завершилась отказом
				 */
				if(!item.compose(writer))
					// Выводим признак неуспешности записи
					return false;
			}
			// Выводим признак успешности закрытия узла разметки
			return writer.close();
		}
		// Если узел является текстовым содержимым либо пробельным
		case static_cast <uint8_t> (kind_t::TEXT):
		case static_cast <uint8_t> (kind_t::SPACE):
			// Выводим признак успешности записи текстового содержимого
			return writer.text(this->_text);
		// Если узел является дословным разделом
		case static_cast <uint8_t> (kind_t::CDATA):
			// Выводим признак успешности записи дословного раздела
			return writer.cdata(this->_text);
		// Если узел является примечанием
		case static_cast <uint8_t> (kind_t::COMMENT):
			// Выводим признак успешности записи примечания
			return writer.comment(this->_text);
		// Если узел является указанием обработчику
		case static_cast <uint8_t> (kind_t::PROCESSING):
			// Выводим признак успешности записи указания обработчику
			return writer.processing(this->_local, this->_text);
		/**
		 * Если узел является описанием типа документа
		 *
		 * @note Записи описания типа документа поток записи не имеет вовсе: описание это
		 *       принадлежит тексту исходному, а собранный нами текст своего описания не
		 *       требует. Узел такой пропускается молча
		 */
		case static_cast <uint8_t> (kind_t::DOCTYPE):
			// Выводим признак успешности записи
			return true;
	}
	// Выводим признак неуспешности записи узла неопознанного вида
	return false;
}
/**
 * @brief Метод снятия значения с узла дерева разметки
 *
 * @param node узел дерева разметки
 *
 */
void awh::codec::xml::Value::absorb(const node_t & node) noexcept {
	// Выполняем очистку прежнего содержимого значения
	this->clear();
	/**
	 * Если узел дерева разметки недействителен
	 */
	if(!node.valid())
		// Выходим из метода, оставляя значение неопределённым
		return;
	// Устанавливаем вид хранимого узла
	this->_kind = node.kind();
	// Получаем имя узла дерева разметки
	const name_t name = node.name();
	// Выполняем снятие префикса пространства имён собственной памятью
	this->_prefix.assign(name.prefix.data(), name.prefix.size());
	// Выполняем снятие местного имени узла собственной памятью
	this->_local.assign(name.local.data(), name.local.size());
	// Выполняем снятие обозначения пространства имён собственной памятью
	this->_uri.assign(name.uri.data(), name.uri.size());
	/**
	 * Определяем вид хранимого узла
	 */
	switch(static_cast <uint8_t> (this->_kind)){
		// Если узел владеет содержимым своим
		case static_cast <uint8_t> (kind_t::TEXT):
		case static_cast <uint8_t> (kind_t::CDATA):
		case static_cast <uint8_t> (kind_t::SPACE):
		case static_cast <uint8_t> (kind_t::COMMENT):
		case static_cast <uint8_t> (kind_t::PROCESSING):
		case static_cast <uint8_t> (kind_t::DOCTYPE):
			// Выполняем снятие собственного содержимого узла
			this->_text = node.text();
		break;
		/**
		 * Если узел является узлом разметки
		 */
		case static_cast <uint8_t> (kind_t::ELEMENT): {
			// Получаем свойства узла дерева разметки
			const vector <attribute_t> attributes = node.attributes();
			// Выполняем выделение памяти под свойства узла
			this->_attributes.reserve(attributes.size());
			/**
			 * Выполняем перебор всех свойств узла дерева разметки
			 */
			for(auto & item : attributes){
				// Заводим свойство узла разметки
				this->_attributes.emplace_back();
				// Выполняем снятие префикса пространства имён свойства собственной памятью
				this->_attributes.back().prefix.assign(item.name.prefix.data(), item.name.prefix.size());
				// Выполняем снятие местного имени свойства собственной памятью
				this->_attributes.back().local.assign(item.name.local.data(), item.name.local.size());
				// Выполняем снятие обозначения пространства имён свойства собственной памятью
				this->_attributes.back().uri.assign(item.name.uri.data(), item.name.uri.size());
				// Выполняем снятие значения свойства собственной памятью
				this->_attributes.back().value.assign(item.value.data(), item.value.size());
			}
			// Получаем связывания префиксов узла дерева разметки
			const vector <binding_t> bindings = node.bindings();
			// Выполняем выделение памяти под связывания префиксов
			this->_bindings.reserve(bindings.size());
			/**
			 * Выполняем перебор всех связываний префиксов узла дерева разметки
			 */
			for(auto & item : bindings){
				// Заводим связывание префикса
				this->_bindings.emplace_back();
				// Выполняем снятие префикса собственной памятью
				this->_bindings.back().prefix.assign(item.prefix.data(), item.prefix.size());
				// Выполняем снятие обозначения пространства имён собственной памятью
				this->_bindings.back().uri.assign(item.uri.data(), item.uri.size());
			}
		} break;
	}
	// Количество вложенных узлов дерева разметки
	size_t count = 0;
	/**
	 * Выполняем подсчёт всех вложенных узлов дерева разметки
	 *
	 * @details Перечень отводится сразу под всё содержимое, а не растёт удвоением:
	 *          рост удвоением переносит уже снятые значения с места на место, и на
	 *          узле с многими детьми переносы эти складываются в объём, кратный
	 *          самому перечню. Замер на документе в 4.6 МБ показал двадцать
	 *          перевыделений корневого перечня и 23 МБ переноса сверх нужного
	 *
	 * @note Проход этот идёт по ссылкам соседей и памяти не берёт вовсе, тогда как
	 *       устраняет он именно выделения памяти
	 */
	for(node_t item = node.first(); item.valid(); item = item.next())
		// Увеличиваем количество вложенных узлов
		count++;
	// Выполняем отвод места под всё вложенное содержимое сразу
	this->_items.reserve(count);
	/**
	 * Выполняем перебор всех вложенных узлов дерева разметки
	 */
	for(node_t item = node.first(); item.valid(); item = item.next()){
		// Добавляем в перечень вложенных узлов узел неопределённый
		this->_items.push_back(Value());
		// Выполняем снятие очередного вложенного узла
		this->_items.back().absorb(item);
	}
}
/**
 * @brief Метод разбора текста разметки во владеющее значение
 *
 * @param text разбираемый текст разметки
 * @return     признак успешности разбора
 *
 */
bool awh::codec::xml::Value::parse(const string & text) noexcept {
	// Дерево разметки, разбирающее поданный текст
	document_t document;
	/**
	 * Если разбор текста разметки завершился отказом
	 */
	if(!document.parse(text)){
		// Выполняем очистку прежнего содержимого значения
		this->clear();
		// Выводим признак неудачного разбора
		return false;
	}
	// Выполняем снятие разобранного дерева собственной памятью
	this->absorb(document.root());
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод разбора текста разметки из файла
 *
 * @param filename адрес разбираемого файла
 * @return         признак успешности разбора
 *
 */
bool awh::codec::xml::Value::load(const string & filename) noexcept {
	// Открываем файл разметки для чтения
	ifstream file(filename, ios::binary);
	/**
	 * Если файл разметки открыть не удалось
	 */
	if(!file.is_open()){
		// Выполняем очистку прежнего содержимого значения
		this->clear();
		// Выводим признак неудачного разбора
		return false;
	}
	// Собираемый текст разметки
	string text;
	// Хранилище куска читаемого файла
	string chunk(::CHUNK, 0);
	/**
	 * Выполняем чтение файла разметки кусками
	 */
	while(file.read(chunk.data(), static_cast <streamsize> (chunk.size())) || (file.gcount() > 0))
		// Добавляем прочитанный кусок файла к собираемому тексту
		text.append(chunk.data(), static_cast <size_t> (file.gcount()));
	// Выводим признак успешности разбора собранного текста
	return this->parse(text);
}
/**
 * @brief Метод перезаписи значения в текст разметки
 *
 * @param format вид записи собираемого текста
 * @return       текст разметки, пустой - записать значение не удалось
 *
 */
string awh::codec::xml::Value::dump(const format_t format) const noexcept {
	// Настройки записи текста разметки
	writer_t::settings_t settings;
	// Устанавливаем затребованный вид записи собираемого текста
	settings.format = format;
	// Выводим собранный текст значения
	return this->dump(settings);
}
/**
 * @brief Метод перезаписи значения в текст разметки с указанными настройками
 *
 * @param settings настройки записи текста
 * @return         текст разметки, пустой - записать значение не удалось
 *
 */
string awh::codec::xml::Value::dump(const writer_t::settings_t & settings) const noexcept {
	// Поток записи текста разметки
	writer_t writer;
	// Выполняем установку настроек записи текста
	writer.settings(settings);
	/**
	 * Если запись значения в поток записи завершилась отказом
	 */
	if(!this->compose(writer))
		// Выводим пустой текст значения
		return string();
	// Выводим собранный текст значения
	return writer.text();
}
/**
 * @brief Метод записи значения в файл
 *
 * @param filename адрес записываемого файла
 * @param format   вид записи собираемого текста
 * @return         признак успешности записи
 *
 */
bool awh::codec::xml::Value::save(const string & filename, const format_t format) const noexcept {
	// Получаем собранный текст значения
	const string text = this->dump(format);
	/**
	 * Если собрать текст значения не удалось
	 */
	if(text.empty())
		// Выводим признак неудачной записи
		return false;
	// Открываем файл разметки для записи
	ofstream file(filename, ios::binary | ios::trunc);
	/**
	 * Если файл разметки открыть не удалось
	 */
	if(!file.is_open())
		// Выводим признак неудачной записи
		return false;
	// Выполняем запись текста значения в файл
	file.write(text.data(), static_cast <streamsize> (text.size()));
	// Выводим признак успешности записи
	return static_cast <bool> (file);
}
/**
 * @brief Метод сличения значений
 *
 * @param value сличаемое значение
 * @return      признак совпадения значений
 *
 */
bool awh::codec::xml::Value::operator == (const Value & value) const noexcept {
	/**
	 * Если виды сличаемых узлов разнятся
	 */
	if(this->_kind != value._kind)
		// Выводим признак несовпадения значений
		return false;
	/**
	 * Если имена сличаемых узлов разнятся
	 *
	 * @note Префикс сличению не подлежит: одно и то же имя, записанное разными
	 *       префиксами, есть одно и то же имя
	 */
	if((this->_local.compare(value._local) != 0) || (this->_uri.compare(value._uri) != 0))
		// Выводим признак несовпадения значений
		return false;
	/**
	 * Если собственное содержимое сличаемых узлов разнится
	 */
	if(this->_text.compare(value._text) != 0)
		// Выводим признак несовпадения значений
		return false;
	/**
	 * Если количество свойств сличаемых узлов разнится
	 */
	if(this->_attributes.size() != value._attributes.size())
		// Выводим признак несовпадения значений
		return false;
	/**
	 * Выполняем перебор всех свойств узла
	 *
	 * @note Порядок свойств сличению не подлежит: свойства узла суть набор, и порядок
	 *       записи их описание разметки не предписывает вовсе
	 */
	for(auto & item : this->_attributes){
		/**
		 * Если свойства с таким именем у сличаемого узла нет
		 */
		if(!value.has(item.local, item.uri))
			// Выводим признак несовпадения значений
			return false;
		/**
		 * Если значения свойств сличаемых узлов разнятся
		 */
		if(value.attribute(item.local, item.uri).compare(item.value) != 0)
			// Выводим признак несовпадения значений
			return false;
	}
	/**
	 * Если количество вложенных узлов разнится
	 */
	if(this->_items.size() != value._items.size())
		// Выводим признак несовпадения значений
		return false;
	/**
	 * Выполняем перебор всех вложенных узлов
	 *
	 * @note Порядок вложенных узлов сличению подлежит: содержимое разметки определено
	 *       порядком своим, и два узла, разнящиеся порядком детей, разнятся по сути
	 */
	for(size_t i = 0; i < this->_items.size(); i++){
		/**
		 * Если очередные вложенные узлы разнятся
		 */
		if(this->_items.at(i) != value._items.at(i))
			// Выводим признак несовпадения значений
			return false;
	}
	// Выводим признак совпадения значений
	return true;
}
/**
 * @brief Метод сличения значений на несовпадение
 *
 * @param value сличаемое значение
 * @return      признак несовпадения значений
 *
 */
bool awh::codec::xml::Value::operator != (const Value & value) const noexcept {
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
awh::codec::xml::Value & awh::codec::xml::Value::operator = (const Value & value) noexcept {
	/**
	 * Если значение присваивается само себе
	 */
	if(&value == this)
		// Выводим ссылку на текущее значение
		return (* this);
	// Выполняем копирование вида хранимого узла
	this->_kind = value._kind;
	// Выполняем копирование префикса пространства имён
	this->_prefix = value._prefix;
	// Выполняем копирование местного имени узла
	this->_local = value._local;
	// Выполняем копирование обозначения пространства имён
	this->_uri = value._uri;
	// Выполняем копирование собственного содержимого узла
	this->_text = value._text;
	// Выполняем копирование свойств узла
	this->_attributes = value._attributes;
	// Выполняем копирование связываний префиксов
	this->_bindings = value._bindings;
	// Выполняем копирование вложенных узлов
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
awh::codec::xml::Value & awh::codec::xml::Value::operator = (Value && value) noexcept {
	/**
	 * Если значение присваивается само себе
	 */
	if(&value == this)
		// Выводим ссылку на текущее значение
		return (* this);
	// Выполняем перенос вида хранимого узла
	this->_kind = value._kind;
	// Выполняем перенос префикса пространства имён
	this->_prefix = ::std::move(value._prefix);
	// Выполняем перенос местного имени узла
	this->_local = ::std::move(value._local);
	// Выполняем перенос обозначения пространства имён
	this->_uri = ::std::move(value._uri);
	// Выполняем перенос собственного содержимого узла
	this->_text = ::std::move(value._text);
	// Выполняем перенос свойств узла
	this->_attributes = ::std::move(value._attributes);
	// Выполняем перенос связываний префиксов
	this->_bindings = ::std::move(value._bindings);
	// Выполняем перенос вложенных узлов
	this->_items = ::std::move(value._items);
	// Выполняем очистку значения, у какого содержимое отобрано
	value.clear();
	// Выводим ссылку на текущее значение
	return (* this);
}
/**
 * @brief Конструктор
 *
 */
awh::codec::xml::Value::Value() noexcept : _kind(kind_t::NONE) {}
/**
 * @brief Конструктор узла указанного вида
 *
 * @param kind вид заводимого узла
 *
 */
awh::codec::xml::Value::Value(const kind_t kind) noexcept : _kind(kind) {}
/**
 * @brief Конструктор узла разметки с именем
 *
 * @param local  местное имя заводимого узла
 * @param uri    обозначение пространства имён узла
 * @param prefix префикс пространства имён узла
 *
 */
awh::codec::xml::Value::Value(const string & local, const string & uri, const string & prefix) noexcept :
 _kind(kind_t::ELEMENT), _prefix(prefix), _local(local), _uri(uri) {}
/**
 * @brief Конструктор узла указанного вида с содержимым
 *
 * @param kind вид заводимого узла
 * @param text содержимое заводимого узла
 *
 */
awh::codec::xml::Value::Value(const kind_t kind, const string & text) noexcept : _kind(kind), _text(text) {}
/**
 * @brief Конструктор снятия значения с узла дерева разметки
 *
 * @param node узел дерева разметки
 *
 */
awh::codec::xml::Value::Value(const node_t & node) noexcept : _kind(kind_t::NONE) {
	// Выполняем снятие значения с узла дерева разметки
	this->absorb(node);
}
/**
 * @brief Конструктор копирования
 *
 * @param value копируемое значение
 *
 */
awh::codec::xml::Value::Value(const Value & value) noexcept :
 _kind(value._kind), _prefix(value._prefix), _local(value._local), _uri(value._uri),
 _text(value._text), _attributes(value._attributes), _bindings(value._bindings), _items(value._items) {}
/**
 * @brief Конструктор переноса
 *
 * @param value переносимое значение
 *
 */
awh::codec::xml::Value::Value(Value && value) noexcept :
 _kind(value._kind), _prefix(::std::move(value._prefix)), _local(::std::move(value._local)),
 _uri(::std::move(value._uri)), _text(::std::move(value._text)),
 _attributes(::std::move(value._attributes)), _bindings(::std::move(value._bindings)),
 _items(::std::move(value._items)) {
	// Выполняем очистку значения, у какого содержимое отобрано
	value.clear();
}
/**
 * @brief Метод помещения собранного узла на своё место
 *
 * @param value помещаемый узел
 * @return      указание на помещённый узел, ноль - помещение не удалось
 *
 */
awh::codec::xml::value_t * awh::codec::xml::Builder::attach(const value_t & value) noexcept {
	/**
	 * Если ни одного узла ещё не открыто
	 */
	if(this->_nesting.empty()){
		/**
		 * Если собираемое значение ещё не заведено
		 *
		 * @note Собираемое заводится корнем дерева: разметка одного корня требует, а
		 *       сборщик волен собрать перед ним и примечание, и указание обработчику
		 */
		if(!this->_result.valid())
			// Заводим собираемое значение корнем дерева
			this->_result = value_t(kind_t::DOCUMENT);
		/**
		 * Если добавление узла к корню дерева завершилось отказом
		 */
		if(!this->_result.push(value))
			// Выводим признак неудачного помещения
			return nullptr;
		// Выводим указание на помещённый узел
		return &this->_result[this->_result.size() - 1];
	}
	// Получаем узел, открытый последним
	value_t * parent = this->_nesting.back();
	/**
	 * Если добавление узла к открытому узлу завершилось отказом
	 */
	if(!parent->push(value))
		// Выводим признак неудачного помещения
		return nullptr;
	// Выводим указание на помещённый узел
	return &(* parent)[parent->size() - 1];
}
/**
 * @brief Метод открытия узла разметки
 *
 * @param local  местное имя открываемого узла
 * @param uri    обозначение пространства имён узла
 * @param prefix префикс пространства имён узла
 * @return       признак успешности открытия
 *
 */
bool awh::codec::xml::Builder::open(const string & local, const string & uri, const string & prefix) noexcept {
	/**
	 * Если местное имя открываемого узла пусто
	 */
	if(local.empty())
		// Выводим признак неудачного открытия
		return false;
	// Выполняем помещение узла разметки на своё место
	value_t * result = this->attach(value_t(local, uri, prefix));
	/**
	 * Если помещение узла разметки завершилось отказом
	 */
	if(result == nullptr)
		// Выводим признак неудачного открытия
		return false;
	// Добавляем открытый узел разметки в стек открытых узлов
	this->_nesting.push_back(result);
	// Выводим признак успешного открытия
	return true;
}
/**
 * @brief Метод закрытия открытого узла разметки
 *
 * @return признак успешности закрытия
 *
 */
bool awh::codec::xml::Builder::close() noexcept {
	/**
	 * Если ни одного узла не открыто
	 */
	if(this->_nesting.empty())
		// Выводим признак неудачного закрытия
		return false;
	// Выполняем снятие узла разметки со стека открытых узлов
	this->_nesting.pop_back();
	// Выводим признак успешного закрытия
	return true;
}
/**
 * @brief Метод объявления свойства открытого узла разметки
 *
 * @param local  местное имя свойства
 * @param value  значение свойства
 * @param uri    обозначение пространства имён свойства
 * @param prefix префикс пространства имён свойства
 * @return       признак успешности объявления
 *
 */
bool awh::codec::xml::Builder::attribute(const string & local, const string & value, const string & uri, const string & prefix) noexcept {
	/**
	 * Если ни одного узла не открыто
	 *
	 * @note Свойство принадлежит узлу, а не сборке: объявить его в пустоте нельзя
	 */
	if(this->_nesting.empty())
		// Выводим признак неудачного объявления
		return false;
	// Выводим признак успешности установки свойства открытого узла разметки
	return this->_nesting.back()->attribute(local, value, uri, prefix);
}
/**
 * @brief Метод объявления связывания префикса открытым узлом разметки
 *
 * @param prefix объявляемый префикс, пустой - объявление по умолчанию
 * @param uri    обозначение пространства имён
 * @return       признак успешности объявления
 *
 */
bool awh::codec::xml::Builder::binding(const string & prefix, const string & uri) noexcept {
	/**
	 * Если ни одного узла не открыто
	 */
	if(this->_nesting.empty())
		// Выводим признак неудачного объявления
		return false;
	// Выводим признак успешности объявления связывания префикса открытым узлом
	return this->_nesting.back()->binding(prefix, uri);
}
/**
 * @brief Метод записи текстового содержимого
 *
 * @param text записываемое содержимое
 * @return     признак успешности записи
 *
 */
bool awh::codec::xml::Builder::text(const string & text) noexcept {
	// Выводим признак успешности записи текстового содержимого
	return (this->attach(value_t(kind_t::TEXT, text)) != nullptr);
}
/**
 * @brief Метод записи дословного раздела
 *
 * @param text записываемое содержимое
 * @return     признак успешности записи
 *
 */
bool awh::codec::xml::Builder::cdata(const string & text) noexcept {
	// Выводим признак успешности записи дословного раздела
	return (this->attach(value_t(kind_t::CDATA, text)) != nullptr);
}
/**
 * @brief Метод записи примечания
 *
 * @param text записываемое содержимое
 * @return     признак успешности записи
 *
 */
bool awh::codec::xml::Builder::comment(const string & text) noexcept {
	// Выводим признак успешности записи примечания
	return (this->attach(value_t(kind_t::COMMENT, text)) != nullptr);
}
/**
 * @brief Метод записи указания обработчику
 *
 * @param target цель указания обработчику
 * @param text   содержимое указания обработчику
 * @return       признак успешности записи
 *
 */
bool awh::codec::xml::Builder::processing(const string & target, const string & text) noexcept {
	/**
	 * Если цель указания обработчику пуста
	 */
	if(target.empty())
		// Выводим признак неудачной записи
		return false;
	// Заводим узел указания обработчику
	value_t value(kind_t::PROCESSING, text);
	/**
	 * Устанавливаем цель указания обработчику
	 *
	 * @note Цель хранится местным именем узла: своего поля под неё не заведено, ибо
	 *       имени у указания обработчику нет, а цель его именем и является
	 */
	value.name(target);
	// Выводим признак успешности записи указания обработчику
	return (this->attach(value) != nullptr);
}
/**
 * @brief Метод записи готового узла
 *
 * @param value записываемый узел
 * @return      признак успешности записи
 *
 */
bool awh::codec::xml::Builder::value(const value_t & value) noexcept {
	// Выводим признак успешности записи готового узла
	return (this->attach(value) != nullptr);
}
/**
 * @brief Метод извлечения текущей глубины вложенности
 *
 * @return количество открытых и ещё не закрытых узлов
 *
 */
size_t awh::codec::xml::Builder::depth() const noexcept {
	// Выводим количество открытых и ещё не закрытых узлов
	return this->_nesting.size();
}
/**
 * @brief Метод сброса состояния сборки
 *
 */
void awh::codec::xml::Builder::reset() noexcept {
	// Выполняем очистку собираемого значения
	this->_result.clear();
	// Выполняем очистку стека открытых узлов
	this->_nesting.clear();
}
/**
 * @brief Метод завершения сборки и изъятия собранного значения
 *
 * @return собранное значение
 *
 */
awh::codec::xml::value_t awh::codec::xml::Builder::finish() noexcept {
	// Изымаем собранное значение
	value_t result = ::std::move(this->_result);
	// Выполняем сброс состояния сборки
	this->reset();
	/**
	 * Если собранное значение содержит ровно один узел
	 *
	 * @note Корень дерева заводится сборкой всегда, а нужен он лишь тогда, когда узлов у
	 *       сборки несколько: собравшему один узел разметки корень ни к чему
	 */
	if((result.kind() == kind_t::DOCUMENT) && (result.size() == 1))
		// Выводим единственный собранный узел
		return result[static_cast <size_t> (0)];
	// Выводим собранное значение
	return result;
}
/**
 * @brief Метод переноса владеющего значения в арену дерева
 *
 * @param value  переносимое владеющее значение
 * @param parent индекс родительского узла переносимого значения
 * @return       индекс заведённого узла либо признак недействительности
 *
 */
awh::codec::xml::node_id_t awh::codec::xml::Document::transplant(const xml::Value & value, const node_id_t parent) noexcept {
	/**
	 * Если переносимое значение является корнем дерева
	 *
	 * @details Корень вмещает содержимое текста целиком и узлом разметки не является
	 * вовсе: узлом внутри дерева ему не бывать, ибо родителя корень не имеет по
	 * устройству. Записи наружу изъян этот не торчал - запись корень пропускает,
	 * выводя одно содержимое его, - зато обход дерева отдавал бы корень ребёнком
	 *
	 * @note Проверка стоит в переносе, а не в прививке, и оттого стережёт и вложенное
	 *       содержимое: значение, собранное вручную, вправе нести корень и внутри себя
	 *
	 */
	if(value.kind() == kind_t::DOCUMENT)
		// Выводим признак недействительности заведённого узла
		return INVALID_NODE;
	// Признак выхода дерева за отведённый ему предел
	bool overflow = false;
	/**
	 * @brief Метод размещения последовательности знаков в хранилище дерева
	 *
	 * @param text размещаемая последовательность знаков
	 * @return     отрезок общего хранилища знаков
	 *
	 */
	auto store = [this, &overflow](const string & text) noexcept -> span_t {
		/**
		 * Если размещение выведет хранилище знаков за предел
		 *
		 * @note Отрезок хранилища задан положением в четыре байта, и выход за предел
		 *       усёк бы положение молча, оставив дерево с отрезками, указывающими не туда
		 */
		if((this->_storage.size() + text.length()) > STORAGE_LIMIT){
			// Запоминаем, что дерево вышло за отведённый ему предел
			overflow = true;
			// Выводим пустой отрезок общего хранилища знаков
			return span_t();
		}
		// Собираемый отрезок общего хранилища знаков
		const span_t result(static_cast <uint32_t> (this->_storage.size()), static_cast <uint32_t> (text.length()));
		// Выполняем размещение последовательности знаков в хранилище
		this->_storage.append(text);
		// Выводим собранный отрезок общего хранилища знаков
		return result;
	};
	// Получаем индекс заводимого узла дерева разметки
	const node_id_t result = static_cast <node_id_t> (this->_nodes.size());
	/**
	 * Если арена узлов дерева вышла за отведённый ей предел
	 */
	if(static_cast <size_t> (result) >= static_cast <size_t> (INVALID_NODE))
		// Выводим признак недействительности заведённого узла
		return INVALID_NODE;
	// Выполняем заведение записи узла в арене дерева разметки
	this->_nodes.emplace_back();
	// Устанавливаем вид заводимого узла дерева разметки
	this->_nodes.at(result).kind = value.kind();
	// Устанавливаем индекс родительского узла
	this->_nodes.at(result).parent = parent;
	// Выполняем размещение префикса имени узла
	this->_nodes.at(result).name.prefix = store(value.prefix());
	// Выполняем размещение местного имени узла
	this->_nodes.at(result).name.local = store(value.local());
	// Выполняем размещение обозначения пространства имён узла
	this->_nodes.at(result).name.uri = store(value.uri());
	/**
	 * Если узел разметки содержимого не несёт
	 *
	 * @note Содержимое узла разметки лежит вложенными узлами, а собственное содержимое
	 *       есть у текстовых узлов, примечаний и указаний обработчику
	 */
	if(value.kind() != kind_t::ELEMENT)
		// Выполняем размещение содержимого узла
		this->_nodes.at(result).value = store(value.text());
	/**
	 * Выполняем размещение атрибутов узла отрезком подряд
	 *
	 * @note Размещаются они прежде обхода вложенного содержимого намеренно: отрезок задан
	 *       началом и количеством, и атрибуты вложенных узлов, размещённые посреди,
	 *       разорвали бы отрезок надвое
	 */
	{
		// Устанавливаем индекс первого атрибута узла в хранилище атрибутов
		this->_nodes.at(result).attribute = static_cast <uint32_t> (this->_attributes.size());
		/**
		 * Выполняем перебор всех атрибутов переносимого значения
		 */
		for(auto & item : value.attributes()){
			/**
			 * Если добавление выведет количество атрибутов за предел
			 *
			 * @note Отрезок атрибутов узла задан началом и количеством в четыре байта,
			 *       и выход за предел усёк бы их так же молча, как и отрезок хранилища
			 */
			if(this->_attributes.size() >= STORAGE_LIMIT)
				// Выводим признак недействительности заведённого узла
				return INVALID_NODE;
			// Выполняем заведение записи атрибута в хранилище атрибутов
			this->_attributes.emplace_back();
			// Выполняем размещение префикса имени атрибута
			this->_attributes.back().name.prefix = store(item.prefix);
			// Выполняем размещение местного имени атрибута
			this->_attributes.back().name.local = store(item.local);
			// Выполняем размещение обозначения пространства имён атрибута
			this->_attributes.back().name.uri = store(item.uri);
			// Выполняем размещение значения атрибута
			this->_attributes.back().value = store(item.value);
		}
		// Устанавливаем количество атрибутов узла
		this->_nodes.at(result).attributes = static_cast <uint32_t> (this->_attributes.size() - this->_nodes.at(result).attribute);
	}
	/**
	 * Если переносимое значение объявляет пространства имён
	 */
	if(!value.bindings().empty()){
		// Получаем индекс первого связывания префикса в хранилище связываний
		const uint32_t offset = static_cast <uint32_t> (this->_scopes.size());
		/**
		 * Выполняем перебор всех связываний префиксов переносимого значения
		 */
		for(auto & item : value.bindings()){
			/**
			 * Если добавление выведет количество связываний префиксов за предел
			 */
			if(this->_scopes.size() >= STORAGE_LIMIT)
				// Выводим признак недействительности заведённого узла
				return INVALID_NODE;
			// Выполняем заведение записи связывания префикса в хранилище связываний
			this->_scopes.emplace_back();
			// Выполняем размещение префикса связывания
			this->_scopes.back().prefix = store(item.prefix);
			// Выполняем размещение обозначения объявляемого пространства имён
			this->_scopes.back().uri = store(item.uri);
		}
		// Выполняем привязку отрезка связываний к заведённому узлу
		this->_scoped.emplace(result, span_t(offset, static_cast <uint32_t> (this->_scopes.size() - offset)));
	}
	/**
	 * Если размещение вывело дерево за отведённый ему предел
	 *
	 * @note Проверка стоит после размещения всего собственного содержимого узла - имени,
	 *       текста, атрибутов и объявлений: хранилище пополняется всеми ими, и одна
	 *       проверка наперёд стерегла бы лишь часть
	 */
	if(overflow)
		// Выводим признак недействительности заведённого узла
		return INVALID_NODE;
	/**
	 * Выполняем перебор всего вложенного содержимого переносимого значения
	 */
	for(size_t i = 0; i < value.size(); i++){
		// Выполняем перенос вложенного содержимого в арену дерева разметки
		const node_id_t child = this->transplant(value[i], result);
		/**
		 * Если перенос вложенного содержимого завершился отказом
		 */
		if(child == INVALID_NODE)
			// Выводим признак недействительности заведённого узла
			return INVALID_NODE;
		/**
		 * Если вложенное содержимое у узла первое
		 */
		if(this->_nodes.at(result).first == INVALID_NODE)
			// Устанавливаем индекс первого вложенного узла
			this->_nodes.at(result).first = child;
		/**
		 * Если вложенное содержимое у узла не первое
		 */
		else {
			// Устанавливаем индекс следующего узла того же уровня у предыдущего соседа
			this->_nodes.at(this->_nodes.at(result).last).next = child;
			// Устанавливаем индекс предыдущего узла того же уровня у заведённого узла
			this->_nodes.at(child).prev = this->_nodes.at(result).last;
		}
		// Устанавливаем индекс последнего вложенного узла
		this->_nodes.at(result).last = child;
	}
	// Выводим индекс заведённого узла дерева разметки
	return result;
}
/**
 * @brief Метод прививки владеющего значения в дерево разметки
 *
 * @param path  путь к прививаемому месту
 * @param value прививаемое владеющее значение
 * @return      признак успешности прививки
 *
 */
bool awh::codec::xml::Document::graft(const string & path, const xml::Value & value) noexcept {
	// Перечень звеньев пути к прививаемому месту
	vector <string> parts;
	/**
	 * Если разбор пути на звенья завершился отказом
	 */
	if(!::tokens(path, parts))
		// Выводим признак неудачной прививки
		return false;
	/**
	 * Если прививаемое значение недействительно либо дерево разметки пусто
	 */
	if(!value.valid() || this->_nodes.empty())
		// Выводим признак неудачной прививки
		return false;
	/**
	 * Если путь к прививаемому месту пуст
	 *
	 * @note Корень дерева узлом разметки не является вовсе, и стать им прививаемому
	 *       значению неоткуда: заменить его значило бы отбросить дерево целиком
	 */
	if(parts.empty())
		// Выводим признак неудачной прививки
		return false;
	// Индекс узла, куда прививается значение
	node_id_t target = 0;
	/**
	 * Выполняем разбор пути звено за звеном
	 */
	for(auto & token : parts){
		// Номер вложенного узла, разыскиваемый звеном пути
		size_t index = 0;
		// Индекс разыскиваемого узла дерева разметки
		node_id_t found = INVALID_NODE;
		/**
		 * Если звено пути обращается к вложенному узлу по номеру
		 */
		if(::numbered(token, index)){
			// Устанавливаем индекс первого вложенного узла
			found = this->_nodes.at(target).first;
			/**
			 * Выполняем пропуск вложенных узлов до разыскиваемого
			 */
			for(size_t i = 0; ((i < index) && (found != INVALID_NODE)); i++)
				// Выполняем переход к следующему вложенному узлу
				found = this->_nodes.at(found).next;
		/**
		 * Если звено пути обращается к узлу разметки по местному имени
		 */
		} else {
			/**
			 * Выполняем перебор всех вложенных узлов
			 */
			for(node_id_t child = this->_nodes.at(target).first; child != INVALID_NODE; child = this->_nodes.at(child).next){
				/**
				 * Если вложенный узел является узлом разметки с разыскиваемым именем
				 */
				if((this->_nodes.at(child).kind == kind_t::ELEMENT) &&
				   (this->get(this->_nodes.at(child).name.local).compare(token) == 0)){
					// Запоминаем индекс разысканного узла разметки
					found = child;
					// Прекращаем перебор вложенных узлов
					break;
				}
			}
		}
		/**
		 * Если узел, звеном пути разыскиваемый, не разыскан
		 */
		if(found == INVALID_NODE)
			// Выводим признак неудачной прививки
			return false;
		// Выполняем переход к разысканному узлу
		target = found;
	}
	// Получаем индекс родительского узла прививаемого места
	const node_id_t parent = this->_nodes.at(target).parent;
	/**
	 * Если прививаемое место родителя не имеет вовсе
	 *
	 * @note Родителя не имеет один лишь корень дерева, а его прививка отвергнута выше
	 */
	if(parent == INVALID_NODE)
		// Выводим признак неудачной прививки
		return false;
	/**
	 * Выполняем перенос прививаемого значения в арену дерева разметки
	 *
	 * @note Значение, несущее корень дерева с единственным узлом, прививается узлом
	 *       этим: разбор текста во владеющее значение корень заводит всегда, и правило
	 *       это то же самое, каким сборка отдаёт собранный узел без корня над ним.
	 *       Корень с иным числом узлов отвергается переносом - одним узлом ему не стать
	 */
	const node_id_t graft = this->transplant(((value.kind() == kind_t::DOCUMENT) && (value.size() == 1)) ?
		value[static_cast <size_t> (0)] : value, parent);
	/**
	 * Если перенос прививаемого значения завершился отказом
	 */
	if(graft == INVALID_NODE)
		// Выводим признак неудачной прививки
		return false;
	// Получаем индексы соседей заменяемого узла
	const node_id_t prev = this->_nodes.at(target).prev, next = this->_nodes.at(target).next;
	// Устанавливаем индекс предыдущего узла того же уровня
	this->_nodes.at(graft).prev = prev;
	// Устанавливаем индекс следующего узла того же уровня
	this->_nodes.at(graft).next = next;
	/**
	 * Если предыдущий сосед у заменяемого узла есть
	 */
	if(prev != INVALID_NODE)
		// Устанавливаем индекс следующего узла того же уровня у предыдущего соседа
		this->_nodes.at(prev).next = graft;
	// Если предыдущего соседа у заменяемого узла нет, привитый узел становится первым
	else this->_nodes.at(parent).first = graft;
	/**
	 * Если следующий сосед у заменяемого узла есть
	 */
	if(next != INVALID_NODE)
		// Устанавливаем индекс предыдущего узла того же уровня у следующего соседа
		this->_nodes.at(next).prev = graft;
	// Если следующего соседа у заменяемого узла нет, привитый узел становится последним
	else this->_nodes.at(parent).last = graft;
	/**
	 * Выполняем отвязку заменённого узла от дерева
	 *
	 * @note Узлы заменённого поддерева остаются в арене недостижимыми: перенумерование
	 *       их обесценило бы всякую ссылку на дерево, выданную наружу прежде
	 */
	this->_nodes.at(target).parent = INVALID_NODE;
	// Устанавливаем отсутствие предыдущего узла того же уровня у заменённого узла
	this->_nodes.at(target).prev = INVALID_NODE;
	// Устанавливаем отсутствие следующего узла того же уровня у заменённого узла
	this->_nodes.at(target).next = INVALID_NODE;
	// Выводим признак успешной прививки
	return true;
}
