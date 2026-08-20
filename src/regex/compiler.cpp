/**
 * @file compiler.cpp
 * @date 2026-07-31
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
 * @brief Реализация компиляции регулярных выражений — преобразование синтаксического дерева
 *        в программу недетерминированного конечного автомата с разворачиванием кванторов
 *        повторения и размещением инструкций захвата групп
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <algorithm>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/compiler.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Конструктор
 *
 */
awh::regex::Compiler::Compiler() noexcept :
 _parser(nullptr), _program(nullptr), _reverse(false), _full(false),
 _cells(0), _atomics(0), _error(error_t::NONE) {}
/**
 * @brief Метод извлечения кода ошибки компиляции
 *
 * @return код ошибки последней операции компиляции
 *
 */
awh::regex::error_t awh::regex::Compiler::error() const noexcept {
	// Выводим код ошибки последней операции компиляции
	return this->_error;
}
/**
 * @brief Метод извлечения адреса следующей размещаемой инструкции
 *
 * @return адрес следующей размещаемой инструкции программы
 *
 */
awh::regex::address_t awh::regex::Compiler::position() const noexcept {
	// Выводим адрес следующей размещаемой инструкции программы
	return static_cast <address_t> (this->_program->instructions.size());
}
/**
 * @brief Метод размещения инструкции программы
 *
 * @param type  код операции размещаемой инструкции
 * @param flags набор режимов компиляции инструкции
 * @return      адрес размещённой инструкции программы
 *
 */
awh::regex::address_t awh::regex::Compiler::emit(const opcode_t type, const uint32_t flags) noexcept {
	// Получаем адрес размещаемой инструкции программы
	const address_t result = this->position();
	/**
	 * Если количество инструкций программы превышает допустимое
	 */
	if(static_cast <size_t> (result) >= MAX_PROGRAM) {
		/**
		 * Если ошибка компиляции ещё не установлена
		 */
		if(this->_error == error_t::NONE)
			// Выполняем установку ошибки превышения размера выражения
			this->_error = error_t::PATTERN_TOO_LARGE;
		// Выводим адрес отсутствующей инструкции программы
		return INVALID_ADDRESS;
	}
	// Выполняем размещение инструкции программы
	this->_program->instructions.emplace_back();
	// Выполняем установку кода операции инструкции
	this->_program->instructions.back().type = type;
	// Выполняем установку набора режимов компиляции инструкции
	this->_program->instructions.back().flags = flags;
	// Выводим адрес размещённой инструкции программы
	return result;
}
/**
 * @brief Метод размещения класса символов в программе
 *
 * @param value класс символов для размещения в программе
 * @return      индекс класса символов в хранилище классов
 *
 */
