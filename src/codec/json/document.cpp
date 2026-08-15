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
#include <fstream>
#include <algorithm>

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

}

/**
 * @brief Конструктор
 *
 */
awh::codec::json::Document::Document() noexcept : _error(error_t::NONE), _named(0), _keyed(false), _pointer(0), _base(0), _callback(nullptr) {}
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
	if(!this->valid() || (this->_doc->_nodes[this->_index].kind != kind_t::OBJECT))
		// Выводим недействительную ссылку
		return Value();
	// Получаем узел, на какой указывает ссылка
	const node_t & node = this->_doc->_nodes[this->_index];
	// Получаем номер узла за последним узлом объекта
	const uint32_t bound = (this->_index + node.extent);
	/**
	 * Если количество полей объекта превышает порог заведения отображения
	 */
	if(node.length > INDEX_THRESHOLD){
		// Выполняем поиск отображения имён полей объекта
		auto i = this->_doc->_index.find(this->_index);
		/**
		 * Если отображение имён полей объекта ещё не заведено
		 */
		if(i == this->_doc->_index.end()){
			// Заводимое отображение имён полей объекта
			unordered_map <string_view, uint32_t> index;
			// Выполняем выделение памяти под отображение имён полей объекта
			index.reserve(static_cast <size_t> (node.length));
			/**
			 * Выполняем перебор всех полей объекта
			 */
			for(uint32_t child = (this->_index + 1); child < bound; child += this->_doc->_nodes[child].extent){
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
	for(uint32_t child = (this->_index + 1); child < bound; child += this->_doc->_nodes[child].extent){
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
	if(((node.kind != kind_t::ARRAY) && (node.kind != kind_t::OBJECT)) || (index >= static_cast <size_t> (node.length)))
		// Выводим недействительную ссылку
		return Value();
	// Получаем номер узла за последним узлом вместилища
	const uint32_t bound = (this->_index + node.extent);
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
		child += this->_doc->_nodes[child].extent;
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
	result = (this->_doc->_nodes[this->_index].length == 4);
	// Выводим признак успешного извлечения
	return true;
}
/**
 * @brief Метод извлечения целого числа
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::json::Document::Value::value(int64_t & result) const noexcept {
	// Получаем запись числа как она есть
	const string_view text = this->raw();
	/**
	 * Если узел числом не является
	 */
	if(text.empty())
		// Выводим признак неудачного извлечения
		return false;
	// Выполняем разбор записи числа
	const lexical_t::result_t <char> res = lexical_t::fromChars(text.data(), text.data() + text.size(), result);
	// Выводим признак успешного извлечения, если запись разобрана целиком
	return (static_cast <bool> (res) && (res.ptr == (text.data() + text.size())));
}
/**
 * @brief Метод извлечения беззнакового целого числа
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::json::Document::Value::value(uint64_t & result) const noexcept {
	// Получаем запись числа как она есть
	const string_view text = this->raw();
	/**
	 * Если узел числом не является либо число записано со знаком минуса
	 */
	if(text.empty() || (text.front() == '-'))
		// Выводим признак неудачного извлечения
		return false;
	// Выполняем разбор записи числа
	const lexical_t::result_t <char> res = lexical_t::fromChars(text.data(), text.data() + text.size(), result);
	// Выводим признак успешного извлечения, если запись разобрана целиком
	return (static_cast <bool> (res) && (res.ptr == (text.data() + text.size())));
}
/**
 * @brief Метод извлечения числа с плавающей запятой
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
bool awh::codec::json::Document::Value::value(double & result) const noexcept {
	/**
	 * Если документ хранит значение числа вместо записи его
	 */
	if((this->kind() == kind_t::NUMBER) && (this->_doc->_settings.numbers != number_t::LAZY)){
		// Выводим значение числа, преобразованное при разборе
		result = this->_doc->_numbers[this->_doc->_nodes[this->_index].length];
		// Выводим признак успешного извлечения
		return true;
	}
	// Получаем запись числа как она есть
	const string_view text = this->raw();
	/**
	 * Если узел числом не является
	 */
	if(text.empty())
		// Выводим признак неудачного извлечения
		return false;
	// Выполняем разбор записи числа
	const lexical_t::result_t <char> res = lexical_t::fromChars(text.data(), text.data() + text.size(), result);
	// Выводим признак успешного извлечения, если запись разобрана целиком
	return (static_cast <bool> (res) && (res.ptr == (text.data() + text.size())));
}
/**
 * @brief Метод извлечения строкового значения
 *
 * @param result переменная, куда помещается извлечённое значение
 * @return       признак успешности извлечения
 *
 */
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
	return string_view(this->_doc->_storage.data() + node.offset, node.length);
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
		}
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
			node.kind = kind_t::OBJECT;
		break;
		// Если событие является открытием массива
		case static_cast <uint8_t> (event_t::ARRAY_BEGIN):
			// Устанавливаем вид заводимого узла документа
			node.kind = kind_t::ARRAY;
		break;
		/**
		 * Если событие является закрытием вместилища
		 */
		case static_cast <uint8_t> (event_t::OBJECT_END):
		case static_cast <uint8_t> (event_t::ARRAY_END): {
			/**
			 * Если закрывается вместилище, какое не открывалось
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
			this->_nodes[parent].extent = (index - parent);
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
			node.kind = kind_t::NUL;
		break;
		// Если событие является логическим значением
		case static_cast <uint8_t> (event_t::BOOL):
			// Устанавливаем вид заводимого узла документа
			node.kind = kind_t::BOOL;
		break;
		// Если событие является строковым значением
		case static_cast <uint8_t> (event_t::STRING):
			// Устанавливаем вид заводимого узла документа
			node.kind = kind_t::STRING;
		break;
		/**
		 * Если событие является числом
		 */
		case static_cast <uint8_t> (event_t::NUMBER): {
			// Устанавливаем вид заводимого узла документа
			node.kind = kind_t::NUMBER;
			/**
			 * Если преобразование числа затребовано настройками
			 */
			if(this->_settings.numbers != number_t::LAZY){
				// Получаем запись разбираемого числа
				const string_view text(reader.storage().data() + content.offset, content.length);
				// Значение разбираемого числа
				double result = 0.;
				// Выполняем разбор записи числа
				const lexical_t::result_t <char> res = lexical_t::fromChars(text.data(), text.data() + text.size(), result);
				/**
				 * Если запись числа разобрать не удалось
				 */
				if(!static_cast <bool> (res) || (res.ptr != (text.data() + text.size()))){
					/**
					 * Запоминаем код отказа разбора
					 *
					 * @note Запись числа разбор уже сличил со стандартом, оттого отказ
					 *       преобразования означает непредставимость числа, а не негодность
					 *       записи. Сличение здесь повторяется лишь затем, чтобы отказ
					 *       назывался своим именем и в случае, о каком мы не подумали
					 */
					this->_error = (numeric(string(text)) ? error_t::NUMBER_OUT_OF_RANGE : error_t::INVALID_NUMBER);
					// Запоминаем положение отказа разбора в исходном тексте
					this->_position = reader.location();
					// Выводим признак неудачной сборки
					return false;
				}
				/**
				 * Если преобразование числа затребовано с проверкой представимости
				 */
				if((this->_settings.numbers == number_t::CHECK) && ::isinf(result)){
					// Запоминаем код отказа разбора
					this->_error = error_t::NUMBER_OUT_OF_RANGE;
					// Запоминаем положение отказа разбора в исходном тексте
					this->_position = reader.location();
					// Выводим признак неудачной сборки
					return false;
				}
				/**
				 * Если количество преобразованных чисел превышает допустимое
				 */
				if(this->_numbers.size() >= NO_OFFSET){
					// Запоминаем код отказа разбора
					this->_error = error_t::OVERFLOW_LIMIT;
					// Выводим признак неудачной сборки
					return false;
				}
				/**
				 * Запоминаем место значения числа в перечне преобразованных
				 *
				 * @details Преобразование при всяком обращении стоило трёх четвертей всего
				 * обхода дерева на документе из одних чисел: настройка велит преобразовать
				 * число единожды, а значит и хранить его следует значением
				 *
				 * @note Длина записи числа узлу больше не нужна: записи её документ не
				 *       хранит вовсе, коль скоро хранит значение
				 */
				node.length = static_cast <uint32_t> (this->_numbers.size());
				// Выполняем добавление значения числа к преобразованным
				this->_numbers.push_back(result);
			}
		} break;
		/**
		 * Если событие не опознано
		 */
		default: {
			// Запоминаем код отказа разбора
			this->_error = error_t::INTERNAL;
			// Выводим признак неудачной сборки
			return false;
		}
	}
	/**
	 * Если узел вместилищем не является
	 */
	if((node.kind != kind_t::ARRAY) && (node.kind != kind_t::OBJECT) &&
	   ((node.kind != kind_t::NUMBER) || (this->_settings.numbers == number_t::LAZY))){
		// Устанавливаем длину содержимого узла
		node.length = content.length;
		// Устанавливаем признак изменения содержимого разбором
		node.modified = modified;
	}
	/**
	 * Если узел заводится внутри вместилища
	 */
	if(!this->_nesting.empty())
		// Увеличиваем количество детей вместилища
		this->_nodes[this->_nesting.back()].length++;
	// Добавляем заводимый узел в перечень узлов документа
	this->_nodes.push_back(node);
	/**
	 * Если узел вместилищем является
	 */
	if((node.kind == kind_t::ARRAY) || (node.kind == kind_t::OBJECT)){
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
		// Выводим имя поля объекта из хранилища документа
		return string_view(this->_storage.data() + (position - this->_base), node.named);
	};
	// Получаем количество полей разбираемого объекта
	const uint32_t count = this->_nodes[parent].length;
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
	const uint32_t bound = (parent + this->_nodes[parent].extent);
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
	for(uint32_t child = (parent + 1); child < bound; child += this->_nodes[child].extent){
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
		const uint32_t extent = this->_nodes[child].extent;
		// Выполняем снос поддерева поля объекта из перечня узлов документа
		this->_nodes.erase(this->_nodes.begin() + child, this->_nodes.begin() + child + extent);
		// Уменьшаем размер поддерева объекта на размер снесённого поддерева
		this->_nodes[parent].extent -= extent;
		// Уменьшаем количество полей объекта
		this->_nodes[parent].length--;
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
	// Выполняем очистку значений чисел, преобразованных при разборе
	this->_numbers.clear();
	// Выполняем очистку отображения имён полей в номера узлов
	this->_index.clear();
	// Выполняем очистку стека номеров узлов открытых вместилищ
	this->_nesting.clear();
	// Сбрасываем длину имени поля объекта, ожидающего своего значения
	this->_named = 0;
	// Снимаем признак разбора имени поля объекта
	this->_keyed = false;
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
		// Запоминаем код отказа разбора
		this->_error = error_t::INTERNAL;
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
	writer_t writer;
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
		switch(static_cast <uint8_t> (node.kind)){
			// Если узел является пустым значением
			case static_cast <uint8_t> (kind_t::NUL):
				// Выполняем запись пустого значения
				writer.null();
			break;
			// Если узел является логическим значением
			case static_cast <uint8_t> (kind_t::BOOL):
				// Выполняем запись логического значения
				writer.value(node.length == 4);
			break;
			// Если узел является числом
			/**
			 * Если узел является числом
			 */
			case static_cast <uint8_t> (kind_t::NUMBER): {
				/**
				 * Если документ хранит значение числа вместо записи его
				 */
				if(this->_settings.numbers != number_t::LAZY)
					// Выполняем запись значения числа
					writer.value(this->_numbers[node.length]);
				// Если документ хранит запись числа
				else writer.raw(string(this->_storage.data() + node.offset, node.length));
			} break;
			// Если узел является строкой
			case static_cast <uint8_t> (kind_t::STRING):
				// Выполняем запись строкового значения
				writer.value(string(this->_storage.data() + node.offset, node.length));
			break;
			// Если узел является массивом
			case static_cast <uint8_t> (kind_t::ARRAY): {
				// Выполняем открытие массива
				writer.array();
				// Добавляем номер узла за последним узлом массива в стек
				nesting.push_back(index + node.extent);
			} break;
			// Если узел является объектом
			case static_cast <uint8_t> (kind_t::OBJECT): {
				// Выполняем открытие объекта
				writer.object();
				// Добавляем номер узла за последним узлом объекта в стек
				nesting.push_back(index + node.extent);
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
	 */
	if(!file.is_open())
		// Выводим признак неудачной записи
		return false;
	// Получаем собранный текст документа
	const string text = this->dump(format);
	// Выполняем запись текста документа в файл
	file.write(text.data(), static_cast <streamsize> (text.size()));
	// Выводим признак успешности записи
	return static_cast <bool> (file);
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
const awh::codec::json::location_t & awh::codec::json::Document::location() const noexcept {
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
