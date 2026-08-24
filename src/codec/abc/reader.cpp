/**
 * @file reader.cpp
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
 * @brief Файл реализации поточного чтения бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the streaming reading of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/reader.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <limits>

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
	 * @brief Функция получения самого узкого вида, вмещающего целое без знака
	 *
	 * @param value вмещаемое значение
	 * @return      вид значения документа
	 *
	 */
	awh::codec::abc::type_t narrow(const uint64_t value) noexcept {
		// Используем пространство имён бинарного контейнера
		using namespace awh::codec::abc;
		// Если значение умещается в один октет
		if(value <= static_cast <uint64_t> (numeric_limits <uint8_t>::max()))
			// Выводим вид целого без знака шириною в один октет
			return type_t::UINT8;
		// Если значение умещается в два октета
		else if(value <= static_cast <uint64_t> (numeric_limits <uint16_t>::max()))
			// Выводим вид целого без знака шириною в два октета
			return type_t::UINT16;
		// Если значение умещается в четыре октета
		else if(value <= static_cast <uint64_t> (numeric_limits <uint32_t>::max()))
			// Выводим вид целого без знака шириною в четыре октета
			return type_t::UINT32;
		// Выводим вид целого без знака шириною в восемь октетов
		return type_t::UINT64;
	}
	/**
	 * @brief Функция получения самого узкого вида, вмещающего целое со знаком
	 *
	 * @param value вмещаемое значение
	 * @return      вид значения документа
	 *
	 */
	awh::codec::abc::type_t narrow(const int64_t value) noexcept {
		// Используем пространство имён бинарного контейнера
		using namespace awh::codec::abc;
		// Если значение умещается в один октет
		if((value >= static_cast <int64_t> (numeric_limits <int8_t>::min())) &&
		   (value <= static_cast <int64_t> (numeric_limits <int8_t>::max())))
			// Выводим вид целого со знаком шириною в один октет
			return type_t::INT8;
		// Если значение умещается в два октета
		else if((value >= static_cast <int64_t> (numeric_limits <int16_t>::min())) &&
		        (value <= static_cast <int64_t> (numeric_limits <int16_t>::max())))
			// Выводим вид целого со знаком шириною в два октета
			return type_t::INT16;
		// Если значение умещается в четыре октета
		else if((value >= static_cast <int64_t> (numeric_limits <int32_t>::min())) &&
		        (value <= static_cast <int64_t> (numeric_limits <int32_t>::max())))
			// Выводим вид целого со знаком шириною в четыре октета
			return type_t::INT32;
		// Выводим вид целого со знаком шириною в восемь октетов
		return type_t::INT64;
	}
};

/**
 * @brief Конструктор настроек разбора записи
 *
 */
awh::codec::abc::Reader::Settings::Settings() noexcept :
 stream(false), validate(true), maxString(0), maxBlob(0), maxDepth(0), maxNodes(0) {}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 *
 */
awh::codec::abc::Reader::Reader(const log_t * log) noexcept :
 _state(state_t::ITEM), _error(error_t::NONE), _offset(0), _origin(0), _reserved(0), _head(0),
 _nodes(0), _pending(0), _awaited(type_t::UNDEFINED), _extend(extend_t::BIGNUM), _span(0), _beyond(0), _skipping(false), _exponent(0), _subtype(0),
 _negative(false), _document(false), _handler(nullptr), _context(nullptr), _log(log)  {
	// Выполняем установку начального положения разбора
	this->_mark = location_t();
	this->_location.offset = 0;
	// Выполняем установку начальной глубины вложенности разбора
	this->_location.depth = 0;
}
/**
 * @brief Метод сброса состояния разбора
 *
 */
void awh::codec::abc::Reader::reset() noexcept {
	// Выполняем установку начального состояния разбора
	this->_state = state_t::ITEM;
	// Выполняем сброс кода отказа разбора
	this->_error = error_t::NONE;
	// Выполняем сброс положения разбора в поданной записи
	this->_mark = location_t();
	this->_location.offset = 0;
	// Выполняем сброс глубины вложенности разбора
	this->_location.depth = 0;
	// Выполняем очистку буфера накопленных октетов
	this->_buffer.clear();
	// Выполняем сброс смещения разбора в буфере
	this->_offset = 0;
	// Выполняем сброс смещения начала буфера
	this->_origin = 0;
	// Выполняем сброс размера выданного места под приём октетов
	this->_reserved = 0;
	// Выполняем очистку стека вместимых
	this->_stack.clear();
	// Выполняем очистку очереди собранных событий
	this->_events.clear();
	// Выполняем сброс смещения первого невыданного события
	this->_head = 0;
	// Выполняем сброс события, выданного последним переходом
	this->_current = record_t();
	// Выполняем сброс количества разобранных узлов
	this->_nodes = 0;
	// Выполняем сброс количества недостающих октетов
	this->_pending = 0;
	// Выполняем сброс вида ожидаемого значения
	this->_awaited = type_t::UNDEFINED;
	// Выполняем сброс разновидности ожидаемого расширения
	this->_extend = extend_t::BIGNUM;
	// Выполняем сброс объявленного размаха вместимого
	this->_span = 0;
	// Выполняем сброс места записи за вместимым
	this->_beyond = 0;
	// Выполняем сброс признака затребованного пропуска
	this->_skipping = false;
	// Выполняем сброс номера подвида открытого расширения
	this->_subtype = 0;
	// Выполняем сброс десятичного порядка величины
	this->_exponent = 0;
	// Выполняем сброс признака отрицательности величины
	this->_negative = false;
	// Выполняем сброс признака завершённости документа
	this->_document = false;
}
/**
 * @brief Метод прекращения разбора
 *
 */
void awh::codec::abc::Reader::abort() noexcept {
	// Выполняем перевод разбора в состояние отказа
	this->_state = state_t::FAILED;
	// Выполняем очистку очереди собранных событий
	this->_events.clear();
	// Выполняем сброс смещения первого невыданного события
	this->_head = 0;
}
/**
 * @brief Метод объявления отказа разбора
 *
 * @param error код отказа разбора
 * @return      признак успешности разбора, всегда ложь
 *
 */