uint32_t awh::regex::Compiler::store(const class_t & value) noexcept {
	// Получаем индекс размещаемого класса символов
	const uint32_t result = static_cast <uint32_t> (this->_program->classes.size());
	// Создаём ссылку на размещаемый класс символов
	classref_t record;
	// Выполняем установку признака отрицания класса символов
	record.negative = value.negative;
	// Выполняем установку номера первого диапазона класса
	record.ranges = static_cast <uint32_t> (this->_program->ranges.size());
	// Выполняем установку количества диапазонов класса
	record.rangeCount = static_cast <uint32_t> (value.ranges.size());
	// Выполняем установку номера первого свойства класса
	record.properties = static_cast <uint32_t> (this->_program->properties.size());
	// Выполняем установку количества свойств класса
	record.propertyCount = static_cast <uint32_t> (value.properties.size());
	// Выполняем перенос диапазонов класса в сплошной набор программы
	this->_program->ranges.append(value.ranges.data(), value.ranges.size());
	// Выполняем перенос свойств класса в сплошной набор программы
	this->_program->properties.append(value.properties.data(), value.properties.size());
	// Выполняем размещение ссылки на класс символов в хранилище
	this->_program->classes.push_back(record);
	// Выводим индекс класса символов в хранилище классов
	return result;
}
/**
 * @brief Метод компиляции узла выбора одной из ветвей
 *
 * @param id индекс узла выбора одной из ветвей в арене узлов
 * @return   результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compileAlternate(const node_id_t id) noexcept {
	// Получаем узел выбора одной из ветвей
	const node_data_t & node = this->_parser->node(id);
	// Создаём набор адресов инструкций перехода к завершению выражения
	vector <address_t> exits;
	// Получаем индекс очередной ветви выражения
	node_id_t branch = node.child;
	/**
	 * Выполняем компиляцию ветвей выражения
	 */
	while(branch != INVALID_NODE) {
		// Получаем индекс следующей ветви выражения
		const node_id_t next = this->_parser->node(branch).next;
		/**
		 * Если ветвь выражения является последней
		 */
		if(next == INVALID_NODE) {
			/**
			 * Если компиляция последней ветви выражения не выполнена
			 */
			if(!this->compileNode(branch))
				// Выводим результат выполнения компиляции
				return false;
			// Выходим из цикла компиляции ветвей выражения
			break;
		}
		// Выполняем размещение инструкции перехода по двум ветвям
		const address_t split = this->emit(opcode_t::SPLIT, node.flags);
		/**
		 * Если размещение инструкции не выполнено
		 */
		if(split == INVALID_ADDRESS)
			// Выводим результат выполнения компиляции
			return false;
		// Выполняем установку адреса ветви с наибольшим приоритетом
		this->_program->instructions.at(split).split.first = this->position();
		/**
		 * Если компиляция очередной ветви выражения не выполнена
		 */
		if(!this->compileNode(branch))
			// Выводим результат выполнения компиляции
			return false;
		// Выполняем размещение инструкции перехода к завершению выражения
		const address_t jump = this->emit(opcode_t::JUMP, node.flags);
		/**
		 * Если размещение инструкции не выполнено
		 */
		if(jump == INVALID_ADDRESS)
			// Выводим результат выполнения компиляции
			return false;
		// Выполняем добавление адреса инструкции перехода к завершению
		exits.push_back(jump);
		// Выполняем установку адреса ветви с наименьшим приоритетом
		this->_program->instructions.at(split).split.second = this->position();
		// Переходим к следующей ветви выражения
		branch = next;
	}
	/**
	 * Выполняем установку адресов перехода к завершению выражения
	 */
	for(auto & jump : exits)
		// Выполняем установку адреса инструкции перехода
		this->_program->instructions.at(jump).jump.target = this->position();
	// Выводим результат выполнения компиляции
	return true;
}
/**
 * @brief Метод компиляции узла повторения
 *
 * @param id индекс узла повторения в арене узлов
 * @return   результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compileRepeat(const node_id_t id) noexcept {
	// Получаем узел повторения
	const node_data_t & node = this->_parser->node(id);
	/**
	 * Если квантор повторения является захватывающим
	 *
	 * @details Захватывающий квантор запрещает возврат внутрь повторяемого элемента,
	 *          что недостижимо исполнением без возврата.
	 *
	 */
	if(node.repeat.greed == greed_t::POSSESSIVE) {
		/**
		 * Если выполняется компиляция регулярного подмножества
		 */
		if(!this->_full) {
			// Выполняем установку ошибки неподдерживаемой конструкции
			this->_error = error_t::UNSUPPORTED;
			// Выводим результат выполнения компиляции
			return false;
		}
		// Получаем номер ячейки отметки состояния возврата
		const uint32_t cell = this->_atomics++;
		// Выполняем размещение инструкции запоминания состояния возврата
		const address_t mark = this->emit(opcode_t::MARK, node.flags);
		/**
		 * Если размещение инструкции не выполнено
		 */
		if(mark == INVALID_ADDRESS)
			// Выводим результат выполнения компиляции
			return false;
		// Выполняем установку номера ячейки отметки состояния возврата
		this->_program->instructions.at(mark).atomic.cell = cell;
		/**
		 * Если компиляция повторения не выполнена
		 */
		if(!this->compileIteration(id))
			// Выводим результат выполнения компиляции
			return false;
		// Выполняем размещение инструкции отказа от точек возврата
		const address_t cut = this->emit(opcode_t::CUT, node.flags);
		/**
		 * Если размещение инструкции не выполнено
		 */
		if(cut == INVALID_ADDRESS)
			// Выводим результат выполнения компиляции
			return false;
		// Выполняем установку номера ячейки отметки состояния возврата
		this->_program->instructions.at(cut).atomic.cell = cell;
		// Выводим результат выполнения компиляции
		return true;
	}
	// Выводим результат компиляции повторения элемента выражения
	return this->compileIteration(id);
}
/**
 * @brief Метод компиляции повторения элемента выражения
 *
 * @param id индекс узла повторения в арене узлов
 * @return   результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compileIteration(const node_id_t id) noexcept {
	// Получаем узел повторения
	const node_data_t & node = this->_parser->node(id);
	// Получаем индекс повторяемого элемента выражения
	const node_id_t child = node.child;
	/**
	 * Если неограниченно повторяется элемент, допускающий пустое сопоставление
	 *
	 * @details Пустое сопоставление тела прекращает повторение, что требует
	 *          учёта продвижения по тексту в пределах одного повторения и
	 *          недостижимо исполнением без возврата.
	 *
	 */
	if(!this->_full && (node.repeat.max == UNBOUNDED) && this->_parser->nullable(child)) {
		// Выполняем установку ошибки неподдерживаемой конструкции
		this->_error = error_t::UNSUPPORTED;
		// Выводим результат выполнения компиляции
		return false;
	}
	/**
	 * Определяем необходимость проверки продвижения по тексту
	 *
	 * @details Проверка размещается для каждого неограниченного повторения выражения,
	 *          компилируемого целиком: пустое сопоставление тела определяется составом
	 *          конструкций вне регулярного подмножества и выводится ненадёжно, тогда
	 *          как для тела, продвигающегося по тексту, проверка не выполняется никогда.
	 *
	 */
	const bool nullable = ((node.repeat.max == UNBOUNDED) && this->_full && !advancing(child));
	// Получаем наименьшее число повторений элемента выражения
	const uint32_t least = node.repeat.min;
	// Получаем наибольшее число повторений элемента выражения
	const uint32_t most = node.repeat.max;
	// Определяем приоритет ветви продолжения повторения
	const bool greedy = (node.repeat.greed != greed_t::LAZY);
	/**
	 * Выполняем размещение обязательных повторений элемента выражения
	 */
	for(uint32_t i = 0; i < least; i++) {
		/**
		 * Если компиляция повторяемого элемента выражения не выполнена
		 */
		if(!this->compileNode(child))
			// Выводим результат выполнения компиляции
			return false;
	}
	/**
	 * Если число повторений элемента выражения не ограничено
	 */
	if(most == UNBOUNDED) {
		// Получаем адрес начала повторения элемента выражения
		const address_t start = this->position();
		// Выполняем размещение инструкции перехода по двум ветвям
		const address_t split = this->emit(opcode_t::SPLIT, node.flags);
		/**
		 * Если размещение инструкции не выполнено
		 */
		if(split == INVALID_ADDRESS)
			// Выводим результат выполнения компиляции
			return false;
		// Получаем адрес повторяемого элемента выражения
		const address_t body = this->position();
		// Адрес инструкции проверки продвижения по тексту
		address_t guard = INVALID_ADDRESS;
		/**
		 * Если повторяется элемент, допускающий пустое сопоставление
		 *
		 * @details Позиция начала очередного повторения размещается в ячейке состояния,
		 *          благодаря чему завершение повторения определяется отсутствием
		 *          продвижения по тексту, а не исчерпанием сопоставлений тела.
		 *
		 */
		if(nullable) {
			// Выполняем размещение ячейки позиции начала повторения
			const uint32_t cell = this->reserve();
			// Выполняем размещение инструкции сохранения позиции начала повторения
			const address_t save = this->emit(opcode_t::SAVE, node.flags);
			/**
			 * Если размещение инструкции не выполнено
			 */
			if(save == INVALID_ADDRESS)
				// Выводим результат выполнения компиляции
				return false;
			// Выполняем установку номера ячейки позиции начала повторения
			this->_program->instructions.at(save).save.slot = cell;
			// Выполняем сохранение номера ячейки позиции начала повторения
			guard = cell;
		}
		/**
		 * Если компиляция повторяемого элемента выражения не выполнена
		 */
		if(!this->compileNode(child))
			// Выводим результат выполнения компиляции
			return false;
		// Адрес инструкции проверки продвижения по тексту
		address_t progress = INVALID_ADDRESS;
		/**
		 * Если повторяется элемент, допускающий пустое сопоставление
		 */
		if(nullable) {
			// Выполняем размещение инструкции проверки продвижения по тексту
			progress = this->emit(opcode_t::PROGRESS, node.flags);
			/**
			 * Если размещение инструкции не выполнено
			 */
			if(progress == INVALID_ADDRESS)
				// Выводим результат выполнения компиляции
				return false;
			// Выполняем установку номера ячейки позиции начала повторения
			this->_program->instructions.at(progress).progress.cell = guard;
		}
		// Выполняем размещение инструкции перехода к началу повторения
		const address_t jump = this->emit(opcode_t::JUMP, node.flags);
		/**
		 * Если размещение инструкции не выполнено
		 */
		if(jump == INVALID_ADDRESS)
			// Выводим результат выполнения компиляции
			return false;
		// Выполняем установку адреса перехода к началу повторения
		this->_program->instructions.at(jump).jump.target = start;
		/**
		 * Если размещена инструкция проверки продвижения по тексту
		 */
		if(progress != INVALID_ADDRESS)
			// Выполняем установку адреса завершения повторения
			this->_program->instructions.at(progress).progress.target = this->position();
		// Выполняем установку адреса ветви повторения элемента выражения
		this->_program->instructions.at(split).split.first = (greedy ? body : this->position());
		// Выполняем установку адреса ветви завершения повторения
		this->_program->instructions.at(split).split.second = (greedy ? this->position() : body);
		// Выводим результат выполнения компиляции
		return true;
	}
	// Создаём набор адресов инструкций завершения необязательных повторений
	vector <address_t> exits;
	/**
	 * Выполняем размещение необязательных повторений элемента выражения
	 */
	for(uint32_t i = least; i < most; i++) {
		// Выполняем размещение инструкции перехода по двум ветвям
		const address_t split = this->emit(opcode_t::SPLIT, node.flags);
		/**
		 * Если размещение инструкции не выполнено
		 */
		if(split == INVALID_ADDRESS)
			// Выводим результат выполнения компиляции
			return false;
		// Выполняем установку адреса ветви повторения элемента выражения
		this->_program->instructions.at(split).split.first = (greedy ? this->position() : INVALID_ADDRESS);
		/**
		 * Если компиляция повторяемого элемента выражения не выполнена
		 */
		if(!this->compileNode(child))
			// Выводим результат выполнения компиляции
			return false;
		/**
		 * Если квантор повторения является ленивым
		 */
		if(!greedy)
			// Выполняем установку адреса ветви повторения элемента выражения
			this->_program->instructions.at(split).split.second = (split + 1);
		// Выполняем добавление адреса инструкции перехода по двум ветвям
		exits.push_back(split);
	}
	/**
	 * Выполняем установку адресов завершения необязательных повторений
	 */
	for(auto & split : exits) {
		/**
		 * Если квантор повторения является жадным
		 */
		if(greedy)
			// Выполняем установку адреса ветви завершения повторения
			this->_program->instructions.at(split).split.second = this->position();
		// Выполняем установку адреса ветви завершения повторения
		else this->_program->instructions.at(split).split.first = this->position();
	}
	// Выводим результат выполнения компиляции
	return true;
}
/**
 * @brief Метод компиляции узла синтаксического дерева
 *
 * @param id индекс узла в арене узлов
 * @return   результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compileNode(const node_id_t id) noexcept {
	/**
	 * Если индекс узла отсутствует
	 */
	if(id == INVALID_NODE)
		// Выводим результат выполнения компиляции
		return true;
	// Получаем узел синтаксического дерева
	const node_data_t & node = this->_parser->node(id);
	/**
	 * Определяем тип узла синтаксического дерева
	 */
	switch(static_cast <uint8_t> (node.type)) {
		// Выполняем компиляцию узла пустого выражения
		case static_cast <uint8_t> (node_t::EMPTY): return true;
		// Выполняем компиляцию узла одиночного символа
		case static_cast <uint8_t> (node_t::LITERAL): {
			// Выполняем размещение инструкции сопоставления одиночного символа
			const address_t address = this->emit(opcode_t::CHAR, node.flags);
			/**
			 * Если размещение инструкции не выполнено
			 */
			if(address == INVALID_ADDRESS)
				// Выводим результат выполнения компиляции
				return false;
			// Выполняем установку кодового значения сопоставляемого символа
			this->_program->instructions.at(address).letter.code = node.literal.code;
			// Выводим результат выполнения компиляции
			return true;
		}
		// Выполняем компиляцию узла последовательности символов
		case static_cast <uint8_t> (node_t::STRING): {
			// Получаем адрес начала последовательности в хранилище разбора
			const uint32_t * source = this->_parser->sequence(node.string.offset, node.string.length);
			/**
			 * Если последовательность символов отсутствует
			 */
			if(source == nullptr) {
				// Выполняем установку внутренней ошибки компиляции
				this->_error = error_t::INTERNAL;
				// Выводим результат выполнения компиляции
				return false;
			}
			/**
			 * Выполняем размещение инструкций сопоставления символов последовательности
			 *
			 * @details Последовательность разворачивается в набор инструкций сопоставления
			 *          одиночных символов, благодаря чему исполнение программы остаётся
			 *          посимвольным и не требует отдельной обработки последовательностей.
			 *
			 */
			for(uint32_t i = 0; i < node.string.length; i++) {
				// Определяем положение сопоставляемого символа последовательности
				const uint32_t index = (this->_reverse ? ((node.string.length - i) - 1) : i);
				// Выполняем размещение инструкции сопоставления одиночного символа
				const address_t address = this->emit(opcode_t::CHAR, node.flags);
				/**
				 * Если размещение инструкции не выполнено
				 */
				if(address == INVALID_ADDRESS)
					// Выводим результат выполнения компиляции
					return false;
				// Выполняем установку кодового значения сопоставляемого символа
				this->_program->instructions.at(address).letter.code = source[index];
			}
			// Выводим результат выполнения компиляции
			return true;
		}
		// Выполняем компиляцию узла класса символов
		case static_cast <uint8_t> (node_t::CLASS): {
			// Получаем класс символов синтаксического дерева
			const class_t & value = this->_parser->charClass(node.charclass.index);
			// Выполняем размещение класса символов в программе
			const uint32_t index = this->store(value);
			// Выполняем размещение инструкции сопоставления класса символов
			const address_t address = this->emit(opcode_t::CLASS, node.flags);
			/**
			 * Если размещение инструкции не выполнено
			 */
			if(address == INVALID_ADDRESS)
				// Выводим результат выполнения компиляции
				return false;
			// Выполняем установку индекса класса символов
			this->_program->instructions.at(address).charclass.index = index;
			// Выводим результат выполнения компиляции
			return true;
		}
		// Выполняем компиляцию узла любого символа
		case static_cast <uint8_t> (node_t::ANY):
		// Выполняем компиляцию узла одиночной единицы кодирования
		case static_cast <uint8_t> (node_t::CODEUNIT): {
			/**
			 * Если сопоставляется единица кодирования в режиме разбора UTF-8
			 *
			 * @details Единица кодирования сопоставляет один байт, тогда как исполнение
			 *          без возврата продвигается по тексту посимвольно. Совмещение
			 *          обоих шагов в режиме UTF-8 недостижимо.
			 *
			 */
			if(!this->_full && (node.type == node_t::CODEUNIT) && ((this->_program->flags & static_cast <uint32_t> (flag_t::UTF)) != 0)) {
				// Выполняем установку ошибки неподдерживаемой конструкции
				this->_error = error_t::UNSUPPORTED;
				// Выводим результат выполнения компиляции
				return false;
			}
			// Определяем код операции размещаемой инструкции
			const opcode_t type = ((node.type == node_t::ANY) ? opcode_t::ANY : opcode_t::CODEUNIT);
			// Выполняем размещение инструкции сопоставления символа
			const address_t address = this->emit(type, node.flags);
			// Выводим результат выполнения компиляции
			return (address != INVALID_ADDRESS);
		}
		// Выполняем компиляцию узла привязки к позиции в тексте
		case static_cast <uint8_t> (node_t::ANCHOR): {
			/**
			 * Если привязка сбрасывает начало совпадения
			 *
			 * @details Сброс начала совпадения изменяет границы найденного совпадения
			 *          и требует исполнения с возвратом.
			 */
			if(node.anchor.type == anchor_t::KEEP_OUT) {
				/**
				 * Если выполняется компиляция регулярного подмножества
				 */
				if(!this->_full) {
					// Выполняем установку ошибки неподдерживаемой конструкции
					this->_error = error_t::UNSUPPORTED;
					// Выводим результат выполнения компиляции
					return false;
				}
				// Выводим результат размещения инструкции сброса начала совпадения
				return (this->emit(opcode_t::KEEP, node.flags) != INVALID_ADDRESS);
			}
			// Выполняем размещение инструкции проверки привязки
			const address_t address = this->emit(opcode_t::ANCHOR, node.flags);
			/**
			 * Если размещение инструкции не выполнено
			 */
			if(address == INVALID_ADDRESS)
				// Выводим результат выполнения компиляции
				return false;
			// Выполняем установку типа привязки к позиции в тексте
			this->_program->instructions.at(address).assertion.type = node.anchor.type;
			// Выводим результат выполнения компиляции
			return true;
		}
		// Выполняем компиляцию узла последовательности элементов
		case static_cast <uint8_t> (node_t::CONCAT): return this->compileChain(node.child);
		// Выполняем компиляцию узла выбора одной из ветвей
		case static_cast <uint8_t> (node_t::ALTERNATE): return this->compileAlternate(id);
		// Выполняем компиляцию узла повторения
		case static_cast <uint8_t> (node_t::REPEAT): return this->compileRepeat(id);
		// Выполняем компиляцию узла группы
		case static_cast <uint8_t> (node_t::GROUP): {
			/**
			 * Если группа является атомарной
			 *
			 * @details Атомарная группа запрещает возврат внутрь себя,
			 *          что недостижимо исполнением без возврата.
			 *
			 */
			if(node.group.type == group_t::ATOMIC) {
				/**
				 * Если выполняется компиляция регулярного подмножества
				 */
				if(!this->_full) {
					// Выполняем установку ошибки неподдерживаемой конструкции
					this->_error = error_t::UNSUPPORTED;
					// Выводим результат выполнения компиляции
					return false;
				}
				// Получаем номер ячейки отметки состояния возврата
				const uint32_t cell = this->_atomics++;
				// Выполняем размещение инструкции запоминания состояния возврата
				const address_t mark = this->emit(opcode_t::MARK, node.flags);
				/**
				 * Если размещение инструкции не выполнено
				 */
				if(mark == INVALID_ADDRESS)
					// Выводим результат выполнения компиляции
					return false;
				// Выполняем установку номера ячейки отметки состояния возврата
				this->_program->instructions.at(mark).atomic.cell = cell;
				/**
				 * Если компиляция тела группы не выполнена
				 */
				if(!this->compileChain(node.child))
					// Выводим результат выполнения компиляции
					return false;
				// Выполняем размещение инструкции отказа от точек возврата
				const address_t cut = this->emit(opcode_t::CUT, node.flags);
				/**
				 * Если размещение инструкции не выполнено
				 */
				if(cut == INVALID_ADDRESS)
					// Выводим результат выполнения компиляции
					return false;
				// Выполняем установку номера ячейки отметки состояния возврата
				this->_program->instructions.at(cut).atomic.cell = cell;
				// Выводим результат выполнения компиляции
				return true;
			}
			/**
			 * Если группа не выполняет захвата
			 */
			if(node.group.number == 0)
				// Выводим результат компиляции тела группы
				return this->compileChain(node.child);
			// Выполняем размещение инструкции сохранения начала захвата
			const address_t begin = this->emit(opcode_t::SAVE, node.flags);
			/**
			 * Если размещение инструкции не выполнено
			 */
			if(begin == INVALID_ADDRESS)
				// Выводим результат выполнения компиляции
				return false;
			// Выполняем установку номера ячейки начала захвата
			this->_program->instructions.at(begin).save.slot = (node.group.number * 2);
			/**
			 * Если компиляция тела группы не выполнена
			 */
			if(!this->compileChain(node.child))
				// Выводим результат выполнения компиляции
				return false;
			// Выполняем размещение инструкции сохранения конца захвата
			const address_t end = this->emit(opcode_t::SAVE, node.flags);
			/**
			 * Если размещение инструкции не выполнено
			 */
			if(end == INVALID_ADDRESS)
				// Выводим результат выполнения компиляции
				return false;
			// Выполняем установку номера ячейки конца захвата
			this->_program->instructions.at(end).save.slot = ((node.group.number * 2) + 1);
			// Выводим результат выполнения компиляции
			return true;
		}
	}
	/**
	 * Если выполняется компиляция выражения целиком
	 */
	if(this->_full) {
		/**
		 * Определяем тип узла синтаксического дерева
		 */
		switch(static_cast <uint8_t> (node.type)) {
			/**
			 * Выполняем компиляцию узла ссылки на захваченную группу
			 */
			case static_cast <uint8_t> (node_t::BACKREF): {
				// Выполняем размещение инструкции сопоставления захваченного текста
				const address_t address = this->emit(opcode_t::BACKREF, node.flags);
				/**
				 * Если размещение инструкции не выполнено
				 */
				if(address == INVALID_ADDRESS)
					// Выводим результат выполнения компиляции
					return false;
				// Выполняем установку номера группы, захваченный текст которой сопоставляется
				this->_program->instructions.at(address).backref.number = node.backref.number;
				// Выводим результат выполнения компиляции
				return true;
			}
			/**
			 * Выполняем компиляцию узла расширенного графемного кластера
			 *
			 * @details Графемный кластер сопоставляет переменное количество символов,
			 *          что несовместимо с продвижением по тексту в едином темпе,
			 *          и потому доступен только исполнению с возвратом.
			 *
			 */
			case static_cast <uint8_t> (node_t::GRAPHEME):
				// Выводим результат размещения инструкции сопоставления графемного кластера
				return (this->emit(opcode_t::GRAPHEME, node.flags) != INVALID_ADDRESS);
			/**
			 * Выполняем компиляцию узла рекурсивного вызова
			 */
			case static_cast <uint8_t> (node_t::RECURSE): {
				// Выполняем размещение инструкции рекурсивного вызова
				const address_t address = this->emit(opcode_t::CALL, node.flags);
				/**
				 * Если размещение инструкции не выполнено
				 */
				if(address == INVALID_ADDRESS)
					// Выводим результат выполнения компиляции
					return false;
				// Выполняем установку номера вызываемой группы
				this->_program->instructions.at(address).call.number = node.recurse.number;
				// Выполняем установку неизвестного адреса тела вызываемой группы
				this->_program->instructions.at(address).call.body = INVALID_ADDRESS;
				// Выполняем добавление инструкции в набор рекурсивных вызовов
				this->_calls.emplace_back(address, node.recurse.number);
				// Выводим результат выполнения компиляции
				return true;
			}
			/**
			 * Выполняем компиляцию узла проверки окружения
			 */
			case static_cast <uint8_t> (node_t::LOOKAROUND): {
				// Адрес размещённой инструкции проверки окружения
				address_t address = INVALID_ADDRESS;
				// Выводим результат компиляции узла проверки окружения
				return this->compileLook(id, address);
			}
			// Выводим результат компиляции узла условного выражения
			case static_cast <uint8_t> (node_t::CONDITION): return this->compileCondition(id);
		}
	}
	/**
	 * Выполняем компиляцию прочих узлов синтаксического дерева
	 *
	 * @details Графемные кластеры и свойства Юникода требуют таблиц свойств,
	 *          размещаемых отдельным модулем.
	 *
	 */
	this->_error = error_t::UNSUPPORTED;
	// Выводим результат выполнения компиляции
	return false;
}
/**
 * @brief Метод размещения ячейки состояния исполнения
 *
 * @return номер ячейки состояния в наборе позиций захвата групп
 *
 */
