/**
 * @file document.cpp
 * @date 2026-08-14
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
 * @brief Реализация документа JSON — сборка дерева по событиям разбора, обход его,
 *        извлечение значений и перезапись в текст
 *
 * \~english
 * @brief Implementation of a JSON document — the assembly of the tree by the events of the parsing, its traversal,
 *        the extraction of the values and the rewriting into a text
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
#include <vector>
#include <limits>
#include <fstream>
#include <algorithm>
#include <type_traits>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <num/lexical/lexical.hpp>
#include <codec/json/document.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Безымянное пространство имён вспомогательных объявлений документа
 *
 */
namespace {
	/**
	 * Размер куска, каким читается файл документа
	 */
	static constexpr size_t CHUNK = 0x10000;
	/**
	 * Запас памяти под перечень узлов дерева документа
	 *
	 * @note Величины запаса взяты замером: сверх достаточного они выигрыша уже не дают,
	 *       ибо выигрыш этот - в отсутствии роста, а не в объёме взятой памяти
	 */
	static constexpr size_t NODES = 32;
	/**
	 * Запас памяти под хранилище знаков всех строк и имён документа
	 */
	static constexpr size_t STORAGE = 256;
	/**
	 * Запас памяти под стек номеров узлов открытых вместилищ
	 */
	static constexpr size_t NESTING = 16;
	/**
	 * Запас памяти под перечень имён полей разбираемого объекта
	 */
	static constexpr size_t NAMING = 16;
	/**
	 * @brief Шаблонная функция приведения дробного числа к затребованному виду
	 *
	 * @details Приведение отвечает языку: дробная часть отбрасывается усечением к нулю.
	 * Разница с `static_cast` одна - дробное, чья целая часть лежит за пределами
	 * затребованного целого вида, выдаётся пределом этого вида
	 *
	 * @note Стандарт зовёт такое приведение неопределённым поведением, а неопределённого
	 *       поведения в кодеке не будет: значение `1e300`, затребованное видом `int32_t`,
	 *       обязано выдать хоть что-нибудь, а не разрушить работу приложения
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
		 *
		 * @note Пределы сличаются дробным видом, а не целым: предел `int64_t` целым видом
		 *       точно не представим дробным, и сличение целых дало бы промах на единицу
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
		/**
		 * Выводим приведённое число, округлив дробную часть
		 *
		 * @note Округление ведётся по правилам математики с уводом половины ОТ НУЛЯ:
		 *       «1.5» даёт 2, «-1.5» даёт -2, «1.4» даёт 1. Усечение к нулю, стоявшее
		 *       здесь прежде, отдавало «1.5» единицей, расходясь с обиходным счётом
		 * @warning Округление стоит ПОСЛЕ сличения с пределами вида нарочно: округлить
		 *          прежде значило бы приводить к целому виду число, ему не отвечающее,
		 *          а это поведение неопределённое
		 */
		return static_cast <T> (::round(value));
	}

}

/**
 * @brief Конструктор
 *
 */
void awh::codec::json::Document::setLogger(const log_t * log) noexcept {
	// Устанавливаем объект ведения журнала работы
	this->_log = log;
	// Выполняем установку объекта ведения журнала хранимому чтению
	this->_reader.setLogger(log);
}
/**
 * @brief Конструктор
 *
 * @param log объект ведения журнала работы
 *
 */
awh::codec::json::Document::Document(const log_t * log) noexcept : _reader(log), _error(error_t::NONE), _log(log), _named(0), _keyed(false), _completed(false), _pointer(0), _base(0), _callback(nullptr) {
	/**
	 * Выполняем заведение запаса памяти под сборку дерева документа
	 *
	 * @details Запас берётся единожды на объект, а не по мере надобности: без него всякое
	 * вместилище растёт с нуля, а рост этот на малом документе стоит нескольких обращений
	 * к куче с перекладыванием прежнего содержимого. Разбор мелких документов — а таковых
	 * у служб большинство — почти целиком из этого роста и состоит
	 *
	 * @note Запас переживает сброс: вместилища очищаются, но память свою удерживают, и
	 *       второй разобранный тем же объектом документ не платит уже ничего
	 *
	 */
	this->_nodes.reserve(NODES);
	this->_storage.reserve(STORAGE);
	this->_nesting.reserve(NESTING);
	this->_naming.reserve(NAMING);
}
/**
 * @brief Метод проверки наличия поля объекта с указанным именем
 *
 * @param name разыскиваемое имя поля объекта
 * @return     признак наличия поля объекта
 *
 */
bool awh::codec::json::Document::Value::contains(const string & name) const noexcept {
	// Выводим признак наличия поля объекта
	return (* this)[name].valid();
}
/**
 * @brief Метод обращения к полю объекта по имени
 *
 * @details Мелкие объекты разыскиваются перебором детей, а крупные - отображением
 * имён в номера узлов, заводимым по требованию. Перебор мелкого объекта дешевле
 * заведения отображения, и большинство объектов таковы
 *
 * @param name имя поля объекта
 * @return     ссылка на узел поля объекта
 *
 */
awh::codec::json::Document::Value awh::codec::json::Document::Value::operator [] (const string & name) const noexcept {
	/**
	 * Если ссылка недействительна либо узел объектом не является
	 */
	if(!this->valid() || (this->_doc->_nodes[this->_index].type != type_t::OBJECT))
		// Выводим недействительную ссылку
		return Value();
	// Получаем узел, на какой указывает ссылка
	const node_t & node = this->_doc->_nodes[this->_index];
	// Получаем номер узла за последним узлом объекта
	const uint32_t bound = (this->_index + node.extent());
	/**
	 * Если количество полей объекта превышает порог заведения отображения
	 */
	if(node.length() > INDEX_THRESHOLD){
		// Выполняем поиск отображения имён полей объекта
		auto i = this->_doc->_index.find(this->_index);
		/**
		 * Если отображение имён полей объекта ещё не заведено
		 */
		if(i == this->_doc->_index.end()){
			// Заводимое отображение имён полей объекта
			unordered_map <string_view, uint32_t> index;
			// Выполняем выделение памяти под отображение имён полей объекта
			index.reserve(static_cast <size_t> (node.length()));
			/**
			 * Выполняем перебор всех полей объекта
			 */
			for(uint32_t child = (this->_index + 1); child < bound; child += this->_doc->_nodes[child].extent()){
				// Получаем узел очередного поля объекта
				const node_t & item = this->_doc->_nodes[child];
				// Добавляем имя поля объекта в отображение, если оно ещё не занято
				index.emplace(string_view(this->_doc->_storage.data() + (item.offset - item.named), item.named), child);
			}
			// Запоминаем заведённое отображение имён полей объекта
			i = this->_doc->_index.emplace(this->_index, ::std::move(index)).first;
		}
		// Выполняем поиск имени поля объекта в отображении
		auto j = i->second.find(string_view(name));
		// Выводим ссылку на узел поля объекта, если имя поля разыскано
		return ((j != i->second.end()) ? Value(this->_doc, j->second, bound) : Value());
	}
	/**
	 * Выполняем перебор всех полей объекта
	 */
	for(uint32_t child = (this->_index + 1); child < bound; child += this->_doc->_nodes[child].extent()){
		// Получаем узел очередного поля объекта
		const node_t & item = this->_doc->_nodes[child];
		/**
		 * Если имя поля объекта совпадает с разыскиваемым
		 */
		if(string_view(this->_doc->_storage.data() + (item.offset - item.named), item.named) == string_view(name))
			// Выводим ссылку на узел поля объекта
			return Value(this->_doc, child, bound);
	}
	// Выводим недействительную ссылку
	return Value();
}
/**
 * @brief Метод обращения к значению вместилища по номеру
 *
 * @param index номер значения во вместилище
 * @return      ссылка на узел значения
 *
 */
