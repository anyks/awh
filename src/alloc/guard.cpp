/**
 * @file guard.cpp
 * @date 2026-08-21
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
 * @brief Файл заслонов и карантина
 *
 * \~english
 * @brief Guard pages and quarantine file
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/guard.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Метка снесённого места таблицы поиска
 *
 */
awh::alloc::Guard::record_t * const awh::alloc::Guard::_tomb = reinterpret_cast <awh::alloc::Guard::record_t *> (static_cast <uintptr_t> (1));

/**
 * @brief Конструктор
 *
 */
awh::alloc::Guard::Guard() noexcept :
 _source(nullptr), _table(nullptr), _length(0), _enrolled(0), _meta(nullptr),
 _metaLeft(0), _spare(nullptr), _oldest(nullptr), _newest(nullptr), _rate(0), _counter(0) {}

/**
 * @brief Метод выдачи памяти под учётную запись
 *
 * @return адрес выданной памяти либо nullptr
 *
 */
void * awh::alloc::Guard::meta() noexcept {
	// Если есть повторно используемая учётная запись
	if(this->_spare != nullptr){
		// Запоминаем повторно используемую учётную запись
		record_t * result = this->_spare;
		// Сдвигаем список повторно используемых записей
		this->_spare = result->queue;
		// Выводим повторно используемую учётную запись
		return result;
	}
	// Приводим размер записи к границе восьми байт
	const size_t need = ((sizeof(record_t) + 7u) & ~static_cast <size_t> (7u));
	// Если в текущем куске места не осталось
	if(this->_metaLeft < need){
		// Требуемый размер куска под учётные записи
		const size_t span = (64u * 1024u);
		// Действительно выданный размер
		size_t actual = 0;
		// Берём у источника кусок под учётные записи
		uint8_t * block = reinterpret_cast <uint8_t *> (this->_source->alloc(span, 0, actual));
		// Если кусок не выдан
		if(block == nullptr)
			// Отвечаем отказом
			return nullptr;
		// Запоминаем начало выдачи
		this->_meta = block;
		// Запоминаем остаток куска
		this->_metaLeft = actual;
	}
	// Запоминаем адрес выдачи
	void * result = this->_meta;
	// Сдвигаем начало выдачи
	this->_meta += need;
	// Уменьшаем остаток куска
	this->_metaLeft -= need;
	// Выводим адрес выданной памяти
	return result;
}
/**
 * @brief Метод перестроения таблицы поиска
 *
 * @param length требуемая длина таблицы в местах
 * @return       признак перестроения таблицы
 *
 */