uint32_t awh::regex::Compiler::reserve() noexcept {
	/**
	 * Выполняем размещение ячейки состояния за ячейками захвата групп
	 *
	 * @details Ячейки состояния размещаются в наборе позиций захвата групп,
	 *          благодаря чему их изменение отменяется возвратом наравне
	 *          с изменением ячеек захвата.
	 *
	 */
	return (((this->_program->captures + 1) * 2) + (this->_cells++));
}
/**
 * @brief Метод сбора узлов захватывающих групп выражения
 *
 * @param id индекс узла, с которого начинается сбор
 *
 */
void awh::regex::Compiler::collect(const node_id_t id) noexcept {
	// Получаем индекс обходимого узла синтаксического дерева
	node_id_t current = id;
	/**
	 * Выполняем обход цепочки узлов синтаксического дерева
	 */
	while(current != INVALID_NODE) {
		// Получаем обходимый узел синтаксического дерева
		const node_data_t & node = this->_parser->node(current);
		/**
		 * Если узел является захватывающей группой
		 */
		if((node.type == node_t::GROUP) && (node.group.number != 0)) {
			/**
			 * Если группа с таким номером ещё не обнаружена
			 *
			 * @details Конструкция сброса нумерации ветвей допускает несколько групп
			 *          с одним номером, рекурсивный вызов при этом обращается к первой.
			 *
			 */
			if(this->_groups.count(node.group.number) == 0)
				// Выполняем сохранение индекса узла захватывающей группы
				this->_groups.emplace(node.group.number, current);
		}
		// Выполняем сбор узлов вложенной цепочки
		this->collect(node.child);
		// Переходим к следующему узлу цепочки
		current = node.next;
	}
}
/**
 * @brief Метод компиляции цепочки узлов синтаксического дерева
 *
 * @param id индекс первого узла цепочки в арене узлов
 * @return   результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compileChain(const node_id_t id) noexcept {
	/**
	 * Если выполняется компиляция развёрнутого регулярного выражения
	 *
	 * @details Развёрнутое выражение сопоставляет последовательность элементов
	 *          в обратном порядке, поэтому узлы цепочки компилируются с конца.
	 *
	 */
	if(this->_reverse) {
		// Создаём набор индексов узлов цепочки
		vector <node_id_t> items;
		/**
		 * Выполняем обход цепочки узлов одного уровня вложенности
		 */
		for(node_id_t index = id; index != INVALID_NODE; index = this->_parser->node(index).next)
			// Выполняем добавление индекса очередного узла цепочки
			items.push_back(index);
		/**
		 * Выполняем компиляцию узлов цепочки в обратном порядке
		 */
		for(size_t i = items.size(); i > 0; i--) {
			/**
			 * Если компиляция очередного узла цепочки не выполнена
			 */
			if(!this->compileNode(items.at(i - 1)))
				// Выводим результат выполнения компиляции
				return false;
		}
		// Выводим результат выполнения компиляции
		return true;
	}
	/**
	 * Выполняем обход цепочки узлов одного уровня вложенности
	 */
	for(node_id_t index = id; index != INVALID_NODE; index = this->_parser->node(index).next) {
		/**
		 * Если компиляция очередного узла цепочки не выполнена
		 */
		if(!this->compileNode(index))
			// Выводим результат выполнения компиляции
			return false;
	}
	// Выводим результат выполнения компиляции
	return true;
}
/**
 * @brief Метод извлечения литерала, сопоставляемого узлом целиком
 *
 * @param id индекс узла в арене узлов
 * @return   литерал, сопоставляемый узлом целиком
 *
 */
string awh::regex::Compiler::literal(const node_id_t id) const noexcept {
	// Получаем узел синтаксического дерева
	const node_data_t & node = this->_parser->node(id);
	/**
	 * Если узел сопоставляется без учёта регистра символов
	 *
	 * @details Литерал, сопоставляемый без учёта регистра, поиском по точному
	 *          совпадению не отыскивается и обязательным литералом не считается.
	 *
	 */
	if((node.flags & static_cast <uint32_t> (flag_t::CASELESS)) != 0)
		// Выводим отсутствие литерала узла
		return string();
	/**
	 * Если узел сопоставляет одиночный символ
	 */
	if(node.type == node_t::LITERAL) {
		/**
		 * Если символ не принадлежит набору ASCII
		 */
		if(node.literal.code > 0x7F)
			// Выводим отсутствие литерала узла
			return string();
		// Выводим литерал одиночного символа
		return string(1, static_cast <char> (node.literal.code));
	}
	/**
	 * Если узел сопоставляет последовательность символов
	 */
	if(node.type == node_t::STRING) {
		// Получаем адрес начала последовательности в хранилище разбора
		const uint32_t * source = this->_parser->sequence(node.string.offset, node.string.length);
		/**
		 * Если последовательность символов отсутствует
		 */
		if(source == nullptr)
			// Выводим отсутствие литерала узла
			return string();
		// Формируемый литерал последовательности символов
		string result;
		/**
		 * Выполняем формирование литерала последовательности символов
		 */
		for(uint32_t i = 0; i < node.string.length; i++) {
			/**
			 * Если символ не принадлежит набору ASCII
			 */
			if(source[i] > 0x7F)
				// Выводим отсутствие литерала узла
				return string();
			// Выполняем добавление символа в литерал последовательности
			result.append(1, static_cast <char> (source[i]));
		}
		// Выводим литерал последовательности символов
		return result;
	}
	// Выводим отсутствие литерала узла
	return string();
}
/**
 * @brief Метод извлечения обязательного литерала узла
 *
 * @param id индекс узла в арене узлов
 * @return   обязательный литерал совпадения узла
 *
 */
string awh::regex::Compiler::requiredNode(const node_id_t id) const noexcept {
	// Удаление обязательного литерала от начала сопоставления узла
	size_t distance = 0;
	// Выводим обязательный литерал совпадения узла
	return this->requiredNode(id, distance);
}
/**
 * @brief Метод извлечения обязательного литерала цепочки узлов
 *
 * @param id индекс первого узла цепочки в арене узлов
 * @return   обязательный литерал совпадения цепочки узлов
 *
 */
string awh::regex::Compiler::required(const node_id_t id) const noexcept {
	// Удаление обязательного литерала от начала совпадения
	size_t distance = 0;
	// Выводим обязательный литерал совпадения цепочки узлов
	return this->required(id, distance);
}
/**
 * @brief Метод извлечения наибольшей длины сопоставления узла
 *
 * @details Длина выводится в байтах текста, а не в кодовых значениях: позиционное
 *          употребление обязательного литерала ведётся над байтами. В режиме
 *          разбора UTF-8 одиночному символу отводится наибольшая длина
 *          последовательности, то есть четыре байта: точная длина зависит
 *          от кодового значения, а у класса символов не определена вовсе,
 *          и завышение здесь безопасно - оно лишь ослабляет пропуск позиций.
 *
 * @param id индекс узла в арене узлов
 * @return   наибольшая длина сопоставления узла в байтах
 *
 */
size_t awh::regex::Compiler::spanningNode(const node_id_t id) const noexcept {
	/**
	 * Если индекс узла отсутствует
	 */
	if(id == INVALID_NODE)
		// Выводим наибольшую длину сопоставления узла
		return 0;
	// Получаем узел синтаксического дерева
	const node_data_t & node = this->_parser->node(id);
	// Получаем наибольшую длину одиночного символа в байтах
	const size_t letter = (((this->_program->flags & static_cast <uint32_t> (flag_t::UTF)) != 0) ? 4 : 1);
	/**
	 * Определяем тип узла синтаксического дерева
	 */
	switch(static_cast <uint8_t> (node.type)) {
		// Пустое выражение текста не поглощает вовсе
		case static_cast <uint8_t> (node_t::EMPTY):
		// Привязка к позиции в тексте текста не поглощает вовсе
		case static_cast <uint8_t> (node_t::ANCHOR):
		// Проверка окружения текста не поглощает вовсе
		case static_cast <uint8_t> (node_t::LOOKAROUND):
			// Выводим наибольшую длину сопоставления узла
			return 0;
		// Одиночный символ поглощает единицу кодирования
		case static_cast <uint8_t> (node_t::LITERAL):
		// Класс символов поглощает единицу кодирования
		case static_cast <uint8_t> (node_t::CLASS):
		// Любой символ поглощает единицу кодирования
		case static_cast <uint8_t> (node_t::ANY):
			// Выводим наибольшую длину сопоставления узла
			return letter;
		// Одиночная единица кодирования поглощает ровно байт
		case static_cast <uint8_t> (node_t::CODEUNIT):
			// Выводим наибольшую длину сопоставления узла
			return 1;
		/**
		 * Выводим наибольшую длину последовательности символов
		 */
		case static_cast <uint8_t> (node_t::STRING):
			// Выводим наибольшую длину сопоставления узла
			return (static_cast <size_t> (node.string.length) * letter);
		// Выводим наибольшую длину тела последовательности элементов
		case static_cast <uint8_t> (node_t::CONCAT): return this->spanning(node.child);
		// Выводим наибольшую длину тела группы
		case static_cast <uint8_t> (node_t::GROUP): return this->spanning(node.child);
		/**
		 * Выводим наибольшую длину выбора одной из ветвей
		 *
		 * @details Ветви лежат дочерними узлами одного уровня, и длина выбора
		 *          есть наибольшая из длин их, а не сумма: сопоставляется
		 *          ветвь одна.
		 *
		 */
		case static_cast <uint8_t> (node_t::ALTERNATE): {
			// Наибольшая длина сопоставления ветвей выбора
			size_t result = 0;
			/**
			 * Выполняем обход ветвей выбора одной из них
			 */
			for(node_id_t index = node.child; index != INVALID_NODE; index = this->_parser->node(index).next) {
				// Получаем наибольшую длину сопоставления очередной ветви
				const size_t length = this->spanningNode(index);
				/**
				 * Если длина ветви не ограничена
				 */
				if(length == string_view::npos)
					// Выводим длину сопоставления неограниченную
					return string_view::npos;
				/**
				 * Если длина ветви наибольшую превышает
				 */
				if(length > result)
					// Выполняем установку наибольшей длины сопоставления
					result = length;
			}
			// Выводим наибольшую длину сопоставления узла
			return result;
		}
		/**
		 * Выводим наибольшую длину повторения дочернего узла
		 */
		case static_cast <uint8_t> (node_t::REPEAT): {
			/**
			 * Если верхнего предела числа повторений нет
			 */
			if(node.repeat.max == UNBOUNDED)
				// Выводим длину сопоставления неограниченную
				return string_view::npos;
			// Получаем наибольшую длину сопоставления тела повторения
			const size_t length = this->spanningNode(node.child);
			/**
			 * Если длина тела повторения не ограничена
			 */
			if(length == string_view::npos)
				// Выводим длину сопоставления неограниченную
				return string_view::npos;
			/**
			 * Если тело повторения текста не поглощает вовсе
			 */
			if(length == 0)
				// Выводим наибольшую длину сопоставления узла
				return 0;
			/**
			 * Если произведение предел разрядности превышает
			 *
			 * @details Число повторений задаётся выражением и доходит до предела
			 *          разрядности, отчего умножение обязано проверяться:
			 *          переполнение обратило бы длину неограниченную в малую
			 *          и пропустило бы позиции, пропуску не подлежащие.
			 *
			 */
			if(static_cast <size_t> (node.repeat.max) > (string_view::npos / length))
				// Выводим длину сопоставления неограниченную
				return string_view::npos;
			// Выводим наибольшую длину сопоставления узла
			return (static_cast <size_t> (node.repeat.max) * length);
		}
	}
	// Выводим длину сопоставления неограниченную
	return string_view::npos;
}
/**
 * @brief Метод извлечения наибольшей длины сопоставления цепочки узлов
 *
 * @param id индекс первого узла цепочки в арене узлов
 * @return   наибольшая длина сопоставления цепочки узлов в байтах
 *
 */