awh::codec::json::Document::Value awh::codec::json::Document::Value::operator [] (const size_t index) const noexcept {
	/**
	 * Если ссылка недействительна
	 */
	if(!this->valid())
		// Выводим недействительную ссылку
		return Value();
	// Получаем узел, на какой указывает ссылка
	const node_t & node = this->_doc->_nodes[this->_index];
	/**
	 * Если узел вместилищем не является либо значения с таким номером у него нет
	 */
	if((!node.nested()) || (index >= static_cast <size_t> (node.length())))
		// Выводим недействительную ссылку
		return Value();
	// Получаем номер узла за последним узлом вместилища
	const uint32_t bound = (this->_index + node.extent());
	// Номер разыскиваемого узла вместилища
	uint32_t child = (this->_index + 1);
	/**
	 * Выполняем пропуск значений вместилища до разыскиваемого
	 *
	 * @note Пропуск вложенного вместилища стоит одного сложения: размер поддерева
	 *       узел несёт сам
	 */
	for(size_t i = 0; i < index; i++)
		// Выполняем переход к следующему значению вместилища
		child += this->_doc->_nodes[child].extent();
	// Выводим ссылку на узел значения вместилища
	return Value(this->_doc, child, bound);
}
/**
 * @brief Метод обращения к значению по указателю JSON Pointer
 *
 * @details Указатель записывается по RFC 6901: `/response/users/0/id`. Отменяющие
 * записи `~1` и `~0` снимаются, а пустой указатель отдаёт сам узел
 *
 * @param pointer указатель на значение по RFC 6901
 * @return        ссылка на узел значения
 *
 */
awh::codec::json::Document::Value awh::codec::json::Document::Value::at(const string & pointer) const noexcept {
	/**
	 * Если указатель пуст
	 */
	if(pointer.empty())
		// Выводим ссылку на сам узел
		return (* this);
	/**
	 * Если указатель не начинается с косой черты
	 */
	if(pointer.front() != '/')
		// Выводим недействительную ссылку
		return Value();
	// Ссылка на узел, разыскиваемый указателем
	Value result = (* this);
	// Положение разбираемого знака указателя
	size_t offset = 1;
	/**
	 * Выполняем разбор указателя звено за звеном
	 */
	while(offset <= pointer.size()){
		/**
		 * Если ссылка на узел уже недействительна
		 */
		if(!result.valid())
			// Выводим недействительную ссылку
			return Value();
		// Выполняем поиск конца очередного звена указателя
		const size_t end = pointer.find('/', offset);
		// Получаем содержимое очередного звена указателя
		string token = pointer.substr(offset, ((end == string::npos) ? string::npos : (end - offset)));
		// Выполняем переход к следующему звену указателя
		offset = ((end == string::npos) ? (pointer.size() + 1) : (end + 1));
		/**
		 * Выполняем снятие отменяющих записей звена указателя
		 *
		 * @note Порядок снятия предписан стандартом: снятие `~0` прежде `~1`
		 *       обратило бы записанное `~01` в косую черту вместо `~1`
		 */
		for(size_t i = token.find('~'); i != string::npos; i = token.find('~', i + 1)){
			/**
			 * Если за знаком отмены не осталось знаков
			 */
			if((i + 1) >= token.size())
				// Выводим недействительную ссылку
				return Value();
			/**
			 * Определяем отменяющую запись звена указателя
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
					// Выводим недействительную ссылку
					return Value();
			}
		}
		/**
		 * Если звено указателя обращается к значению массива
		 */
		if(result.kind() == kind_t::ARRAY){
			/**
			 * Если содержимое звена номером значения не является
			 */
			if(token.empty() || ((token.size() > 1) && (token.front() == '0')))
				// Выводим недействительную ссылку
				return Value();
			// Номер значения массива, разыскиваемый звеном указателя
			size_t index = 0;
			/**
			 * Выполняем разбор номера значения массива
			 */
			for(const char letter : token){
				/**
				 * Если знак цифрой не является
				 */
				if((letter < '0') || (letter > '9'))
					// Выводим недействительную ссылку
					return Value();
				// Добавляем разряд к номеру значения массива
				index = ((index * 10) + static_cast <size_t> (letter - '0'));
			}
			// Выполняем обращение к значению массива по номеру
			result = result[index];
		/**
		 * Если звено указателя обращается к полю объекта
		 */
		} else result = result[token];
	}
	// Выводим ссылку на разысканный узел
	return result;
}
/**
 * @brief Метод извлечения логического значения
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::json::Document::Value::value(bool & result) const noexcept {
	/**
	 * Если узел логическим значением не является
	 */
	if(this->kind() != kind_t::BOOL)
		// Выводим признак неудачного извлечения
		return false;
	// Устанавливаем извлечённое логическое значение
	result = (this->_doc->_nodes[this->_index].length() == 4);
	// Выводим признак успешного извлечения
	return true;
}
/**
 * @brief Шаблонный метод извлечения числа затребованным видом
 *
 * @details Извлечение сличает само значение с пределами затребованного вида, а не вид
 * хранения с видом затребованным: узел, хранящий `INT8`, извлекается и как `double`, и
 * как `uint64_t`. Отказом извлечение завершается лишь тогда, когда узел числом не
 * является вовсе
 *
 * @note Дробное, чья целая часть лежит за пределами затребованного целого вида, выдаётся
 *       пределом этого вида: стандарт зовёт такое приведение неопределённым поведением,
 *       а неопределённого поведения в кодеке не будет
 *
 * @tparam T      затребованный вид числа
 * @param  result переменная, куда помещается извлечённое значение
 * @return        признак успешности извлечения
 *
 */