bool awh::codec::abc::Reader::fail(const error_t error) noexcept {
	// Выполняем установку кода отказа разбора
	this->_error = error;
	// Выполняем перевод разбора в состояние отказа
	this->_state = state_t::FAILED;
	// Выполняем установку положения, где отказ произошёл
	this->_location.offset = (this->_origin + static_cast <uint64_t> (this->_offset));
	// Выполняем установку глубины вложенности, где отказ произошёл
	this->_location.depth = static_cast <uint32_t> (this->_stack.size());
	/**
	 * Если объект логирования отдан, доносим об отказе разбора.
	 *
	 * Донесение идёт отсюда, из единственного места объявления отказа: разбор
	 * отвечает отказом двумя десятками путей, и запись в каждом из них разошлась бы
	 * с прочими. Потребителю остаётся код отказа, а сводить записи со всей рамки
	 * в одну точку - работа журнала, а не его
	 */
	/**
	 * @warning Сброс кода отказа сюда НЕ идёт: воронка эта объявляет отказ, а сброс
	 *          лишь снимает прежний, и донесение о нём наполняло бы журнал записями
	 *          «no error» на всякий успешный вызов. Проверено на себе
	 */
	if((error != error_t::NONE) && (this->_log != nullptr)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("ABC: %s", __PRETTY_FUNCTION__,
			 make_tuple(static_cast <uint16_t> (error), this->_location.offset, this->_location.depth),
			 log_t::flag_t::WARNING, abc::message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("ABC: %s", log_t::flag_t::WARNING, abc::message(error));
		#endif
	}
	// Сообщаем, что разбор отвечен отказом
	return false;
}
/**
 * @brief Метод выдачи собранного события разбора
 *
 * @param record собранное событие разбора
 *
 */
void awh::codec::abc::Reader::emit(const record_t & item) noexcept {
	// Выдаваемое событие разбора с запомненным местом его
	record_t record = item;
	/**
	 * Выполняем запоминание места события самим событием: события копятся очередью,
	 * и состояние разбирателя к выдаче события уходит вперёд тем дальше, чем крупнее
	 * поданный кусок
	 */
	record.location = this->_mark;
	/**
	 * Если обработчик прямой выдачи событий установлен, очередь выдачи минуется.
	 *
	 * Событие ложилось бы в неё, а потом снималось с неё же копией: работа двойная, а
	 * очередь при том растёт без предела, ибо снимать с неё некому - обработчик
	 * событие уже принял
	 */
	if(this->_handler != nullptr){
		// Выполняем установку выдаваемого события текущим
		this->_current = record;
		// Выполняем прямую выдачу события разбора
		this->_handler(this->_context, (* this), record.event);
		// Выходим из работы
		return;
	}
	// Выполняем добавление события в очередь выдачи
	this->_events.push_back(record);
}
/**
 * @brief Метод закрытия исчерпанных вместимых
 *
 * @return признак успешности разбора
 *
 */
/**
 * @brief Метод пропуска открытого вместимого целиком
 *
 * @return признак успешного пропуска вместимого
 *
 */
bool awh::codec::abc::Reader::skip() noexcept {
	/**
	 * Если разбор отвечен отказом
	 */
	if(this->_state == state_t::FAILED)
		// Выводим признак неудачного пропуска
		return false;
	/**
	 * Если размах открываемого вместимого меткою не объявлен
	 *
	 * @note Звать пропуск надлежит ИЗ ОБРАБОТЧИКА прямой выдачи, по событию начала
	 *       вместимого: события копятся очередью, и к снятию события разбор уходит
	 *       вперёд - пропускать к тому времени уже нечего
	 */
	if(this->_beyond == 0)
		// Выводим признак неудачного пропуска
		return false;
	/**
	 * Если место за вместимым лежит ПРЕЖДЕ нынешнего места разбора
	 *
	 * @details Размах приходит извне, и объявленный слишком малым увёл бы разбор назад
	 * по записи - вместимое пошло бы разбираться наново, и обработчик, пропускающий его
	 * всякий раз, закружил бы разбор навсегда. Равенство дозволено: у пустого вместимого
	 * место за ним и есть нынешнее
	 */
	if(this->_beyond < (this->_origin + static_cast <uint64_t> (this->_offset)))
		// Выводим признак неудачного пропуска
		return false;
	/**
	 * Если октеты пропускаемого поданы ещё не целиком
	 *
	 * @note Отказ здесь не беда, а договор: поточному чтению пропускать нечего,
	 *       покуда пропускаемое не пришло, и потребителю остаётся читать обычным ходом
	 */
	if((this->_beyond - this->_origin) > static_cast <uint64_t> (this->_buffer.size()))
		// Выводим признак неудачного пропуска
		return false;
	// Выполняем объявление затребованного пропуска
	this->_skipping = true;
	// Выводим признак успешного пропуска
	return true;
}
bool awh::codec::abc::Reader::unwind() noexcept {
	/**
	 * Выполняем закрытие всех вместимых, чьи значения исчерпаны
	 */
	while(!this->_stack.empty()){
		// Выполняем получение верхнего звена стека вместимых
		const frame_t & frame = this->_stack.back();
		/**
		 * Если значение собирается кусками, закрывается оно лишь своим концом
		 */
		if(frame.segment != type_t::UNDEFINED)
			// Выходим из обхода стека вместимых
			break;
		// Если длина вместимого неопределённая, закрывается оно лишь своим концом
		if(frame.indefinite)
			// Выходим из обхода стека вместимых
			break;
		// Если значения вместимого ещё не исчерпаны
		if(frame.remain > 0)
			// Выходим из обхода стека вместимых
			break;
		/**
		 * Если объявленный размах с настоящим концом вместимого не сошёлся
		 *
		 * @details Размах приходит извне, и до сих пор ему верили на слово: пропуск
		 * уводил разбор по объявленному месту, а объявление лживое вскрывалось лишь
		 * тем, что разбор натыкался на середину значения. Здесь оно поверяется прямо:
		 * у закрывающегося вместимого конец известен, и объявленному он обязан
		 * равняться. Поверка ничего не стоит, а лживый размах отвергает СРАЗУ - и у
		 * того, кто пропуском не пользуется вовсе
		 *
		 * @note Места считаются от начала записи, а не от начала буфера: буфер
		 * усекается по мере разбора, и смещение в нём от нарезки на куски зависит
		 */
		if((frame.beyond > 0) && (frame.beyond != (this->_origin + static_cast <uint64_t> (this->_offset))))
			// Выполняем объявление отказа разбора
			return this->fail(error_t::INVALID_LENGTH);
		// Собираемое событие конца вместимого
		record_t record;
		// Выполняем установку вида события конца вместимого
		record.event = (frame.mapping ? event_t::MAP_END : event_t::ARRAY_END);
		// Выполняем снятие звена со стека вместимых
		this->_stack.pop_back();
		// Выполняем выдачу события конца вместимого
		this->emit(record);
		// Если стек вместимых опустел, документ разобран до конца
		if(this->_stack.empty()){
			// Собираемое событие завершённого документа
			record_t finish;
			// Выполняем установку вида события завершённого документа
			finish.event = event_t::DOCUMENT;
			// Выполняем выдачу события завершённого документа
			this->emit(finish);
			// Выполняем установку признака завершённости документа
			this->_document = true;
			// Выполняем перевод разбора в завершённое состояние
			this->_state = state_t::FINISHED;
			// Сообщаем, что разбор успешен
			return true;
		}
		// Выполняем получение звена вместимого, вместившего закрытое
		frame_t & parent = this->_stack.back();
		// Если вместившее является отображением, следующим ожидается имя поля
		if(parent.mapping)
			// Выполняем установку признака ожидания имени поля
			parent.expectKey = true;
		// Если длина вместившего определена
		if(!parent.indefinite)
			// Выполняем учёт закрытого вместимого значением вместившего
			parent.remain--;
	}
	// Сообщаем, что разбор успешен
	return true;
}
/**
 * @brief Метод выдачи события завершённого значения
 *
 * @param record собранное событие разбора
 * @return       признак успешности разбора
 *
 */
bool awh::codec::abc::Reader::settle(record_t & record) noexcept {
	/**
	 * Если значение собирается кусками, всякое иное значение внутри него негодно:
	 * куски выдаются своим путём, а сюда попадает лишь то, что кусками не является
	 */
	if(!this->_stack.empty() && (this->_stack.back().segment != type_t::UNDEFINED))
		// Выполняем объявление отказа разбора
		return this->fail(error_t::INVALID_SEGMENT);
	// Выполняем получение предела количества узлов документа
	const uint32_t limit = ((this->_settings.maxNodes > 0) ? this->_settings.maxNodes : MAX_NODES);
	// Если количество узлов документа превышает допустимое
	if(++this->_nodes > limit)
		// Выполняем объявление отказа разбора
		return this->fail(error_t::TOO_MANY_NODES);
	// Если стек вместимых не пуст
	if(!this->_stack.empty()){
		// Выполняем получение верхнего звена стека вместимых
		frame_t & frame = this->_stack.back();
		// Если ожидается имя поля отображения
		if(frame.mapping && frame.expectKey){
			// Выполняем установку вида события имени поля отображения
			record.event = event_t::KEY;
			// Выполняем снятие признака ожидания имени поля
			frame.expectKey = false;
			// Выполняем выдачу события имени поля отображения
			this->emit(record);
			// Сообщаем, что разбор успешен: пара ещё не завершена
			return true;
		}
	}
	// Выполняем учёт завершённого значения вместившим его
	return this->finalize(record);
}
/**
 * @brief Метод учёта завершённого значения вместившим его
 *
 * @param record выдаваемое событие завершённого значения
 * @return       признак успешности разбора
 *
 */
bool awh::codec::abc::Reader::finalize(record_t & record) noexcept {
	// Выполняем выдачу события значения
	this->emit(record);
	// Если стек вместимых пуст, документ разобран до конца
	if(this->_stack.empty()){
		// Собираемое событие завершённого документа
		record_t finish;
		// Выполняем установку вида события завершённого документа
		finish.event = event_t::DOCUMENT;
		// Выполняем выдачу события завершённого документа
		this->emit(finish);
		// Выполняем установку признака завершённости документа
		this->_document = true;
		// Выполняем перевод разбора в завершённое состояние
		this->_state = state_t::FINISHED;
		// Сообщаем, что разбор успешен
		return true;
	}
	// Выполняем получение верхнего звена стека вместимых
	frame_t & frame = this->_stack.back();
	// Если вместимое является отображением, следующим ожидается имя поля
	if(frame.mapping)
		// Выполняем установку признака ожидания имени поля
		frame.expectKey = true;
	// Если длина вместимого определена
	if(!frame.indefinite)
		// Выполняем учёт значения вместимого
		frame.remain--;
	// Выполняем закрытие исчерпанных вместимых
	return this->unwind();
}
/**
 * @brief Метод разбора очередной единицы проволочной записи
 *
 * @param done признак того, что единицу разобрать не удалось
 * @return     признак успешности разбора
 *
 */
bool awh::codec::abc::Reader::item(bool & done) noexcept {
	// Выполняем сброс признака недостачи октетов
	done = false;
	// Снятая единица проволочной записи
	item_t unit;
	// Код отказа снятия единицы
	error_t error = error_t::NONE;
	// Смещение, с какого следует снимать единицу
	size_t offset = this->_offset;
	// Если снять единицу проволочной записи не удалось
	if(!abc::take(this->_buffer.data(), this->_buffer.size(), offset, unit, error)){
		// Если октетов единицы недостаёт
		if(error == error_t::UNEXPECTED_EOF){
			// Выполняем установку признака недостачи октетов
			done = true;
			// Сообщаем, что разбор успешен: единица дождётся следующей подачи
			return true;
		}
		// Выполняем объявление отказа разбора
		return this->fail(error);
	}
	// Собираемое событие разбора
	record_t record;
	/**
	 * Если снятая единица вместимым не является, объявленный размах ей не принадлежит
	 *
	 * @details Размах объявляется меткою, стоящей ВПЕРЕДИ вместимого, и принадлежит
	 * вместимому ближайшему. Своя сборка иначе метку и не кладёт, но запись приходит
	 * извне: метка, поставленная перед невместимым, оставляла размах висеть до
	 * следующего вместимого, и то пропускалось по чужому размаху - потребитель его не
	 * пропускал, а содержимое пропадало. Затребованный пропуск снимается здесь же: он
	 * относился к той же единице
	 *
	 * @note Проверка стоит ЗДЕСЬ, а не в самом пропуске: пропуск зовётся из обработчика
	 * прямой выдачи, то есть ПРЕЖДЕ, чем разбор дойдёт до следующей единицы, и поверка
	 * места там годна лишь на миг вызова
	 */
	if((unit.group != group_t::ARRAY) && (unit.group != group_t::MAP)){
		// Выполняем сброс объявленного размаха
		this->_span = 0;
		// Выполняем сброс места записи за вместимым
		this->_beyond = 0;
		// Выполняем сброс признака затребованного пропуска
		this->_skipping = false;
	}
	/**
	 * Определяем крупный вид снятой единицы
	 */
	switch(static_cast <uint8_t> (unit.group)){
		/**
		 * Если значение является целым без знака
		 */
		case static_cast <uint8_t> (group_t::UNSIGNED): {
			// Выполняем сдвиг смещения разбора
			this->_offset = offset;
			// Выполняем установку вида события числа
			record.event = event_t::NUMBER;
			// Выполняем установку самого узкого вида, вмещающего значение
			record.type = narrow(unit.value);
			// Выполняем установку значения целого без знака
			record.number = unit.value;
			// Если значение представимо и видом со знаком
			if(unit.value <= static_cast <uint64_t> (numeric_limits <int64_t>::max()))
				// Выполняем установку значения целого со знаком
				record.integer = static_cast <int64_t> (unit.value);
			// Выполняем установку дробного представления значения
			record.real = static_cast <double> (unit.value);
			// Выполняем выдачу события завершённого значения
			return this->settle(record);
		}
		/**
		 * Если значение является целым со знаком, меньшим нуля
		 */
		case static_cast <uint8_t> (group_t::NEGATIVE): {
			// Обращённое число со знаком
			int64_t value = 0;
			// Если запись дополнения до −1 видом целого со знаком не представима
			if(!abc::negative(unit.value, value))
				// Выполняем объявление отказа разбора
				return this->fail(error_t::NUMBER_OUT_OF_RANGE);
			// Выполняем сдвиг смещения разбора
			this->_offset = offset;
			// Выполняем установку вида события числа
			record.event = event_t::NUMBER;
			// Выполняем установку самого узкого вида, вмещающего значение
			record.type = narrow(value);
			// Выполняем установку значения целого со знаком
			record.integer = value;
			// Выполняем установку разрядной записи значения
			record.number = unit.value;
			// Выполняем установку дробного представления значения
			record.real = static_cast <double> (value);
			// Выполняем выдачу события завершённого значения
			return this->settle(record);
		}
		/**
		 * Если значение является строкой либо двоичными данными
		 */
		case static_cast <uint8_t> (group_t::STRING):
		case static_cast <uint8_t> (group_t::BLOB): {
			// Признак того, что значение является строкой
			const bool text = (unit.group == group_t::STRING);
			// Вид значения, собираемого кусками
			const type_t segment = (text ? type_t::STRING : type_t::BLOB);
			/**
			 * Если значение объявлено неопределённой длины, собирается оно кусками
			 */
			if(unit.indefinite){
				// Если стек вместимых не пуст
				if(!this->_stack.empty()){
					// Выполняем получение верхнего звена стека вместимых
					const frame_t & frame = this->_stack.back();
					/**
					 * Если значение, собираемое кусками, стоит внутри такого же:
					 * вложенности здесь нет, кусок обязан быть определённой длины
					 */
					if(frame.segment != type_t::UNDEFINED)
						// Выполняем объявление отказа разбора
						return this->fail(error_t::INVALID_SEGMENT);
					/**
					 * Если значение, собираемое кусками, стоит именем поля отображения:
					 * имя обязано быть цельным, иначе строгий вид записи невозможен
					 */
					if(frame.mapping && frame.expectKey)
						// Выполняем объявление отказа разбора
						return this->fail(error_t::INVALID_KEY);
				}
				// Выполняем получение предела глубины вложенности
				const uint32_t depth = ((this->_settings.maxDepth > 0) ?
				 ((this->_settings.maxDepth < MAX_DEPTH) ? this->_settings.maxDepth : MAX_DEPTH) : MAX_DEPTH);
				// Если глубина вложенности превышает допустимую
				if(static_cast <uint32_t> (this->_stack.size() + 1) > depth)
					// Выполняем объявление отказа разбора
					return this->fail(error_t::DEPTH_EXCEEDED);
				// Выполняем сдвиг смещения разбора
				this->_offset = offset;
				// Выполняем установку вида события начала значения
				record.event = (text ? event_t::STRING_BEGIN : event_t::BLOB_BEGIN);
				// Выполняем установку вида значения
				record.type = segment;
				// Выполняем установку признака неопределённой длины значения
				record.indefinite = true;
				// Выполняем выдачу события начала значения
				this->emit(record);
				// Заводимое звено стека вместимых
				frame_t frame;
				// Выполняем установку вида значения, собираемого кусками
				frame.segment = segment;
				// Выполняем установку признака неопределённой длины
				frame.indefinite = true;
				// Выполняем добавление звена в стек вместимых
				this->_stack.push_back(frame);
				// Сообщаем, что разбор успешен
				return true;
			}
			/**
			 * Если значение собирается кусками, кусок обязан быть того же вида
			 */
			if(!this->_stack.empty() && (this->_stack.back().segment != type_t::UNDEFINED) &&
			   (this->_stack.back().segment != segment))
				// Выполняем объявление отказа разбора
				return this->fail(error_t::INVALID_SEGMENT);
			// Выполняем получение предела длины значения
			const uint64_t limit = (text ? this->_settings.maxString : this->_settings.maxBlob);
			// Если длина значения превышает допустимую
			if((limit > 0) && (unit.value > limit))
				// Выполняем объявление отказа разбора
				return this->fail(text ? error_t::STRING_TOO_LONG : error_t::BLOB_TOO_LONG);
			// Выполняем сдвиг смещения разбора
			this->_offset = offset;
			// Выполняем установку количества недостающих октетов
			this->_pending = unit.value;
			// Выполняем установку вида ожидаемого значения
			this->_awaited = (text ? type_t::STRING : type_t::BLOB);
			// Выполняем перевод разбора в ожидание октетов значения
			this->_state = state_t::SEGMENT;
			// Сообщаем, что разбор успешен
			return true;
		}
		/**
		 * Если значение является вместимым
		 */
		case static_cast <uint8_t> (group_t::ARRAY):
		case static_cast <uint8_t> (group_t::MAP): {
			// Признак того, что вместимое является отображением
			const bool mapping = (unit.group == group_t::MAP);
			// Если стек вместимых не пуст
			if(!this->_stack.empty()){
				// Выполняем получение верхнего звена стека вместимых
				const frame_t & frame = this->_stack.back();
				/**
				 * Если вместимое стоит внутри значения, собираемого кусками: кусками
				 * собираются лишь строка и двоичные данные, вместимого там быть не может
				 */
				if(frame.segment != type_t::UNDEFINED)
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_SEGMENT);
				// Если вместимое стоит именем поля отображения
				if(frame.mapping && frame.expectKey)
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_KEY);
			}
			// Выполняем получение предела глубины вложенности
			const uint32_t limit = ((this->_settings.maxDepth > 0) ?
			 ((this->_settings.maxDepth < MAX_DEPTH) ? this->_settings.maxDepth : MAX_DEPTH) : MAX_DEPTH);
			// Если глубина вложенности превышает допустимую
			if(static_cast <uint32_t> (this->_stack.size() + 1) > limit)
				// Выполняем объявление отказа разбора
				return this->fail(error_t::DEPTH_EXCEEDED);
			// Выполняем получение предела количества узлов документа
			const uint32_t nodes = ((this->_settings.maxNodes > 0) ? this->_settings.maxNodes : MAX_NODES);
			// Если количество узлов документа превышает допустимое
			if(++this->_nodes > nodes)
				// Выполняем объявление отказа разбора
				return this->fail(error_t::TOO_MANY_NODES);
			// Выполняем сдвиг смещения разбора
			this->_offset = offset;
			// Выполняем установку вида события начала вместимого
			record.event = (mapping ? event_t::MAP_BEGIN : event_t::ARRAY_BEGIN);
			// Выполняем установку вида значения вместимого
			record.type = (mapping ? type_t::MAP : type_t::ARRAY);
			// Выполняем установку количества значений вместимого
			record.count = (unit.indefinite ? 0 : unit.value);
			// Выполняем установку признака неопределённой длины вместимого
			record.indefinite = unit.indefinite;
			// Выполняем выдачу события начала вместимого
			this->emit(record);
			// Заводимое звено стека вместимых
			frame_t frame;
			// Выполняем установку признака отображения
			frame.mapping = mapping;
			// Выполняем установку признака неопределённой длины
			frame.indefinite = unit.indefinite;
			// Выполняем установку признака ожидания имени поля
			frame.expectKey = mapping;
			// Выполняем установку количества значений вместимого
			frame.remain = (unit.indefinite ? 0 : unit.value);
			// Выполняем установку места записи за вместимым, объявленного размахом
			frame.beyond = this->_beyond;
			/**
			 * Если обработчик прямой выдачи затребовал пропуск вместимого
			 *
			 * @details Значения вместимого объявляются исчерпанными, а разбор переводится
			 * за него: закрытие вместе с учётом вместившим ведёт то же самое сматывание,
			 * что и при обычном ходе, и особого пути пропуску не нужно
			 */
			if(this->_skipping){
				// Выполняем сброс признака затребованного пропуска
				this->_skipping = false;
				// Выполняем перевод разбора за пропускаемое вместимое
				this->_offset = static_cast <size_t> (this->_beyond - this->_origin);
				// Выполняем объявление значений вместимого исчерпанными
				frame.remain = 0;
			}
			// Выполняем сброс объявленного размаха: он принадлежит открытому вместимому
			this->_span = 0;
			// Выполняем сброс места записи за вместимым
			this->_beyond = 0;
			// Выполняем добавление звена в стек вместимых
			this->_stack.push_back(frame);
			// Выполняем закрытие вместимого, если значений у него нет вовсе
			return this->unwind();
		}
		/**
		 * Если значение является одиночным
		 */
		case static_cast <uint8_t> (group_t::SINGLE): {
			/**
			 * Определяем разновидность одиночного значения
			 */
			switch(unit.detail){
				/**
				 * Если значение является пустым
				 */
				case static_cast <uint8_t> (single_t::NUL): {
					// Выполняем сдвиг смещения разбора
					this->_offset = offset;
					// Выполняем установку вида события пустого значения
					record.event = event_t::NUL;
					// Выполняем установку вида пустого значения
					record.type = type_t::NUL;
					// Выполняем выдачу события завершённого значения
					return this->settle(record);
				}
				/**
				 * Если значение является логическим
				 */
				case static_cast <uint8_t> (single_t::FALSE):
				case static_cast <uint8_t> (single_t::TRUE): {
					// Выполняем сдвиг смещения разбора
					this->_offset = offset;
					// Выполняем установку вида события логического значения
					record.event = event_t::BOOL;
					// Выполняем установку вида логического значения
					record.type = type_t::BOOL;
					// Выполняем установку логического значения
					record.boolean = (unit.detail == static_cast <uint8_t> (single_t::TRUE));
					// Выполняем выдачу события завершённого значения
					return this->settle(record);
				}
				/**
				 * Если значение ведёт за собой запись установленной ширины
				 */
				case static_cast <uint8_t> (single_t::FLOAT):
				case static_cast <uint8_t> (single_t::DOUBLE):
				case static_cast <uint8_t> (single_t::TIME):
				case static_cast <uint8_t> (single_t::UUID): {
					// Выполняем сдвиг смещения разбора
					this->_offset = offset;
					// Если значение является дробным одинарной точности
					if(unit.detail == static_cast <uint8_t> (single_t::FLOAT)){
						// Выполняем установку количества недостающих октетов
						this->_pending = 4;
						// Выполняем установку вида ожидаемого значения
						this->_awaited = type_t::FLOAT;
					// Если значение является дробным двойной точности
					} else if(unit.detail == static_cast <uint8_t> (single_t::DOUBLE)) {
						// Выполняем установку количества недостающих октетов
						this->_pending = 8;
						// Выполняем установку вида ожидаемого значения
						this->_awaited = type_t::DOUBLE;
					// Если значение является отметкой времени
					} else if(unit.detail == static_cast <uint8_t> (single_t::TIME)) {
						// Выполняем установку количества недостающих октетов
						this->_pending = TIME_WIDTH;
						// Выполняем установку вида ожидаемого значения
						this->_awaited = type_t::TIME;
					// Если значение является опознавателем
					} else {
						// Выполняем установку количества недостающих октетов
						this->_pending = UUID_WIDTH;
						// Выполняем установку вида ожидаемого значения
						this->_awaited = type_t::UUID;
					}
					// Выполняем перевод разбора в ожидание октетов значения
					this->_state = state_t::SINGLE;
					// Сообщаем, что разбор успешен
					return true;
				}
				/**
				 * Если встречен конец вместимого неопределённой длины
				 */
				case static_cast <uint8_t> (single_t::BREAK): {
					// Если стек вместимых пуст
					if(this->_stack.empty())
						// Выполняем объявление отказа разбора
						return this->fail(error_t::UNBALANCED_BREAK);
					// Выполняем получение верхнего звена стека вместимых
					const frame_t & frame = this->_stack.back();
					/**
					 * Если концом закрывается значение, собираемое кусками
					 */
					if(frame.segment != type_t::UNDEFINED){
						// Признак того, что закрываемое значение является строкой
						const bool string = (frame.segment == type_t::STRING);
						// Выполняем сдвиг смещения разбора
						this->_offset = offset;
						// Выполняем снятие звена со стека вместимых
						this->_stack.pop_back();
						// Выполняем установку вида события конца значения
						record.event = (string ? event_t::STRING_END : event_t::BLOB_END);
						// Выполняем установку вида значения
						record.type = (string ? type_t::STRING : type_t::BLOB);
						/**
						 * Выполняем учёт завершённого значения вместившим его: значение,
						 * собранное кусками, есть одно значение, а не череда их
						 */
						return this->finalize(record);
					}
					// Если длина вместимого объявлена
					if(!frame.indefinite)
						// Выполняем объявление отказа разбора
						return this->fail(error_t::UNBALANCED_BREAK);
					// Если отображение оборвалось на имени поля
					if(frame.mapping && !frame.expectKey)
						// Выполняем объявление отказа разбора
						return this->fail(error_t::MISSING_VALUE);
					/**
					 * Если объявленный размах с настоящим концом вместимого не сошёлся
					 *
					 * @details Поверка та же, что и у вместимого объявленной длины, но
					 * дорога закрытия здесь ВТОРАЯ: вместимое неопределённой длины
					 * закрывается меткою конца, а не исчерпанием значений, и сматывание
					 * его не касается. Своя сборка размаха неопределённому вместимому не
					 * объявляет вовсе, но запись приходит извне, и объявление это надлежит
					 * поверить на обеих дорогах - иначе поверенным окажется один вид
					 * записи из двух
					 *
					 * @note Место конца берётся ЗА меткою конца: она принадлежит вместимому
					 * так же, как и его значения
					 */
					if((frame.beyond > 0) && (frame.beyond != (this->_origin + static_cast <uint64_t> (offset))))
						// Выполняем объявление отказа разбора
						return this->fail(error_t::INVALID_LENGTH);
					// Признак того, что закрываемое вместимое является отображением
					const bool mapping = frame.mapping;
					// Выполняем сдвиг смещения разбора
					this->_offset = offset;
					// Выполняем снятие звена со стека вместимых
					this->_stack.pop_back();
					// Выполняем установку вида события конца вместимого
					record.event = (mapping ? event_t::MAP_END : event_t::ARRAY_END);
					// Выполняем выдачу события конца вместимого
					this->emit(record);
					// Если стек вместимых опустел, документ разобран до конца
					if(this->_stack.empty()){
						// Собираемое событие завершённого документа
						record_t finish;
						// Выполняем установку вида события завершённого документа
						finish.event = event_t::DOCUMENT;
						// Выполняем выдачу события завершённого документа
						this->emit(finish);
						// Выполняем установку признака завершённости документа
						this->_document = true;
						// Выполняем перевод разбора в завершённое состояние
						this->_state = state_t::FINISHED;
						// Сообщаем, что разбор успешен
						return true;
					}
					// Выполняем получение звена вместимого, вместившего закрытое
					frame_t & parent = this->_stack.back();
					// Если вместившее является отображением, следующим ожидается имя поля
					if(parent.mapping)
						// Выполняем установку признака ожидания имени поля
						parent.expectKey = true;
					// Если длина вместившего определена
					if(!parent.indefinite)
						// Выполняем учёт закрытого вместимого значением вместившего
						parent.remain--;
					// Выполняем закрытие исчерпанных вместимых
					return this->unwind();
				}
			}
			// Выполняем объявление отказа разбора
			return this->fail(error_t::RESERVED_TAG);
		}
		/**
		 * Если значение является расширением
		 */
		case static_cast <uint8_t> (group_t::EXTEND): {
			// Выполняем сдвиг смещения разбора
			this->_offset = offset;
			// Выполняем установку разновидности ожидаемого расширения
			this->_extend = static_cast <extend_t> (unit.detail);
			// Выполняем сброс десятичного порядка величины
			this->_exponent = 0;
			// Если расширение является десятичным числом
			if(this->_extend == extend_t::DECIMAL)
				// Выполняем перевод разбора в ожидание десятичного порядка
				this->_state = state_t::EXTEND_EXPONENT;
			// Иначе, если расширение заведено потребителем
			else if(this->_extend == extend_t::CUSTOM)
				// Выполняем перевод разбора в ожидание номера подвида расширения
				this->_state = state_t::CUSTOM_SUBTYPE;
			/**
			 * Иначе, если расширением объявлен размах вместимого
			 *
			 * @note Метка эта разбору ПРОЗРАЧНА: за размахом стоит само вместимое, и
			 *       разбор, снявший размах, продолжается им как ни в чём не бывало.
			 *       Размах служит лишь пропуску вместимого целиком
			 */
			else if(this->_extend == extend_t::SPANNED)
				// Выполняем перевод разбора в ожидание октетов размаха
				this->_state = state_t::SPANNED_WIDTH;
			// Если расширение является целым любой ширины
			else this->_state = state_t::EXTEND_LENGTH;
			// Сообщаем, что разбор успешен
			return true;
		}
	}
	// Выполняем объявление отказа разбора
	return this->fail(error_t::UNKNOWN_TAG);
}
/**
 * @brief Метод разбора накопленных октетов записи
 *
 * @return признак успешности разбора
 *
 */