size_t awh::regex::Compiler::spanning(const node_id_t id) const noexcept {
	// Наибольшая длина сопоставления цепочки узлов
	size_t result = 0;
	/**
	 * Выполняем обход цепочки узлов одного уровня вложенности
	 */
	for(node_id_t index = id; index != INVALID_NODE; index = this->_parser->node(index).next) {
		// Получаем наибольшую длину сопоставления очередного узла
		const size_t length = this->spanningNode(index);
		/**
		 * Если длина узла не ограничена
		 */
		if(length == string_view::npos)
			// Выводим длину сопоставления неограниченную
			return string_view::npos;
		/**
		 * Если сумма предел разрядности превышает
		 */
		if(length > (string_view::npos - result))
			// Выводим длину сопоставления неограниченную
			return string_view::npos;
		// Выполняем накопление наибольшей длины сопоставления
		result += length;
	}
	// Выводим наибольшую длину сопоставления цепочки узлов
	return result;
}
/**
 * @brief Метод извлечения обязательного литерала узла с удалением его
 *
 * @param id       индекс узла в арене узлов
 * @param distance наибольшее удаление литерала от начала сопоставления узла
 * @return         обязательный литерал совпадения узла
 *
 */
string awh::regex::Compiler::requiredNode(const node_id_t id, size_t & distance) const noexcept {
	// Выполняем сброс удаления литерала от начала сопоставления узла
	distance = 0;
	/**
	 * Если индекс узла отсутствует
	 */
	if(id == INVALID_NODE)
		// Выводим отсутствие обязательного литерала
		return string();
	// Получаем узел синтаксического дерева
	const node_data_t & node = this->_parser->node(id);
	/**
	 * Определяем тип узла синтаксического дерева
	 */
	switch(static_cast <uint8_t> (node.type)) {
		// Выводим обязательный литерал тела последовательности элементов
		case static_cast <uint8_t> (node_t::CONCAT): return this->required(node.child, distance);
		// Выводим обязательный литерал тела группы
		case static_cast <uint8_t> (node_t::GROUP): return this->required(node.child, distance);
		/**
		 * Выводим обязательный литерал повторяемого элемента выражения
		 *
		 * @details Литерал повторения содержится в каждом проходе его, отчего
		 *          вхождение ближайшее лежит в проходе первом, и удаление
		 *          берётся внутри тела, числу повторений не умножаясь.
		 *
		 */
		case static_cast <uint8_t> (node_t::REPEAT): {
			/**
			 * Если повторяемый элемент выражения может не сопоставляться ни разу
			 */
			if(node.repeat.min == 0)
				// Выводим отсутствие обязательного литерала
				return string();
			// Выводим обязательный литерал повторяемого элемента выражения
			return this->requiredNode(node.child, distance);
		}
	}
	// Выводим литерал, сопоставляемый узлом целиком
	return this->literal(id);
}
/**
 * @brief Метод извлечения обязательного литерала цепочки узлов с удалением его
 *
 * @details Разбор литерала ведётся здесь, а вид без удаления обращается сюда же:
 *          два разбора разошлись бы рано или поздно, и позиционное употребление
 *          получило бы литерал один, а проверка возможности совпадения - иной.
 *
 *          Удаление накапливается наибольшими длинами узлов, литералу
 *          предшествующих. Узел длины неограниченной обращает его
 *          в «string_view::npos», чем позиционное употребление и отменяется:
 *          литерал такой в тексте отыскивается, но начала совпадения
 *          не ограничивает.
 *
 * @param id       индекс первого узла цепочки в арене узлов
 * @param distance наибольшее удаление литерала от начала совпадения
 * @return         обязательный литерал совпадения цепочки узлов
 *
 */
string awh::regex::Compiler::required(const node_id_t id, size_t & distance) const noexcept {
	// Наибольший обнаруженный обязательный литерал
	string result;
	// Литерал, накапливаемый по смежным узлам цепочки
	string run;
	// Удаление наибольшего обнаруженного литерала от начала совпадения
	size_t found = 0;
	// Удаление накапливаемого литерала от начала совпадения
	size_t reach = 0;
	// Наибольшее число байтов, цепочкой до очередного узла поглощаемое
	size_t passed = 0;
	/**
	 * Выполняем обход цепочки узлов одного уровня вложенности
	 */
	for(node_id_t index = id; index != INVALID_NODE; index = this->_parser->node(index).next) {
		// Получаем литерал, сопоставляемый очередным узлом целиком
		const string value = this->literal(index);
		/**
		 * Если очередной узел сопоставляет литерал целиком
		 *
		 * @details Смежные узлы литералов образуют непрерывную последовательность
		 *          символов, присутствующую в любом совпадении выражения.
		 *
		 */
		if(!value.empty()) {
			/**
			 * Если накопление литерала лишь начинается
			 */
			if(run.empty())
				// Выполняем установку удаления накапливаемого литерала
				reach = passed;
			// Выполняем накопление литерала по смежным узлам
			run.append(value);
			/**
			 * Если длина цепочки ещё ограничена
			 */
			if(passed != string_view::npos)
				// Выполняем накопление длины, цепочкой поглощаемой
				passed += value.size();
			// Переходим к следующему узлу цепочки
			continue;
		}
		/**
		 * Если накопленный литерал длиннее обнаруженного
		 */
		if(run.size() > result.size()) {
			// Выполняем установку наибольшего обнаруженного литерала
			result = run;
			// Выполняем установку удаления обнаруженного литерала
			found = reach;
		}
		// Выполняем сброс накопленного литерала
		run.clear();
		// Удаление обязательного литерала узла от начала сопоставления его
		size_t spacing = 0;
		// Получаем обязательный литерал очередного узла цепочки
		const string nested = this->requiredNode(index, spacing);
		/**
		 * Если обязательный литерал узла длиннее обнаруженного
		 */
		if(nested.size() > result.size()) {
			// Выполняем установку наибольшего обнаруженного литерала
			result = nested;
			/**
			 * Если удаление литерала узла либо длина цепочки не ограничены
			 */
			if((passed == string_view::npos) || (spacing == string_view::npos) ||
			 (spacing > (string_view::npos - passed)))
				// Выполняем установку удаления неограниченного
				found = string_view::npos;
			// Выполняем установку удаления обнаруженного литерала
			else found = (passed + spacing);
		}
		/**
		 * Если длина цепочки ещё ограничена
		 */
		if(passed != string_view::npos) {
			// Получаем наибольшую длину сопоставления очередного узла
			const size_t length = this->spanningNode(index);
			/**
			 * Если длина узла не ограничена
			 */
			if((length == string_view::npos) || (length > (string_view::npos - passed)))
				// Выполняем установку длины цепочки неограниченной
				passed = string_view::npos;
			// Выполняем накопление длины, цепочкой поглощаемой
			else passed += length;
		}
	}
	/**
	 * Если накопленный литерал длиннее обнаруженного
	 */
	if(run.size() > result.size()) {
		// Выполняем установку наибольшего обнаруженного литерала
		result = run;
		// Выполняем установку удаления обнаруженного литерала
		found = reach;
	}
	// Выполняем установку удаления обнаруженного литерала
	distance = found;
	// Выводим наибольший обнаруженный обязательный литерал
	return result;
}
/**
 * @brief Метод дополнения набора допустимых начальных байтов
 *
 * @param address адрес инструкции, с которой начинается обход
 * @return        результат применимости набора допустимых байтов
 *
 */
