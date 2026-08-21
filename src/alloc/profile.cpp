/**
 * @file profile.cpp
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
 * @brief Файл учёта мест выдачи памяти
 *
 * \~english
 * @brief Allocation site accounting file
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/profile.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Метка снесённого места таблицы учёта
 *
 */
awh::alloc::Profile::record_t * const awh::alloc::Profile::_tomb = reinterpret_cast <awh::alloc::Profile::record_t *> (static_cast <uintptr_t> (1));

/**
 * @brief Конструктор
 *
 */
awh::alloc::Profile::Profile() noexcept :
 _source(nullptr), _trace(nullptr), _table(nullptr), _length(0), _enrolled(0),
 _meta(nullptr), _metaLeft(0), _spare(nullptr), _rate(0), _counter(0), _live(0) {}

/**
 * @brief Метод выдачи памяти под учётную запись
 *
 * @return адрес выданной памяти либо nullptr
 *
 */
void * awh::alloc::Profile::meta() noexcept {
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
		const size_t span = (256u * 1024u);
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
 * @brief Метод перестроения таблицы учёта
 *
 * @param length требуемая длина таблицы в местах
 * @return       признак перестроения таблицы
 *
 */
bool awh::alloc::Profile::rehash(const size_t length) noexcept {
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
			this->insert(previous[i]);
	}
	// Если прежняя таблица заведена
	if(previous != nullptr)
		// Отдаём источнику память прежней таблицы
		this->_source->release(previous, (before * sizeof(record_t *)));
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод внесения записи в таблицу учёта
 *
 * @param record вносимая запись
 * @return       признак внесения записи
 *
 */
/**
 * Место в таблице берётся маской, а не остатком от деления
 *
 * Длина таблицы всегда степень двойки: начальная такова, а перестроение её удваивает.
 * Деление же с остатком по длине, известной лишь во время работы, стоит десятков тактов,
 * и стоит оно их на КАЖДОМ освобождении - замеры на FreeBSD дали 139 наносекунд на
 * действие с делением против 123 с маской. Кольцо карантина маской НЕ берётся: длина его
 * считается по объёму и степенью двойки не бывает
 */
bool awh::alloc::Profile::insert(record_t * record) noexcept {
	// Если таблица не заведена либо заполнена наполовину
	if((this->_length == 0) || (((this->_enrolled + 1) * 2) > this->_length)){
		// Перестраиваем таблицу вдвое большей длины
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
	while((this->_table[index] != nullptr) && (this->_table[index] != _tomb)){
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
 * @brief Метод поиска места записи в таблице учёта
 *
 * @param block разбираемый адрес блока
 * @return      место записи в таблице либо длина таблицы
 *
 */
size_t awh::alloc::Profile::seek(const void * block) const noexcept {
	// Если таблица не заведена
	if((this->_table == nullptr) || (this->_length == 0))
		// Искать нечем
		return this->_length;
	// Определяем ключ записи: адрес выданного блока
	const uintptr_t key = reinterpret_cast <uintptr_t> (block);
	// Определяем место записи в таблице
	size_t index = static_cast <size_t> ((key * 0x9E3779B97F4A7C15ull) & (this->_length - 1));
	// Число пройденных мест таблицы
	size_t passed = 0;
	/**
	 * Перебираем места подряд, пока не встретим пустое
	 */
	while((this->_table[index] != nullptr) && (passed < this->_length)){
		// Если место занято живой записью нужного блока
		if((this->_table[index] != _tomb) && (this->_table[index]->block == block))
			// Выводим место записи в таблице
			return index;
		// Переходим к следующему месту таблицы
		index = ((index + 1) & (this->_length - 1));
		// Увеличиваем число пройденных мест
		passed++;
	}
	// Записи в таблице нет
	return this->_length;
}
/**
 * @brief Метод заведения учёта мест выдачи
 *
 * @param source источник страниц
 * @param trace  съём стека вызовов
 * @return       признак заведения учёта
 *
 */
bool awh::alloc::Profile::init(source_t * source, trace_t * trace) noexcept {
	// Если источник страниц либо съём стека не заданы
	if((source == nullptr) || (trace == nullptr))
		// Отвечаем отказом
		return false;
	// Запоминаем источник страниц
	this->_source = source;
	// Запоминаем съём стека вызовов
	this->_trace = trace;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод снятия учёта мест выдачи
 *
 */
void awh::alloc::Profile::reset() noexcept {
	// Захватываем замок учёта
	hold_t hold(this->_lock);
	// Если таблица учёта заведена
	if((this->_table != nullptr) && (this->_source != nullptr))
		// Отдаём источнику память таблицы учёта
		this->_source->release(this->_table, (this->_length * sizeof(record_t *)));
	// Сбрасываем таблицу учёта
	this->_table = nullptr;
	// Сбрасываем длину таблицы учёта
	this->_length = 0;
	// Сбрасываем число внесённых записей
	this->_enrolled = 0;
	// Сбрасываем список повторно используемых записей
	this->_spare = nullptr;
	// Сбрасываем число учитываемых живых блоков
	this->_live.store(0, std::memory_order_relaxed);
	// Сбрасываем состояние учёта
	this->_state = state_t();
	/**
	 * Память под учётные записи источнику НЕ отдаётся
	 *
	 * Куски её взяты вразнобой и списком не связаны. Учёт заводится однажды на процесс,
	 * и куски эти теряются лишь при снятии распределителя, то есть перед выходом
	 */
}
/**
 * @brief Метод определения надобности учёта очередной выдаче
 *
 * @return признак надобности учёта
 *
 */
bool awh::alloc::Profile::wanted() noexcept {
	// Запоминаем действующую долю выборки
	const size_t rate = this->_rate.load(std::memory_order_relaxed);
	// Если учёт выключен
	if(rate == 0)
		// Учёт не нужен
		return false;
	// Если выборка берёт каждую выдачу
	if(rate == 1)
		// Учёт нужен
		return true;
	// Увеличиваем счётчик выдач и берём каждую rate-ю из них
	return ((this->_counter.fetch_add(1, std::memory_order_relaxed) % rate) == 0);
}
/**
 * @brief Метод определения ведения учёта хоть каких-то блоков
 *
 * @return признак наличия учитываемых блоков
 *
 */
bool awh::alloc::Profile::tracking() const noexcept {
	// Выводим признак наличия учитываемых блоков
	return (this->_live.load(std::memory_order_relaxed) > 0);
}
/**
 * @brief Метод взятия выданного блока под учёт
 *
 * @param block адрес выданного блока
 * @param size  затребованный размер блока в байтах
 * @param stamp отметка времени выдачи в миллисекундах
 * @param skip  число ближних уровней стека, какие пропустить
 * @return      признак взятия блока под учёт
 *
 */
bool awh::alloc::Profile::enroll(const void * block, const size_t size, const uint64_t stamp, const size_t skip) noexcept {
	// Если учитывать нечего
	if((block == nullptr) || (this->_source == nullptr) || (this->_trace == nullptr))
		// Учитывать нечего
		return false;
	/**
	 * Снимаем стек ДО взятия замка
	 *
	 * Съём у части систем сам обращается за памятью (проверено на OpenBSD: раскрутка
	 * просит восемьдесят восемь байт изредка), а обращение это придёт в наш же
	 * распределитель. Захвати мы замок прежде съёма - пришло бы оно к взятому замку
	 */
	// Стек вызовов места выдачи
	const void * frames[Trace::DEPTH];
	// Снимаем стек вызовов места выдачи
	const size_t depth = this->_trace->capture(frames, Trace::DEPTH, (skip + 1));
	// Захватываем замок учёта
	hold_t hold(this->_lock);
	// Если учитываемых блоков накопилось сверх меры
	if(this->_state.live >= LIMIT){
		// Увеличиваем число блоков, не взятых под учёт
		this->_state.dropped++;
		// Учитывать больше нечего
		return false;
	}
	// Выдаём память под учётную запись
	record_t * record = reinterpret_cast <record_t *> (this->meta());
	// Если память под учётную запись не выдана
	if(record == nullptr){
		// Увеличиваем число блоков, не взятых под учёт
		this->_state.dropped++;
		// Учитывать больше нечего
		return false;
	}
	// Записываем адрес выданного блока
	record->block = block;
	// Записываем затребованный размер блока
	record->size = size;
	// Записываем отметку времени выдачи
	record->stamp = stamp;
	// Записываем глубину снятого стека
	record->depth = depth;
	// Обнуляем связь списка повторно используемых записей
	record->spare = nullptr;
	/**
	 * Переносим снятый стек в запись
	 */
	for(size_t i = 0; i < depth; i++)
		// Переносим адрес уровня стека
		record->frames[i] = frames[i];
	// Вносим запись в таблицу учёта
	if(!this->insert(record)){
		// Возвращаем учётную запись в список повторно используемых
		record->spare = this->_spare;
		// Запоминаем учётную запись повторно используемой
		this->_spare = record;
		// Увеличиваем число блоков, не взятых под учёт
		this->_state.dropped++;
		// Учитывать больше нечего
		return false;
	}
	// Увеличиваем число учитываемых живых блоков
	this->_state.live++;
	// Увеличиваем объём учитываемых живых блоков
	this->_state.bytes += size;
	// Увеличиваем число блоков, взятых под учёт за время работы
	this->_state.enrolled++;
	// Запоминаем число учитываемых живых блоков
	this->_live.store(this->_state.live, std::memory_order_relaxed);
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод снятия блока с учёта
 *
 * @param block адрес освобождаемого блока
 * @return      признак снятия блока с учёта
 *
 */
bool awh::alloc::Profile::expel(const void * block) noexcept {
	// Если снимать нечего
	if(block == nullptr)
		// Снимать нечего
		return false;
	// Захватываем замок учёта
	hold_t hold(this->_lock);
	// Ищем место записи в таблице учёта
	const size_t index = this->seek(block);
	// Если записи в таблице нет
	if(index >= this->_length)
		// Блок под учётом не состоит
		return false;
	// Запоминаем снимаемую с учёта запись
	record_t * record = this->_table[index];
	// Метим место снесённым
	this->_table[index] = _tomb;
	// Уменьшаем число внесённых записей
	this->_enrolled--;
	// Уменьшаем число учитываемых живых блоков
	this->_state.live--;
	// Уменьшаем объём учитываемых живых блоков
	this->_state.bytes -= ((this->_state.bytes < record->size) ? this->_state.bytes : record->size);
	// Запоминаем число учитываемых живых блоков
	this->_live.store(this->_state.live, std::memory_order_relaxed);
	// Возвращаем учётную запись в список повторно используемых
	record->spare = this->_spare;
	// Запоминаем учётную запись повторно используемой
	this->_spare = record;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод перебора удерживаемых прикладным кодом блоков
 *
 * @param callback отклик перебора
 * @param context  предмет, отдаваемый отклику
 * @return         число перебранных блоков
 *
 */
size_t awh::alloc::Profile::walk(walker_t callback, void * context) noexcept {
	// Если перебирать нечем
	if(callback == nullptr)
		// Перебирать нечем
		return 0;
	// Захватываем замок учёта
	hold_t hold(this->_lock);
	// Число перебранных блоков
	size_t result = 0;
	/**
	 * Перебираем места таблицы учёта
	 */
	for(size_t i = 0; i < this->_length; i++){
		// Запоминаем запись места таблицы
		record_t * record = this->_table[i];
		// Если место таблицы свободно
		if((record == nullptr) || (record == _tomb))
			// Переходим к следующему месту
			continue;
		// Сведения об удерживаемом блоке
		holding_t holding;
		// Записываем адрес удерживаемого блока
		holding.block = record->block;
		// Записываем затребованный размер блока
		holding.size = record->size;
		// Записываем отметку времени выдачи
		holding.stamp = record->stamp;
		// Записываем стек вызовов места выдачи
		holding.frames = record->frames;
		// Записываем глубину снятого стека
		holding.depth = record->depth;
		// Увеличиваем число перебранных блоков
		result++;
		// Если отклик прекратил перебор
		if(!(* callback)(holding, context))
			// Перебирать больше нечего
			break;
	}
	// Выводим число перебранных блоков
	return result;
}
/**
 * @brief Метод задания доли выборки
 *
 * @param rate одна выдача из скольких: нуль - учёт выключен
 *
 */
void awh::alloc::Profile::rate(const size_t rate) noexcept {
	// Запоминаем долю выборки
	this->_rate.store(rate, std::memory_order_relaxed);
}
/**
 * @brief Метод получения состояния учёта мест выдачи
 *
 * @return состояние учёта мест выдачи
 *
 */
awh::alloc::Profile::state_t awh::alloc::Profile::state() noexcept {
	// Захватываем замок учёта
	hold_t hold(this->_lock);
	// Выводим состояние учёта мест выдачи
	return this->_state;
}
/**
 * @brief Метод захвата замка перед ветвлением процесса
 *
 */
void awh::alloc::Profile::prepare() noexcept {
	// Захватываем замок учёта
	this->_lock.acquire();
}
/**
 * @brief Метод отпускания замка после ветвления процесса
 *
 */
void awh::alloc::Profile::resume() noexcept {
	// Отпускаем замок учёта
	this->_lock.release();
}
/**
 * @brief Метод приведения замка в порядок у потомка ветвления
 *
 */
void awh::alloc::Profile::adopt() noexcept {
	// Приводим замок учёта в порядок
	this->_lock.reset();
}