bool awh::codec::abc::Reader::process() noexcept {
	/**
	 * Выполняем разбор накопленных октетов записи
	 */
	for(;;){
		/**
		 * Определяем состояние разбора записи
		 */
		switch(static_cast <uint8_t> (this->_state)){
			/**
			 * Если ожидается ведущий октет очередной единицы
			 */
			case static_cast <uint8_t> (state_t::ITEM): {
				// Выполняем установку смещения начала разбираемой единицы
				this->_mark.offset = (this->_origin + static_cast <uint64_t> (this->_offset));
				// Выполняем установку глубины вложенности разбираемой единицы
				this->_mark.depth = static_cast <uint32_t> (this->_stack.size());
				// Признак того, что единицу разобрать не удалось
				bool done = false;
				// Если разбор очередной единицы отвечен отказом
				if(!this->item(done))
					// Сообщаем, что разбор отвечен отказом
					return false;
				// Если октетов единицы недостаёт
				if(done)
					// Сообщаем, что разбор успешен
					return true;
			} break;
			/**
			 * Если ожидаются октеты строки либо двоичных данных
			 */
			case static_cast <uint8_t> (state_t::SEGMENT): {
				// Если октетов значения недостаёт
				if((this->_buffer.size() - this->_offset) < this->_pending)
					// Сообщаем, что разбор успешен
					return true;
				// Если строку следует проверить на соответствие кодировке
				if((this->_awaited == type_t::STRING) && this->_settings.validate){
					// Смещение первой негодной последовательности
					size_t position = 0;
					// Если строка кодировке UTF-8 не отвечает
					if(!abc::utf8(this->_buffer.data() + this->_offset, static_cast <size_t> (this->_pending), position)){
						// Выполняем сдвиг смещения разбора к негодной последовательности
						this->_offset += position;
						// Выполняем объявление отказа разбора
						return this->fail(error_t::INVALID_ENCODING);
					}
				}
				// Собираемое событие разбора
				record_t record;
				// Выполняем установку вида события значения
				record.event = ((this->_awaited == type_t::STRING) ? event_t::STRING : event_t::BLOB);
				// Выполняем установку вида значения
				record.type = this->_awaited;
				// Выполняем установку отрезка содержимого значения
				record.span = span_t(static_cast <uint32_t> (this->_offset), static_cast <uint32_t> (this->_pending));
				// Выполняем сдвиг смещения разбора
				this->_offset += static_cast <size_t> (this->_pending);
				// Выполняем сброс количества недостающих октетов
				this->_pending = 0;
				// Выполняем перевод разбора в ожидание очередной единицы
				this->_state = state_t::ITEM;
				/**
				 * Если значение собирается кусками, кусок выдаётся сам по себе.
				 *
				 * Учитывать его вместившим нельзя: значений у вместившего от того
				 * прибавилось бы, а кусок есть часть значения, а не значение
				 */
				if(!this->_stack.empty() && (this->_stack.back().segment != type_t::UNDEFINED)){
					// Выполняем выдачу события куска значения
					this->emit(record);
					// Прекращаем разбор куска значения
					break;
				}
				// Если выдача события завершённого значения отвечена отказом
				if(!this->settle(record))
					// Сообщаем, что разбор отвечен отказом
					return false;
			} break;
			/**
			 * Если ожидаются октеты одиночного значения
			 */
			case static_cast <uint8_t> (state_t::SINGLE): {
				// Если октетов значения недостаёт
				if((this->_buffer.size() - this->_offset) < this->_pending)
					// Сообщаем, что разбор успешен
					return true;
				// Собираемое событие разбора
				record_t record;
				// Выполняем установку вида значения
				record.type = this->_awaited;
				/**
				 * Определяем вид ожидаемого значения
				 */
				switch(static_cast <uint32_t> (this->_awaited)){
					/**
					 * Если значение является дробным одинарной точности
					 */
					case static_cast <uint32_t> (type_t::FLOAT): {
						// Разрядная запись дробного числа
						const uint32_t bits = static_cast <uint32_t> (abc::gather(this->_buffer.data() + this->_offset, 4));
						// Снятое дробное число одинарной точности
						float value = 0.0f;
						// Выполняем снятие дробного числа из разрядной записи
						::memcpy(&value, &bits, sizeof(value));
						// Выполняем установку вида события числа
						record.event = event_t::NUMBER;
						// Выполняем установку дробного значения
						record.real = static_cast <double> (value);
					} break;
					/**
					 * Если значение является дробным двойной точности
					 */
					case static_cast <uint32_t> (type_t::DOUBLE): {
						// Разрядная запись дробного числа
						const uint64_t bits = abc::gather(this->_buffer.data() + this->_offset, 8);
						// Снятое дробное число двойной точности
						double value = 0.0;
						// Выполняем снятие дробного числа из разрядной записи
						::memcpy(&value, &bits, sizeof(value));
						// Выполняем установку вида события числа
						record.event = event_t::NUMBER;
						// Выполняем установку дробного значения
						record.real = value;
					} break;
					/**
					 * Если значение является отметкой времени
					 */
					case static_cast <uint32_t> (type_t::TIME): {
						// Разрядная запись отметки времени
						const uint64_t bits = abc::gather(this->_buffer.data() + this->_offset, 8);
						// Выполняем установку вида события отметки времени
						record.event = event_t::TIME;
						// Выполняем установку значения отметки времени
						record.integer = static_cast <int64_t> (bits);
						// Выполняем установку разрядной записи отметки времени
						record.number = bits;
					} break;
					/**
					 * Если значение является опознавателем
					 */
					case static_cast <uint32_t> (type_t::UUID): {
						// Выполняем установку вида события опознавателя
						record.event = event_t::UUID;
						// Выполняем установку отрезка содержимого опознавателя
						record.span = span_t(static_cast <uint32_t> (this->_offset), static_cast <uint32_t> (UUID_WIDTH));
					} break;
				}
				// Выполняем сдвиг смещения разбора
				this->_offset += static_cast <size_t> (this->_pending);
				// Выполняем сброс количества недостающих октетов
				this->_pending = 0;
				// Выполняем перевод разбора в ожидание очередной единицы
				this->_state = state_t::ITEM;
				/**
				 * Если значение собирается кусками, кусок выдаётся сам по себе.
				 *
				 * Учитывать его вместившим нельзя: значений у вместившего от того
				 * прибавилось бы, а кусок есть часть значения, а не значение
				 */
				if(!this->_stack.empty() && (this->_stack.back().segment != type_t::UNDEFINED)){
					// Выполняем выдачу события куска значения
					this->emit(record);
					// Прекращаем разбор куска значения
					break;
				}
				// Если выдача события завершённого значения отвечена отказом
				if(!this->settle(record))
					// Сообщаем, что разбор отвечен отказом
					return false;
			} break;
			/**
			 * Если ожидается десятичный порядок величины
			 */
			case static_cast <uint8_t> (state_t::EXTEND_EXPONENT): {
				// Снятая единица проволочной записи
				item_t unit;
				// Код отказа снятия единицы
				error_t error = error_t::NONE;
				// Смещение, с какого следует снимать единицу
				size_t offset = this->_offset;
				// Если снять единицу проволочной записи не удалось
				if(!abc::take(this->_buffer.data(), this->_buffer.size(), offset, unit, error)){
					// Если октетов единицы недостаёт
					if(error == error_t::UNEXPECTED_EOF)
						// Сообщаем, что разбор успешен
						return true;
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_DECIMAL);
				}
				// Если десятичный порядок является целым без знака
				if(unit.group == group_t::UNSIGNED){
					// Если порядок видом целого со знаком не представим
					if(unit.value > static_cast <uint64_t> (numeric_limits <int64_t>::max()))
						// Выполняем объявление отказа разбора
						return this->fail(error_t::INVALID_DECIMAL);
					// Выполняем установку десятичного порядка величины
					this->_exponent = static_cast <int64_t> (unit.value);
				// Если десятичный порядок является целым со знаком
				} else if(unit.group == group_t::NEGATIVE) {
					// Если обратить запись дополнения до −1 не удалось
					if(!abc::negative(unit.value, this->_exponent))
						// Выполняем объявление отказа разбора
						return this->fail(error_t::INVALID_DECIMAL);
				// Если на месте десятичного порядка стоит иное
				} else return this->fail(error_t::INVALID_DECIMAL);
				// Выполняем сдвиг смещения разбора
				this->_offset = offset;
				// Выполняем перевод разбора в ожидание длины октетов величины
				this->_state = state_t::EXTEND_LENGTH;
			} break;
			/**
			 * Если ожидается длина октетов величины
			 */
			/**
			 * Если ожидаются октеты объявленного размаха вместимого
			 */
			case static_cast <uint8_t> (state_t::SPANNED_WIDTH): {
				// Если октетов размаха вместимого недостаёт
				if((this->_offset + SPAN_LENGTH) > this->_buffer.size())
					// Сообщаем, что разбор успешен
					return true;
				// Снимаемый размах вместимого
				uint64_t width = 0;
				// Выполняем перебор всех октетов записи размаха
				for(size_t i = 0; i < SPAN_LENGTH; i++)
					// Выполняем снятие очередного октета размаха
					width |= (static_cast <uint64_t> (this->_buffer.at(this->_offset + i)) << (i * 8));
				// Выполняем сдвиг смещения разбора за запись размаха
				this->_offset += SPAN_LENGTH;
				// Выполняем получение места, с какого отсчитывается размах
				const uint64_t place = (this->_origin + static_cast <uint64_t> (this->_offset));
				/**
				 * Если объявленный размах переполняет счёт мест записи
				 *
				 * @details Размах приходит ИЗВНЕ и доверия не заслуживает: сложение его с
				 * местом переполнилось бы, и место за вместимым вышло бы МЕНЬШЕ нынешнего.
				 * Пропуск по такому месту увёл бы разбор назад по буферу, а взятый кругом -
				 * и вовсе закружил бы его навсегда
				 */
				if(width > (numeric_limits <uint64_t>::max() - place))
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_LENGTH);
				// Выполняем установку размаха вместимого, стоящего следом
				this->_span = width;
				/**
				 * Выполняем установку места записи за вместимым.
				 *
				 * Место берётся ПОЛНЫМ, от начала записи: буфер ужимается по выдаче
				 * событий, и смещение в нём ужатия не переживает
				 */
				this->_beyond = (place + width);
				// Выполняем перевод разбора в ожидание очередной единицы
				this->_state = state_t::ITEM;
			} break;
			case static_cast <uint8_t> (state_t::EXTEND_LENGTH): {
				// Снятая единица проволочной записи
				item_t unit;
				// Код отказа снятия единицы
				error_t error = error_t::NONE;
				// Смещение, с какого следует снимать единицу
				size_t offset = this->_offset;
				// Если снять единицу проволочной записи не удалось
				if(!abc::take(this->_buffer.data(), this->_buffer.size(), offset, unit, error)){
					// Если октетов единицы недостаёт
					if(error == error_t::UNEXPECTED_EOF)
						// Сообщаем, что разбор успешен
						return true;
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_BIGNUM);
				}
				// Если на месте длины октетов величины стоит иное
				if(unit.group != group_t::UNSIGNED)
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_BIGNUM);
				// Выполняем сдвиг смещения разбора
				this->_offset = offset;
				// Выполняем установку количества недостающих октетов
				this->_pending = unit.value;
				// Выполняем перевод разбора в ожидание знака величины
				this->_state = state_t::EXTEND_SIGN;
			} break;
			/**
			 * Если ожидается знак величины
			 */
			case static_cast <uint8_t> (state_t::EXTEND_SIGN): {
				// Если октета знака величины недостаёт
				if(this->_offset >= this->_buffer.size())
					// Сообщаем, что разбор успешен
					return true;
				// Выполняем снятие октета знака величины
				const uint8_t sign = this->_buffer.at(this->_offset);
				// Если знак величины не опознан
				if(sign > 1)
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_BIGNUM);
				// Если величина объявлена отрицательным нулём
				if((sign == 1) && (this->_pending == 0))
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_BIGNUM);
				// Выполняем установку признака отрицательности величины
				this->_negative = (sign == 1);
				// Выполняем сдвиг смещения разбора
				this->_offset++;
				// Выполняем перевод разбора в ожидание октетов величины
				this->_state = state_t::EXTEND_DATA;
			} break;
			/**
			 * Если ожидаются октеты величины
			 */
			case static_cast <uint8_t> (state_t::EXTEND_DATA): {
				// Если октетов величины недостаёт
				if((this->_buffer.size() - this->_offset) < this->_pending)
					// Сообщаем, что разбор успешен
					return true;
				// Если старший октет величины нулевой
				if((this->_pending > 0) && (this->_buffer.at(this->_offset + static_cast <size_t> (this->_pending) - 1) == 0))
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_BIGNUM);
				// Собираемое событие разбора
				record_t record;
				// Выполняем установку вида события числа
				record.event = event_t::NUMBER;
				// Выполняем установку вида значения
				record.type = ((this->_extend == extend_t::DECIMAL) ? type_t::DECIMAL : type_t::EXTENDED);
				// Выполняем установку отрезка содержимого величины
				record.span = span_t(static_cast <uint32_t> (this->_offset), static_cast <uint32_t> (this->_pending));
				// Выполняем установку признака отрицательности величины
				record.negative = this->_negative;
				// Выполняем установку десятичного порядка величины
				record.exponent = this->_exponent;
				// Выполняем сдвиг смещения разбора
				this->_offset += static_cast <size_t> (this->_pending);
				// Выполняем сброс количества недостающих октетов
				this->_pending = 0;
				// Выполняем перевод разбора в ожидание очередной единицы
				this->_state = state_t::ITEM;
				/**
				 * Если выдача события завершённого значения отвечена отказом.
				 *
				 * Куском собираемого значения число стоять не вправе: кусками собираются
				 * строка да двоичные данные, и число внутри них выдачу вправе лишь
				 * отвергнуть, - отчего работа выдачи и зовётся здесь без изъятий
				 */
				if(!this->settle(record))
					// Сообщаем, что разбор отвечен отказом
					return false;
			} break;
			/**
			 * Если ожидается номер подвида открытого расширения
			 */
			case static_cast <uint8_t> (state_t::CUSTOM_SUBTYPE): {
				// Снятая единица проволочной записи
				item_t unit;
				// Код отказа снятия единицы
				error_t error = error_t::NONE;
				// Смещение, с какого следует снимать единицу
				size_t offset = this->_offset;
				// Если снять единицу проволочной записи не удалось
				if(!abc::take(this->_buffer.data(), this->_buffer.size(), offset, unit, error)){
					// Если октетов единицы недостаёт
					if(error == error_t::UNEXPECTED_EOF)
						// Сообщаем, что разбор успешен
						return true;
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_EXTENSION);
				}
				// Если на месте номера подвида расширения стоит иное
				if(unit.group != group_t::UNSIGNED)
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_EXTENSION);
				// Выполняем сдвиг смещения разбора
				this->_offset = offset;
				// Выполняем установку номера подвида расширения
				this->_subtype = unit.value;
				// Выполняем перевод разбора в ожидание длины октетов расширения
				this->_state = state_t::CUSTOM_LENGTH;
			} break;
			/**
			 * Если ожидается длина октетов открытого расширения
			 */
			case static_cast <uint8_t> (state_t::CUSTOM_LENGTH): {
				// Снятая единица проволочной записи
				item_t unit;
				// Код отказа снятия единицы
				error_t error = error_t::NONE;
				// Смещение, с какого следует снимать единицу
				size_t offset = this->_offset;
				// Если снять единицу проволочной записи не удалось
				if(!abc::take(this->_buffer.data(), this->_buffer.size(), offset, unit, error)){
					// Если октетов единицы недостаёт
					if(error == error_t::UNEXPECTED_EOF)
						// Сообщаем, что разбор успешен
						return true;
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_EXTENSION);
				}
				// Если на месте длины октетов расширения стоит иное
				if(unit.group != group_t::UNSIGNED)
					// Выполняем объявление отказа разбора
					return this->fail(error_t::INVALID_EXTENSION);
				// Если длина октетов расширения превышает допустимую
				if((this->_settings.maxBlob > 0) && (unit.value > this->_settings.maxBlob))
					// Выполняем объявление отказа разбора
					return this->fail(error_t::BLOB_TOO_LONG);
				// Выполняем сдвиг смещения разбора
				this->_offset = offset;
				// Выполняем установку количества недостающих октетов
				this->_pending = unit.value;
				// Выполняем перевод разбора в ожидание октетов расширения
				this->_state = state_t::CUSTOM_DATA;
			} break;
			/**
			 * Если ожидаются октеты открытого расширения
			 */
			case static_cast <uint8_t> (state_t::CUSTOM_DATA): {
				// Если октетов расширения недостаёт
				if((this->_buffer.size() - this->_offset) < this->_pending)
					// Сообщаем, что разбор успешен
					return true;
				// Собираемое событие разбора
				record_t record;
				// Выполняем установку вида события открытого расширения
				record.event = event_t::CUSTOM;
				// Выполняем установку вида значения
				record.type = type_t::CUSTOM;
				// Выполняем установку отрезка октетов расширения
				record.span = span_t(static_cast <uint32_t> (this->_offset), static_cast <uint32_t> (this->_pending));
				// Выполняем установку номера подвида расширения
				record.number = this->_subtype;
				// Выполняем сдвиг смещения разбора
				this->_offset += static_cast <size_t> (this->_pending);
				// Выполняем сброс количества недостающих октетов
				this->_pending = 0;
				// Выполняем перевод разбора в ожидание очередной единицы
				this->_state = state_t::ITEM;
				// Если выдача события завершённого значения отвечена отказом
				if(!this->settle(record))
					// Сообщаем, что разбор отвечен отказом
					return false;
			} break;
			/**
			 * Если разбор завершён
			 */
			case static_cast <uint8_t> (state_t::FINISHED): {
				// Если октетов за окончанием документа нет
				if(this->_offset >= this->_buffer.size())
					// Сообщаем, что разбор успешен
					return true;
				// Если разбор потока документов не затребован
				if(!this->_settings.stream)
					// Выполняем объявление отказа разбора
					return this->fail(error_t::TRAILING_OCTETS);
				// Выполняем перевод разбора в ожидание очередной единицы
				this->_state = state_t::ITEM;
			} break;
			/**
			 * Если разбор отвечен отказом
			 */
			case static_cast <uint8_t> (state_t::FAILED):
				// Сообщаем, что разбор отвечен отказом
				return false;
		}
	}
}
/**
 * @brief Метод подачи куска разбираемой записи
 *
 * @param buffer буфер подаваемой записи
 * @param size   размер буфера подаваемой записи
 * @param last   признак того, что кусок последний
 * @return       признак успешности разбора
 *
 */