bool awh::regex::Compiler::reachable(const address_t address) noexcept {
	// Получаем предварительный отбор позиций сопоставления
	prefilter_t & prefilter = this->_program->prefilter;
	// Создаём набор отметок посещения инструкций программы
	vector <bool> visited(this->_program->instructions.size(), false);
	// Создаём стек обхода инструкций программы
	vector <address_t> stack;
	// Выполняем размещение исходной инструкции в стеке обхода
	stack.push_back(address);
	/**
	 * Выполняем обход инструкций, достижимых без сопоставления символов
	 */
	while(!stack.empty()) {
		// Получаем адрес инструкции из вершины стека обхода
		const address_t current = stack.back();
		// Выполняем удаление адреса инструкции из стека обхода
		stack.pop_back();
		/**
		 * Если адрес инструкции находится за пределами программы
		 */
		if(static_cast <size_t> (current) >= this->_program->instructions.size())
			// Переходим к следующему адресу стека обхода
			continue;
		/**
		 * Если инструкция уже была посещена
		 */
		if(visited.at(current))
			// Переходим к следующему адресу стека обхода
			continue;
		// Выполняем отметку посещения инструкции
		visited.at(current) = true;
		// Получаем обходимую инструкцию программы
		const instruction_t & instruction = this->_program->instructions.at(current);
		/**
		 * Определяем код операции обходимой инструкции
		 */
		switch(static_cast <uint8_t> (instruction.type)) {
			/**
			 * Выполняем обход ветвей перехода по двум ветвям
			 */
			case static_cast <uint8_t> (opcode_t::SPLIT): {
				// Выполняем размещение ветви с наибольшим приоритетом в стеке
				stack.push_back(instruction.split.first);
				// Выполняем размещение ветви с наименьшим приоритетом в стеке
				stack.push_back(instruction.split.second);
			} break;
			// Выполняем обход инструкции безусловного перехода
			case static_cast <uint8_t> (opcode_t::JUMP): stack.push_back(instruction.jump.target); break;
			// Выполняем обход инструкции сохранения позиции
			case static_cast <uint8_t> (opcode_t::SAVE):
			// Выполняем обход инструкции проверки привязки к позиции в тексте
			case static_cast <uint8_t> (opcode_t::ANCHOR): stack.push_back(current + 1); break;
			/**
			 * Если достигнуто завершение сопоставления
			 *
			 * @details Достижение завершения сопоставления без сопоставления символов
			 *          означает возможность совпадения нулевой длины в любой позиции,
			 *          при котором пропуск позиций недопустим.
			 *
			 */
			case static_cast <uint8_t> (opcode_t::MATCH): return false;
			/**
			 * Выполняем дополнение набора допустимых начальных байтов
			 */
			case static_cast <uint8_t> (opcode_t::CHAR): {
				// Получаем кодовое значение сопоставляемого символа
				const uint32_t code = instruction.letter.code;
				/**
				 * Если символ не принадлежит набору ASCII
				 */
				if(code > 0x7F) {
					/**
					 * Выполняем разрешение байтов начала последовательности UTF-8
					 */
					for(size_t i = 0x80; i < 256; i++)
						// Выполняем разрешение очередного байта
						prefilter.bytes[i] = true;
					// Выходим из обработки инструкции
					break;
				}
				// Выполняем разрешение байта сопоставляемого символа
				prefilter.bytes[code] = true;
				/**
				 * Если установлен режим сопоставления без учёта регистра
				 */
				if((instruction.flags & static_cast <uint32_t> (flag_t::CASELESS)) != 0) {
					/**
					 * Если символ является строчной буквой набора ASCII
					 */
					if((code >= 0x61) && (code <= 0x7A))
						// Выполняем разрешение байта прописной буквы
						prefilter.bytes[code - 0x20] = true;
					/**
					 * Если символ является прописной буквой набора ASCII
					 */
					else if((code >= 0x41) && (code <= 0x5A))
						// Выполняем разрешение байта строчной буквы
						prefilter.bytes[code + 0x20] = true;
				}
			} break;
			/**
			 * Выполняем дополнение набора байтами класса символов
			 */
			case static_cast <uint8_t> (opcode_t::CLASS): {
				// Получаем класс символов сопоставляемой инструкции
				const classview_t value = this->_program->charclass(instruction.charclass.index);
				/**
				 * Если класс символов задан свойствами Юникода
				 *
				 * @details Набор символов, обладающих свойством Юникода, задан таблицами
				 *          свойств и перечислению байтами не подлежит, поэтому набор
				 *          допустимых начальных байтов для такого класса неприменим.
				 *
				 */
				if(!value.properties.empty())
					// Выводим результат применимости набора допустимых байтов
					return false;
				/**
				 * Выполняем разрешение байтов, принадлежащих классу символов
				 */
				for(size_t i = 0; i < 256; i++) {
					// Флаг принадлежности байта набору диапазонов класса
					bool belongs = false;
					// Создаём набор проверяемых кодовых значений байта
					uint32_t codes[2] = {static_cast <uint32_t> (i), static_cast <uint32_t> (i)};
					/**
					 * Если установлен режим сопоставления без учёта регистра
					 *
					 * @details Класс символов проверяется также для символа в ином
					 *          регистре, иначе байты, допустимые лишь в ином регистре,
					 *          не попали бы в набор и совпадение было бы пропущено.
					 *
					 */
					if((instruction.flags & static_cast <uint32_t> (flag_t::CASELESS)) != 0) {
						/**
						 * Если байт является строчной буквой набора ASCII
						 */
						if((i >= 0x61) && (i <= 0x7A))
							// Выполняем установку кодового значения прописной буквы
							codes[1] = static_cast <uint32_t> (i - 0x20);
						/**
						 * Если байт является прописной буквой набора ASCII
						 */
						else if((i >= 0x41) && (i <= 0x5A))
							// Выполняем установку кодового значения строчной буквы
							codes[1] = static_cast <uint32_t> (i + 0x20);
					}
					/**
					 * Выполняем поиск проверяемых кодовых значений в диапазонах класса
					 */
					for(auto & code : codes) {
						/**
						 * Выполняем поиск кодового значения в наборе диапазонов класса
						 */
						for(auto & range : value.ranges) {
							/**
							 * Если кодовое значение принадлежит очередному диапазону класса
							 */
							if((code >= range.begin) && (code <= range.end)) {
								// Выполняем установку флага принадлежности байта
								belongs = true;
								// Выходим из цикла поиска кодового значения
								break;
							}
						}
						/**
						 * Если принадлежность байта набору диапазонов подтверждена
						 */
						if(belongs)
							// Выходим из цикла поиска проверяемых кодовых значений
							break;
					}
					/**
					 * Если принадлежность байта классу символов подтверждена
					 */
					if(belongs != value.negative)
						// Выполняем разрешение байта класса символов
						prefilter.bytes[i] = true;
				}
				/**
				 * Если класс символов задан со знаком отрицания либо охватывает символы вне ASCII
				 *
				 * @details Диапазоны класса заданы кодовыми значениями символов, тогда как
				 *          отбор выполняется по байтам, поэтому символам вне набора ASCII
				 *          соответствуют байты начала последовательности целиком.
				 *
				 */
				if(this->_program->prefilter.utf) {
					/**
					 * Выполняем разрешение байтов начала последовательности UTF-8
					 */
					for(size_t i = 0x80; i < 256; i++)
						// Выполняем разрешение очередного байта
						prefilter.bytes[i] = true;
				}
			} break;
			/**
			 * Выполняем дополнение набора байтами любого символа
			 */
			case static_cast <uint8_t> (opcode_t::ANY):
			/**
			 * Выполняем дополнение набора байтами одиночной единицы кодирования
			 */
			case static_cast <uint8_t> (opcode_t::CODEUNIT): {
				// Определяем разрешение байта перевода строки
				const bool newline = ((instruction.type == opcode_t::CODEUNIT) ||
				 ((instruction.flags & static_cast <uint32_t> (flag_t::DOTALL)) != 0));
				/**
				 * Выполняем разрешение байтов любого символа
				 */
				for(size_t i = 0; i < 256; i++) {
					/**
					 * Если байт является переводом строки и его разрешение недопустимо
					 */
					if((i == 0x0A) && !newline)
						// Переходим к следующему байту
						continue;
					// Выполняем разрешение очередного байта
					prefilter.bytes[i] = true;
				}
			} break;
			/**
			 * Выполняем отказ от набора допустимых начальных байтов
			 *
			 * @details Инструкции конструкций вне регулярного подмножества передают
			 *          исполнение способами, не выводимыми обходом программы, поэтому
			 *          набор байтов для них неприменим.
			 *
			 */
			default: return false;
		}
	}
	// Выводим результат применимости набора допустимых байтов
	return true;
}
/**
 * @brief Метод извлечения ведущего литерала совпадения
 *
 * @param id индекс первого узла цепочки в арене узлов
 * @return   ведущий литерал совпадения выражения
 *
 */
string awh::regex::Compiler::leading(const node_id_t id) const noexcept {
	// Формируемый ведущий литерал совпадения
	string result;
	/**
	 * Выполняем обход цепочки узлов одного уровня вложенности
	 */
	for(node_id_t index = id; index != INVALID_NODE; index = this->_parser->node(index).next) {
		// Получаем очередной узел синтаксического дерева
		const node_data_t & node = this->_parser->node(index);
		/**
		 * Если узел является привязкой к позиции в тексте
		 *
		 * @details Привязка длины не имеет и начала совпадения не смещает,
		 *          поэтому ведущий литерал ею не прерывается. Сброс начала
		 *          совпадения смещает его границу и литерал прерывает.
		 *
		 */
		if(node.type == node_t::ANCHOR) {
			/**
			 * Если привязка сбрасывает начало совпадения
			 */
			if(node.anchor.type == anchor_t::KEEP_OUT)
				// Выводим отсутствие ведущего литерала
				return string();
			// Переходим к следующему узлу цепочки
			continue;
		}
		/**
		 * Если узел является группой
		 *
		 * @details Захватывающая группа границ совпадения не смещает, поэтому
		 *          ведущий литерал её тела является ведущим литералом цепочки.
		 *
		 */
		if(node.type == node_t::GROUP) {
			// Выполняем добавление ведущего литерала тела группы
			result.append(this->leading(node.child));
			// Выходим из обхода цепочки узлов
			break;
		}
		/**
		 * Если узел является последовательностью элементов
		 *
		 * @details Ведущий литерал вложенной цепочки продолжает накопленный,
		 *          но продолжение обхода за нею недопустимо: полнота сопоставления
		 *          вложенной цепочки литералом отсюда не выводится.
		 *
		 */
		if(node.type == node_t::CONCAT) {
			// Выполняем добавление ведущего литерала вложенной цепочки
			result.append(this->leading(node.child));
			// Выходим из обхода цепочки узлов
			break;
		}
		// Получаем литерал, сопоставляемый очередным узлом целиком
		const string value = this->literal(index);
		/**
		 * Если очередной узел литерал не сопоставляет
		 */
		if(value.empty())
			// Выходим из обхода цепочки узлов
			break;
		// Выполняем добавление литерала узла в ведущий литерал
		result.append(value);
	}
	// Выводим ведущий литерал совпадения выражения
	return result;
}
/**
 * @brief Метод распознавания выражения, сопоставляемого литералом
 *
 */
void awh::regex::Compiler::condense() noexcept {
	// Получаем компилируемую программу регулярного выражения
	program_t & program = * this->_program;
	// Выполняем сброс признака сопоставления выражения литералом
	program.plain = false;
	// Выполняем очистку последовательности символов выражения
	program.text.clear();
	/**
	 * Если выражение выполняет захват групп
	 *
	 * @details Захват групп требует установки их границ, что достижимо
	 *          исполнением программы, но не поиском последовательности.
	 *
	 */
	if(program.captures > 0)
		// Выходим из метода распознавания выражения
		return;
	// Получаем набор инструкций программы регулярного выражения
	const Sequence <instruction_t> & instructions = program.instructions;
	/**
	 * Если набор инструкций короче наименьшего допустимого
	 *
	 * @details Наименьший набор состоит из сохранения границ совпадения
	 *          и завершения сопоставления с успехом.
	 *
	 */
	if(instructions.size() < 3)
		// Выходим из метода распознавания выражения
		return;
	/**
	 * Если выражение привязано к позиции в тексте
	 *
	 * @details Привязка ограничивает позиции совпадения, тогда как поиск
	 *          последовательности находит её в любой позиции текста.
	 *
	 */
	if(((program.flags & static_cast <uint32_t> (flag_t::ANCHORED)) != 0) || ((program.flags & static_cast <uint32_t> (flag_t::NOTEMPTY)) != 0))
		// Выходим из метода распознавания выражения
		return;
	/**
	 * Если первая инструкция границу совпадения не сохраняет
	 */
	if((instructions.front().type != opcode_t::SAVE) || (instructions.front().save.slot != 0))
		// Выходим из метода распознавания выражения
		return;
	/**
	 * Если последняя инструкция сопоставление не завершает
	 */
	if(instructions.back().type != opcode_t::MATCH)
		// Выходим из метода распознавания выражения
		return;
	/**
	 * Если предпоследняя инструкция границу совпадения не сохраняет
	 */
	if((instructions.at(instructions.size() - 2).type != opcode_t::SAVE) ||
	 (instructions.at(instructions.size() - 2).save.slot != 1))
		// Выходим из метода распознавания выражения
		return;
	/**
	 * Выполняем сбор последовательности символов выражения
	 */
	for(size_t i = 1; i < (instructions.size() - 2); i++) {
		// Получаем очередную инструкцию программы
		const instruction_t & instruction = instructions.at(i);
		/**
		 * Если инструкция одиночный символ не сопоставляет
		 */
		if(instruction.type != opcode_t::CHAR)
			// Выходим из метода распознавания выражения
			return;
		/**
		 * Если символ сопоставляется без учёта регистра
		 *
		 * @details Приведение регистра изменяет длину символа в байтах,
		 *          поэтому поиск последовательности к нему неприменим.
		 *
		 */
		if((instruction.flags & static_cast <uint32_t> (flag_t::CASELESS)) != 0)
			// Выходим из метода распознавания выражения
			return;
		// Получаем кодовое значение сопоставляемого символа
		const uint32_t code = instruction.letter.code;
		/**
		 * Если символ принадлежит набору ASCII
		 */
		if(code < 0x80) {
			// Выполняем добавление символа в последовательность выражения
			program.text.append(1, static_cast <char> (code));
			// Переходим к следующей инструкции программы
			continue;
		}
		/**
		 * Если режим разбора текста как последовательности UTF-8 не установлен
		 *
		 * @details Вне режима UTF-8 инструкция сопоставляет одиночный байт,
		 *          кодовое значение которого за пределы байта не выходит.
		 *
		 */
		if((instruction.flags & static_cast <uint32_t> (flag_t::UTF)) == 0) {
			/**
			 * Если кодовое значение за пределы байта выходит
			 */
			if(code > 0xFF)
				// Выходим из метода распознавания выражения
				return;
			// Выполняем добавление байта в последовательность выражения
			program.text.append(1, static_cast <char> (code));
			// Переходим к следующей инструкции программы
			continue;
		}
		/**
		 * Если символ состоит из двух байтов
		 */
		if(code < 0x800) {
			// Выполняем добавление первого байта последовательности UTF-8
			program.text.append(1, static_cast <char> (0xC0 | (code >> 6)));
			// Выполняем добавление продолжающего байта последовательности UTF-8
			program.text.append(1, static_cast <char> (0x80 | (code & 0x3F)));
		/**
		 * Если символ состоит из трёх байтов
		 */
		} else if(code < 0x10000) {
			// Выполняем добавление первого байта последовательности UTF-8
			program.text.append(1, static_cast <char> (0xE0 | (code >> 12)));
			// Выполняем добавление продолжающего байта последовательности UTF-8
			program.text.append(1, static_cast <char> (0x80 | ((code >> 6) & 0x3F)));
			// Выполняем добавление продолжающего байта последовательности UTF-8
			program.text.append(1, static_cast <char> (0x80 | (code & 0x3F)));
		/**
		 * Выполняем добавление символа из четырёх байтов
		 */
		} else {
			// Выполняем добавление первого байта последовательности UTF-8
			program.text.append(1, static_cast <char> (0xF0 | (code >> 18)));
			// Выполняем добавление продолжающего байта последовательности UTF-8
			program.text.append(1, static_cast <char> (0x80 | ((code >> 12) & 0x3F)));
			// Выполняем добавление продолжающего байта последовательности UTF-8
			program.text.append(1, static_cast <char> (0x80 | ((code >> 6) & 0x3F)));
			// Выполняем добавление продолжающего байта последовательности UTF-8
			program.text.append(1, static_cast <char> (0x80 | (code & 0x3F)));
		}
	}
	/**
	 * Если последовательность символов выражения пуста
	 *
	 * @details Пустое выражение сопоставляется в любой позиции текста,
	 *          что поиском последовательности не выражается.
	 *
	 */
	if(program.text.empty())
		// Выходим из метода распознавания выражения
		return;
	// Выполняем установку признака сопоставления выражения литералом
	program.plain = true;
}
/**
 * @brief Метод формирования предварительного отбора позиций
 *
 */