bool awh::alloc::Guard::rehash(const size_t length) noexcept {
	// Действительно выданный размер
	size_t actual = 0;
	// Требуемый размер новой таблицы в байтах
	const size_t size = (length * sizeof(record_t *));
	// Берём у источника память под новую таблицу
	record_t ** table = reinterpret_cast <record_t **> (this->_source->alloc(size, 0, actual));
	// Если память под таблицу не выдана
	if(table == nullptr)
		// Отвечаем отказом
		return false;
	// Обнуляем места новой таблицы
	::memset(table, 0, size);
	// Запоминаем прежнюю таблицу прежде подмены
	record_t ** previous = this->_table;
	// Запоминаем длину прежней таблицы
	const size_t before = this->_length;
	// Подменяем таблицу новой
	this->_table = table;
	// Запоминаем длину новой таблицы
	this->_length = length;
	// Внесённых записей у новой таблицы пока нет
	this->_enrolled = 0;
	/**
	 * Переносим в новую таблицу записи прежней
	 */
	for(size_t i = 0; i < before; i++){
		// Если место прежней таблицы занято живой записью
		if((previous[i] != nullptr) && (previous[i] != _tomb))
			// Вносим запись в новую таблицу
			this->enroll(previous[i]);
	}
	// Если прежняя таблица заведена
	if(previous != nullptr)
		// Отдаём источнику память прежней таблицы
		this->_source->release(previous, (before * sizeof(record_t *)));
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод внесения записи в таблицу поиска
 *
 * @param record вносимая запись
 * @return       признак внесения записи
 *
 */
bool awh::alloc::Guard::enroll(record_t * record) noexcept {
	// Если таблица не заведена либо заполнена наполовину
	if((this->_length == 0) || (((this->_enrolled + 1) * 2) > this->_length)){
		/**
		 * Перестраиваем таблицу вдвое большей длины
		 *
		 * Порог в половину длины взят не для скорости, а для завершимости: перебор мест
		 * идёт подряд, и при плотном заполнении он вырождается в перебор всей таблицы
		 */
		if(!this->rehash((this->_length == 0) ? TABLE : (this->_length * 2)))
			// Отвечаем отказом
			return false;
	}
	// Определяем ключ записи: адрес выданного блока
	const uintptr_t key = reinterpret_cast <uintptr_t> (record->block);
	// Определяем место записи в таблице
	size_t index = static_cast <size_t> ((key * 0x9E3779B97F4A7C15ull) % this->_length);
	/**
	 * Ищем свободное место, перебирая места подряд
	 */
	while((this->_table[index] != nullptr) && (this->_table[index] != this->_tomb)){
		// Если запись в таблице уже есть
		if(this->_table[index]->block == record->block)
			// Вносить нечего
			return true;
		// Переходим к следующему месту таблицы
		index = ((index + 1) % this->_length);
	}
	// Записываем запись в найденное место
	this->_table[index] = record;
	// Увеличиваем число внесённых записей
	this->_enrolled++;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод поиска записи по адресу блока
 *
 * @param addr  разбираемый адрес
 * @param exact признак поиска ровно по началу блока
 * @return      найденная запись либо nullptr
 *
 */
awh::alloc::Guard::record_t * awh::alloc::Guard::lookup(const void * addr, const bool exact) const noexcept {
	// Если таблица не заведена
	if((this->_table == nullptr) || (this->_length == 0))
		// Искать нечем
		return nullptr;
	// Приводим разбираемый адрес к байтовому виду
	const uint8_t * probe = reinterpret_cast <const uint8_t *> (addr);
	/**
	 * Если требуется поиск ровно по началу блока
	 */
	if(exact){
		// Определяем ключ записи: адрес выданного блока
		const uintptr_t key = reinterpret_cast <uintptr_t> (addr);
		// Определяем место записи в таблице
		size_t index = static_cast <size_t> ((key * 0x9E3779B97F4A7C15ull) % this->_length);
		// Число пройденных мест таблицы
		size_t passed = 0;
		/**
		 * Перебираем места подряд, пока не встретим пустое
		 */
		while((this->_table[index] != nullptr) && (passed < this->_length)){
			// Если место занято живой записью нужного блока
			if((this->_table[index] != this->_tomb) && (this->_table[index]->block == probe))
				// Выводим найденную запись
				return this->_table[index];
			// Переходим к следующему месту таблицы
			index = ((index + 1) % this->_length);
			// Увеличиваем число пройденных мест
			passed++;
		}
		// Записи в таблице нет
		return nullptr;
	}
	/**
	 * Перебираем таблицу целиком
	 *
	 * Внутри области адрес лежит где угодно, и начало её из адреса не вычисляется:
	 * области разного размера. Перебор здесь допустим - поиск этот идёт при разборе
	 * сбоя обращения, то есть однажды, а заслонённых блоков немного по самой выборке
	 */
	for(size_t i = 0; i < this->_length; i++){
		// Запоминаем запись места таблицы
		record_t * record = this->_table[i];
		// Если место таблицы свободно
		if((record == nullptr) || (record == this->_tomb))
			// Переходим к следующему месту
			continue;
		// Если адрес лежит внутри взятой у источника области
		if((probe >= record->base) && (probe < (record->base + record->span)))
			// Выводим найденную запись
			return record;
	}
	// Записи в таблице нет
	return nullptr;
}
/**
 * @brief Метод заведения заслонов
 *
 * @param source источник страниц
 * @return       признак заведения заслонов
 *
 */
bool awh::alloc::Guard::init(source_t * source) noexcept {
	// Если источник страниц не задан
	if(source == nullptr)
		// Отвечаем отказом
		return false;
	// Запоминаем источник страниц
	this->_source = source;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод снятия заслонов
 *
 */
void awh::alloc::Guard::reset() noexcept {
	// Захватываем замок заслонов
	hold_t hold(this->_lock);
	// Если источник страниц не задан
	if(this->_source == nullptr)
		// Снимать нечего
		return;
	/**
	 * Отдаём источнику области заслонённых блоков
	 */
	for(size_t i = 0; i < this->_length; i++){
		// Запоминаем запись места таблицы
		record_t * record = this->_table[i];
		// Если место таблицы свободно
		if((record == nullptr) || (record == this->_tomb))
			// Переходим к следующему месту
			continue;
		/**
		 * Открываем область прежде отдачи
		 *
		 * Отдача закрытой области системе годна у всех наших систем, но открытие стоит
		 * одного вызова и снимает всякий разговор о том, годна ли она у следующей
		 */
		this->_source->protect(record->base, record->span, true);
		// Отдаём источнику область заслонённого блока
		this->_source->release(record->base, record->span);
	}
	// Если таблица поиска заведена
	if(this->_table != nullptr)
		// Отдаём источнику память таблицы поиска
		this->_source->release(this->_table, (this->_length * sizeof(record_t *)));
	// Сбрасываем таблицу поиска
	this->_table = nullptr;
	// Сбрасываем длину таблицы поиска
	this->_length = 0;
	// Сбрасываем число внесённых записей
	this->_enrolled = 0;
	// Сбрасываем очередь закрытых областей
	this->_oldest = nullptr;
	// Сбрасываем конец очереди закрытых областей
	this->_newest = nullptr;
	// Сбрасываем список повторно используемых записей
	this->_spare = nullptr;
	// Сбрасываем состояние заслонов
	this->_state = state_t();
	/**
	 * Память под учётные записи источнику НЕ отдаётся
	 *
	 * Куски её взяты вразнобой и списком не связаны: связь их лежала бы в первых
	 * байтах куска, а те уже отданы под записи. Заслоны заводятся однажды на процесс,
	 * и куски эти теряются лишь при снятии распределителя, то есть перед выходом
	 */
}
/**
 * @brief Метод определения надобности заслона очередной выдаче
 *
 * @param size требуемый размер в байтах
 * @return     признак надобности заслона
 *
 */
bool awh::alloc::Guard::wanted(const size_t size) noexcept {
	// Запоминаем действующую долю выборки
	const size_t rate = this->_rate.load(std::memory_order_relaxed);
	// Если заслоны выключены либо размер для них непомерен
	if((rate == 0) || (size == 0) || (size > MAXIMUM))
		// Заслон не нужен
		return false;
	// Если выборка берёт каждую выдачу
	if(rate == 1)
		// Заслон нужен
		return true;
	// Увеличиваем счётчик выдач и берём каждую rate-ю из них
	return ((this->_counter.fetch_add(1, std::memory_order_relaxed) % rate) == 0);
}
/**
 * @brief Метод выдачи заслонённого блока
 *
 * @param size требуемый размер в байтах
 * @return     адрес выданного блока либо nullptr
 *
 */
void * awh::alloc::Guard::alloc(const size_t size) noexcept {
	// Если заслоны не заведены
	if((this->_source == nullptr) || (size == 0) || (size > MAXIMUM))
		// Выдавать нечего
		return nullptr;
	// Определяем размер страницы источника
	const size_t page = this->_source->granularity();
	// Если размер страницы источника не определён
	if(page == 0)
		// Выдавать нечего
		return nullptr;
	// Приводим требуемый размер к границе выравнивания
	const size_t need = ((size + 15u) & ~static_cast <size_t> (15u));
	// Определяем размер области данных, приведённый к страницам источника
	const size_t data = (((need + (page - 1)) / page) * page);
	// Определяем размер всей области: данные да два заслона
	const size_t span = (data + (page * 2u));
	// Действительно выданный размер
	size_t actual = 0;
	// Берём у источника область под заслонённый блок
	uint8_t * base = reinterpret_cast <uint8_t *> (this->_source->alloc(span, page, actual));
	// Если область не выдана
	if(base == nullptr)
		// Выдавать нечего
		return nullptr;
	/**
	 * Закрываем передний заслон
	 *
	 * Отказ здесь означает, что источник страницами системы не владеет: заслоны с
	 * таким источником невозможны вовсе, и выборка выключается насовсем - иначе
	 * каждая выдача брала бы у него область впустую
	 */
	if(!this->_source->protect(base, page, false)){
		// Выключаем выборку заслонов
		this->_rate.store(0, std::memory_order_relaxed);
		// Отдаём источнику взятую область
		this->_source->release(base, actual);
		// Выдавать нечего
		return nullptr;
	}
	// Закрываем задний заслон
	if(!this->_source->protect((base + page + data), page, false)){
		// Выключаем выборку заслонов
		this->_rate.store(0, std::memory_order_relaxed);
		// Открываем передний заслон прежде отдачи
		this->_source->protect(base, page, true);
		// Отдаём источнику взятую область
		this->_source->release(base, actual);
		// Выдавать нечего
		return nullptr;
	}
	/**
	 * Прижимаем блок к заднему заслону
	 *
	 * Пропуск выравнивания уходит при этом в начало области данных, где недобор
	 * ловится передним заслоном, а переполнение на единственный байт валит обращение
	 * немедленно
	 */
	uint8_t * block = ((base + page + data) - need);
	// Захватываем замок заслонов
	hold_t hold(this->_lock);
	// Выдаём память под учётную запись
	record_t * record = reinterpret_cast <record_t *> (this->meta());
	// Если память под учётную запись не выдана
	if(record == nullptr){
		// Открываем область прежде отдачи
		this->_source->protect(base, actual, true);
		// Отдаём источнику взятую область
		this->_source->release(base, actual);
		// Выдавать нечего
		return nullptr;
	}
	// Записываем адрес выданного блока
	record->block = block;
	// Записываем адрес начала взятой области
	record->base = base;
	// Записываем размер взятой области
	record->span = actual;
	// Записываем затребованный размер
	record->size = size;
	// Отмечаем область открытой
	record->sealed = false;
	// Обнуляем связь очереди закрытых областей
	record->queue = nullptr;
	// Вносим запись в таблицу поиска
	if(!this->enroll(record)){
		// Возвращаем учётную запись в список повторно используемых
		record->queue = this->_spare;
		// Запоминаем учётную запись повторно используемой
		this->_spare = record;
		// Открываем область прежде отдачи
		this->_source->protect(base, actual, true);
		// Отдаём источнику взятую область
		this->_source->release(base, actual);
		// Выдавать нечего
		return nullptr;
	}
	// Увеличиваем число живых заслонённых блоков
	this->_state.live++;
	// Увеличиваем взятое у источника под заслонённые блоки
	this->_state.taken += actual;
	// Увеличиваем выданное прикладному коду под заслонами
	this->_state.given += size;
	// Выводим адрес выданного блока
	return block;
}
/**
 * @brief Метод освобождения заслонённого блока
 *
 * @param ptr адрес освобождаемого блока
 * @return    затребованный размер блока в байтах, либо нуль если блок не наш
 *
 */
size_t awh::alloc::Guard::free(void * ptr) noexcept {
	// Если освобождать нечего
	if((ptr == nullptr) || (this->_source == nullptr))
		// Освобождать нечего
		return 0;
	// Захватываем замок заслонов
	hold_t hold(this->_lock);
	// Ищем запись освобождаемого блока
	record_t * record = this->lookup(ptr, true);
	// Если запись не найдена либо область уже закрыта
	if((record == nullptr) || record->sealed)
		// Блок не наш
		return 0;
	// Запоминаем затребованный размер блока
	const size_t result = record->size;
	/**
	 * Закрываем область целиком, а не отдаём её системе
	 *
	 * Отданный адрес достался бы следующей выдаче, и обращение по висячему указателю
	 * прошло бы молча. Закрытая же область валит обращение в точке дефекта
	 */
	this->_source->protect(record->base, record->span, false);
	// Отмечаем область закрытой
	record->sealed = true;
	// Уменьшаем число живых заслонённых блоков
	this->_state.live--;
	// Уменьшаем выданное прикладному коду под заслонами
	this->_state.given -= ((this->_state.given < result) ? this->_state.given : result);
	// Увеличиваем число закрытых областей
	this->_state.sealed++;
	/**
	 * Вносим область в конец очереди закрытых
	 */
	if(this->_newest != nullptr)
		// Связываем область с концом очереди
		this->_newest->queue = record;
	// Иначе область становится началом очереди
	else this->_oldest = record;
	// Запоминаем область концом очереди
	this->_newest = record;
	/**
	 * Отдаём системе старейшую закрытую область, если их накопилось сверх меры
	 */
	while((this->_state.sealed > SEALED) && (this->_oldest != nullptr)){
		// Запоминаем старейшую закрытую область
		record_t * oldest = this->_oldest;
		// Сдвигаем начало очереди
		this->_oldest = oldest->queue;
		// Если очередь опустела
		if(this->_oldest == nullptr)
			// Сбрасываем конец очереди
			this->_newest = nullptr;
		// Сносим запись из таблицы поиска
		if((this->_table != nullptr) && (this->_length > 0)){
			// Определяем ключ записи: адрес выданного блока
			const uintptr_t key = reinterpret_cast <uintptr_t> (oldest->block);
			// Определяем место записи в таблице
			size_t index = static_cast <size_t> ((key * 0x9E3779B97F4A7C15ull) % this->_length);
			// Число пройденных мест таблицы
			size_t passed = 0;
			/**
			 * Перебираем места подряд, пока не встретим пустое
			 */
			while((this->_table[index] != nullptr) && (passed < this->_length)){
				// Если место занято сносимой записью
				if(this->_table[index] == oldest){
					// Метим место снесённым
					this->_table[index] = this->_tomb;
					// Уменьшаем число внесённых записей
					this->_enrolled--;
					// Сносить больше нечего
					break;
				}
				// Переходим к следующему месту таблицы
				index = ((index + 1) % this->_length);
				// Увеличиваем число пройденных мест
				passed++;
			}
		}
		// Открываем область прежде отдачи
		this->_source->protect(oldest->base, oldest->span, true);
		// Уменьшаем взятое у источника под заслонённые блоки
		this->_state.taken -= ((this->_state.taken < oldest->span) ? this->_state.taken : oldest->span);
		// Отдаём источнику область заслонённого блока
		this->_source->release(oldest->base, oldest->span);
		// Возвращаем учётную запись в список повторно используемых
		oldest->queue = this->_spare;
		// Запоминаем учётную запись повторно используемой
		this->_spare = oldest;
		// Уменьшаем число закрытых областей
		this->_state.sealed--;
	}
	// Выводим затребованный размер блока
	return result;
}
/**
 * @brief Метод определения принадлежности адреса заслонам
 *
 * @param ptr  разбираемый адрес
 * @param size размер блока, если он определён
 * @return     признак принадлежности адреса заслонам
 *
 */
bool awh::alloc::Guard::owner(const void * ptr, size_t * size) const noexcept {
	// Если разбирать нечего
	if((ptr == nullptr) || (this->_source == nullptr))
		// Разбирать нечего
		return false;
	// Захватываем замок заслонов
	hold_t hold(const_cast <spin_t &> (this->_lock));
	// Ищем запись разбираемого блока
	record_t * record = this->lookup(ptr, true);
	// Если запись не найдена либо область уже закрыта
	if((record == nullptr) || record->sealed)
		// Адрес заслонам не принадлежит
		return false;
	// Если требуется размер блока
	if(size != nullptr)
		// Записываем затребованный размер блока
		(* size) = record->size;
	// Адрес принадлежит заслонам
	return true;
}
/**
 * @brief Метод разбора адреса обращения
 *
 * @param addr   разбираемый адрес
 * @param begin  адрес начала блока, если он определён
 * @param size   размер блока, если он определён
 * @param offset смещение разбираемого адреса от начала блока
 * @param sealed признак освобождённого блока
 * @return       признак принадлежности адреса заслонённой области
 *
 */
bool awh::alloc::Guard::resolve(const void * addr, const void ** begin, size_t * size, ptrdiff_t * offset, bool * sealed) const noexcept {
	// Если разбирать нечего
	if((addr == nullptr) || (this->_source == nullptr))
		// Разбирать нечего
		return false;
	// Захватываем замок заслонов
	hold_t hold(const_cast <spin_t &> (this->_lock));
	// Ищем запись разбираемого адреса
	record_t * record = this->lookup(addr, false);
	// Если запись не найдена
	if(record == nullptr)
		// Адрес заслонённой области не принадлежит
		return false;
	// Если требуется адрес начала блока
	if(begin != nullptr)
		// Записываем адрес начала блока
		(* begin) = record->block;
	// Если требуется размер блока
	if(size != nullptr)
		// Записываем затребованный размер блока
		(* size) = record->size;
	// Если требуется смещение разбираемого адреса
	if(offset != nullptr)
		// Записываем смещение разбираемого адреса от начала блока
		(* offset) = (reinterpret_cast <const uint8_t *> (addr) - record->block);
	// Если требуется признак освобождённого блока
	if(sealed != nullptr)
		// Записываем признак освобождённого блока
		(* sealed) = record->sealed;
	// Адрес принадлежит заслонённой области
	return true;
}
/**
 * @brief Метод задания доли выборки
 *
 * @param rate одна выдача из скольких: нуль - заслоны выключены
 *
 */
void awh::alloc::Guard::rate(const size_t rate) noexcept {
	// Запоминаем долю выборки
	this->_rate.store(rate, std::memory_order_relaxed);
}
/**
 * @brief Метод получения состояния заслонов
 *
 * @return состояние заслонов
 *
 */
awh::alloc::Guard::state_t awh::alloc::Guard::state() noexcept {
	// Захватываем замок заслонов
	hold_t hold(this->_lock);
	// Выводим состояние заслонов
	return this->_state;
}
/**
 * @brief Метод захвата замка перед ветвлением процесса
 *
 */
void awh::alloc::Guard::prepare() noexcept {
	// Захватываем замок заслонов
	this->_lock.acquire();
}
/**
 * @brief Метод отпускания замка после ветвления процесса
 *
 */
void awh::alloc::Guard::resume() noexcept {
	// Отпускаем замок заслонов
	this->_lock.release();
}
/**
 * @brief Метод приведения замка в порядок у потомка ветвления
 *
 */
void awh::alloc::Guard::adopt() noexcept {
	// Приводим замок заслонов в порядок
	this->_lock.reset();
}

/**
 * @brief Конструктор
 *
 */
awh::alloc::Quarantine::Quarantine() noexcept :
 _source(nullptr), _ring(nullptr), _length(0), _region(0), _head(0), _tail(0),
 _held(0), _bytes(0), _limit(0), _junk(false) {}

/**
 * @brief Метод сличения засева удерживаемого блока
 *
 * @param block разбираемый блок
 * @param size  размер блока в байтах
 * @return      признак сохранности засева
 *
 */
bool awh::alloc::Quarantine::intact(const void * block, const size_t size) noexcept {
	// Приводим разбираемый блок к байтовому виду
	const uint8_t * probe = reinterpret_cast <const uint8_t *> (block);
	/**
	 * Сличаем засев блока побайтово
	 */
	for(size_t i = 0; i < size; i++){
		// Если знак засева не сохранился
		if(probe[i] != JUNK){
			// Увеличиваем число испорченных блоков
			this->_state.spoiled++;
			// Если испорченный блок первый по счёту
			if(this->_state.culprit == nullptr){
				// Запоминаем адрес испорченного блока
				this->_state.culprit = block;
				// Запоминаем смещение первой порчи
				this->_state.offset = i;
			}
			// Засев не сохранился
			return false;
		}
	}
	// Засев сохранился
	return true;
}
/**
 * @brief Метод заведения карантина
 *
 * @param source источник страниц
 * @param limit  потолок объёма карантина в байтах: нуль - карантин выключен
 * @return       признак заведения карантина
 *
 */
bool awh::alloc::Quarantine::init(source_t * source, const size_t limit) noexcept {
	// Если источник страниц не задан
	if(source == nullptr)
		// Отвечаем отказом
		return false;
	// Запоминаем источник страниц
	this->_source = source;
	// Запоминаем потолок объёма карантина
	this->_limit = limit;
	// Если карантин выключен
	if(limit == 0)
		// Заводить кольцо незачем
		return true;
	/**
	 * Определяем размер кольца по объёму карантина
	 *
	 * Кольцо неизменно, а размер блоков заранее неизвестен: доля объёма на место взята
	 * средней по разрядам. Переполнись кольцо раньше объёма - карантин просто окажется
	 * короче заказанного, но верным останется
	 */
	size_t length = (limit / SHARE);
	// Если мест кольца выходит меньше наименьшего
	if(length < MINIMUM)
		// Берём наименьшее число мест
		length = MINIMUM;
	// Если мест кольца выходит больше наибольшего
	if(length > LIMIT)
		// Берём наибольшее число мест
		length = LIMIT;
	// Действительно выданный размер
	size_t actual = 0;
	// Берём у источника память под кольцо
	slot_t * ring = reinterpret_cast <slot_t *> (source->alloc((length * sizeof(slot_t)), 0, actual));
	// Если память под кольцо не выдана
	if(ring == nullptr)
		// Отвечаем отказом
		return false;
	// Обнуляем места кольца
	::memset(ring, 0, (length * sizeof(slot_t)));
	// Запоминаем кольцо
	this->_ring = ring;
	// Запоминаем размер кольца
	this->_length = length;
	// Запоминаем размер взятой под кольцо области
	this->_region = actual;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод снятия карантина
 *
 */
void awh::alloc::Quarantine::reset() noexcept {
	// Захватываем замок карантина
	hold_t hold(this->_lock);
	// Если кольцо заведено
	if((this->_ring != nullptr) && (this->_source != nullptr))
		// Отдаём источнику память кольца
		this->_source->release(this->_ring, this->_region);
	// Сбрасываем кольцо
	this->_ring = nullptr;
	// Сбрасываем размер кольца
	this->_length = 0;
	// Сбрасываем размер взятой под кольцо области
	this->_region = 0;
	// Сбрасываем место записи очередного блока
	this->_head = 0;
	// Сбрасываем место изъятия старейшего блока
	this->_tail = 0;
	// Сбрасываем число удерживаемых блоков
	this->_held = 0;
	// Сбрасываем объём удерживаемой памяти
	this->_bytes = 0;
}
/**
 * @brief Метод удержания освобождённого блока
 *
 * @param ptr   адрес удерживаемого блока
 * @param size  размер удерживаемого блока в байтах
 * @param index номер разряда, которому принадлежит блок
 * @return      признак принятия блока карантином
 *
 */
bool awh::alloc::Quarantine::hold(void * ptr, const size_t size, const size_t index) noexcept {
	// Если удерживать нечего
	if((ptr == nullptr) || (size == 0))
		// Удерживать нечего
		return false;
	// Захватываем замок карантина
	hold_t hold(this->_lock);
	// Если карантин не заведён либо выключен
	if((this->_ring == nullptr) || (this->_limit == 0))
		// Удерживать нечем
		return false;
	/**
	 * Если блок не вмещается в карантин целиком
	 *
	 * Приняв такой блок, карантин вытеснил бы им всё удерживаемое разом и обратился бы
	 * в отсрочку на один блок - то есть перестал бы ловить обращение к освобождённому
	 */
	if(size > this->_limit)
		// Удерживать блок незачем
		return false;
	// Если кольцо заполнено целиком
	if(this->_held >= this->_length)
		// Удерживать блок негде
		return false;
	// Если требуется засевать удерживаемую память
	if(this->_junk)
		// Засеваем удерживаемую память заметным образом
		::memset(ptr, JUNK, size);
	// Записываем удерживаемый блок в место кольца
	this->_ring[this->_head].block = ptr;
	// Записываем размер удерживаемого блока
	this->_ring[this->_head].size = size;
	// Записываем номер разряда удерживаемого блока
	this->_ring[this->_head].index = index;
	// Сдвигаем место записи очередного блока
	this->_head = ((this->_head + 1) % this->_length);
	// Увеличиваем число удерживаемых блоков
	this->_held++;
	// Увеличиваем объём удерживаемой памяти
	this->_bytes += size;
	// Записываем число удерживаемых блоков в состояние
	this->_state.held = this->_held;
	// Записываем объём удерживаемой памяти в состояние
	this->_state.bytes = this->_bytes;
	// Блок принят карантином
	return true;
}
/**
 * @brief Метод изъятия старейшего удерживаемого блока
 *
 * @param size  размер изъятого блока в байтах
 * @param index номер разряда изъятого блока
 * @return      адрес изъятого блока либо nullptr
 *
 */
void * awh::alloc::Quarantine::release(size_t * size, size_t * index) noexcept {
	// Захватываем замок карантина
	hold_t hold(this->_lock);
	// Если карантин пуст
	if((this->_ring == nullptr) || (this->_held == 0))
		// Изымать нечего
		return nullptr;
	/**
	 * Если карантин вмещает удерживаемое
	 *
	 * Изымать блок раньше времени незачем: чем дольше он лежит, тем длиннее промежуток,
	 * на каком ловится обращение по висячему указателю
	 */
	if((this->_bytes <= this->_limit) && (this->_held < this->_length))
		// Изымать нечего
		return nullptr;
	// Запоминаем старейший удерживаемый блок
	void * result = this->_ring[this->_tail].block;
	// Запоминаем размер старейшего удерживаемого блока
	const size_t served = this->_ring[this->_tail].size;
	// Если требуется размер изъятого блока
	if(size != nullptr)
		// Записываем размер изъятого блока
		(* size) = served;
	// Если требуется номер разряда изъятого блока
	if(index != nullptr)
		// Записываем номер разряда изъятого блока
		(* index) = this->_ring[this->_tail].index;
	/**
	 * Сличаем засев изъятого блока
	 *
	 * Порча его означает запись по указателю, освобождённому прежде: тот самый дефект,
	 * ради какого карантин и заведён. Доклад о нём идёт состоянием, а не журналом -
	 * запись в журнал выделяет память, а мы стоим внутри освобождения
	 */
	if(this->_junk)
		// Сличаем засев изъятого блока
		this->intact(result, served);
	// Обнуляем место кольца
	this->_ring[this->_tail].block = nullptr;
	// Сдвигаем место изъятия старейшего блока
	this->_tail = ((this->_tail + 1) % this->_length);
	// Уменьшаем число удерживаемых блоков
	this->_held--;
	// Уменьшаем объём удерживаемой памяти
	this->_bytes -= ((this->_bytes < served) ? this->_bytes : served);
	// Увеличиваем число блоков, прошедших карантин
	this->_state.passed++;
	// Записываем число удерживаемых блоков в состояние
	this->_state.held = this->_held;
	// Записываем объём удерживаемой памяти в состояние
	this->_state.bytes = this->_bytes;
	// Выводим адрес изъятого блока
	return result;
}
/**
 * @brief Метод определения удержания адреса карантином
 *
 * @param addr  разбираемый адрес
 * @param begin адрес начала блока, если он определён
 * @param size  размер блока, если он определён
 * @return      признак удержания адреса карантином
 *
 */
bool awh::alloc::Quarantine::held(const void * addr, const void ** begin, size_t * size) noexcept {
	// Если разбирать нечего
	if(addr == nullptr)
		// Разбирать нечего
		return false;
	// Захватываем замок карантина
	hold_t hold(this->_lock);
	// Если карантин пуст
	if((this->_ring == nullptr) || (this->_held == 0))
		// Разбирать нечего
		return false;
	// Приводим разбираемый адрес к байтовому виду
	const uint8_t * probe = reinterpret_cast <const uint8_t *> (addr);
	/**
	 * Перебираем места кольца целиком
	 *
	 * Поиск этот идёт при разборе сбоя обращения, то есть однажды: заводить ради него
	 * вторую таблицу значило бы платить памятью на каждом освобождении
	 */
	for(size_t i = 0; i < this->_length; i++){
		// Запоминаем удерживаемый блок места кольца
		const uint8_t * block = reinterpret_cast <const uint8_t *> (this->_ring[i].block);
		// Если место кольца свободно
		if(block == nullptr)
			// Переходим к следующему месту
			continue;
		// Если адрес лежит внутри удерживаемого блока
		if((probe >= block) && (probe < (block + this->_ring[i].size))){
			// Если требуется адрес начала блока
			if(begin != nullptr)
				// Записываем адрес начала блока
				(* begin) = block;
			// Если требуется размер блока
			if(size != nullptr)
				// Записываем размер блока
				(* size) = this->_ring[i].size;
			// Адрес удерживается карантином
			return true;
		}
	}
	// Адрес карантином не удерживается
	return false;
}
/**
 * @brief Метод задания потолка объёма карантина
 *
 * @param limit потолок объёма в байтах: нуль - карантин выключен
 *
 */
void awh::alloc::Quarantine::limit(const size_t limit) noexcept {
	// Захватываем замок карантина
	hold_t hold(this->_lock);
	// Запоминаем потолок объёма карантина
	this->_limit = limit;
}
/**
 * @brief Метод задания засева удерживаемой памяти
 *
 * @param junk признак засева
 *
 */
void awh::alloc::Quarantine::junk(const bool junk) noexcept {
	// Захватываем замок карантина
	hold_t hold(this->_lock);
	// Запоминаем признак засева удерживаемой памяти
	this->_junk = junk;
}
/**
 * @brief Метод получения состояния карантина
 *
 * @return состояние карантина
 *
 */
awh::alloc::Quarantine::state_t awh::alloc::Quarantine::state() noexcept {
	// Захватываем замок карантина
	hold_t hold(this->_lock);
	// Выводим состояние карантина
	return this->_state;
}
/**
 * @brief Метод захвата замка перед ветвлением процесса
 *
 */
void awh::alloc::Quarantine::prepare() noexcept {
	// Захватываем замок карантина
	this->_lock.acquire();
}
/**
 * @brief Метод отпускания замка после ветвления процесса
 *
 */
void awh::alloc::Quarantine::resume() noexcept {
	// Отпускаем замок карантина
	this->_lock.release();
}
/**
 * @brief Метод приведения замка в порядок у потомка ветвления
 *
 */
void awh::alloc::Quarantine::adopt() noexcept {
	// Приводим замок карантина в порядок
	this->_lock.reset();
}