bool awh::codec::abc::Reader::feed(const void * buffer, const size_t size, const bool last) noexcept {
	// Если разбор отвечен отказом
	if(this->_state == state_t::FAILED)
		// Сообщаем, что разбор отвечен отказом
		return false;
	// Если буфер подаваемой записи не существует
	if((buffer == nullptr) && (size > 0))
		// Выполняем объявление отказа разбора
		return this->fail(error_t::INTERNAL);
	/**
	 * Если место под приём октетов было выдано, а принятого подано не было, выданное
	 * место обращается вспять: содержимым записи хвост его не является вовсе
	 */
	if(this->_reserved > 0){
		// Выполняем возврат выданного места под приём октетов
		this->_buffer.resize(this->_buffer.size() - this->_reserved);
		// Выполняем сброс размера выданного места
		this->_reserved = 0;
	}
	// Выполняем усечение разобранной части буфера
	this->trim();
	// Если подаваемая запись не пуста
	if(size > 0){
		// Выполняем получение указателя на подаваемую запись
		const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
		// Выполняем накопление подаваемой записи в буфере разбора
		this->_buffer.insert(this->_buffer.end(), octets, octets + size);
	}
	// Выполняем разбор накопленных октетов
	return this->digest(last);
}
/**
 * @brief Метод усечения разобранной части буфера
 *
 */