void awh::regex::Compiler::analyze() noexcept {
	// Получаем предварительный отбор позиций сопоставления
	prefilter_t & prefilter = this->_program->prefilter;
	// Выполняем очистку предварительного отбора позиций
	prefilter.clear();
	// Выполняем установку режима разбора текста как последовательности UTF-8
	prefilter.utf = ((this->_program->flags & static_cast <uint32_t> (flag_t::UTF)) != 0);
	// Выполняем формирование набора допустимых начальных байтов
	prefilter.active = this->reachable(0);
	/**
	 * Если набор допустимых начальных байтов неприменим
	 */
	if(!prefilter.active)
		// Выполняем очистку набора допустимых начальных байтов
		prefilter.clear();
	// Выполняем установку режима разбора текста как последовательности UTF-8
	prefilter.utf = ((this->_program->flags & static_cast <uint32_t> (flag_t::UTF)) != 0);
	/**
	 * Выполняем определение обязательного литерала совпадения и удаления его
	 *
	 * @details Удаление берётся тем же разбором, что и литерал: оно есть
	 *          наибольшее число байтов, какое способно предшествовать литералу
	 *          внутри совпадения, и позволяет употребить литерал позиционно.
	 *
	 */
	prefilter.literal = this->required(this->_parser->root(), prefilter.distance);
	/**
	 * Если набор допустимых начальных байтов применим
	 *
	 * @details Ведущий литерал отыскивает позиции возможного начала совпадения
	 *          и применяется наравне с набором допустимых начальных байтов,
	 *          поэтому его неприменимость означает и неприменимость литерала.
	 *
	 */
	if(prefilter.active)
		// Выполняем определение ведущего литерала совпадения
		prefilter.leading = this->leading(this->_parser->root());
	// Выполняем завершение формирования отбора позиций сопоставления
	prefilter.finalize();
}
/**
 * @brief Метод проверки обязательного продвижения узла по тексту
 *
 * @details Разбор ведётся по строению узла, а не по одному его типу: цепочка
 *          продвигается, если продвигается хоть один её узел, а выбор ветвей -
 *          если продвигается всякая ветвь его. Узлы, пустое сопоставление
 *          каких определяется составом конструкций вне регулярного
 *          подмножества, продвигающимися не признаются: вывод там ненадёжен,
 *          а цена ошибки - несостоявшийся сторож продвижения и повторение,
 *          на пустом теле не прекращающееся.
 *
 *          Разбор этот заведён затем, что сторож продвижения обходится
 *          дорого: он размещает ячейку состояния и две инструкции на всякое
 *          неограниченное повторение, а ячейку эту порождение машинного кода
 *          от границы захвата не отличает и потому переводит повторение
 *          на запись кадра на проход, а внутри атомарной группы отвергает
 *          порождение вовсе. Прежде разбор признавал продвижение за одним
 *          лишь одиночным символом, отчего «(?:ab|cd)*+» сторожа получало
 *          заведомо напрасно.
 *
 * @param id индекс проверяемого узла в арене узлов
 * @return   результат проверки обязательного продвижения узла по тексту
 *
 */
bool awh::regex::Compiler::advancing(const node_id_t id) const noexcept {
	// Создаём след узлов, проверка каких не завершена
	vector <node_id_t> visited;
	// Выводим результат проверки обязательного продвижения узла по тексту
	return this->advancing(id, visited);
}
/**
 * @brief Метод проверки обязательного продвижения узла по тексту со следом обхода
 *
 * @param id      индекс проверяемого узла в арене узлов
 * @param visited след узлов, проверка каких не завершена
 * @return        результат проверки обязательного продвижения узла по тексту
 *
 */
bool awh::regex::Compiler::advancing(const node_id_t id, vector <node_id_t> & visited) const noexcept {
	/**
	 * Если проверяемый узел отсутствует
	 */
	if(id == INVALID_NODE)
		// Выводим результат проверки обязательного продвижения узла по тексту
		return false;
	/**
	 * Если проверка узла уже ведётся
	 *
	 * @details Круговой обход возникает у рекурсивного вызова: он ссылается
	 *          на выражение, вызов этот содержащее. Продвижения такой узел
	 *          не подтверждает - подтверждение сняло бы сторожа продвижения.
	 *
	 */
	if(std::find(visited.begin(), visited.end(), id) != visited.end())
		// Выводим результат проверки обязательного продвижения узла по тексту
		return false;
	// Получаем проверяемый узел арены узлов
	const node_data_t & node = this->_parser->node(id);
	/**
	 * Определяем тип проверяемого узла выражения
	 *
	 * @details Перечисленные узлы сопоставляют по одному символу и пустого
	 *          сопоставления не допускают ни при каком составе выражения,
	 *          отчего продвижение по тексту им обеспечено.
	 *
	 */
	switch(static_cast <uint8_t> (node.type)) {
		// Одиночный символ продвигается по тексту обязательно
		case static_cast <uint8_t> (node_t::LITERAL):
		// Класс символов продвигается по тексту обязательно
		case static_cast <uint8_t> (node_t::CLASS):
		// Любой символ продвигается по тексту обязательно
		case static_cast <uint8_t> (node_t::ANY):
		// Одиночная единица кодирования продвигается по тексту обязательно
		case static_cast <uint8_t> (node_t::CODEUNIT):
		// Расширенный графемный кластер продвигается по тексту обязательно
		case static_cast <uint8_t> (node_t::GRAPHEME): return true;
		/**
		 * Если узел является последовательностью символов
		 *
		 * @details Последовательность пустой не бывает: узел этот заводится
		 *          разбором лишь для двух символов и более.
		 *
		 */
		case static_cast <uint8_t> (node_t::STRING): return true;
		/**
		 * Если узел является цепочкой последовательного сопоставления
		 *
		 * @details Цепочка продвигается, если продвигается хоть один узел её:
		 *          продвижение одного узла отменить остальные не могут.
		 *
		 */
		case static_cast <uint8_t> (node_t::CONCAT): {
			/**
			 * Выполняем обход узлов цепочки одного уровня вложенности
			 */
			for(node_id_t index = node.child; index != INVALID_NODE; index = this->_parser->node(index).next) {
				/**
				 * Если очередной узел цепочки продвигается по тексту
				 */
				if(this->advancing(index, visited))
					// Выводим результат проверки обязательного продвижения
					return true;
			}
			// Выводим результат проверки обязательного продвижения узла по тексту
			return false;
		}
		/**
		 * Если узел является выбором одной из ветвей
		 *
		 * @details Выбор продвигается, лишь если продвигается всякая ветвь его:
		 *          ветвь, пустое сопоставление допускающая, даёт пустое
		 *          сопоставление выбору целиком.
		 *
		 */
		case static_cast <uint8_t> (node_t::ALTERNATE): {
			// Устанавливаем признак наличия ветвей выбора
			bool branching = false;
			/**
			 * Выполняем обход ветвей выбора одного уровня вложенности
			 */
			for(node_id_t index = node.child; index != INVALID_NODE; index = this->_parser->node(index).next) {
				// Запоминаем наличие ветвей выбора
				branching = true;
				/**
				 * Если очередная ветвь выбора по тексту не продвигается
				 */
				if(!this->advancing(index, visited))
					// Выводим результат проверки обязательного продвижения
					return false;
			}
			// Выводим результат проверки обязательного продвижения узла по тексту
			return branching;
		}
		/**
		 * Если узел является группой
		 *
		 * @details Группа захватывающая от незахватывающей продвижением
		 *          не отличается: запись границ по тексту не двигает.
		 *
		 */
		case static_cast <uint8_t> (node_t::GROUP): return this->advancing(node.child, visited);
		/**
		 * Если узел является повторением
		 *
		 * @details Повторение продвигается, лишь если оно обязательно хотя бы
		 *          однажды и продвигается тело его. Повторение с нулевым
		 *          наименьшим числом пустое сопоставление допускает всегда.
		 *
		 */
		case static_cast <uint8_t> (node_t::REPEAT):
			// Выводим результат проверки обязательного продвижения узла по тексту
			return ((node.repeat.min > 0) && this->advancing(node.child, visited));
		/**
		 * Если узел является рекурсивным вызовом
		 *
		 * @details Вызов продвигается, если продвигается вызываемое им: сопоставление
		 *          вызова есть сопоставление тела его. Вызываемым выступает корень
		 *          дерева при номере нулевом и узел группы при номере ином; группа,
		 *          сборкою ещё не пройденная, в наборе отсутствует, и продвижения
		 *          вызов её не подтверждает.
		 *
		 *          Проверка эта избавляет повторение, рекурсивный вызов содержащее,
		 *          от сторожа продвижения - а сторож тот кодогенерации не поддаётся
		 *          и оставлял выражения вида «\((?:[^()]|(?R))*\)» без порождения
		 *          машинного кода вовсе.
		 *
		 */
		case static_cast <uint8_t> (node_t::RECURSE): {
			// Выполняем добавление проверяемого узла в след обхода
			visited.push_back(id);
			// Индекс узла вызываемого рекурсивным вызовом
			node_id_t called = INVALID_NODE;
			/**
			 * Если рекурсивно вызывается выражение целиком
			 */
			if(node.recurse.number == 0)
				// Выполняем установку корневого узла синтаксического дерева
				called = this->_parser->root();
			/**
			 * Если вызываемая группа сборкою уже пройдена
			 */
			else if(this->_groups.count(node.recurse.number) != 0)
				// Выполняем установку индекса узла вызываемой группы
				called = this->_groups.at(node.recurse.number);
			// Получаем результат проверки продвижения вызываемого узла
			const bool result = this->advancing(called, visited);
			// Выполняем снятие проверяемого узла со следа обхода
			visited.pop_back();
			// Выводим результат проверки обязательного продвижения узла по тексту
			return result;
		}
	}
	// Выводим результат проверки обязательного продвижения узла по тексту
	return false;
}
/**
 * @brief Метод подсчёта повторений любого символа и проверки их вложенности
 *
 * @param id      индекс проверяемого узла в арене узлов
 * @param inside  флаг нахождения узла в пределах повторения
 * @param nested  флаг обнаружения повторения в пределах повторения
 * @return        количество неограниченных повторений любого символа
 *
 */
size_t awh::regex::Compiler::sweeps(const node_id_t id, const bool inside, bool & nested) const noexcept {
	// Количество неограниченных повторений любого символа
	size_t result = 0;
	/**
	 * Выполняем обход цепочки узлов одного уровня вложенности
	 */
	for(node_id_t index = id; index != INVALID_NODE; index = this->_parser->node(index).next) {
		// Получаем очередной узел синтаксического дерева
		const node_data_t & node = this->_parser->node(index);
		/**
		 * Если узел является повторением
		 */
		if(node.type == node_t::REPEAT) {
			/**
			 * Если повторение находится в пределах другого повторения
			 */
			if(inside)
				// Выполняем установку флага обнаружения вложенного повторения
				nested = true;
			/**
			 * Если повторение неограничено и повторяет любой символ
			 */
			if((node.repeat.max == UNBOUNDED) && (this->_parser->node(node.child).type == node_t::ANY))
				// Увеличиваем количество неограниченных повторений любого символа
				result++;
			// Выполняем обход тела повторения
			result += this->sweeps(node.child, true, nested);
			// Переходим к следующему узлу цепочки
			continue;
		}
		// Выполняем обход дочерних узлов
		result += this->sweeps(node.child, inside, nested);
	}
	// Выводим количество неограниченных повторений любого символа
	return result;
}
/**
 * @brief Метод распознавания выражения, проходящего текст единственной попыткой
 *
 */