template <typename T>
bool awh::codec::json::Document::Value::extract(T & result) const noexcept {
	/**
	 * Если ссылка недействительна
	 */
	if(!this->valid())
		// Выводим признак неудачного извлечения
		return false;
	// Получаем узел, на какой указывает ссылка
	const node_t & node = this->_doc->_nodes[this->_index];
	/**
	 * Определяем вид значения узла документа
	 */
	switch(static_cast <uint16_t> (node.type)){
		// Если значение является целым со знаком шириною в один байт
		case static_cast <uint16_t> (type_t::INT8):
		// Если значение является целым со знаком шириною в два байта
		case static_cast <uint16_t> (type_t::INT16):
		// Если значение является целым со знаком шириною в четыре байта
		case static_cast <uint16_t> (type_t::INT32):
		// Если значение является целым со знаком шириною в восемь байтов
		case static_cast <uint16_t> (type_t::INT64):
			// Устанавливаем извлечённое значение приведением языка
			result = static_cast <T> (node.number <int64_t> ());
		break;
		// Если значение является целым без знака шириною в один байт
		case static_cast <uint16_t> (type_t::UINT8):
		// Если значение является целым без знака шириною в два байта
		case static_cast <uint16_t> (type_t::UINT16):
		// Если значение является целым без знака шириною в четыре байта
		case static_cast <uint16_t> (type_t::UINT32):
		// Если значение является целым без знака шириною в восемь байтов
		case static_cast <uint16_t> (type_t::UINT64):
			// Устанавливаем извлечённое значение приведением языка
			result = static_cast <T> (node.number <uint64_t> ());
		break;
		// Если значение является дробным одинарной точности
		case static_cast <uint16_t> (type_t::FLOAT):
			// Устанавливаем извлечённое значение приведением дробного
			result = ::convert <T> (static_cast <double> (node.number <float> ()));
		break;
		// Если значение является дробным двойной точности
		case static_cast <uint16_t> (type_t::DOUBLE):
			// Устанавливаем извлечённое значение приведением дробного
			result = ::convert <T> (node.number <double> ());
		break;
		/**
		 * Если значение является числом, не вместимым ни в один родной вид
		 */
		case static_cast <uint16_t> (type_t::EXTENDED): {
			// Получаем запись числа, хранимую узлом
			const string_view text(this->_doc->_storage.data() + node.offset, node.length());
			// Разбираемое дробное число
			double number = 0.;
			/**
			 * Выполняем разбор записи числа
			 *
			 * @note Разбор здесь неизбежен: число это в родной вид не вместилось, оттого
			 *       и хранится записью. Таких чисел на документ приходятся единицы
			 */
			lexical_t::fromChars(text.data(), (text.data() + text.size()), number);
			// Устанавливаем извлечённое значение приведением дробного
			result = ::convert <T> (number);
		} break;
		/**
		 * Если значение числом не является вовсе
		 */
		default:
			// Выводим признак неудачного извлечения
			return false;
	}
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
bool awh::codec::json::Document::Value::value(int8_t & result) const noexcept {
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
bool awh::codec::json::Document::Value::value(int16_t & result) const noexcept {
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
bool awh::codec::json::Document::Value::value(int32_t & result) const noexcept {
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
bool awh::codec::json::Document::Value::value(int64_t & result) const noexcept {
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
bool awh::codec::json::Document::Value::value(uint8_t & result) const noexcept {
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
bool awh::codec::json::Document::Value::value(uint16_t & result) const noexcept {
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
bool awh::codec::json::Document::Value::value(uint32_t & result) const noexcept {
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
bool awh::codec::json::Document::Value::value(uint64_t & result) const noexcept {
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
bool awh::codec::json::Document::Value::value(float & result) const noexcept {
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
bool awh::codec::json::Document::Value::value(double & result) const noexcept {
	// Выводим признак успешности извлечения числа
	return this->extract(result);
}
/**
 * @brief Метод извлечения записи числа
 *
 * @return запись числа, пусто у прочих узлов
 *
 */
string awh::codec::json::Document::Value::raw() const noexcept {
	/**
	 * Если ссылка недействительна
	 */
	if(!this->valid())
		// Выводим отсутствие записи числа
		return string();
	// Получаем узел, на какой указывает ссылка
	const node_t & node = this->_doc->_nodes[this->_index];
	/**
	 * Если число хранится записью своей
	 */
	if(node.type == type_t::EXTENDED)
		// Выводим запись числа, как она стояла в тексте
		return string(this->_doc->_storage.data() + node.offset, node.length());
	/**
	 * Если узел числом не является
	 */
	if(!this->is(type_t::NUMBER))
		// Выводим отсутствие записи числа
		return string();
	// Объект записи текста документа
	writer_t writer(this->_doc->_log);
	// Выполняем запись числа, хранимого узлом
	this->_doc->compose(writer, node);
	// Выводим собранную запись числа
	return writer.take();
}
/**
 * @brief Метод записи числа, хранимого узлом
 *
 * @details Запись собирается кратчайшей записью, читающейся обратно тем же самым
 * числом. Метод этот один на перезапись документа и на выдачу записи числа: две
 * отдельные записи одного и того же числа неминуемо разошлись бы видом
 *
 * @param writer объект записи текста документа
 * @param node   узел, число какого записывается
 *
 */
void awh::codec::json::Document::compose(writer_t & writer, const node_t & node) const noexcept {
	/**
	 * Определяем вид значения узла документа
	 */
	switch(static_cast <uint16_t> (node.type)){
		// Если значение является целым со знаком любой ширины
		case static_cast <uint16_t> (type_t::INT8):
		case static_cast <uint16_t> (type_t::INT16):
		case static_cast <uint16_t> (type_t::INT32):
		case static_cast <uint16_t> (type_t::INT64):
			// Выполняем запись целого числа со знаком
			writer.value(node.number <int64_t> ());
		break;
		// Если значение является целым без знака любой ширины
		case static_cast <uint16_t> (type_t::UINT8):
		case static_cast <uint16_t> (type_t::UINT16):
		case static_cast <uint16_t> (type_t::UINT32):
		case static_cast <uint16_t> (type_t::UINT64):
			// Выполняем запись целого числа без знака
			writer.value(node.number <uint64_t> ());
		break;
		// Если значение является дробным одинарной точности
		case static_cast <uint16_t> (type_t::FLOAT):
			// Выполняем запись дробного числа одинарной точности
			writer.value(static_cast <double> (node.number <float> ()));
		break;
		// Если значение является дробным двойной точности
		case static_cast <uint16_t> (type_t::DOUBLE):
			// Выполняем запись дробного числа двойной точности
			writer.value(node.number <double> ());
		break;
		/**
		 * Если значение является числом, не вместимым ни в один родной вид
		 */
		case static_cast <uint16_t> (type_t::EXTENDED):
			// Выполняем запись числа его записью, как она стояла в тексте
			writer.raw(string(this->_storage.data() + node.offset, node.length()));
		break;
	}
}
bool awh::codec::json::Document::Value::value(string & result) const noexcept {
	/**
	 * Если узел строкой не является
	 */
	if(this->kind() != kind_t::STRING)
		// Выводим признак неудачного извлечения
		return false;
	// Получаем строковое значение узла
	const string_view text = this->text();
	// Устанавливаем извлечённое строковое значение
	result.assign(text.data(), text.size());
	// Выводим признак успешного извлечения
	return true;
}
/**
 * @brief Метод извлечения строкового значения без копирования
 *
 * @return строковое значение, пусто у прочих узлов
 *
 */
string_view awh::codec::json::Document::Value::text() const noexcept {
	/**
	 * Если узел строкой не является
	 */
	if(this->kind() != kind_t::STRING)
		// Выводим отсутствие строкового значения
		return string_view();
	// Получаем узел, на какой указывает ссылка
	const node_t & node = this->_doc->_nodes[this->_index];
	// Выводим строковое значение узла
	return string_view(this->_doc->_storage.data() + node.offset, node.length());
}
/**
 * @brief Метод переноса знаков разбора в хранилище документа
 *
 * @param reader объект потокового чтения текста
 *
 */
void awh::codec::json::Document::transfer(const reader_t & reader) noexcept {
	// Получаем количество байтов, выброшенных из хранилища знаков разбора
	const uint64_t origin = reader.origin();
	// Получаем хранилище знаков разбора
	const string & storage = reader.storage();
	/**
	 * Если хранилище документа отстаёт от хранилища разбора
	 */
	if((this->_base + this->_storage.size()) < (origin + storage.size())){
		// Получаем количество уже перенесённых знаков хранилища разбора
		const size_t taken = static_cast <size_t> ((this->_base + this->_storage.size()) - origin);
		// Выполняем перенос оставшихся знаков хранилища разбора
		this->_storage.append(storage.data() + taken, storage.size() - taken);
	}
}
/**
 * @brief Метод приёма события разбора, выданного прямо из чтения
 *
 * @param context указание на документ, собирающий дерево
 * @param reader  объект потокового чтения текста
 *
 */
void awh::codec::json::Document::handler(void * context, reader_t & reader, const event_t event, const span_t content, const bool modified) noexcept {
	// Получаем документ, собирающий дерево
	Document * self = reinterpret_cast <Document *> (context);
	/**
	 * Если сборка дерева по очередному событию разбора завершилась отказом
	 */
	if(!self->digest(reader, event, content, modified))
		/**
		 * Выполняем прекращение разбора
		 *
		 * @note Возвращать отказ обработчику некуда, а подача текста обязана
		 *       прекратиться немедля: причина отказа уже записана документом
		 */
		reader.abort();
}
/**
 * @brief Метод сборки дерева по очередному событию разбора
 *
 * @details Дерево собирается сплошным перечнем узлов: очередной узел ложится в
 * конец перечня, отчего дети оказываются сразу за родителем сами собою. Размер
 * поддерева проставляется вместилищу при закрытии его, когда все дети уже легли
 *
 * @param reader объект потокового чтения текста
 * @return       признак успешности сборки
 *
 */
/**
 * @brief Метод определения вида числа вместе с преобразованием его
 *
 * @details Вид выбирается самый узкий из вмещающих: число `1` получает вид `UINT8`, а
 * `-1` - вид `INT8`. Знаковость решается знаком записи, а не величиной: запись без
 * минуса есть число без знака, и потребитель, спросивший `is(type_t::UNSIGNED)`,
 * получает ответ по записи, какую видел сам
 *
 * @note Дробное получает вид `FLOAT` тогда, и только тогда, когда одинарной точности
 *       довольно для точного его представления. Проверяется это обращением туда и
 *       обратно, а не количеством знаков записи: `0.5` представимо точно, а `0.1` - нет
 *
 * @param text разбираемая запись числа
 * @param node узел документа, куда помещается разобранное число
 * @return     признак того, что число вместилось в родной вид
 *
 */
bool awh::codec::json::Document::classify(const string_view text, node_t & node) noexcept {
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
		if(!text.empty() && (text.front() == '-')){
			// Разбираемое целое число со знаком
			int64_t result = 0;
			// Выполняем разбор записи числа
			const lexical_t::result_t <char> res = lexical_t::fromChars(text.data(), end, result);
			/**
			 * Если запись числа целым со знаком не разбирается
			 */
			if(!static_cast <bool> (res) || (res.ptr != end))
				// Выводим признак того, что число в родной вид не вместилось
				return false;
			// Выполняем установку разобранного числа
			node.number(result);
			/**
			 * Устанавливаем самый узкий из вмещающих видов числа
			 */
			node.type = (
				((result >= INT8_MIN) && (result <= INT8_MAX)) ? type_t::INT8 : (
					((result >= INT16_MIN) && (result <= INT16_MAX)) ? type_t::INT16 : (
						((result >= INT32_MIN) && (result <= INT32_MAX)) ? type_t::INT32 : type_t::INT64
					)
				)
			);
			// Выводим признак того, что число вместилось в родной вид
			return true;
		}
		// Разбираемое целое число без знака
		uint64_t result = 0;
		// Выполняем разбор записи числа
		const lexical_t::result_t <char> res = lexical_t::fromChars(text.data(), end, result);
		/**
		 * Если запись числа целым без знака не разбирается
		 */
		if(!static_cast <bool> (res) || (res.ptr != end))
			// Выводим признак того, что число в родной вид не вместилось
			return false;
		// Выполняем установку разобранного числа
		node.number(result);
		/**
		 * Устанавливаем самый узкий из вмещающих видов числа
		 */
		node.type = (
			(result <= UINT8_MAX) ? type_t::UINT8 : (
				(result <= UINT16_MAX) ? type_t::UINT16 : (
					(result <= UINT32_MAX) ? type_t::UINT32 : type_t::UINT64
				)
			)
		);
		// Выводим признак того, что число вместилось в родной вид
		return true;
	}
	// Разбираемое дробное число
	double result = 0.;
	// Выполняем разбор записи числа
	const lexical_t::result_t <char> res = lexical_t::fromChars(text.data(), end, result);
	/**
	 * Если запись числа дробным не разбирается либо число вышло за предел двойной точности
	 *
	 * @note Бесконечность родным видом не является: записать её обратно нельзя вовсе,
	 *       ибо стандарт бесконечности не знает. Такое число хранится записью своей
	 */
	if(!static_cast <bool> (res) || (res.ptr != end) || ::isinf(result))
		// Выводим признак того, что число в родной вид не вместилось
		return false;
	/**
	 * Если одинарной точности довольно для точного представления числа
	 */
	if(static_cast <double> (static_cast <float> (result)) == result){
		// Выполняем установку разобранного числа одинарной точностью
		node.number(static_cast <float> (result));
		// Устанавливаем вид числа одинарной точности
		node.type = type_t::FLOAT;
		// Выводим признак того, что число вместилось в родной вид
		return true;
	}
	// Выполняем установку разобранного числа
	node.number(result);
	// Устанавливаем вид числа двойной точности
	node.type = type_t::DOUBLE;
	// Выводим признак того, что число вместилось в родной вид
	return true;
}
/**
 * @brief Метод сборки дерева документа из события разбора
 */
bool awh::codec::json::Document::digest(reader_t & reader, const event_t event, const span_t content, const bool modified) noexcept {
	/**
	 * Если событие является примечанием
	 */
	if(event == event_t::COMMENT)
		// Выводим признак успешной сборки
		return true;
	/**
	 * Если событие является исчерпанием подаваемого текста
	 */
	if(event == event_t::FINISH)
		// Выводим признак успешной сборки
		return true;
	/**
	 * Если событие является окончанием документа
	 */
	if(event == event_t::DOCUMENT){
		/**
		 * Если выдача значений ведётся потоком
		 */
		if((this->_callback != nullptr) && (* this->_callback)){
			// Выполняем перенос знаков разбора в хранилище документа
			this->transfer(reader);
			/**
			 * Если обработчик потребовал прекращения разбора
			 */
			if(!(* this->_callback)(this->root()))
				// Выводим признак неудачной сборки
				return false;
			// Выполняем очистку перечня узлов документа
			this->_nodes.clear();
			// Сдвигаем сквозное положение первого знака хранилища документа
			this->_base += static_cast <uint64_t> (this->_storage.size());
			// Выполняем очистку хранилища знаков документа
			this->_storage.clear();
			// Выполняем очистку отображения имён полей в номера узлов
			this->_index.clear();
		/**
		 * Если документ потока собран не первым
		 *
		 * @details Поток несёт документы один за другим, а дерево вмещает один. Прежде
		 * второй документ уходил в никуда молча: узлы его ложились за корнем
		 * недостижимыми, выдача текста отдавала первый документ, и разбор отвечал успехом
		 *
		 * @note Отказ выносится лишь при разборе БЕЗ обработчика потоковой выдачи:
		 *       с обработчиком поток разбирается как поток, и всякий документ его
		 *       отдаётся потребителю целым
		 */
		} else if(this->_completed){
			// Запоминаем код отказа разбора
			this->_error = error_t::TRAILING_CHARACTERS;
			// Выводим признак неудачной сборки
			return false;
		// Запоминаем признак собранного целиком документа
		} else this->_completed = true;
		// Выводим признак успешной сборки
		return true;
	}
	/**
	 * Получаем сквозное положение содержимого события в потоке разобранных знаков
	 *
	 * @details Знаки события уже лежат в хранилище разбора готовыми, и переносить
	 * их к себе по одному значению незачем: сквозное положение позволяет заводить
	 * узел сразу, а хранилище перенести целым куском по исчерпании событий
	 *
	 * @note Перенос по одному значению стоил половины всего времени сборки дерева.
	 * Обнаружено разложением стоимости по частям
	 */
	const uint64_t position = (reader.origin() + static_cast <uint64_t> (content.offset));
	/**
	 * Если сквозное положение содержимого выходит за предел хранилища знаков
	 *
	 * @note Хранилище знаков ограничено четырьмя гигабайтами: смещение в нём
	 *       занимает четыре байта, и содержимое за этой границей указать нечем
	 */
	/**
	 * @warning Ограда эта набором НЕ СЛИЧАЕТСЯ и сличена быть не может дёшево: добраться до
	 *          неё требует четырёх гигабайтов исходного текста в одном разборе. Молчание
	 *          карты покрытия здесь означает НЕПРОВЕРЕННОСТЬ, а не исправность, и всякая
	 *          правка счёта сквозных положений обязана перечитывать эту ветвь глазами
	 */
	if(((position - this->_base) + static_cast <uint64_t> (content.length)) > NO_OFFSET){
		// Запоминаем код отказа разбора
		this->_error = error_t::OVERFLOW_LIMIT;
		// Выводим признак неудачной сборки
		return false;
	}
	/**
	 * Если событие является именем поля объекта
	 */
	if(event == event_t::KEY){
		// Запоминаем длину имени поля объекта
		this->_named = content.length;
		// Запоминаем сквозное положение конца имени поля объекта
		this->_pointer = (position + static_cast <uint64_t> (content.length));
		// Устанавливаем признак разбора имени поля объекта
		this->_keyed = true;
		// Выводим признак успешной сборки
		return true;
	}
	/**
	 * Если количество узлов документа превышает допустимое
	 */
	/**
	 * @warning Ограда эта набором НЕ СЛИЧАЕТСЯ, и молчание карты покрытия здесь означает
	 *          НЕПРОВЕРЕННОСТЬ, а не исправность
	 *
	 * @note Причина не в количестве узлов: предел составляет 67 миллионов, и добраться до
	 *       него настоящим текстом стоит 128 МБ разметки и 1.7 секунды разбора. Мешает
	 *       расход памяти - 2.7 ГБ наибольшего размещения, - а отладочные стенды такого
	 *       не выдержат: у стенда Windows памяти 8 ГБ, у гостей BSD и того меньше
	 * @note Проверена щупом порознь 22.08.2026: разбор отвечает отказом `TOO_MANY_NODES`
	 *       на 67108880 узлах. Прежде здесь было сказано про четыре миллиарда узлов -
	 *       число ошибочное в 64 раза, и вывод о несличимости покоился на нём
	 */
	if(this->_nodes.size() >= MAX_NODES){
		// Запоминаем код отказа разбора
		this->_error = error_t::TOO_MANY_NODES;
		// Выводим признак неудачной сборки
		return false;
	}
	// Получаем номер заводимого узла документа
	const uint32_t index = static_cast <uint32_t> (this->_nodes.size());
	// Заводимый узел документа
	node_t node;
	// Устанавливаем длину имени поля объекта
	node.named = this->_named;
	// Устанавливаем признак принадлежности узла объекту
	node.keyed = this->_keyed;
	/**
	 * Устанавливаем смещение содержимого узла в хранилище знаков
	 *
	 * @note У вместилища своего содержимого нет, и указание события пусто: смещением
	 *       ему служит конец имени поля, за каким содержимому и лежать бы
	 */
	node.offset = static_cast <uint32_t> ((((event == event_t::OBJECT_BEGIN) || (event == event_t::ARRAY_BEGIN)) ? this->_pointer : position) - this->_base);
	// Сбрасываем длину имени поля объекта
	this->_named = 0;
	// Сбрасываем сквозное положение конца имени поля объекта
	this->_pointer = 0;
	// Снимаем признак разбора имени поля объекта
	this->_keyed = false;
	/**
	 * Определяем вид очередного события разбора
	 */
	switch(static_cast <uint8_t> (event)){
		// Если событие является открытием объекта
		case static_cast <uint8_t> (event_t::OBJECT_BEGIN):
			// Устанавливаем вид заводимого узла документа
			node.type = type_t::OBJECT;
		break;
		// Если событие является открытием массива
		case static_cast <uint8_t> (event_t::ARRAY_BEGIN):
			// Устанавливаем вид заводимого узла документа
			node.type = type_t::ARRAY;
		break;
		/**
		 * Если событие является закрытием вместилища
		 */
		case static_cast <uint8_t> (event_t::OBJECT_END):
		case static_cast <uint8_t> (event_t::ARRAY_END): {
			/**
			 * Если закрывается вместилище, какое не открывалось
			 */
			/**
			 * @note Заслон этот НЕДОСТИЖИМ и оттого не покрыт: событие закрытия вместилища
			 *       приходит от разбора, а тот открытия и закрытия сличает сам и без пары
			 *       события не выдаёт. Оттого стек вложенности к этому мигу непременно
			 *       непуст
			 *
			 * @warning Снимать заслон нельзя: он оберегает обращение к последнему звену
			 *          пустого стека, и расхождение разбора со сборкой обратилось бы чтением
			 *          мимо памяти вместо кода отказа
			 */
			if(this->_nesting.empty()){
				// Запоминаем код отказа разбора
				this->_error = error_t::INTERNAL;
				// Выводим признак неудачной сборки
				return false;
			}
			// Получаем номер закрываемого вместилища
			const uint32_t parent = this->_nesting.back();
			// Удаляем номер закрываемого вместилища из стека
			this->_nesting.pop_back();
			// Устанавливаем размер поддерева закрываемого вместилища
			this->_nodes[parent].extent(index - parent);
			/**
			 * Если закрывается объект, а повторяющиеся имена полей затребовано разбирать
			 */
			if((event == event_t::OBJECT_END) && (this->_settings.duplicates != duplicate_t::KEEP)){
				/**
				 * Если разбор повторяющихся имён полей объекта завершился отказом
				 */
				if(!this->deduplicate(parent, reader))
					// Выводим признак неудачной сборки
					return false;
			}
			// Выводим признак успешной сборки
			return true;
		}
		// Если событие является пустым значением
		case static_cast <uint8_t> (event_t::NUL):
			// Устанавливаем вид заводимого узла документа
			node.type = type_t::NUL;
		break;
		// Если событие является логическим значением
		case static_cast <uint8_t> (event_t::BOOL):
			// Устанавливаем вид заводимого узла документа
			node.type = type_t::BOOL;
		break;
		// Если событие является строковым значением
		case static_cast <uint8_t> (event_t::STRING):
			// Устанавливаем вид заводимого узла документа
			node.type = type_t::STRING;
		break;
		/**
		 * Если событие является числом
		 */
		case static_cast <uint8_t> (event_t::NUMBER): {
			// Получаем запись разбираемого числа
			const string_view text(reader.storage().data() + content.offset, content.length);
			/**
			 * Выполняем определение вида числа вместе с преобразованием его
			 *
			 * @note Число кладётся в узел готовым: повторного разбора записи при извлечении
			 *       не бывает вовсе. Запись его при этом остаётся в хранилище знаков -
			 *       переносится хранилище целым куском, а не по одному значению
			 */
			if(!Document::classify(text, node)){
				/**
				 * Если число не вместимо ни в один родной вид, а такие затребовано отвергать
				 */
				if(this->_settings.numbers == number_t::CHECK){
					// Запоминаем код отказа разбора
					this->_error = error_t::NUMBER_OUT_OF_RANGE;
					// Запоминаем положение отказа разбора в исходном тексте
					this->_position = reader.location();
					// Выводим признак неудачной сборки
					return false;
				}
				/**
				 * Устанавливаем вид числа, хранимого записью своей
				 *
				 * @note Запись такого числа остаётся в хранилище знаков нетронутой, и
				 *       точность его не теряется вовсе
				 */
				node.type = type_t::EXTENDED;
			}
		} break;
		/**
		 * Если событие не опознано
		 */
		/**
		 * @note Ветвь эта НЕДОСТИЖИМА и оттого не покрыта: разбор выдаёт события лишь тех
		 *       видов, какие разобраны выше, и вида сверх того у него нет вовсе
		 *
		 * @warning Снимать ветвь нельзя: она и есть страж того, что новый вид события,
		 *          заведённый разбором, не пройдёт мимо сборки молча
		 */
		default: {
			// Запоминаем код отказа разбора
			this->_error = error_t::INTERNAL;
			// Выводим признак неудачной сборки
			return false;
		}
	}
	/**
	 * Если узел ни вместилищем, ни числом родного вида не является
	 *
	 * @note Число родного вида хранит в этом самом месте себя, и запись длины его
	 *       записи погубила бы само число. Длина нужна лишь тому, чьё содержимое
	 *       лежит в хранилище знаков: строке да числу вида `EXTENDED`
	 */
	if(!node.nested() && !node.native()){
		// Устанавливаем длину содержимого узла
		node.length(content.length);
		// Устанавливаем признак изменения содержимого разбором
		node.modified = modified;
	}
	/**
	 * Если узел заводится внутри вместилища
	 */
	if(!this->_nesting.empty())
		// Увеличиваем количество детей вместилища
		this->_nodes[this->_nesting.back()].length(this->_nodes[this->_nesting.back()].length() + 1);
	/**
	 * Добавляем заводимый узел в перечень узлов документа
	 *
	 * @note Узел заводится переносом, а не копией: заведение его на месте потребовало
	 *       бы выдавать отказы сборки уже по заведённому узлу, а отказ обязан оставлять
	 *       перечень нетронутым
	 */
	this->_nodes.push_back(::std::move(node));
	/**
	 * Если узел вместилищем является
	 */
	if(node.nested()){
		/**
		 * Если глубина вложенности превышает допустимую
		 */
		if(this->_nesting.size() >= MAX_DEPTH){
			// Запоминаем код отказа разбора
			this->_error = error_t::DEPTH_EXCEEDED;
			// Выводим признак неудачной сборки
			return false;
		}
		// Добавляем номер открытого вместилища в стек
		this->_nesting.push_back(index);
	}
	// Выводим признак успешной сборки
	return true;
}
/**
 * @brief Метод разбора повторяющихся имён полей объекта
 *
 * @details Лишние поля сносятся поддеревьями и с конца перечня к началу: снос с
 * начала сдвинул бы номера ещё не снесённых полей
 *
 * @note Знаки снесённых полей остаются в хранилище знаков мёртвым грузом. Сжатие
 * хранилища потребовало бы правки смещений у всех узлов документа, а повтор имени
 * поля - случай редкий и обыкновенно отвергаемый вовсе
 *
 * @param parent номер узла разбираемого объекта
 * @return       признак успешности разбора
 *
 */
bool awh::codec::json::Document::deduplicate(const uint32_t parent, const reader_t & reader) noexcept {
	/**
	 * @brief Метод извлечения имени поля объекта, где бы знаки его ни лежали
	 *
	 * @details Разбор повторов ведётся при закрытии объекта, а знаки к тому времени
	 * ещё лежат в хранилище разбора: переносятся они целым куском по исчерпании
	 * событий. Оттого имя разыскивается по сквозному положению в том из двух хранилищ,
	 * какое им уже владеет
	 *
	 * @param node узел, имя поля какого извлекается
	 * @return     имя поля объекта
	 *
	 */
	const auto naming = [this, &reader](const node_t & node) noexcept -> string_view {
		// Получаем сквозное положение имени поля объекта
		const uint64_t position = ((this->_base + static_cast <uint64_t> (node.offset)) - static_cast <uint64_t> (node.named));
		/**
		 * Если знаки имени поля объекта в хранилище документа ещё не перенесены
		 */
		if(position >= (this->_base + static_cast <uint64_t> (this->_storage.size())))
			// Выводим имя поля объекта из хранилища разбора
			return string_view(reader.storage().data() + (position - reader.origin()), node.named);
		/**
		 * Выводим имя поля объекта из хранилища документа
		 *
		 * @note Ветвь эта НЕДОСТИЖИМА и оттого не покрыта: снос повторов идёт ПО ХОДУ
		 *       разбора, а знаки разбора переносятся в хранилище документа лишь по его
		 *       окончании, - и к этому мигу хранилище документа пусто. Проверено тремя
		 *       разборами подряд одним и тем же деревом: имя всякий раз бралось из
		 *       хранилища разбора
		 *
		 * @warning Снимать ветвь нельзя: перенос знаков вправе переехать раньше сноса
		 *          повторов, и тогда имя пришлось бы брать именно отсюда
		 */
		return string_view(this->_storage.data() + (position - this->_base), node.named);
	};
	// Получаем количество полей разбираемого объекта
	const uint32_t count = this->_nodes[parent].length();
	/**
	 * Если объект полей не имеет вовсе
	 *
	 * @note Проверка эта стоит впереди намеренно: объектов без полей и об одном поле
	 *       в обиходе много, а повториться в них нечему
	 */
	if(count < 2)
		// Выводим признак успешного разбора
		return true;
	// Получаем номер узла за последним узлом объекта
	const uint32_t bound = (parent + this->_nodes[parent].extent());
	// Перечень имён полей объекта вместе с номерами их узлов
	this->_naming.clear();
	// Выполняем выделение памяти под перечень имён полей объекта
	this->_naming.reserve(static_cast <size_t> (count));
	/**
	 * Если количество полей объекта превышает порог заведения отображения
	 *
	 * @details Мелкие объекты сличаются перебором пар, а крупные - отображением имён.
	 * Заведение отображения обходится дороже сличения немногих пар, а объектов о
	 * немногих полях в обиходе подавляющее большинство: сличение всякого объекта
	 * отображением стоило бы трети всего времени сборки дерева
	 *
	 * @note Порог тот же, каким заводится отображение имён для обращения по имени:
	 *       и там, и здесь речь об одном и том же выборе
	 */
	if(count > INDEX_THRESHOLD)
		// Выполняем очистку отображения имён полей объекта
		this->_lookup.clear();
	// Перечень номеров узлов сносимых полей объекта
	vector <uint32_t> removed;
	/**
	 * Выполняем перебор всех полей объекта
	 */
	for(uint32_t child = (parent + 1); child < bound; child += this->_nodes[child].extent()){
		// Получаем узел очередного поля объекта
		const node_t & node = this->_nodes[child];
		// Получаем имя очередного поля объекта
		const string_view name = naming(node);
		// Номер узла поля объекта, имя какого уже занято
		size_t place = this->_naming.size();
		/**
		 * Если количество полей объекта превышает порог заведения отображения
		 */
		if(count > INDEX_THRESHOLD){
			// Выполняем добавление имени поля объекта в отображение
			const auto found = this->_lookup.emplace(name, place);
			/**
			 * Если имя поля объекта уже занято
			 */
			if(!found.second)
				// Запоминаем место прежнего поля объекта в перечне имён
				place = found.first->second;
		/**
		 * Если объект сличается перебором пар
		 */
		} else {
			/**
			 * Выполняем перебор всех уже собранных имён полей объекта
			 */
			for(size_t i = 0; i < this->_naming.size(); i++){
				/**
				 * Если имя поля объекта совпадает с уже собранным
				 */
				if(this->_naming[i].first == name){
					// Запоминаем место прежнего поля объекта в перечне имён
					place = i;
					// Прекращаем перебор собранных имён полей объекта
					break;
				}
			}
		}
		/**
		 * Если имя поля объекта ещё не занято
		 */
		if(place == this->_naming.size()){
			// Выполняем добавление имени поля объекта к собранным
			this->_naming.emplace_back(name, child);
			// Выполняем переход к следующему полю объекта
			continue;
		}
		/**
		 * Если имя поля объекта уже занято
		 */
		{
			/**
			 * Определяем правило обращения с повторяющимся именем поля объекта
			 */
			switch(static_cast <uint8_t> (this->_settings.duplicates)){
				// Если повторяющееся имя поля объекта отвергается
				case static_cast <uint8_t> (duplicate_t::ERROR): {
					// Запоминаем код отказа разбора
					this->_error = error_t::DUPLICATE_KEY;
					// Выводим признак неудачного разбора
					return false;
				}
				// Если удерживается первое поле объекта
				case static_cast <uint8_t> (duplicate_t::FIRST):
					// Добавляем номер узла повторного поля объекта к сносимым
					removed.push_back(child);
				break;
				// Если удерживается последнее поле объекта
				case static_cast <uint8_t> (duplicate_t::LAST):
					// Добавляем номер узла прежнего поля объекта к сносимым
					removed.push_back(this->_naming[place].second);
					// Запоминаем номер узла последнего поля объекта
					this->_naming[place].second = child;
				break;
			}
		}
	}
	/**
	 * Если сносить нечего
	 */
	if(removed.empty())
		// Выводим признак успешного разбора
		return true;
	// Выполняем упорядочивание номеров узлов сносимых полей объекта
	::std::sort(removed.begin(), removed.end());
	/**
	 * Выполняем снос полей объекта с конца перечня к началу
	 */
	for(size_t i = removed.size(); i > 0; i--){
		// Получаем номер узла сносимого поля объекта
		const uint32_t child = removed[i - 1];
		// Получаем размер поддерева сносимого поля объекта
		const uint32_t extent = this->_nodes[child].extent();
		// Выполняем снос поддерева поля объекта из перечня узлов документа
		this->_nodes.erase(this->_nodes.begin() + child, this->_nodes.begin() + child + extent);
		// Уменьшаем размер поддерева объекта на размер снесённого поддерева
		this->_nodes[parent].extent(this->_nodes[parent].extent() - extent);
		// Уменьшаем количество полей объекта
		this->_nodes[parent].length(this->_nodes[parent].length() - 1);
	}
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод очистки документа
 *
 */
void awh::codec::json::Document::clear() noexcept {
	// Сбрасываем код отказа разбора
	this->_error = error_t::NONE;
	// Выполняем очистку перечня узлов документа
	this->_nodes.clear();
	// Выполняем очистку хранилища знаков документа
	this->_storage.clear();
	// Выполняем очистку отображения имён полей в номера узлов
	this->_index.clear();
	// Выполняем очистку стека номеров узлов открытых вместилищ
	this->_nesting.clear();
	// Сбрасываем длину имени поля объекта, ожидающего своего значения
	this->_named = 0;
	// Снимаем признак разбора имени поля объекта
	this->_keyed = false;
	// Снимаем признак собранного целиком документа
	this->_completed = false;
	// Сбрасываем сквозное положение конца имени поля объекта
	this->_pointer = 0;
	// Сбрасываем сквозное положение первого знака хранилища документа
	this->_base = 0;
	// Сбрасываем положение отказа разбора в исходном тексте
	this->_position = location_t();
}
/**
 * @brief Метод разбора текста документа
 *
 * @param text разбираемый текст документа
 * @return     признак успешности разбора
 *
 */
bool awh::codec::json::Document::parse(const string & text) noexcept {
	// Выполняем разбор текста документа без потоковой выдачи значений
	return this->parse(text, nullptr);
}
/**
 * @brief Метод потоковой выдачи значений разбираемого текста
 *
 * @param text     разбираемый текст документа
 * @param callback обработчик потоковой выдачи значений
 * @return         признак успешности разбора
 *
 */
bool awh::codec::json::Document::parse(const string & text, const callback_t & callback) noexcept {
	// Выполняем очистку документа
	this->clear();
	// Выполняем сброс состояния чтения текста документа
	this->_reader.reset();
	// Получаем чтение текста документа
	reader_t & reader = this->_reader;
	// Выполняем установку настроек разбора текста
	reader.settings(this->_settings.reader);
	// Запоминаем обработчик потоковой выдачи значений
	this->_callback = & callback;
	// Получаем признак потоковой выдачи значений
	const bool streaming = static_cast <bool> (callback);
	/**
	 * Выполняем установку удержания хранилища знаков разбора
	 *
	 * @details Знаки значений копировались дважды: из поданного текста в хранилище
	 * разбора и оттуда в хранилище документа. Вторая копия не нужна вовсе, когда
	 * документ забирает знаки себе целиком: разбор удерживает их у себя, а по
	 * окончании они переносятся без копии
	 *
	 * @note Потоковой выдаче удержание не годится: хранилище росло бы во весь
	 *       разбираемый поток, а он у неё без конца
	 */
	reader.keep(!streaming);
	/**
	 * Выполняем установку обработчика прямой выдачи событий разбора
	 *
	 * @details События приходят прямо в сборку дерева, минуя очередь выдачи чтения:
	 * событие ложилось в неё, а потом снималось с неё же копией
	 *
	 * @note Очередь стоила трети всего времени сборки дерева. Обнаружено разложением
	 *       стоимости по частям
	 */
	reader.handler(& Document::handler, this);
	// Признак успешности разбора текста документа
	bool result = true;
	/**
	 * Выполняем подачу текста документа чтению кусками
	 *
	 * @details Текст подаётся кусками, а не целиком, ради хранилища знаков разбора:
	 * оно копится до исчерпания поданного куска, и на тексте в шестнадцать мегабайт
	 * держало бы весь текст разом вторым его подобием. Куском же оно удерживается в
	 * размере, укладывающемся в кэш процессора
	 *
	 * @note Выдача от нарезки текста на куски не зависит вовсе - тем и позволительно
	 *       резать его здесь по своему усмотрению
	 */
	for(size_t offset = 0; offset <= text.size(); offset += ::CHUNK){
		// Получаем размер очередного подаваемого куска
		const size_t length = (((offset + ::CHUNK) < text.size()) ? ::CHUNK : (text.size() - offset));
		// Выполняем подачу очередного куска текста документа чтению
		result = reader.feed(text.data() + offset, length, ((offset + length) >= text.size()));
		/**
		 * Если знаки разбора удержанию не подлежат
		 */
		if(streaming)
			// Выполняем перенос знаков разбора в хранилище документа
			this->transfer(reader);
		/**
		 * Если разбор куска текста документа завершился отказом
		 */
		if(!result)
			// Прекращаем подачу текста документа
			break;
		/**
		 * Если текст документа исчерпан
		 */
		if((offset + length) >= text.size())
			// Прекращаем подачу текста документа
			break;
	}
	// Выполняем снятие обработчика прямой выдачи событий разбора
	reader.handler(nullptr, nullptr);
	// Сбрасываем обработчик потоковой выдачи значений
	this->_callback = nullptr;
	/**
	 * Если знаки разбора удерживались
	 */
	if(!streaming)
		// Выполняем перенос знаков разбора в хранилище документа целиком, без копии
		reader.release(this->_storage);
	/**
	 * Если разбор текста документа завершился отказом
	 */
	if(!result){
		/**
		 * Если своего отказа сборка дерева не выдала
		 *
		 * @note Сборка дерева прекращает разбор, отказа ему не сообщая: причина
		 *       отказа у неё своя, и затирать её отказом чтения нельзя
		 */
		if(this->_error == error_t::NONE)
			// Запоминаем код отказа разбора
			this->_error = reader.error();
		// Запоминаем положение отказа разбора в исходном тексте
		this->_position = reader.location();
		// Выполняем очистку перечня узлов документа
		this->_nodes.clear();
		// Выводим признак неудачного разбора
		return false;
	}
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод разбора текста документа из файла
 *
 * @details Файл читается кусками, а не целиком: документ в несколько гигабайт иначе
 * держался бы в памяти дважды - текстом и деревом
 *
 * @param filename адрес разбираемого файла
 * @return         признак успешности разбора
 *
 */
bool awh::codec::json::Document::load(const string & filename) noexcept {
	// Выполняем очистку документа
	this->clear();
	// Открываем файл документа для чтения
	ifstream file(filename, ios::binary);
	/**
	 * Если файл документа открыть не удалось
	 */
	if(!file.is_open()){
		//
		// Запоминаем код отказа разбора
		//
		// @note Отказ этот нашего внутреннего изъяна не означает: путь передан извне, и
		//       прежний код внутренней ошибки вводил потребителя в заблуждение, отправляя
		//       искать дефект у нас
		//
		this->_error = error_t::FILE_NOT_OPENED;
		/**
		 * Если объект для работы с логами установлен
		 *
		 * @note Отказ этот идёт мимо чтения, а вывод в лог ведёт именно оно: без
		 *       настоящего вывода открытие файла отказывало бы молча, тогда как отказ
		 *       разбора того же файла в лог уходит
		 */
		if(this->_log != nullptr)
			// Выполняем вывод сообщения об отказе
			this->_log->print("JSON document failed: %s", log_t::flag_t::CRITICAL, awh::codec::json::message(this->_error));
		// Выводим признак неудачного разбора
		return false;
	}
	// Выполняем сброс состояния чтения текста документа
	this->_reader.reset();
	// Получаем чтение текста документа
	reader_t & reader = this->_reader;
	// Выполняем установку настроек разбора текста
	reader.settings(this->_settings.reader);
	/**
	 * Выполняем установку обработчика прямой выдачи событий разбора
	 *
	 * @details События приходят прямо в сборку дерева, минуя очередь выдачи чтения
	 */
	reader.handler(& Document::handler, this);
	/**
	 * Выполняем установку удержания хранилища знаков разбора
	 *
	 * @note Потоковой выдачи значений разбор файла не ведёт, и знаки переносятся
	 *       документу по окончании целиком, без копии
	 */
	reader.keep(true);
	// Буфер очередного куска файла документа
	string buffer(::CHUNK, '\0');
	// Признак успешности разбора текста документа
	bool result = true;
	/**
	 * Выполняем чтение файла документа кусками
	 */
	while(file){
		// Выполняем чтение очередного куска файла документа
		file.read(buffer.data(), static_cast <streamsize> (buffer.size()));
		// Получаем размер прочитанного куска файла документа
		const size_t size = static_cast <size_t> (file.gcount());
		// Выполняем подачу куска файла документа чтению
		result = reader.feed(buffer.data(), size, !static_cast <bool> (file));
		/**
		 * Если подача куска чтению не удалась
		 */
		if(!result)
			// Прекращаем чтение файла документа
			break;
	}
	// Выполняем снятие обработчика прямой выдачи событий разбора
	reader.handler(nullptr, nullptr);
	// Выполняем перенос знаков разбора в хранилище документа целиком, без копии
	reader.release(this->_storage);
	/**
	 * Если разбор текста документа завершился отказом
	 */
	if(!result){
		/**
		 * Если своего отказа сборка дерева не выдала
		 */
		if(this->_error == error_t::NONE)
			// Запоминаем код отказа разбора
			this->_error = reader.error();
		// Запоминаем положение отказа разбора в исходном тексте
		this->_position = reader.location();
		// Выполняем очистку перечня узлов документа
		this->_nodes.clear();
		// Выводим признак неудачного разбора
		return false;
	}
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод перезаписи документа в текст
 *
 * @details Обход идёт вперёд по памяти перечнем узлов, а не спуском по дереву:
 * дети лежат сразу за родителем, и обход оттого не выделяет памяти вовсе, кроме
 * стека закрываемых вместилищ
 *
 * @param format вид оформления собираемого текста
 * @return       собранный текст документа
 *
 */
string awh::codec::json::Document::dump(const format_t format) const noexcept {
	// Запись текста документа
	writer_t writer(this->_log);
	// Получаем настройки записи текста
	writer_t::settings_t settings = this->_settings.writer;
	// Устанавливаем затребованный вид оформления собираемого текста
	settings.format = format;
	/**
	 * Дозволяем записи NaN и бесконечности, коль скоро разбор их принимает
	 *
	 * @note Дозволение это выводится из настроек разбора намеренно: числа такие
	 *       попадают в дерево лишь с его дозволения, а запрет на записи их у
	 *       сборщика молча выбросил бы значение из перезаписанного текста.
	 *       Обнаружено обкаткой искажёнными текстами
	 */
	settings.allowInfinityAndNan = (settings.allowInfinityAndNan || this->_settings.reader.allowInfinityAndNan);
	// Выполняем установку настроек записи текста
	writer.settings(settings);
	/**
	 * Если документ пуст
	 */
	if(this->_nodes.empty())
		// Выводим пустой текст документа
		return string();
	// Стек номеров узлов за последними узлами открытых вместилищ
	vector <uint32_t> nesting;
	/**
	 * Выполняем перебор всех узлов документа
	 */
	for(uint32_t index = 0; index < static_cast <uint32_t> (this->_nodes.size()); index++){
		/**
		 * Выполняем закрытие всех вместилищ, чьи дети исчерпаны
		 */
		while(!nesting.empty() && (index >= nesting.back())){
			// Удаляем номер узла за последним узлом вместилища из стека
			nesting.pop_back();
			// Выполняем закрытие вместилища
			writer.close();
		}
		// Получаем очередной узел документа
		const node_t & node = this->_nodes[index];
		/**
		 * Если узел является полем объекта
		 */
		if(node.keyed)
			// Выполняем запись имени поля объекта
			writer.key(string(this->_storage.data() + (node.offset - node.named), node.named));
		/**
		 * Определяем вид очередного узла документа
		 */
		switch(static_cast <uint8_t> (json::kind(node.type))){
			// Если узел является пустым значением
			case static_cast <uint8_t> (kind_t::NUL):
				// Выполняем запись пустого значения
				writer.null();
			break;
			// Если узел является логическим значением
			case static_cast <uint8_t> (kind_t::BOOL):
				// Выполняем запись логического значения
				writer.value(node.length() == 4);
			break;
			/**
			 * Если узел является числом
			 *
			 * @note Число пишется прямо в общий объект записи: сборка записи его отдельным
			 *       объектом стоила бы заведения такого объекта на всякое число документа
			 */
			case static_cast <uint8_t> (kind_t::NUMBER):
				// Выполняем запись числа, хранимого узлом
				this->compose(writer, node);
			break;
			// Если узел является строкой
			case static_cast <uint8_t> (kind_t::STRING):
				// Выполняем запись строкового значения
				writer.value(string(this->_storage.data() + node.offset, node.length()));
			break;
			// Если узел является массивом
			case static_cast <uint8_t> (kind_t::ARRAY): {
				// Выполняем открытие массива
				writer.array();
				// Добавляем номер узла за последним узлом массива в стек
				nesting.push_back(index + node.extent());
			} break;
			// Если узел является объектом
			case static_cast <uint8_t> (kind_t::OBJECT): {
				// Выполняем открытие объекта
				writer.object();
				// Добавляем номер узла за последним узлом объекта в стек
				nesting.push_back(index + node.extent());
			} break;
		}
	}
	/**
	 * Выполняем закрытие всех оставшихся открытыми вместилищ
	 */
	while(!nesting.empty()){
		// Удаляем номер узла за последним узлом вместилища из стека
		nesting.pop_back();
		// Выполняем закрытие вместилища
		writer.close();
	}
	// Выводим собранный текст документа
	return writer.take();
}
/**
 * @brief Метод записи документа в файл
 *
 * @param filename адрес записываемого файла
 * @param format   вид оформления собираемого текста
 * @return         признак успешности записи
 *
 */
bool awh::codec::json::Document::save(const string & filename, const format_t format) const noexcept {
	// Открываем файл документа для записи
	ofstream file(filename, ios::binary | ios::trunc);
	/**
	 * Если файл документа открыть не удалось
	 *
	 * @note Отказ этот идёт мимо чтения, а вывод в лог ведёт именно оно: без
	 *       настоящего вывода запись файла отказывала бы молча, тогда как отказ
	 *       разбора того же файла в лог уходит
	 */
	if(!file.is_open()){
		/**
		 * Если объект для работы с логами установлен
		 */
		if(this->_log != nullptr)
			// Выполняем вывод сообщения об отказе
			this->_log->print("JSON document failed: %s", log_t::flag_t::CRITICAL, awh::codec::json::message(error_t::FILE_NOT_OPENED));
		// Выводим признак неудачной записи
		return false;
	}
	// Получаем собранный текст документа
	const string text = this->dump(format);
	// Выполняем запись текста документа в файл
	file.write(text.data(), static_cast <streamsize> (text.size()));
	/**
	 * Выполняем закрытие файла документа
	 *
	 * @note Закрытие обязано идти ЯВНО и до вывода признака: поток сбрасывает
	 *       свой буфер разрушением своим, то есть уже ПОСЛЕ вычисления
	 *       возвращаемого значения. Текст, целиком уместившийся в буфер, уходил
	 *       бы отказом сброса молча, а вызов отчитывался бы успехом - замер дал
	 *       успех при 512 байтах из 1101 в файле
	 */
	file.close();
	/**
	 * Если запись текста документа в файл не удалась
	 */
	if(!file){
		/**
		 * Если объект для работы с логами установлен
		 */
		if(this->_log != nullptr)
			// Выполняем вывод сообщения об отказе
			this->_log->print("JSON document failed: %s", log_t::flag_t::CRITICAL, awh::codec::json::message(error_t::FILE_NOT_WRITTEN));
		// Выводим признак неудачной записи
		return false;
	}
	// Выводим признак успешности записи
	return true;
}
/**
 * @brief Метод извлечения корневого значения документа
 *
 * @return ссылка на корневое значение документа
 *
 */
awh::codec::json::Document::value_t awh::codec::json::Document::root() const noexcept {
	// Выводим ссылку на корневое значение документа
	return (this->_nodes.empty() ? value_t() : value_t(this, 0, static_cast <uint32_t> (this->_nodes.size())));
}
/**
 * @brief Метод обращения к полю корневого объекта по имени
 *
 * @param name имя поля корневого объекта
 * @return     ссылка на узел поля объекта
 *
 */
awh::codec::json::Document::value_t awh::codec::json::Document::operator [] (const string & name) const noexcept {
	// Выводим ссылку на узел поля корневого объекта
	return this->root()[name];
}
/**
 * @brief Метод обращения к значению корневого массива по номеру
 *
 * @param index номер значения в корневом массиве
 * @return      ссылка на узел значения
 *
 */
awh::codec::json::Document::value_t awh::codec::json::Document::operator [] (const size_t index) const noexcept {
	// Выводим ссылку на узел значения корневого массива
	return this->root()[index];
}
/**
 * @brief Метод обращения к значению по указателю JSON Pointer
 *
 * @param pointer указатель на значение по RFC 6901
 * @return        ссылка на узел значения
 *
 */
awh::codec::json::Document::value_t awh::codec::json::Document::at(const string & pointer) const noexcept {
	// Выводим ссылку на узел, разысканный указателем
	return this->root().at(pointer);
}
/**
 * @brief Метод извлечения количества узлов документа
 *
 * @return количество узлов документа
 *
 */
size_t awh::codec::json::Document::size() const noexcept {
	// Выводим количество узлов документа
	return this->_nodes.size();
}
/**
 * @brief Метод проверки документа на пустоту
 *
 * @return признак отсутствия узлов в документе
 *
 */
bool awh::codec::json::Document::empty() const noexcept {
	// Выводим признак отсутствия узлов в документе
	return this->_nodes.empty();
}
/**
 * @brief Метод извлечения кода отказа разбора
 *
 * @return код отказа разбора
 *
 */
awh::codec::json::error_t awh::codec::json::Document::error() const noexcept {
	// Выводим код отказа разбора
	return this->_error;
}
/**
 * @brief Метод извлечения положения отказа разбора в исходном тексте
 *
 * @return положение отказа разбора в исходном тексте
 *
 */
const awh::codec::json::location_t & awh::codec::json::Document::errorLocation() const noexcept {
	// Выводим положение отказа разбора в исходном тексте
	return this->_position;
}
/**
 * @brief Метод извлечения настроек документа
 *
 * @return настройки документа
 *
 */
const awh::codec::json::Document::settings_t & awh::codec::json::Document::settings() const noexcept {
	// Выводим настройки документа
	return this->_settings;
}
/**
 * @brief Метод установки настроек документа
 *
 * @param settings устанавливаемые настройки документа
 *
 */
void awh::codec::json::Document::settings(const settings_t & settings) noexcept {
	// Выполняем установку настроек документа
	this->_settings = settings;
}