void awh::codec::abc::Reader::trim() noexcept {
	/**
	 * Выполняем усечение разобранной части буфера. Отрезки собранных событий ссылаются
	 * в буфер смещением, оттого усечение возможно лишь при пустой очереди событий
	 */
	if((this->_head >= this->_events.size()) && (this->_offset > 0)){
		// Выполняем усечение разобранной части буфера
		this->_buffer.erase(this->_buffer.begin(), this->_buffer.begin() + static_cast <ptrdiff_t> (this->_offset));
		// Выполняем сдвиг смещения начала буфера
		this->_origin += static_cast <uint64_t> (this->_offset);
		// Выполняем сброс смещения разбора в буфере
		this->_offset = 0;
	}
}
/**
 * @brief Метод разбора накопленных октетов вместе с окончанием записи
 *
 * @param last признак того, что поданный кусок записи последний
 * @return     признак успешности разбора
 *
 */
bool awh::codec::abc::Reader::digest(const bool last) noexcept {
	// Если разбор накопленных октетов отвечен отказом
	if(!this->process())
		// Сообщаем, что разбор отвечен отказом
		return false;
	// Если поданный кусок записи последний
	if(last){
		// Если разбор оборвался посреди значения
		if(this->_state != state_t::FINISHED)
			// Выполняем объявление отказа разбора
			return this->fail((this->_buffer.empty() && !this->_document) ?
			 error_t::EMPTY_RECORD : error_t::UNEXPECTED_EOF);
		// Собираемое событие окончания записи
		record_t record;
		// Выполняем установку вида события окончания записи
		record.event = event_t::FINISH;
		// Выполняем выдачу события окончания записи
		this->emit(record);
	}
	// Сообщаем, что разбор успешен
	return true;
}
/**
 * @brief Метод выдачи места под приём октетов записи
 *
 * @param size размер запрашиваемого места в октетах
 * @return     указатель на выданное место, ноль - разбор отвечен отказом
 *
 */