void awh::regex::Compiler::sweeping() noexcept {
	// Выполняем сброс признака прохода текста единственной попыткой
	this->_program->sweeping = false;
	// Получаем индекс корневого узла синтаксического дерева
	const node_id_t root = this->_parser->root();
	/**
	 * Если корневой узел синтаксического дерева отсутствует
	 */
	if(root == INVALID_NODE)
		// Выходим из метода распознавания выражения
		return;
	// Получаем индекс первого узла выражения
	node_id_t first = root;
	/**
	 * Выполняем спуск к первому сопоставляющему узлу выражения
	 */
	while((first != INVALID_NODE) && ((this->_parser->node(first).type == node_t::CONCAT) || (this->_parser->node(first).type == node_t::GROUP)))
		// Переходим к первому дочернему узлу
		first = this->_parser->node(first).child;
	/**
	 * Если первый узел выражения отсутствует
	 */
	if(first == INVALID_NODE)
		// Выходим из метода распознавания выражения
		return;
	// Получаем первый сопоставляющий узел выражения
	const node_data_t & node = this->_parser->node(first);
	/**
	 * Если выражение начинается не с неограниченного повторения любого символа
	 */
	if((node.type != node_t::REPEAT) || (node.repeat.max != UNBOUNDED) || (this->_parser->node(node.child).type != node_t::ANY))
		// Выходим из метода распознавания выражения
		return;
	/**
	 * Если повторение выполняется лениво
	 *
	 * @details Ленивое повторение поглощает наименьшее число символов, отчего
	 *          совпадение, начинающееся правее, в позицию начала поиска не переносится.
	 *
	 */
	if(node.repeat.greed == greed_t::LAZY)
		// Выходим из метода распознавания выражения
		return;
	// Флаг обнаружения повторения в пределах повторения
	bool nested = false;
	// Получаем количество неограниченных повторений любого символа
	const size_t count = this->sweeps(root, false, nested);
	/**
	 * Если повторения вложены друг в друга либо повторение любого символа не единственно
	 *
	 * @details Вложенные повторения и второе повторение любого символа дают перебор,
	 *          растущий с длиной текста показательно, отчего единственная попытка
	 *          соразмерной длине текста не остаётся.
	 *
	 */
	if(nested || (count != 1))
		// Выходим из метода распознавания выражения
		return;
	// Выполняем установку признака прохода текста единственной попыткой
	this->_program->sweeping = true;
}
/**
 * @brief Метод проверки начала сопоставления привязкой к позиции начала поиска
 *
 * @param id    индекс проверяемого узла в арене узлов
 * @param chain флаг обхода цепочки узлов одного уровня вложенности
 * @return      результат проверки начала сопоставления привязкой
 *
 */
bool awh::regex::Compiler::anchoring(const node_id_t id, const bool chain) const noexcept {
	/**
	 * Выполняем обход цепочки узлов одного уровня вложенности
	 */
	for(node_id_t index = id; index != INVALID_NODE; index = (chain ? this->_parser->node(index).next : INVALID_NODE)) {
		// Получаем очередной узел синтаксического дерева
		const node_data_t & node = this->_parser->node(index);
		/**
		 * Определяем тип очередного узла синтаксического дерева
		 */
		switch(static_cast <uint8_t> (node.type)) {
			/**
			 * Если узел является привязкой к позиции в тексте
			 */
			case static_cast <uint8_t> (node_t::ANCHOR): {
				/**
				 * Определяем тип привязки к позиции в тексте
				 */
				switch(static_cast <uint8_t> (node.anchor.type)) {
					/**
					 * Если привязка соответствует началу текста
					 */
					case static_cast <uint8_t> (anchor_t::TEXT_BEGIN):
					// Если привязка соответствует началу текущей попытки поиска
					case static_cast <uint8_t> (anchor_t::SEARCH_HEAD):
						// Выводим начало сопоставления привязкой к позиции начала поиска
						return true;
					/**
					 * Если привязка соответствует началу текста или строки
					 *
					 * @details Привязка ограничивает совпадение началом текста лишь вне
					 *          режима «MULTILINE», иначе она выполнима после каждого
					 *          перевода строки и позиций совпадения не ограничивает.
					 *
					 */
					case static_cast <uint8_t> (anchor_t::LINE_BEGIN):
						// Выводим начало сопоставления привязкой к позиции начала поиска
						return ((node.flags & static_cast <uint32_t> (flag_t::MULTILINE)) == 0);
					/**
					 * Если привязка сбрасывает начало совпадения
					 */
					case static_cast <uint8_t> (anchor_t::KEEP_OUT):
						// Выводим отсутствие привязки к позиции начала поиска
						return false;
				}
				/**
				 * Переходим к следующему узлу цепочки
				 *
				 * @details Прочие привязки длины не имеют и позиций совпадения
				 *          не ограничивают, поэтому обходятся насквозь.
				 *
				 */
			} break;
			/**
			 * Если узел является группой
			 */
			case static_cast <uint8_t> (node_t::GROUP):
			// Если узел является последовательностью элементов
			case static_cast <uint8_t> (node_t::CONCAT):
				// Выводим результат проверки начала сопоставления телом узла
				return this->anchoring(node.child, true);
			/**
			 * Если узел является выбором одной из ветвей
			 */
			case static_cast <uint8_t> (node_t::ALTERNATE): {
				/**
				 * Если ветви выражения отсутствуют
				 */
				if(node.child == INVALID_NODE)
					// Выводим отсутствие привязки к позиции начала поиска
					return false;
				/**
				 * Выполняем обход ветвей выражения
				 */
				for(node_id_t branch = node.child; branch != INVALID_NODE; branch = this->_parser->node(branch).next) {
					/**
					 * Если ветвь выражения привязкой не начинается
					 */
					if(!this->anchoring(branch, false))
						// Выводим отсутствие привязки к позиции начала поиска
						return false;
				}
				// Выводим начало сопоставления привязкой к позиции начала поиска
				return true;
			}
			/**
			 * Если узел сопоставляет символы либо выводится ненадёжно
			 */
			default:
				// Выводим отсутствие привязки к позиции начала поиска
				return false;
		}
	}
	// Выводим отсутствие привязки к позиции начала поиска
	return false;
}
/**
 * @brief Метод распознавания выражения, привязанного к позиции начала поиска
 *
 */
void awh::regex::Compiler::anchored() noexcept {
	/**
	 * Если сопоставление выполняется только с начала текста
	 */
	if((this->_program->flags & static_cast <uint32_t> (flag_t::ANCHORED)) != 0) {
		// Выполняем установку признака привязки к позиции начала поиска
		this->_program->anchored = true;
		// Выходим из метода распознавания выражения
		return;
	}
	// Выполняем установку признака привязки к позиции начала поиска
	this->_program->anchored = this->anchoring(this->_parser->root(), true);
}
/**
 * @brief Метод пометки повторений одиночного символа
 *
 * @details Повторение одиночного символа компилируется в переход по двум ветвям,
 *          тело повторения и переход к его началу. Метод отыскивает переходы,
 *          устроенные именно так, и помечает их адресом тела повторения, благодаря
 *          чему исполнение проходит ряд подходящих символов одним ходом.
 *
 */
