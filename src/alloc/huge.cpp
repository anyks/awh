/**
 * @file huge.cpp
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
 * @brief Файл слоя крупных выдач
 *
 * \~english
 * @brief Huge allocation layer file
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/huge.hpp>
#include <alloc/pages.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Метка снесённого места таблицы поиска
 *
 */
awh::alloc::Huge::record_t * const awh::alloc::Huge::_tomb = reinterpret_cast <awh::alloc::Huge::record_t *> (static_cast <uintptr_t> (1));

/**
 * @brief Конструктор
 *
 */
awh::alloc::Huge::Huge() noexcept :
 _source(nullptr), _table(nullptr), _length(0), _enrolled(0),
 _meta(nullptr), _metaLeft(0), _spare(nullptr) {}

/**
 * @brief Метод выдачи памяти под учётную запись
 *
 * @return адрес выданной памяти либо nullptr
 *
 */
void * awh::alloc::Huge::meta() noexcept {
	// Если есть повторно используемая учётная запись
	if(this->_spare != nullptr){
		// Запоминаем повторно используемую учётную запись
		record_t * result = this->_spare;
		// Сдвигаем список повторно используемых записей
		this->_spare = result->spare;
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
bool awh::alloc::Huge::rehash(const size_t length) noexcept {
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
 * @note Место в таблице берётся маской: длина её всегда степень двойки - начальная
 *       такова, а перестроение её удваивает
 *
 * @param record вносимая запись
 * @return       признак внесения записи
 *
 */
bool awh::alloc::Huge::enroll(record_t * record) noexcept {
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
	size_t index = static_cast <size_t> ((key * 0x9E3779B97F4A7C15ull) & (this->_length - 1));
	/**
	 * Ищем свободное место, перебирая места подряд
	 */
	while((this->_table[index] != nullptr) && (this->_table[index] != this->_tomb)){
		// Если запись в таблице уже есть
		if(this->_table[index]->block == record->block)
			// Вносить нечего
			return true;
		// Переходим к следующему месту таблицы
		index = ((index + 1) & (this->_length - 1));
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
awh::alloc::Huge::record_t * awh::alloc::Huge::lookup(const void * addr, const bool exact) const noexcept {
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
		size_t index = static_cast <size_t> ((key * 0x9E3779B97F4A7C15ull) & (this->_length - 1));
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
			index = ((index + 1) & (this->_length - 1));
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
	 * адреса, то есть по требованию прикладного кода, а крупных выдач немного
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
 * @brief Метод заведения слоя крупных выдач
 *
 * @param source источник страниц
 * @return       признак заведения слоя
 *
 */
bool awh::alloc::Huge::init(source_t * source) noexcept {
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
 * @brief Метод снятия слоя крупных выдач
 *
 */
void awh::alloc::Huge::reset() noexcept {
	// Захватываем замок слоя
	hold_t hold(this->_lock);
	// Если источник страниц не задан
	if(this->_source == nullptr)
		// Снимать нечего
		return;
	/**
	 * Отдаём источнику области крупных выдач
	 */
	for(size_t i = 0; i < this->_length; i++){
		// Запоминаем запись места таблицы
		record_t * record = this->_table[i];
		// Если место таблицы свободно
		if((record == nullptr) || (record == this->_tomb))
			// Переходим к следующему месту
			continue;
		// Отдаём источнику область крупной выдачи
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
	// Сбрасываем список повторно используемых записей
	this->_spare = nullptr;
	// Сбрасываем состояние слоя
	this->_state = state_t();
	/**
	 * Память под учётные записи источнику НЕ отдаётся
	 *
	 * Куски её взяты вразнобой и списком не связаны: связь их лежала бы в первых
	 * байтах куска, а те уже отданы под записи. Слой заводится однажды на процесс, и
	 * куски эти теряются лишь при снятии распределителя, то есть перед выходом
	 */
}
/**
 * @brief Метод определения обслуживаемости размера слоем
 *
 * @param size требуемый размер в байтах
 * @return     признак обслуживаемости размера слоем
 *
 */
bool awh::alloc::Huge::wanted(const size_t size) noexcept {
	// Если размер не задан
	if(size == 0)
		// Слою здесь делать нечего
		return false;
	/**
	 * Сверяем размер на переполнение при приведении к границе страницы
	 *
	 * Счёт страниц у размера близ предела переполняется в нуль, и проверка «страниц
	 * больше, чем в куске» отвечает ложью на САМЫЙ крупный запрос: тот уходил бы к
	 * страничной куче, где счёт этот переполняется ровно так же. Такие размеры слой
	 * берёт на себя и отвечает по ним отказом сам
	 */
	if(size > (static_cast <size_t> (-1) - (awh::alloc::Pages::PAGE - 1)))
		// Запрос обслуживается слоем
		return true;
	// Определяем требуемое число страниц кучи
	const size_t pages = ((size + (awh::alloc::Pages::PAGE - 1)) / awh::alloc::Pages::PAGE);
	// Выводим признак непомещаемости запроса в кусок страничной кучи
	return (pages > awh::alloc::Pages::PAGES);
}
/**
 * @brief Метод крупной выдачи памяти
 *
 * @param size      требуемый размер в байтах
 * @param alignment требуемое выравнивание в байтах, нуль - страничное
 * @return          адрес выданной памяти либо nullptr
 *
 */
void * awh::alloc::Huge::alloc(const size_t size, const size_t alignment) noexcept {
	// Если источник страниц не задан либо размер не задан
	if((this->_source == nullptr) || (size == 0))
		// Выдавать нечего
		return nullptr;
	/**
	 * Сверяем запрос на переполнение при приведении к границе страницы
	 *
	 * Запрос вида (size_t)-1 приходит и от прикладного кода, и от переполнения счёта
	 * длины у стандартной библиотеки. Приведи мы его к границе страницы без сверки -
	 * получили бы нуль, выдача удалась бы, а запись по ней ушла бы в никуда
	 */
	if(size > (static_cast <size_t> (-1) - (awh::alloc::Pages::PAGE - 1)))
		// Отвечаем отказом
		return nullptr;
	// Действительно выданный размер
	size_t actual = 0;
	/**
	 * Захватываем замок слоя ПРЕЖДЕ обращения к источнику
	 *
	 * Прежде замок брался после, и между выдачей у источника и заведением записи
	 * область не значилась НИГДЕ, кроме местной переменной. Ветвление в этот миг
	 * оставляло потомку отображение без записи: освободить его там некому - поток,
	 * который завёл бы запись, остался у родителя, - и область текла безвозвратно
	 *
	 * Цена невелика: обращение к источнику это `mmap`, а его ядро и так проводит под
	 * своим замком по всему пространству процесса; крупные же выдачи по своей природе
	 * редки. Зато отклик ветвления, забирающий этот замок, теперь и вправду ограждает
	 * действие целиком
	 */
	hold_t hold(this->_lock);
	// Берём у источника область под крупную выдачу
	uint8_t * base = reinterpret_cast <uint8_t *> (this->_source->alloc(size, alignment, actual));
	// Если область не выдана
	if(base == nullptr)
		// Выдавать нечего
		return nullptr;
	// Выдаём память под учётную запись
	record_t * record = reinterpret_cast <record_t *> (this->meta());
	// Если память под учётную запись не выдана
	if(record == nullptr){
		// Отдаём источнику взятую область
		this->_source->release(base, actual);
		// Выдавать нечего
		return nullptr;
	}
	// Запоминаем адрес выданного блока
	record->block = base;
	// Запоминаем адрес начала взятой области
	record->base = base;
	// Запоминаем размер взятой области
	record->span = actual;
	// Запоминаем затребованный размер
	record->size = size;
	// Сбрасываем связь повторного использования
	record->spare = nullptr;
	// Если запись в таблицу поиска не внесена
	if(!this->enroll(record)){
		// Возвращаем запись в список повторно используемых
		record->spare = this->_spare;
		// Запоминаем список повторно используемых записей
		this->_spare = record;
		// Отдаём источнику взятую область
		this->_source->release(base, actual);
		// Выдавать нечего
		return nullptr;
	}
	// Увеличиваем число живых крупных выдач
	this->_state.live++;
	// Увеличиваем взятое у источника
	this->_state.taken += actual;
	// Увеличиваем выданное прикладному коду
	this->_state.given += size;
	// Выводим выданный блок
	return base;
}
/**
 * @brief Метод освобождения крупной выдачи
 *
 * @param ptr адрес освобождаемой памяти
 * @return    затребованный прикладным кодом размер блока либо нуль
 *
 */
size_t awh::alloc::Huge::free(void * ptr) noexcept {
	// Если освобождать нечего
	if((ptr == nullptr) || (this->_source == nullptr))
		// Освобождать нечего
		return 0;
	// Адрес начала отдаваемой области
	uint8_t * base = nullptr;
	// Размер отдаваемой области в байтах
	size_t span = 0;
	// Затребованный прикладным кодом размер блока
	size_t size = 0;
	/**
	 * Снимаем запись с учёта под замком, а область отдаём после
	 */
	{
		// Захватываем замок слоя
		hold_t hold(this->_lock);
		// Если таблица поиска не заведена
		if((this->_table == nullptr) || (this->_length == 0))
			// Освобождать нечего
			return 0;
		// Определяем ключ записи: адрес освобождаемого блока
		const uintptr_t key = reinterpret_cast <uintptr_t> (ptr);
		// Определяем место записи в таблице
		size_t index = static_cast <size_t> ((key * 0x9E3779B97F4A7C15ull) & (this->_length - 1));
		// Число пройденных мест таблицы
		size_t passed = 0;
		// Найденная запись крупной выдачи
		record_t * record = nullptr;
		/**
		 * Перебираем места подряд, пока не встретим пустое
		 */
		while((this->_table[index] != nullptr) && (passed < this->_length)){
			// Если место занято живой записью нужного блока
			if((this->_table[index] != this->_tomb) && (this->_table[index]->block == reinterpret_cast <uint8_t *> (ptr))){
				// Запоминаем найденную запись
				record = this->_table[index];
				/**
				 * Оставляем в месте таблицы метку сноса, а не пустоту
				 *
				 * Перебор идёт подряд и останавливается на пустом месте: обнули мы место
				 * снесённой записи - соседние записи, легшие за ним при столкновении
				 * ключей, стали бы ненаходимыми
				 */
				this->_table[index] = this->_tomb;
				// Уменьшаем число внесённых записей
				this->_enrolled--;
				// Перебирать больше нечего
				break;
			}
			// Переходим к следующему месту таблицы
			index = ((index + 1) & (this->_length - 1));
			// Увеличиваем число пройденных мест
			passed++;
		}
		// Если запись не найдена
		if(record == nullptr)
			// Блок не наш
			return 0;
		// Запоминаем адрес начала отдаваемой области
		base = record->base;
		// Запоминаем размер отдаваемой области
		span = record->span;
		// Запоминаем затребованный прикладным кодом размер
		size = record->size;
		// Уменьшаем число живых крупных выдач
		this->_state.live--;
		// Уменьшаем взятое у источника
		this->_state.taken -= ((this->_state.taken < span) ? this->_state.taken : span);
		// Уменьшаем выданное прикладному коду
		this->_state.given -= ((this->_state.given < size) ? this->_state.given : size);
		// Возвращаем запись в список повторно используемых
		record->spare = this->_spare;
		// Запоминаем список повторно используемых записей
		this->_spare = record;
	}
	/**
	 * Отдаём область системе вне замка
	 *
	 * Отдача крупной области стоит вызова системы и правки таблиц страниц: держи мы на
	 * ней замок слоя - соседние потоки ждали бы работы ядра, к их выдаче отношения не
	 * имеющей
	 */
	this->_source->release(base, span);
	// Выводим затребованный прикладным кодом размер блока
	return size;
}
/**
 * @brief Метод опознания крупной выдачи по адресу её начала
 *
 * @param ptr  разбираемый адрес
 * @param size размер блока в байтах
 * @return     признак принадлежности адреса слою
 *
 */
bool awh::alloc::Huge::owner(const void * ptr, size_t * size) const noexcept {
	// Если разбирать нечего
	if(ptr == nullptr)
		// Разбирать нечего
		return false;
	// Захватываем замок слоя
	hold_t hold(const_cast <spin_t &> (this->_lock));
	// Ищем запись ровно по началу блока
	record_t * record = this->lookup(ptr, true);
	// Если запись не найдена
	if(record == nullptr)
		// Блок не наш
		return false;
	// Если требуется размер блока
	if(size != nullptr)
		// Записываем затребованный прикладным кодом размер
		(* size) = record->size;
	// Блок наш
	return true;
}
/**
 * @brief Метод разбора адреса, лежащего внутри крупной выдачи
 *
 * @param addr   разбираемый адрес
 * @param begin  адрес начала блока
 * @param size   размер блока в байтах
 * @param offset смещение разбираемого адреса от начала блока
 * @return       признак принадлежности адреса слою
 *
 */
bool awh::alloc::Huge::resolve(const void * addr, const void ** begin, size_t * size, ptrdiff_t * offset) const noexcept {
	// Если разбирать нечего
	if(addr == nullptr)
		// Разбирать нечего
		return false;
	// Захватываем замок слоя
	hold_t hold(const_cast <spin_t &> (this->_lock));
	// Ищем запись, чья область накрывает разбираемый адрес
	record_t * record = this->lookup(addr, false);
	// Если запись не найдена
	if(record == nullptr)
		// Адрес не наш
		return false;
	// Если требуется адрес начала блока
	if(begin != nullptr)
		// Записываем адрес начала блока
		(* begin) = record->block;
	// Если требуется размер блока
	if(size != nullptr)
		// Записываем затребованный прикладным кодом размер
		(* size) = record->size;
	// Если требуется смещение от начала блока
	if(offset != nullptr)
		// Записываем смещение разбираемого адреса от начала блока
		(* offset) = (reinterpret_cast <const uint8_t *> (addr) - record->block);
	// Адрес наш
	return true;
}
/**
 * @brief Метод получения состояния слоя крупных выдач
 *
 * @return состояние слоя крупных выдач
 *
 */
awh::alloc::Huge::state_t awh::alloc::Huge::state() noexcept {
	// Захватываем замок слоя
	hold_t hold(this->_lock);
	// Выводим состояние слоя крупных выдач
	return this->_state;
}
/**
 * @brief Метод подготовки слоя к ветвлению процесса
 *
 */
void awh::alloc::Huge::prepare() noexcept {
	// Захватываем замок слоя
	this->_lock.acquire();
}
/**
 * @brief Метод возобновления работы слоя после ветвления
 *
 */
void awh::alloc::Huge::resume() noexcept {
	// Отпускаем замок слоя
	this->_lock.release();
}
/**
 * @brief Метод приведения слоя в порядок в потомке ветвления
 *
 */
void awh::alloc::Huge::adopt() noexcept {
	// Приводим замок слоя в порядок
	this->_lock.reset();
}