void * awh::codec::abc::Reader::reserve(const size_t size) noexcept {
	// Если разбор отвечен отказом
	if(this->_state == state_t::FAILED)
		// Сообщаем, что места под приём октетов нет
		return nullptr;
	// Если места под приём октетов запрошено не было
	if(size == 0){
		// Выполняем сброс размера выданного места
		this->_reserved = 0;
		// Сообщаем, что места под приём октетов нет
		return nullptr;
	}
	// Выполняем усечение разобранной части буфера
	this->trim();
	// Выполняем получение размера накопленных октетов записи
	const size_t offset = this->_buffer.size();
	// Выполняем выдачу места под приём октетов записи
	this->_buffer.resize(offset + size);
	// Выполняем запоминание размера выданного места
	this->_reserved = size;
	// Выводим указатель на выданное место
	return (this->_buffer.data() + offset);
}
/**
 * @brief Метод подачи октетов, принятых в выданное место
 *
 * @param size размер принятых октетов, не свыше выданного места
 * @param last признак того, что кусок последний
 * @return     признак успешности разбора
 *
 */
bool awh::codec::abc::Reader::commit(const size_t size, const bool last) noexcept {
	// Если разбор отвечен отказом
	if(this->_state == state_t::FAILED)
		// Сообщаем, что разбор отвечен отказом
		return false;
	// Если принятых октетов больше, чем места было выдано
	if(size > this->_reserved)
		// Выполняем объявление отказа разбора
		return this->fail(error_t::INTERNAL);
	/**
	 * Выполняем возврат невостребованной части выданного места. Принято бывает меньше
	 * запрошенного, и хвост выданного места содержимым записи не является вовсе
	 */
	this->_buffer.resize(this->_buffer.size() - (this->_reserved - size));
	// Выполняем сброс размера выданного места
	this->_reserved = 0;
	// Выполняем разбор накопленных октетов
	return this->digest(last);
}
/**
 * @brief Метод перехода к следующему собранному событию
 *
 * @return признак наличия события
 *
 */
