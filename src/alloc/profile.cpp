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
 _source(nullptr), _trace(nullptr), _table(nullptr), _length(0), _region(0), _enrolled(0),
 _buried(0), _meta(nullptr), _metaLeft(0), _metaChunks(nullptr), _spare(nullptr),
 _rate(0), _counter(0), _live(0) {}

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
		/**
		 * Вносим кусок в общий список
		 *
		 * Первые байты куска отводятся под заголовок, а записи режутся ЗА ним, - оттого
		 * списку не нужно собственной памяти. Прежде здесь стояло, что связать куски
		 * нельзя, но слой крупных выдач ведёт свой такой же список ровно этим приёмом
		 */
		/**
		 * В заголовке куска хранится и его РАЗМЕР, а не только связь
		 *
		 * Источник вправе выдать больше запрошенного, округлив запрос по своему зерну,
		 * и размер этот у кусков волен разниться. Отдавать их постоянной величиной
		 * значило бы возвращать системе не то, что у неё взято
		 */
		// Размер заголовка куска: связь и размер
		const size_t header = (sizeof(void *) + sizeof(size_t));
		// Записываем в начало куска указатель на предыдущий
		::memcpy(block, &this->_metaChunks, sizeof(void *));
		// Записываем следом размер выданного куска
		::memcpy((block + sizeof(void *)), &actual, sizeof(size_t));
		// Запоминаем кусок как последний
		this->_metaChunks = block;
		// Сдвигаем начало выдачи за заголовок
		this->_meta = (block + header);
		// Запоминаем остаток куска
		this->_metaLeft = (actual - header);
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
	/**
	 * Запоминаем размер прежней области, а не считаем его по длине таблицы
	 *
	 * Источник округляет запрос по своему зерну и вправе выдать больше запрошенного.
	 * Отдай мы область запрошенным размером - счёт взятого у системы не сошёлся бы на
	 * разницу, и заказанный приложением потолок закрыл бы выдачу тем раньше, чем
	 * дольше работа
	 */
	const size_t region = this->_region;
	// Подменяем таблицу новой
	this->_table = table;
	// Запоминаем длину новой таблицы
	this->_length = length;
	// Запоминаем размер взятой под новую таблицу области
	this->_region = actual;
	// Внесённых записей у новой таблицы пока нет
	this->_enrolled = 0;
	// Снесённых мест у новой таблицы тоже нет: перестроение их и убирает
	this->_buried = 0;
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
		this->_source->release(previous, region);
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
	/**
	 * Считаем занятыми и снесённые места, а не одни живые записи
	 *
	 * Место снесённой записи пустым не становится: поиск идёт подряд до первого
	 * ПУСТОГО места, и надгробие его не останавливает. Длина же при перестроении
	 * выбирается по живым записям: забита таблица надгробиями - перестроение той же
	 * длины их и уберёт, расти ей положено лишь от тесноты живых
	 */
	// Если таблица не заведена либо занята наполовину
	if((this->_length == 0) || (((this->_enrolled + this->_buried + 1) * 2) > this->_length)){
		// Выбираем длину перестраиваемой таблицы
		const size_t length = ((this->_length == 0) ? TABLE :
		 ((((this->_enrolled + 1) * 2) > this->_length) ? (this->_length * 2) : this->_length));
		// Перестраиваем таблицу выбранной длины
		if(!this->rehash(length))
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
	/**
	 * Перебор мест ОГРАНИЧЕН длиною таблицы
	 *
	 * Порог занятости держит таблицу заполненной не более чем наполовину, и свободное
	 * место находится всегда - пока счётчики живых записей и надгробий верны. Сойдись
	 * они с делом хоть однажды, и перебор без границы завис бы навсегда, а зависание от
	 * отказа наружу неотличимо. Поиск записи в этом же файле границу имеет; внесение её
	 * не имело, и подмена это доказала: таблице велели не расти, и набор проверок ПОВИС
	 * вместо того, чтобы упасть. Граница обращает зависание в честный отказ, какой
	 * звавший уже умеет разбирать, и стоит она одного сличения на холодном пути
	 */
	// Число пройденных мест таблицы
	size_t passed = 0;
	while((this->_table[index] != nullptr) && (this->_table[index] != _tomb)){
		// Если запись в таблице уже есть
		if(this->_table[index]->block == record->block)
			// Вносить нечего
			return true;
		// Переходим к следующему месту таблицы
		index = ((index + 1) & (this->_length - 1));
		// Если пройдена вся таблица
		if((++passed) >= this->_length)
			// Отвечаем отказом: свободного места нет
			return false;
	}
	// Если найденное место помечено снесённым
	if(this->_table[index] == _tomb)
		// Уменьшаем число снесённых мест: надгробие занято живой записью
		this->_buried--;
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
		this->_source->release(this->_table, this->_region);
	// Сбрасываем таблицу учёта
	this->_table = nullptr;
	// Сбрасываем длину таблицы учёта
	this->_length = 0;
	// Сбрасываем размер взятой под таблицу учёта области
	this->_region = 0;
	// Сбрасываем число внесённых записей
	this->_enrolled = 0;
	// Сбрасываем число снесённых мест
	this->_buried = 0;
	// Сбрасываем список повторно используемых записей
	this->_spare = nullptr;
	// Сбрасываем число учитываемых живых блоков
	this->_live.store(0, std::memory_order_relaxed);
	// Сбрасываем состояние учёта
	this->_state = state_t();
	/**
	 * Отдаём источнику куски памяти под учётные записи
	 *
	 * Отдаются ПОСЛЕДНИМИ: учётные записи блоков лежат в них же, и отдать их прежде
	 * значило бы читать освобождённую память при работе выше
	 */
	for(void * block = ((this->_source != nullptr) ? this->_metaChunks : nullptr); block != nullptr;){
		// Читаем указатель на предыдущий кусок из его начала
		void * previous = nullptr;
		// Извлекаем указатель на предыдущий кусок
		::memcpy(&previous, block, sizeof(void *));
		// Размер отдаваемого куска
		size_t span = 0;
		// Извлекаем размер куска из его заголовка
		::memcpy(&span, (reinterpret_cast <uint8_t *> (block) + sizeof(void *)), sizeof(size_t));
		// Отдаём кусок источнику
		this->_source->release(block, span);
		// Переходим к предыдущему куску
		block = previous;
	}
	// Сбрасываем общий список кусков памяти под учётные записи
	this->_metaChunks = nullptr;
	// Сбрасываем текущий кусок памяти под учётные записи
	this->_meta = nullptr;
	// Сбрасываем остаток текущего куска
	this->_metaLeft = 0;
}
/**
 * @brief Метод определения надобности учёта очередной выдаче
 *
 * @return признак надобности учёта
 *
 */
bool awh::alloc::Profile::sampled() noexcept {
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
 * @brief Метод правки размера блока, состоящего под учётом
 *
 * @param block адрес блока
 * @param size  новый размер блока в байтах
 * @return      признак состоявшейся правки
 *
 */
bool awh::alloc::Profile::amend(const void * block, const size_t size) noexcept {
	// Если править нечего
	if((block == nullptr) || (size == 0))
		// Править нечего
		return false;
	// Захватываем замок учёта
	hold_t hold(this->_lock);
	// Ищем место записи в таблице учёта
	const size_t index = this->seek(block);
	// Если записи в таблице нет
	if(index >= this->_length)
		// Блок под учётом не состоит
		return false;
	// Запоминаем правимую запись
	record_t * record = this->_table[index];
	// Уменьшаем объём учитываемых живых блоков на прежний размер
	this->_state.bytes -= ((this->_state.bytes < record->size) ? this->_state.bytes : record->size);
	// Записываем новый размер блока
	record->size = size;
	// Увеличиваем объём учитываемых живых блоков на новый размер
	this->_state.bytes += size;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод снятия блока с учёта
 *
 * @param block адрес блока
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
	// Увеличиваем число снесённых мест
	this->_buried++;
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