void awh::regex::Compiler::mark() noexcept {
	// Получаем набор инструкций программы регулярного выражения
	Sequence <instruction_t> & instructions = this->_program->instructions;
	/**
	 * Выполняем обход инструкций программы регулярного выражения
	 */
	for(size_t i = 0; i < instructions.size(); i++) {
		/**
		 * Если инструкция не является переходом по двум ветвям
		 */
		if(instructions.at(i).type != opcode_t::SPLIT)
			// Переходим к следующей инструкции программы
			continue;
		// Выполняем сброс пометки повторения одиночного символа
		instructions.at(i).split.run = INVALID_ADDRESS;
		// Выполняем сброс пометки ленивого повторения одиночного символа
		instructions.at(i).split.lazy = INVALID_ADDRESS;
		/**
		 * Получаем признак ленивого повторения элемента выражения
		 *
		 * @details Ленивое повторение отличается порядком ветвей перехода:
		 *          телом повторения ведает ветвь вторая, а первая продолжает
		 *          сопоставление за повторением.
		 *
		 */
		const bool lazy = (static_cast <size_t> (instructions.at(i).split.second) == (i + 1));
		// Получаем адрес ветви повторения элемента выражения
		const address_t body = (lazy ? instructions.at(i).split.second : instructions.at(i).split.first);
		/**
		 * Если тело повторения и переход к началу повторения выходят за пределы программы
		 */
		if((static_cast <size_t> (body) + 1) >= instructions.size())
			// Переходим к следующей инструкции программы
			continue;
		/**
		 * Определяем сопоставление телом повторения одиночного символа
		 *
		 * @details Прочие инструкции сопоставления обращаются к состоянию исполнения
		 *          либо продвигаются по тексту на переменную длину, отчего проход
		 *          ряда одним ходом к ним неприменим.
		 *
		 */
		switch(static_cast <uint8_t> (instructions.at(body).type)) {
			// Сопоставление одиночного символа проходу ряда доступно
			case static_cast <uint8_t> (opcode_t::CHAR):
			// Сопоставление символа из класса символов проходу ряда доступно
			case static_cast <uint8_t> (opcode_t::CLASS):
			// Сопоставление любого символа проходу ряда доступно
			case static_cast <uint8_t> (opcode_t::ANY):
			// Сопоставление одиночной единицы кодирования проходу ряда доступно
			case static_cast <uint8_t> (opcode_t::CODEUNIT): break;
			// Прочие инструкции сопоставления проходу ряда недоступны
			default: continue;
		}
		/**
		 * Если за телом повторения следует не переход к началу повторения
		 */
		if(instructions.at(body + 1).type != opcode_t::JUMP)
			// Переходим к следующей инструкции программы
			continue;
		/**
		 * Если переход выполняется не к началу повторения
		 */
		if(instructions.at(body + 1).jump.target != static_cast <address_t> (i))
			// Переходим к следующей инструкции программы
			continue;
		/**
		 * Если ветвь завершения повторения следует не за переходом к его началу
		 *
		 * @details Проверка отсекает переходы, ветви которых устроены иначе,
		 *          и оставляет лишь повторение, завершение которого продолжает
		 *          исполнение с инструкции, следующей за переходом к началу.
		 *
		 */
		if((lazy ? instructions.at(i).split.first : instructions.at(i).split.second) != static_cast <address_t> (body + 2))
			// Переходим к следующей инструкции программы
			continue;
		// Выполняем пометку перехода адресом тела повторения одиночного символа
		(lazy ? instructions.at(i).split.lazy : instructions.at(i).split.run) = body;
	}
}
/**
 * @brief Метод компиляции узла проверки окружения
 *
 * @param id      индекс узла проверки окружения в арене узлов
 * @param address адрес размещённой инструкции проверки окружения
 * @return        результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compileLook(const node_id_t id, address_t & address) noexcept {
	// Получаем узел проверки окружения
	const node_data_t & node = this->_parser->node(id);
	// Определяем направление проверки окружения
	const bool backward = ((node.look.type == look_t::BEHIND) || (node.look.type == look_t::BEHIND_NEG));
	// Определяем знак проверки окружения
	const bool negative = ((node.look.type == look_t::AHEAD_NEG) || (node.look.type == look_t::BEHIND_NEG));
	/**
	 * Если длина ретроспективной проверки не ограничена
	 *
	 * @details Ретроспективная проверка сопоставляется отступом от текущей позиции
	 *          на длину проверяемой последовательности, поэтому её неограниченная
	 *          длина делает проверку несопоставимой.
	 *
	 */
	if(backward && (node.look.max == UNBOUNDED)) {
		// Выполняем установку ошибки недопустимой ретроспективной проверки
		this->_error = error_t::LOOKBEHIND_INVALID;
		// Выводим результат выполнения компиляции
		return false;
	}
	// Выполняем размещение инструкции проверки окружения
	address = this->emit(opcode_t::LOOK, node.flags);
	/**
	 * Если размещение инструкции не выполнено
	 */
	if(address == INVALID_ADDRESS)
		// Выводим результат выполнения компиляции
		return false;
	// Выполняем установку знака проверки окружения
	this->_program->instructions.at(address).look.negative = negative;
	// Выполняем установку направления проверки окружения
	this->_program->instructions.at(address).look.backward = backward;
	// Выполняем установку наименьшей длины проверяемой последовательности
	this->_program->instructions.at(address).look.least = node.look.min;
	// Выполняем установку наибольшей длины проверяемой последовательности
	this->_program->instructions.at(address).look.most = node.look.max;
	// Выполняем установку отсутствия ветви невыполнения проверки
	this->_program->instructions.at(address).look.alternate = INVALID_ADDRESS;
	// Выполняем установку адреса тела проверки окружения
	this->_program->instructions.at(address).look.body = this->position();
	/**
	 * Если компиляция тела проверки окружения не выполнена
	 */
	if(!this->compileChain(node.child))
		// Выводим результат выполнения компиляции
		return false;
	/**
	 * Если размещение инструкции завершения проверки не выполнено
	 */
	if(this->emit(opcode_t::RETURN, node.flags) == INVALID_ADDRESS)
		// Выводим результат выполнения компиляции
		return false;
	// Выполняем установку адреса продолжения за проверкой окружения
	this->_program->instructions.at(address).look.target = this->position();
	// Выводим результат выполнения компиляции
	return true;
}
/**
 * @brief Метод компиляции узла условного выражения
 *
 * @param id индекс узла условного выражения в арене узлов
 * @return   результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compileCondition(const node_id_t id) noexcept {
	// Получаем узел условного выражения
	const node_data_t & node = this->_parser->node(id);
	// Получаем индекс первой ветви условного выражения
	node_id_t branch = node.child;
	/**
	 * Если условное выражение является блоком определения групп
	 *
	 * @details Ветвь блока определения не исполняется при сопоставлении и достижима
	 *          исключительно рекурсивным вызовом, поэтому размещается за переходом,
	 *          обходящим её целиком.
	 *
	 */
	if(node.condition.type == condition_t::DEFINE) {
		// Выполняем размещение инструкции обхода блока определения групп
		const address_t jump = this->emit(opcode_t::JUMP, node.flags);
		/**
		 * Если размещение инструкции не выполнено
		 */
		if(jump == INVALID_ADDRESS)
			// Выводим результат выполнения компиляции
			return false;
		/**
		 * Если компиляция ветви блока определения групп не выполнена
		 */
		if(!this->compileNode(branch))
			// Выводим результат выполнения компиляции
			return false;
		// Выполняем установку адреса обхода блока определения групп
		this->_program->instructions.at(jump).jump.target = this->position();
		// Выводим результат выполнения компиляции
		return true;
	}
	// Адрес инструкции проверки окружения, задающей условие
	address_t assertion = INVALID_ADDRESS;
	// Адрес инструкции перехода по ветвям условного выражения
	address_t condition = INVALID_ADDRESS;
	/**
	 * Если условие задано проверкой окружения
	 */
	if(node.condition.type == condition_t::ASSERTION) {
		/**
		 * Если компиляция проверки окружения, задающей условие, не выполнена
		 */
		if(!this->compileLook(branch, assertion))
			// Выводим результат выполнения компиляции
			return false;
		// Переходим к ветви выполненного условия
		branch = this->_parser->node(branch).next;
	/**
	 * Выполняем размещение инструкции перехода по ветвям условного выражения
	 */
	} else {
		// Выполняем размещение инструкции перехода по ветвям условного выражения
		condition = this->emit(opcode_t::CONDITION, node.flags);
		/**
		 * Если размещение инструкции не выполнено
		 */
		if(condition == INVALID_ADDRESS)
			// Выводим результат выполнения компиляции
			return false;
		// Определяем тип условия условного выражения
		const test_t type = ((node.condition.type == condition_t::RECURSING) ? test_t::RECURSING : test_t::CAPTURED);
		// Выполняем установку типа условия условного выражения
		this->_program->instructions.at(condition).condition.type = type;
		// Выполняем установку номера проверяемой группы условного выражения
		this->_program->instructions.at(condition).condition.number = node.condition.number;
		// Выполняем установку адреса ветви выполненного условия
		this->_program->instructions.at(condition).condition.positive = this->position();
	}
	/**
	 * Если компиляция ветви выполненного условия не выполнена
	 */
	if(!this->compileNode(branch))
		// Выводим результат выполнения компиляции
		return false;
	// Выполняем размещение инструкции перехода к завершению условного выражения
	const address_t jump = this->emit(opcode_t::JUMP, node.flags);
	/**
	 * Если размещение инструкции не выполнено
	 */
	if(jump == INVALID_ADDRESS)
		// Выводим результат выполнения компиляции
		return false;
	/**
	 * Если условие задано проверкой окружения
	 */
	if(assertion != INVALID_ADDRESS)
		// Выполняем установку адреса ветви невыполнения проверки окружения
		this->_program->instructions.at(assertion).look.alternate = this->position();
	// Выполняем установку адреса ветви невыполненного условия
	else this->_program->instructions.at(condition).condition.negative = this->position();
	/**
	 * Если условное выражение содержит ветвь невыполненного условия
	 */
	if(branch != INVALID_NODE) {
		/**
		 * Если компиляция ветви невыполненного условия не выполнена
		 */
		if(!this->compileNode(this->_parser->node(branch).next))
			// Выводим результат выполнения компиляции
			return false;
	}
	// Выполняем установку адреса завершения условного выражения
	this->_program->instructions.at(jump).jump.target = this->position();
	// Выводим результат выполнения компиляции
	return true;
}
/**
 * @brief Метод компиляции тел рекурсивно вызываемых подвыражений
 *
 * @return результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compileSections() noexcept {
	// Индекс обрабатываемого рекурсивного вызова
	size_t index = 0;
	/**
	 * Выполняем размещение тел рекурсивно вызываемых подвыражений
	 *
	 * @details Размещаемое тело способно содержать рекурсивные вызовы, поэтому
	 *          обход набора вызовов продолжается до исчерпания добавленных
	 *          в ходе размещения.
	 *
	 */
	while(index < this->_calls.size()) {
		// Получаем номер вызываемой рекурсивным вызовом группы
		const uint32_t number = this->_calls.at(index++).second;
		/**
		 * Если тело вызываемой группы уже размещено
		 */
		if(this->_sections.count(number) != 0)
			// Переходим к следующему рекурсивному вызову
			continue;
		// Индекс узла тела вызываемой группы
		node_id_t node = INVALID_NODE;
		/**
		 * Если рекурсивно вызывается выражение целиком
		 */
		if(number == 0)
			// Выполняем установку корневого узла синтаксического дерева
			node = this->_parser->root();
		/**
		 * Если вызываемая группа обнаружена в синтаксическом дереве
		 */
		else if(this->_groups.count(number) != 0)
			// Выполняем установку индекса узла вызываемой группы
			node = this->_groups.at(number);
		/**
		 * Выполняем отказ от размещения тела отсутствующей группы
		 */
		else {
			// Выполняем установку ошибки некорректного рекурсивного вызова
			this->_error = error_t::BAD_RECURSION;
			// Выводим результат выполнения компиляции
			return false;
		}
		/**
		 * Выполняем сохранение адреса тела вызываемой группы
		 *
		 * @details Адрес сохраняется до размещения тела, благодаря чему вызов
		 *          группой самой себя обращается к уже известному адресу
		 *          и не приводит к бесконечному размещению.
		 *
		 */
		this->_sections.emplace(number, this->position());
		/**
		 * Если компиляция тела вызываемой группы не выполнена
		 */
		if(!this->compileNode(node))
			// Выводим результат выполнения компиляции
			return false;
		/**
		 * Если размещение инструкции завершения рекурсивного вызова не выполнено
		 */
		if(this->emit(opcode_t::RESUME, 0) == INVALID_ADDRESS)
			// Выводим результат выполнения компиляции
			return false;
	}
	/**
	 * Выполняем установку адресов тел рекурсивно вызываемых подвыражений
	 */
	for(auto & call : this->_calls)
		// Выполняем установку адреса тела вызываемой группы
		this->_program->instructions.at(call.first).call.body = this->_sections.at(call.second);
	// Выводим результат выполнения компиляции
	return true;
}
/**
 * @brief Метод компиляции регулярного выражения целиком
 *
 * @param parser  объект разбора регулярного выражения
 * @param program компилируемая программа регулярного выражения
 * @return        результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compileFull(const Parser & parser, program_t & program) noexcept {
	// Выполняем установку флага компиляции выражения целиком
	this->_full = true;
	// Выполняем компиляцию регулярного выражения целиком
	const bool result = this->compile(parser, program);
	// Выполняем сброс флага компиляции выражения целиком
	this->_full = false;
	// Выводим результат выполнения компиляции
	return result;
}
/**
 * @brief Метод компиляции развёрнутого регулярного выражения
 *
 * @param parser  объект разбора регулярного выражения
 * @param program компилируемая программа регулярного выражения
 * @return        результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compileReverse(const Parser & parser, program_t & program) noexcept {
	// Выполняем установку флага компиляции развёрнутого выражения
	this->_reverse = true;
	// Выполняем компиляцию развёрнутого регулярного выражения
	const bool result = this->compile(parser, program);
	// Выполняем сброс флага компиляции развёрнутого выражения
	this->_reverse = false;
	// Выводим результат выполнения компиляции
	return result;
}
/**
 * @brief Метод компиляции регулярного выражения
 *
 * @param parser  объект разбора регулярного выражения
 * @param program компилируемая программа регулярного выражения
 * @return        результат выполнения компиляции
 *
 */
bool awh::regex::Compiler::compile(const Parser & parser, program_t & program) noexcept {
	// Выполняем установку объекта разбора регулярного выражения
	this->_parser = &parser;
	// Выполняем установку компилируемой программы
	this->_program = &program;
	// Выполняем сброс кода ошибки компиляции
	this->_error = error_t::NONE;
	// Выполняем сброс количества размещённых ячеек состояния
	this->_cells = 0;
	// Выполняем сброс количества ячеек отметки состояния возврата
	this->_atomics = 0;
	// Выполняем очистку набора инструкций рекурсивного вызова
	this->_calls.clear();
	// Выполняем очистку соответствия номеров групп адресам их тел
	this->_sections.clear();
	// Выполняем очистку соответствия номеров групп индексам их узлов
	this->_groups.clear();
	// Выполняем очистку компилируемой программы
	program.clear();
	/**
	 * Выполняем присвоение опознания компилируемой программе
	 *
	 * @details Опознание отличает содержимое программы от содержимого программы,
	 *          скомпилированной ранее, в том числе занимавшей то же расположение
	 *          в памяти, благодаря чему кэш состояний детерминированного
	 *          исполнения не переживает пересборки выражения.
	 *
	 */
	static atomic <uint64_t> counter(0);
	// Выполняем установку опознания компилируемой программы
	program.id = (++counter);
	// Выполняем установку количества захватывающих групп
	program.captures = parser.captures();
	/**
	 * Если выполняется компиляция выражения целиком
	 */
	if(this->_full)
		// Выполняем сбор узлов захватывающих групп выражения
		this->collect(parser.root());
	// Выполняем установку набора режимов компиляции программы
	program.flags = parser.options();
	// Выполняем размещение инструкции сохранения начала совпадения
	const address_t begin = this->emit(opcode_t::SAVE, 0);
	/**
	 * Если размещение инструкции не выполнено
	 */
	if(begin == INVALID_ADDRESS)
		// Выводим результат выполнения компиляции
		return false;
	// Выполняем установку номера ячейки начала совпадения
	program.instructions.at(begin).save.slot = 0;
	/**
	 * Если компиляция синтаксического дерева не выполнена
	 */
	if(!this->compileNode(parser.root())) {
		// Выполняем очистку компилируемой программы
		program.clear();
		// Выводим результат выполнения компиляции
		return false;
	}
	// Выполняем размещение инструкции сохранения конца совпадения
	const address_t end = this->emit(opcode_t::SAVE, 0);
	/**
	 * Если размещение инструкции не выполнено
	 */
	if(end == INVALID_ADDRESS)
		// Выводим результат выполнения компиляции
		return false;
	// Выполняем установку номера ячейки конца совпадения
	program.instructions.at(end).save.slot = 1;
	/**
	 * Если размещение инструкции завершения сопоставления не выполнено
	 */
	if(this->emit(opcode_t::MATCH, 0) == INVALID_ADDRESS)
		// Выводим результат выполнения компиляции
		return false;
	/**
	 * Если размещение тел рекурсивно вызываемых подвыражений не выполнено
	 */
	if(!this->compileSections()) {
		// Выполняем очистку компилируемой программы
		program.clear();
		// Выводим результат выполнения компиляции
		return false;
	}
	// Выполняем установку количества ячеек состояния исполнения
	program.cells = this->_cells;
	/**
	 * Если компилируется прямое регулярное выражение
	 *
	 * @details Предварительный отбор позиций применяется при поиске совпадения
	 *          в прямом направлении, поэтому для развёрнутой программы не формируется.
	 *
	 */
	if(!this->_reverse) {
		// Выполняем формирование предварительного отбора позиций
		this->analyze();
		// Выполняем распознавание выражения, сопоставляемого литералом
		this->condense();
	}
	// Выполняем пометку повторений одиночного символа
	this->mark();
	/**
	 * Если компилируется программа сопоставления в прямом направлении
	 */
	if(!this->_reverse) {
		// Выполняем распознавание выражения, проходящего текст единственной попыткой
		this->sweeping();
		// Выполняем распознавание выражения, привязанного к позиции начала поиска
		this->anchored();
	}
	// Выводим результат выполнения компиляции
	return true;
}