bool awh::codec::abc::Reader::next() noexcept {
	/**
	 * Если все собранные события выданы, очередь опорожняется целиком.
	 *
	 * Память её при том удерживается: событий у крупной записи миллионы, и заводись
	 * вместилище под них заново на всякую подачу, расход выделений рос бы с размером
	 * записи, а не оставался постоянным
	 */
	if(this->_head >= this->_events.size()){
		// Выполняем опорожнение очереди выдачи
		this->_events.clear();
	// Выполняем сброс смещения первого невыданного события
	this->_head = 0;
		// Выполняем сброс смещения первого невыданного события
		this->_head = 0;
		// Выполняем сброс события, выданного последним переходом
		this->_current = record_t();
		// Сообщаем, что события нет
		return false;
	}
	// Выполняем снятие события с очереди выдачи
	this->_current = this->_events.at(this->_head);
	// Выполняем сдвиг смещения первого невыданного события
	this->_head++;
	/**
	 * Выполняем установку положения разбора местом снятого события: место берётся
	 * событием, а не состоянием разбирателя, иначе оно разошлось бы от одной лишь
	 * нарезки записи на куски
	 */
	this->_location = this->_current.location;
	// Сообщаем, что событие снято
	return true;
}
/**
 * @brief Метод извлечения вида текущего события
 *
 * @return вид текущего события разбора
 *
 */
