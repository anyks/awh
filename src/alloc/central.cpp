/**
 * @file central.cpp
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
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/link.hpp>
#include <alloc/central.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <thread>
#include <chrono>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	/**
	 * @brief Метод чтения указателя на следующий свободный блок
	 *
	 * @param block читаемый свободный блок
	 * @return      следующий свободный блок
	 *
	 */
	static void * following(void * block) noexcept {
		// Выводим следующий свободный блок, разбирая перемешивание
		return awh::alloc::Link::next(block);
	}
	/**
	 * @brief Метод записи указателя на следующий свободный блок
	 *
	 * @param block      изменяемый свободный блок
	 * @param subsequent записываемый следующий свободный блок
	 *
	 */
	static void following(void * block, void * subsequent) noexcept {
		// Записываем указатель перемешанным
		awh::alloc::Link::next(block, subsequent);
	}
};

/**
 * @brief Конструктор
 *
 */
awh::alloc::Central::Central() noexcept : _pages(nullptr), _classes(nullptr), _heap() {}
/**
 * @brief Метод заведения центральных списков
 *
 * @param pages   страничная куча
 * @param classes разряды размеров
 * @return        признак заведения списков
 *
 */
bool awh::alloc::Central::init(pages_t * pages, classes_t * classes) noexcept {
	// Если куча либо разряды не заданы
	if((pages == nullptr) || (classes == nullptr))
		// Отвечаем отказом
		return false;
	// Если разряды не построены
	if(classes->count() == 0)
		// Отвечаем отказом
		return false;
	// Сеем зерно перемешивания указателей прежде, чем связан первый блок
	link_t::seed();
	// Запоминаем страничную кучу
	this->_pages = pages;
	// Запоминаем разряды размеров
	this->_classes = classes;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод сброса центральных списков
 *
 */
void awh::alloc::Central::reset() noexcept {
	/**
	 * Перебираем разряды
	 */
	for(size_t i = 0; i < Classes::LIMIT; i++){
		// Захватываем замок разряда
		this->_lists[i].lock.acquire();
		// Обнуляем голову списка свободных блоков
		this->_lists[i].free = nullptr;
		// Обнуляем число свободных блоков
		this->_lists[i].count = 0;
		// Обнуляем число выданных кэшам блоков
		this->_lists[i].live = 0;
		// Обнуляем число нарезанных областей
		this->_lists[i].regions = 0;
		// Освобождаем замок разряда
		this->_lists[i].lock.release();
	}
	// Обнуляем разряды размеров
	this->_classes = nullptr;
	// Обнуляем страничную кучу
	this->_pages = nullptr;
}
/**
 * @brief Метод нарезки новой области под разряд
 *
 * @param index номер разряда
 * @return      признак нарезки области
 *
 */
bool awh::alloc::Central::carve(const size_t index) noexcept {
	// Получаем размер блока разряда
	const size_t size = this->_classes->size(index);
	// Получаем число страниц области разряда
	const size_t pages = this->_classes->pages(index);
	// Получаем число блоков в области разряда
	const size_t blocks = this->_classes->blocks(index);
	// Если разряд не заведён
	if((size == 0) || (pages == 0) || (blocks == 0))
		// Нарезать нечего
		return false;
	// Адрес нарезаемой области
	uint8_t * base = nullptr;
	/**
	 * Берём у кучи новую область
	 */
	{
		/**
		 * Берём у кучи область ВНЕ замка
		 *
		 * Взятие уступает время тому, кто отдаёт память системе: держи мы замок кучи -
		 * ждали бы того, чему сами мешаем
		 */
		base = reinterpret_cast <uint8_t *> (this->take(pages));
		// Если область не выдана
		if(base == nullptr)
			// Отвечаем отказом
			return false;
		// Захватываем замок кучи под разметку области
		hold_t hold(this->_heap);
		/**
		 * Помечаем область разрядом, которому она отдана
		 *
		 * Метка на единицу больше номера разряда: нуль означает память, выданную сверх
		 * разрядов, и отличить его от нулевого разряда иначе было бы нечем
		 */
		if(!this->_pages->tag(base, static_cast <uint32_t> (index + 1))){
			// Возвращаем куче область: без метки её не опознать при освобождении
			this->_pages->free(base, 0);
			// Отвечаем отказом
			return false;
		}
	}
	/**
	 * Нарезаем область на блоки и связываем их в цепочку
	 *
	 * Связываем от конца к началу: так голова цепочки приходится на начало области,
	 * а выдача блоков подряд идёт по возрастанию адреса и щадит упреждающее чтение
	 */
	for(size_t i = 0; i < blocks; i++){
		// Определяем адрес очередного блока
		uint8_t * block = (base + (i * size));
		// Связываем блок со следующим, а последний - с прежней головой списка
		::following(block, ((i + 1) < blocks) ? (base + ((i + 1) * size)) : this->_lists[index].free);
	}
	// Головой списка становится первый блок нарезанной области
	this->_lists[index].free = base;
	// Увеличиваем число свободных блоков
	this->_lists[index].count += blocks;
	// Увеличиваем число нарезанных областей
	this->_lists[index].regions++;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод изъятия пачки блоков разряда
 *
 * @param index номер разряда
 * @param head  голова изъятой цепочки блоков
 * @param tail  хвост изъятой цепочки блоков
 * @param count требуемое число блоков
 * @return      действительно изъятое число блоков
 *
 */
size_t awh::alloc::Central::fetch(const size_t index, void ** head, void ** tail, const size_t count) noexcept {
	// Если списки не заведены либо разряд неведом
	if((this->_classes == nullptr) || (index >= this->_classes->count()))
		// Изымать нечего
		return 0;
	// Если требуемое число блоков не задано либо выводить некуда
	if((count == 0) || (head == nullptr) || (tail == nullptr))
		// Изымать нечего
		return 0;
	// Обнуляем голову изымаемой цепочки
	(* head) = nullptr;
	// Обнуляем хвост изымаемой цепочки
	(* tail) = nullptr;
	// Захватываем замок разряда
	hold_t hold(this->_lists[index].lock);
	// Если свободных блоков не осталось
	if(this->_lists[index].free == nullptr){
		// Нарезаем новую область под разряд
		if(!this->carve(index))
			// Изымать нечего
			return 0;
	}
	// Число действительно изъятых блоков
	size_t result = 0;
	// Голова изымаемой цепочки
	void * first = this->_lists[index].free;
	// Хвост изымаемой цепочки
	void * last = first;
	/**
	 * Снимаем со списка требуемое число блоков
	 */
	while((result < count) && (last != nullptr)){
		// Увеличиваем число изъятых блоков
		result++;
		// Если требуемое число блоков набрано
		if(result == count)
			// Прекращаем набор: хвостом остаётся текущий блок
			break;
		// Получаем следующий свободный блок
		void * subsequent = ::following(last);
		// Если следующего блока нет
		if(subsequent == nullptr)
			// Прекращаем набор: список исчерпан
			break;
		// Хвостом становится следующий блок
		last = subsequent;
	}
	// Головой списка становится блок, следующий за хвостом изъятой цепочки
	this->_lists[index].free = ::following(last);
	// Обрываем цепочку у хвоста
	::following(last, nullptr);
	// Уменьшаем число свободных блоков
	this->_lists[index].count -= result;
	// Увеличиваем число выданных кэшам блоков
	this->_lists[index].live += result;
	// Выводим голову изъятой цепочки
	(* head) = first;
	// Выводим хвост изъятой цепочки
	(* tail) = last;
	// Выводим число изъятых блоков
	return result;
}
/**
 * @brief Метод возврата пачки блоков разряда
 *
 * @param index номер разряда
 * @param head  голова возвращаемой цепочки блоков
 * @param tail  хвост возвращаемой цепочки блоков
 * @param count число возвращаемых блоков
 *
 */
void awh::alloc::Central::back(const size_t index, void * head, void * tail, const size_t count) noexcept {
	// Если списки не заведены либо разряд неведом
	if((this->_classes == nullptr) || (index >= this->_classes->count()))
		// Возвращать некуда
		return;
	// Если возвращаемая цепочка пуста
	if((head == nullptr) || (tail == nullptr) || (count == 0))
		// Возвращать нечего
		return;
	// Захватываем замок разряда
	hold_t hold(this->_lists[index].lock);
	// Связываем хвост возвращаемой цепочки с прежней головой списка
	::following(tail, this->_lists[index].free);
	// Головой списка становится голова возвращаемой цепочки
	this->_lists[index].free = head;
	// Увеличиваем число свободных блоков
	this->_lists[index].count += count;
	// Уменьшаем число выданных кэшам блоков
	this->_lists[index].live -= ((this->_lists[index].live < count) ? this->_lists[index].live : count);
}
/**
 * @brief Метод выдачи памяти сверх разрядов
 *
 * @note Размер, не помещающийся в кусок кучи, здесь не обслуживается: такие запросы
 *       обращаются к источнику страниц напрямую слоем выше
 *
 * @param size требуемый размер в байтах
 * @return     адрес выданной памяти либо nullptr
 *
 */
void * awh::alloc::Central::take(const size_t pages) noexcept {
	/**
	 * Пробуем взять область, пока есть изымаемые
	 *
	 * Ждём СРОКОМ, а не числом уступок. Уступка стоит по-разному: у MS Windows она
	 * почти ничего не стоит, когда есть кому работать, и шесть десятков уступок там
	 * проходят за микросекунды - быстрее, чем длится обращение к системе. Замерено:
	 * семьсот четыре уступки и одиннадцать сдач подряд, обернувшихся отказами выдачи на
	 * ровном месте. Первые заходы уступают, дальнейшие спят понемногу: так ожидание не
	 * зависит от того, чего стоит уступка у этой системы
	 */
	// Число заходов, отводимых уступке времени
	static constexpr size_t SPINS = 64;
	// Наибольшее время ожидания изымаемых областей
	static constexpr auto PATIENCE = std::chrono::milliseconds(50);
	/**
	 * Часы спрашиваем ЛЕНИВО - лишь когда первый заход не удался
	 *
	 * Обращение к часам стоит заметно: замер пути выдачи сверх разрядов показал около
	 * семи сотых времени на `clock_gettime` через vDSO, и половина того приходилась
	 * сюда. Удачный заход - а он и есть обычный - часов теперь не спрашивает вовсе
	 */
	// Отметка начала ожидания, берётся при первой неудаче
	std::chrono::steady_clock::time_point began;
	// Признак взятой отметки
	bool timing = false;
	// Число сделанных заходов
	size_t attempt = 0;
	/**
	 * Пробуем, пока не выйдет срок
	 */
	while(true){
		{
			// Захватываем замок кучи
			hold_t hold(this->_heap);
			// Берём у кучи область требуемого размера
			void * result = this->_pages->alloc(pages);
			// Если область выдана
			if(result != nullptr)
				// Выводим выданную область
				return result;
			// Если изымаемых областей нет
			if(this->_pages->pending() == 0)
				// Памяти и вправду нет
				return nullptr;
		}
		/**
		 * Уступаем время тому, кто отдаёт
		 *
		 * Уступаем ВНЕ замка кучи: обход отдачи возвращает изъятые области под тем же
		 * замком, и держи мы его - ждали бы того, чему сами мешаем
		 */
		if(attempt++ < SPINS)
			// Уступаем время тому, кто отдаёт
			std::this_thread::yield();
		// Спим понемногу: уступка у этой системы, видимо, ничего не стоит
		else std::this_thread::sleep_for(std::chrono::microseconds(200));
		// Если отметка начала ожидания ещё не взята
		if(!timing){
			// Запоминаем начало ожидания
			began = std::chrono::steady_clock::now();
			// Отмечаем отметку взятой
			timing = true;
		// Если срок ожидания вышел
		} else if((std::chrono::steady_clock::now() - began) >= PATIENCE)
			// Выходим: изымаемых областей мы не дождались
			break;
	}
	// Выдавать нечего
	return nullptr;
}
/**
 * @brief Метод выдачи памяти сверх разрядов
 *
 * @param size требуемый размер в байтах
 * @return     адрес выданной памяти либо nullptr
 *
 */
void * awh::alloc::Central::alloc(const size_t size) noexcept {
	// Если куча не заведена либо размер не задан
	if((this->_pages == nullptr) || (size == 0))
		// Выдавать нечего
		return nullptr;
	/**
	 * Сверяем размер на переполнение при приведении к границе страницы
	 *
	 * Счёт страниц у размера близ предела переполняется в нуль, и сверка с куском
	 * пропустила бы САМЫЙ крупный запрос вглубь кучи - той нашлась бы нулевая область
	 */
	if(size > (static_cast <size_t> (-1) - (Pages::PAGE - 1)))
		// Отвечаем отказом
		return nullptr;
	// Определяем требуемое число страниц кучи
	const size_t pages = ((size + (Pages::PAGE - 1)) / Pages::PAGE);
	// Если требуемое число страниц не помещается в кусок кучи
	if(pages > Pages::PAGES)
		// Отвечаем отказом
		return nullptr;
	// Выводим выданную кучей область
	return this->take(pages);
}
/**
 * @brief Метод возврата памяти, выданной сверх разрядов
 *
 * @param addr адрес возвращаемой памяти
 * @param now  текущее время в миллисекундах
 * @return     признак возврата памяти
 *
 */
bool awh::alloc::Central::free(void * addr, const uint64_t now) noexcept {
	// Если куча не заведена либо адрес не задан
	if((this->_pages == nullptr) || (addr == nullptr))
		// Возвращать нечего
		return false;
	// Захватываем замок кучи
	hold_t hold(this->_heap);
	// Выводим признак возврата области куче
	return this->_pages->free(addr, now);
}
/**
 * @brief Метод определения разряда, которому принадлежит адрес
 *
 * @note Замок кучи захватывается и на чтении: таблица поиска куска перестраивается
 *       при её росте, и читающий без замка увидел бы её недостроенной
 *
 * @param addr  разбираемый адрес
 * @param index номер разряда, либо LIMIT если память выдана сверх разрядов
 * @param begin адрес начала области, которой принадлежит адрес
 * @param size  размер области в байтах
 * @return      признак того, что адрес выдан нами
 *
 */
bool awh::alloc::Central::owner(const void * addr, size_t * index, void ** begin, size_t * size, void ** hint) noexcept {
	// Если куча не заведена либо адрес не задан
	if((this->_pages == nullptr) || (addr == nullptr))
		// Разбирать нечего
		return false;
	// Адрес начала области
	void * base = nullptr;
	// Размер области в страницах кучи
	size_t pages = 0;
	// Метка владельца области
	uint32_t tag = 0;
	/**
	 * Описываем область, которой принадлежит адрес
	 */
	{
		/**
		 * Разбираем адрес БЕЗ замка кучи
		 *
		 * Замок здесь стоял на пути КАЖДОГО освобождения и обращал его в очередь: восемь
		 * потоков на мелкой выдаче давали 285 наносекунд на действие против 84 без него,
		 * и время на действие РОСЛО с числом потоков - верный признак очереди.
		 *
		 * Читать без замка позволено потому, что живая область неизменна: дробят и
		 * сливают лишь свободные, а у живой ни границы, ни метка не меняются, пока её не
		 * освободят. Таблица же поиска куска сменяется целой записью, и читатель берёт
		 * её одним неделимым обращением
		 */
		if(!this->_pages->describe(addr, &base, &pages, &tag, hint))
			// Адрес выдан не нами
			return false;
	}
	// Если требуется адрес начала области
	if(begin != nullptr)
		// Записываем адрес начала области
		(* begin) = base;
	// Если требуется номер разряда
	if(index != nullptr)
		// Записываем номер разряда, либо признак выдачи сверх разрядов
		(* index) = ((tag > 0) ? static_cast <size_t> (tag - 1) : Classes::LIMIT);
	// Если требуется размер области
	if(size != nullptr)
		// Записываем размер области в байтах
		(* size) = (pages * Pages::PAGE);
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод отдачи системе свободной памяти
 *
 * @param now текущее время в миллисекундах
 * @param all отдавать всё, не глядя на отсрочку
 * @return    объём отданной системе памяти в байтах
 *
 */
size_t awh::alloc::Central::purge(const uint64_t now, const bool all) noexcept {
	// Если куча не заведена
	if(this->_pages == nullptr)
		// Отдавать нечего
		return 0;
	// Объём отданной системе памяти
	size_t result = 0;
	// Место перебора списков свободных областей
	size_t cursor = 0;
	// Изымаемые области
	void * spans[Pages::BATCH];
	// Признаки состоявшейся отдачи
	bool given[Pages::BATCH];
	/**
	 * Отдаём память пачками, отпуская замок кучи на время обращения к системе
	 *
	 * Обращение к системе - это `madvise` либо его подобие, то есть работа ядра по
	 * каждой странице области. Держи мы на этом замок кучи, всякий поток, которому в
	 * это время понадобится память, кружил бы и уступал время всё время работы ядра, -
	 * а отдача по требованию приложения обходит куче все списки разом.
	 *
	 * Пачками, а не всё разом: изъятые области никому не выдаются, и изыми мы их все,
	 * куча на время отдачи осталась бы пустой
	 */
	while(cursor <= (Pages::LISTS + 1)){
		// Число изъятых областей
		size_t count = 0;
		/**
		 * Изымаем очередную пачку областей под замком
		 */
		{
			// Захватываем замок кучи
			hold_t hold(this->_heap);
			// Изымаем очередную пачку областей
			count = this->_pages->detach(now, all, spans, Pages::BATCH, &cursor);
		}
		// Если изъять нечего
		if(count == 0)
			// Прекращаем отдачу
			break;
		/**
		 * Отдаём содержимое изъятых областей системе БЕЗ замка
		 */
		for(size_t i = 0; i < count; i++)
			// Отдаём содержимое очередной области системе
			given[i] = this->_pages->discharge(spans[i]);
		/**
		 * Возвращаем изъятые области в списки свободных под замком
		 */
		{
			// Захватываем замок кучи
			hold_t hold(this->_heap);
			// Возвращаем изъятые области в списки свободных
			result += this->_pages->attach(spans, count, given);
		}
	}
	// Выводим объём отданной системе памяти
	return result;
}
/**
 * @brief Метод задания порядка отдачи памяти системе
 *
 * @param delay отсрочка в миллисекундах: -1 - не отдавать вовсе
 * @param block наименьший отдаваемый кусок в байтах
 *
 */
void awh::alloc::Central::policy(const int64_t delay, const size_t block) noexcept {
	// Если куча не заведена
	if(this->_pages == nullptr)
		// Задавать порядок нечему
		return;
	// Захватываем замок кучи
	hold_t hold(this->_heap);
	// Задаём куче порядок отдачи памяти системе
	this->_pages->policy(delay, block);
}
/**
 * @brief Метод задания потолка взятого у источника
 *
 * @param limit потолок в байтах: нуль - без потолка
 *
 */
bool awh::alloc::Central::occupy(const size_t arena, const bool confined) noexcept {
	// Если куча не заведена
	if(this->_pages == nullptr)
		// Отвечать нечем
		return false;
	// Захватываем замок кучи
	hold_t hold(this->_heap);
	// Занимаем требуемую область у кучи
	return this->_pages->occupy(arena, confined);
}
/**
 * @brief Метод задания потолка взятого у источника
 *
 * @param limit потолок взятого у источника в байтах
 *
 */
void awh::alloc::Central::ceiling(const size_t limit) noexcept {
	// Захватываем замок кучи
	hold_t hold(this->_heap);
	// Задаём куче потолок взятого у источника
	this->_pages->ceiling(limit);
}
/**
 * @brief Метод определения упёртости кучи в потолок
 *
 * @return признак упёртости кучи в потолок
 *
 */
bool awh::alloc::Central::jammed() noexcept {
	// Захватываем замок кучи
	hold_t hold(this->_heap);
	// Выводим признак упёртости кучи в потолок
	return this->_pages->jammed();
}
/**
 * @brief Метод получения состояния центральных списков
 *
 * @return состояние списков
 *
 */
awh::alloc::Central::state_t awh::alloc::Central::state() noexcept {
	// Состояние центральных списков
	state_t result;
	// Если списки не заведены
	if(this->_classes == nullptr)
		// Выводим пустое состояние
		return result;
	/**
	 * Перебираем заведённые разряды
	 */
	for(size_t i = 0; i < this->_classes->count(); i++){
		// Получаем размер блока разряда
		const size_t size = this->_classes->size(i);
		// Захватываем замок разряда
		hold_t hold(this->_lists[i].lock);
		// Увеличиваем лежащее в центральных списках
		result.cached += (this->_lists[i].count * size);
		// Увеличиваем выданное кэшам потоков
		result.live += (this->_lists[i].live * size);
		// Увеличиваем число нарезанных областей
		result.regions += this->_lists[i].regions;
	}
	// Выводим состояние центральных списков
	return result;
}
/**
 * @brief Метод получения состояния страничной кучи
 *
 * @return состояние кучи
 *
 */
awh::alloc::Pages::state_t awh::alloc::Central::heap() noexcept {
	// Если куча не заведена
	if(this->_pages == nullptr)
		// Выводим пустое состояние
		return Pages::state_t();
	// Захватываем замок кучи
	hold_t hold(this->_heap);
	// Выводим состояние кучи копией: за её замком оно жить не должно
	return this->_pages->state();
}
/**
 * @brief Метод захвата всех замков перед ветвлением процесса
 *
 */
void awh::alloc::Central::prepare() noexcept {
	/**
	 * Захватываем замки разрядов
	 *
	 * Захватываем в том же порядке, что и работа, - сперва разряды, затем кучу, -
	 * иначе ветвление само стало бы вторым порядком захвата и дало бы взаимную блокировку
	 */
	for(size_t i = 0; i < Classes::LIMIT; i++)
		// Захватываем замок очередного разряда
		this->_lists[i].lock.acquire();
	// Захватываем замок кучи
	this->_heap.acquire();
}
/**
 * @brief Метод освобождения всех замков после ветвления процесса
 *
 */
void awh::alloc::Central::resume() noexcept {
	// Освобождаем замок кучи
	this->_heap.release();
	/**
	 * Освобождаем замки разрядов
	 */
	for(size_t i = 0; i < Classes::LIMIT; i++)
		// Освобождаем замок очередного разряда
		this->_lists[Classes::LIMIT - (i + 1)].lock.release();
}
/**
 * @brief Метод принудительного освобождения всех замков у потомка
 *
 */
void awh::alloc::Central::adopt() noexcept {
	// Освобождаем замок кучи, не глядя на его состояние
	this->_heap.reset();
	/**
	 * Освобождаем замки разрядов
	 */
	for(size_t i = 0; i < Classes::LIMIT; i++)
		// Освобождаем замок очередного разряда
		this->_lists[i].lock.reset();
	/**
	 * Возвращаем в списки области, изъятые на время отдачи системе
	 *
	 * Отдача изымает области из списков и держит их в местном массиве НА СТЕКЕ
	 * отдающего потока, отпуская замок кучи на время обращения к системе. Ветвление в
	 * этот миг переносит потомку и области, и счётчик изъятых, но не поток, обязанный
	 * вернуть их на место: без возврата области выпадают из кучи навсегда, а всякая
	 * выдача, какой не хватило места, ждёт их полную отсрочку впустую
	 *
	 * Проверено щупом: при расширенном окне изъятия 86 потомков из 200 наследовали
	 * изъятые области, тогда как с отключённой отдачей - ни один из 200
	 */
	if(this->_pages != nullptr)
		// Возвращаем изъятые области в списки свободных
		this->_pages->reclaim();
}