awh::codec::abc::event_t awh::codec::abc::Reader::event() const noexcept {
	// Выводим вид текущего события разбора
	return this->_current.event;
}
/**
 * @brief Метод извлечения значения текущего события
 *
 * @return значение текущего события разбора
 *
 */
awh::codec::abc::Reader::value_t awh::codec::abc::Reader::value() const noexcept {
	// Выдаваемое значение текущего события
	value_t result;
	// Выполняем установку вида значения
	result.type = this->_current.type;
	// Выполняем установку количества значений вместимого
	result.count = this->_current.count;
	// Выполняем установку целого без знака
	result.number = this->_current.number;
	// Выполняем установку целого со знаком
	result.integer = this->_current.integer;
	// Выполняем установку дробного числа
	result.real = this->_current.real;
	// Выполняем установку десятичного порядка величины
	result.exponent = this->_current.exponent;
	// Выполняем установку логического значения
	result.boolean = this->_current.boolean;
	// Выполняем установку признака отрицательности величины
	result.negative = this->_current.negative;
	// Выполняем установку признака неопределённой длины вместимого
	result.indefinite = this->_current.indefinite;
	// Если содержимое значения не пусто
	if(this->_current.span.length > 0)
		// Выполняем установку содержимого значения видом на буфер разбора
		result.data = string_view(reinterpret_cast <const char *> (this->_buffer.data() + this->_current.span.offset),
		 static_cast <size_t> (this->_current.span.length));
	// Выводим значение текущего события
	return result;
}
/**
 * @brief Метод установки обработчика прямой выдачи событий разбора
 *
 * @param callback устанавливаемый обработчик, ноль - снятие обработчика
 * @param context  опора обработчика
 *
 */
void awh::codec::abc::Reader::handler(handler_t callback, void * context) noexcept {
	// Выполняем установку обработчика прямой выдачи событий
	this->_handler = callback;
	// Выполняем установку опоры обработчика
	this->_context = context;
}
/**
 * @brief Метод извлечения кода отказа разбора
 *
 * @return код отказа разбора
 *
 */
awh::codec::abc::error_t awh::codec::abc::Reader::error() const noexcept {
	// Выводим код отказа разбора
	return this->_error;
}
/**
 * @brief Метод извлечения положения разбора в поданной записи
 *
 * @return положение разбора в поданной записи
 *
 */
const awh::codec::abc::location_t & awh::codec::abc::Reader::location() const noexcept {
	// Выводим положение разбора в поданной записи
	return this->_location;
}
/**
 * @brief Метод извлечения глубины вложенности разбора
 *
 * @return глубина вложенности разбора
 *
 */
uint32_t awh::codec::abc::Reader::depth() const noexcept {
	// Выводим глубину вложенности разбора
	return this->_location.depth;
}
/**
 * @brief Метод извлечения настроек разбора записи
 *
 * @return настройки разбора записи
 *
 */
const awh::codec::abc::Reader::settings_t & awh::codec::abc::Reader::settings() const noexcept {
	// Выводим настройки разбора записи
	return this->_settings;
}
/**
 * @brief Метод установки настроек разбора записи
 *
 * @param settings устанавливаемые настройки разбора записи
 *
 */
void awh::codec::abc::Reader::settings(const settings_t & settings) noexcept {
	// Выполняем установку настроек разбора записи
	this->_settings = settings;
}
