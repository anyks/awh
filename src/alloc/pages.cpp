/**
 * @file pages.cpp
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
 * @brief Реализация страничной кучи распределителя памяти
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <alloc/pages.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * @brief Конструктор
 *
 */
awh::alloc::Pages::Pages() noexcept :
 _source(nullptr), _chunks(nullptr), _large(nullptr), _spare(nullptr),
 _meta(nullptr), _metaLeft(0), _metaChunks(nullptr), _confined(false),
 _delay(10), _block(0), _state() {
	// Обнуляем списки свободных областей
	::memset(this->_lists, 0, sizeof(this->_lists));
}
/**
 * @brief Метод выдачи памяти под учётную запись
 *
 * @return адрес выданной памяти либо nullptr
 *
 */
void * awh::alloc::Pages::meta() noexcept {
	// Размер выдаваемой учётной записи: наибольшая из двух
	const size_t size = ((sizeof(span_t) > sizeof(chunk_t)) ? sizeof(span_t) : sizeof(chunk_t));
	// Приводим размер к границе восьми байт
	const size_t need = ((size + 7u) & ~static_cast <size_t> (7u));
	// Если в текущем куске места не осталось
	if(this->_metaLeft < need){
		/**
		 * Берём у источника новый кусок под учётные записи
		 *
		 * Куски эти невелики и системе не отдаются вовсе: учётные записи живут столько
		 * же, сколько сама куча, а отдавать их порознь значило бы вести учёт учёта
		 */
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
		 * Вносим кусок в общий список: первые восемь байт куска отводятся под указатель
		 * на предыдущий, оттого списку не нужно собственной памяти
		 */
		// Записываем в начало куска указатель на предыдущий
		::memcpy(block, &this->_metaChunks, sizeof(void *));
		// Запоминаем кусок как последний
		this->_metaChunks = block;
		// Сдвигаем начало выдачи за указатель
		this->_meta = (block + sizeof(void *));
		// Запоминаем остаток куска
		this->_metaLeft = (actual - sizeof(void *));
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
 * @brief Метод записи области в указатели куска
 *
 * @param span записываемая область
 *
 */
void awh::alloc::Pages::mark(span_t * span) noexcept {
	// Определяем номер первой страницы области в куске
	const size_t first = static_cast <size_t> ((span->base - span->chunk->base) / PAGE);
	/**
	 * Записываем область по всем её страницам
	 *
	 * Запись по всем, а не по одной первой, нужна для поиска области по произвольному
	 * адресу внутри неё - без этого разбор адреса сбоя пришлось бы вести перебором
	 */
	for(size_t i = 0; i < span->pages; i++)
		// Записываем область по очередной странице
		span->chunk->index[first + i] = span;
}
/**
 * @brief Метод внесения области в список свободных
 *
 * @param span вносимая область
 *
 */
void awh::alloc::Pages::push(span_t * span) noexcept {
	// Определяем список, которому принадлежит область
	span_t ** list = ((span->pages <= LISTS) ? &this->_lists[span->pages] : &this->_large);
	// Предыдущей области у вносимой нет
	span->prev = nullptr;
	// Следующей становится прежняя голова списка
	span->next = (* list);
	// Если список не пуст
	if((* list) != nullptr)
		// Прежняя голова списка получает предыдущую
		(* list)->prev = span;
	// Вносимая область становится головой списка
	(* list) = span;
}
/**
 * @brief Метод изъятия области из списка свободных
 *
 * @param span изымаемая область
 *
 */
void awh::alloc::Pages::pull(span_t * span) noexcept {
	// Определяем список, которому принадлежит область
	span_t ** list = ((span->pages <= LISTS) ? &this->_lists[span->pages] : &this->_large);
	// Если у области есть предыдущая
	if(span->prev != nullptr)
		// Связываем предыдущую со следующей
		span->prev->next = span->next;
	// Если предыдущей нет, изымаемая была головой списка
	else (* list) = span->next;
	// Если у области есть следующая
	if(span->next != nullptr)
		// Связываем следующую с предыдущей
		span->next->prev = span->prev;
	// Обнуляем связи изъятой области
	span->prev = nullptr;
	// Обнуляем связь со следующей
	span->next = nullptr;
}
/**
 * @brief Метод взятия у источника нового куска
 *
 * @return взятый кусок либо nullptr
 *
 */
awh::alloc::Pages::chunk_t * awh::alloc::Pages::grow() noexcept {
	// Если обращаться к источнику запрещено
	if(this->_confined)
		// Отвечаем отказом: расти некуда
		return nullptr;
	// Выдаём память под учётную запись куска
	chunk_t * chunk = reinterpret_cast <chunk_t *> (this->meta());
	// Если память под учётную запись не выдана
	if(chunk == nullptr)
		// Отвечаем отказом
		return nullptr;
	// Действительно выданный размер
	size_t actual = 0;
	/**
	 * Берём у источника кусок, выровненный по своему размеру
	 *
	 * Выравнивание это позволяет по любому адресу внутри куска найти его начало одной
	 * маской, без поиска по списку
	 */
	uint8_t * base = reinterpret_cast <uint8_t *> (this->_source->alloc(CHUNK, CHUNK, actual));
	// Если кусок не выдан
	if(base == nullptr)
		// Отвечаем отказом
		return nullptr;
	// Запоминаем адрес начала куска
	chunk->base = base;
	// Запоминаем размер куска
	chunk->size = actual;
	// Выданных наружу страниц у нового куска нет
	chunk->used = 0;
	// Вносим кусок в общий список
	chunk->next = this->_chunks;
	// Запоминаем кусок как первый
	this->_chunks = chunk;
	// Обнуляем указатели областей куска
	::memset(chunk->index, 0, sizeof(chunk->index));
	// Выдаём память под учётную запись области, накрывающей кусок целиком
	span_t * span = reinterpret_cast <span_t *> (this->meta());
	// Если память под учётную запись не выдана
	if(span == nullptr)
		// Отвечаем отказом
		return nullptr;
	// Область начинается с начала куска
	span->base = base;
	// Область накрывает кусок целиком
	span->pages = PAGES;
	// Область свободна
	span->released = true;
	// Содержимое области системе не отдано
	span->purged = false;
	// Отметки времени у области ещё нет
	span->stamp = 0;
	// Область принадлежит новому куску
	span->chunk = chunk;
	// Связей в списке свободных у области пока нет
	span->prev = nullptr;
	// Связь со следующей пока пуста
	span->next = nullptr;
	// Записываем область в указатели куска
	this->mark(span);
	// Вносим область в список свободных
	this->push(span);
	// Увеличиваем взятое у источника
	this->_state.total += actual;
	// Увеличиваем свободное
	this->_state.free += (PAGES * PAGE);
	// Увеличиваем число взятых кусков
	this->_state.chunks++;
	// Выводим взятый кусок
	return chunk;
}
/**
 * @brief Метод поиска свободной области требуемого размера
 *
 * @param pages требуемое число страниц
 * @return      найденная область либо nullptr
 *
 */
awh::alloc::Pages::span_t * awh::alloc::Pages::search(const size_t pages) noexcept {
	/**
	 * Перебираем списки от требуемого размера вверх
	 *
	 * Список точного размера просматривается первым, и лишь при его пустоте берётся
	 * область покрупнее с последующим делением
	 */
	if(pages <= LISTS){
		// Перебираем списки по числу страниц
		for(size_t i = pages; i <= LISTS; i++){
			// Если список не пуст
			if(this->_lists[i] != nullptr)
				// Выводим голову списка
				return this->_lists[i];
		}
	}
	// Наименьшая подходящая область из числа крупных
	span_t * result = nullptr;
	/**
	 * Перебираем крупные свободные области
	 *
	 * Берётся наименьшая подходящая, а не первая попавшаяся: так дробление идёт от
	 * меньших областей, и крупные остаются целыми под крупные же запросы
	 */
	for(span_t * span = this->_large; span != nullptr; span = span->next){
		// Если область мала для запроса
		if(span->pages < pages)
			// Переходим к следующей
			continue;
		// Если подходящей области ещё не находилось либо найденная меньше прежней
		if((result == nullptr) || (span->pages < result->pages))
			// Запоминаем область как найденную
			result = span;
	}
	// Выводим найденную область
	return result;
}
/**
 * @brief Метод слияния области с соседями по куску
 *
 * @param span сливаемая область
 * @return     область после слияния
 *
 */
awh::alloc::Pages::span_t * awh::alloc::Pages::merge(span_t * span) noexcept {
	// Получаем кусок, которому принадлежит область
	chunk_t * chunk = span->chunk;
	// Определяем номер первой страницы области в куске
	size_t first = static_cast <size_t> ((span->base - chunk->base) / PAGE);
	/**
	 * Сливаем с соседом слева
	 *
	 * Граница куска для слияния непроницаема: два соседних по адресу куска могут быть
	 * отданы системе порознь, и область, слитая через границу, при отдаче распалась бы
	 */
	if(first > 0){
		// Получаем соседа слева
		span_t * left = chunk->index[first - 1];
		// Если сосед слева свободен
		if((left != nullptr) && left->released){
			// Изымаем соседа из списка свободных
			this->pull(left);
			/**
			 * Слитая область считается отданной лишь тогда, когда отданы обе части
			 *
			 * Если отдана была лишь одна, учёт отданного обязан её позабыть: содержимое
			 * слитой области отданным больше не считается, а байты её из учёта отданного
			 * никто не вычел бы, и тот рос бы без основания
			 */
			// Определяем, отданы ли обе части
			const bool both = (span->purged && left->purged);
			// Если отдана была лишь сливаемая область
			if(span->purged && !both)
				// Вычитаем её из учёта отданного
				this->_state.purged -= (span->pages * PAGE);
			// Если отдан был лишь сосед слева
			if(left->purged && !both)
				// Вычитаем его из учёта отданного
				this->_state.purged -= (left->pages * PAGE);
			// Отмечаем слитую область отданной только при отданных обеих
			left->purged = both;
			// Отметкой времени слитой области становится более поздняя из двух
			left->stamp = ((span->stamp > left->stamp) ? span->stamp : left->stamp);
			// Увеличиваем размер соседа на размер сливаемой области
			left->pages += span->pages;
			// Учётная запись сливаемой области больше не нужна
			span->next = this->_spare;
			// Вносим её в список повторно используемых
			this->_spare = span;
			// Сливаемой становится сосед слева
			span = left;
			// Пересчитываем номер первой страницы
			first = static_cast <size_t> ((span->base - chunk->base) / PAGE);
			// Записываем слитую область в указатели куска
			this->mark(span);
		}
	}
	// Определяем номер страницы за концом области
	const size_t after = (first + span->pages);
	/**
	 * Сливаем с соседом справа
	 */
	if(after < PAGES){
		// Получаем соседа справа
		span_t * right = chunk->index[after];
		// Если сосед справа свободен
		if((right != nullptr) && right->released){
			// Изымаем соседа из списка свободных
			this->pull(right);
			// Определяем, отданы ли обе части
			const bool both = (span->purged && right->purged);
			// Если отдана была лишь сливаемая область
			if(span->purged && !both)
				// Вычитаем её из учёта отданного
				this->_state.purged -= (span->pages * PAGE);
			// Если отдан был лишь сосед справа
			if(right->purged && !both)
				// Вычитаем его из учёта отданного
				this->_state.purged -= (right->pages * PAGE);
			// Отмечаем слитую область отданной только при отданных обеих
			span->purged = both;
			// Отметкой времени слитой области становится более поздняя из двух
			span->stamp = ((right->stamp > span->stamp) ? right->stamp : span->stamp);
			// Увеличиваем размер области на размер соседа
			span->pages += right->pages;
			// Учётная запись соседа больше не нужна
			right->next = this->_spare;
			// Вносим её в список повторно используемых
			this->_spare = right;
			// Записываем слитую область в указатели куска
			this->mark(span);
		}
	}
	// Выводим область после слияния
	return span;
}
/**
 * @brief Метод заведения кучи
 *
 * @param source   источник страниц
 * @param arena    занимаемая при заведении область в байтах
 * @param confined запрет обращаться к источнику сверх занятого
 * @return         признак заведения кучи
 *
 */
bool awh::alloc::Pages::init(source_t * source, const size_t arena, const bool confined) noexcept {
	// Если источник не задан
	if(source == nullptr)
		// Отвечаем отказом
		return false;
	// Запоминаем источник страниц
	this->_source = source;
	// Запрет обращаться к источнику при заведении не действует
	this->_confined = false;
	// Если при заведении требуется занять область
	if(arena > 0){
		// Определяем требуемое число кусков
		const size_t count = ((arena + (CHUNK - 1)) / CHUNK);
		/**
		 * Занимаем требуемое число кусков
		 */
		for(size_t i = 0; i < count; i++){
			// Берём у источника очередной кусок
			if(this->grow() == nullptr)
				// Отвечаем отказом: занять требуемое не вышло
				return false;
		}
	}
	// Запоминаем запрет обращаться к источнику сверх занятого
	this->_confined = confined;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод разрушения кучи
 *
 */
void awh::alloc::Pages::destroy() noexcept {
	// Если источник не задан
	if(this->_source == nullptr)
		// Разрушать нечего
		return;
	/**
	 * Отдаём источнику взятые куски
	 */
	for(chunk_t * chunk = this->_chunks; chunk != nullptr;){
		// Запоминаем следующий кусок прежде отдачи текущего
		chunk_t * next = chunk->next;
		// Отдаём кусок источнику
		this->_source->release(chunk->base, chunk->size);
		// Переходим к следующему куску
		chunk = next;
	}
	/**
	 * Отдаём источнику куски памяти под учётные записи
	 *
	 * Отдаются последними: учётные записи кусков лежат в них же, и отдать их прежде
	 * значило бы читать освобождённую память при обходе списка кусков
	 */
	for(void * block = this->_metaChunks; block != nullptr;){
		// Читаем указатель на предыдущий кусок из его начала
		void * previous = nullptr;
		// Извлекаем указатель на предыдущий кусок
		::memcpy(&previous, block, sizeof(void *));
		// Отдаём кусок источнику
		this->_source->release(block, (256u * 1024u));
		// Переходим к предыдущему куску
		block = previous;
	}
	// Обнуляем общий список кусков
	this->_chunks = nullptr;
	// Обнуляем список крупных свободных областей
	this->_large = nullptr;
	// Обнуляем список повторно используемых учётных записей
	this->_spare = nullptr;
	// Обнуляем текущий кусок памяти под учётные записи
	this->_meta = nullptr;
	// Обнуляем остаток текущего куска
	this->_metaLeft = 0;
	// Обнуляем общий список кусков памяти под учётные записи
	this->_metaChunks = nullptr;
	// Обнуляем списки свободных областей
	::memset(this->_lists, 0, sizeof(this->_lists));
	// Обнуляем состояние кучи
	this->_state = state_t();
	// Обнуляем источник страниц
	this->_source = nullptr;
}
/**
 * @brief Метод выдачи области страниц
 *
 * @param pages требуемое число страниц кучи
 * @return      адрес выданной области либо nullptr
 *
 */
void * awh::alloc::Pages::alloc(const size_t pages) noexcept {
	// Если требуемое число страниц не задано либо куча не заведена
	if((pages == 0) || (this->_source == nullptr))
		// Выдавать нечего
		return nullptr;
	// Если требуемое число страниц не помещается в кусок
	if(pages > PAGES)
		// Отвечаем отказом: куски у источника берутся одного размера
		return nullptr;
	// Ищем свободную область требуемого размера
	span_t * span = this->search(pages);
	// Если подходящей области не нашлось
	if(span == nullptr){
		// Берём у источника новый кусок
		if(this->grow() == nullptr)
			// Отвечаем отказом
			return nullptr;
		// Повторяем поиск
		span = this->search(pages);
		// Если и теперь области не нашлось
		if(span == nullptr)
			// Отвечаем отказом
			return nullptr;
	}
	// Изымаем найденную область из списка свободных
	this->pull(span);
	/**
	 * Делим область, если она крупнее требуемого
	 */
	if(span->pages > pages){
		// Выдаём память под учётную запись остатка
		span_t * rest = this->_spare;
		// Если повторно используемая запись нашлась
		if(rest != nullptr)
			// Изымаем её из списка повторно используемых
			this->_spare = rest->next;
		// Если повторно используемой записи нет, берём новую
		else rest = reinterpret_cast <span_t *> (this->meta());
		// Если память под учётную запись не выдана
		if(rest == nullptr){
			/**
			 * Область не делим и выдаём целиком: потерять при этом можно лишь
			 * неиспользованный хвост, а отказать пришлось бы при наличии памяти
			 */
			// Отмечаем область выданной наружу
			span->released = false;
			// Отмечаем содержимое области присутствующим
			span->purged = false;
			// Увеличиваем число выданных страниц куска
			span->chunk->used += span->pages;
			// Уменьшаем свободное
			this->_state.free -= (span->pages * PAGE);
			// Увеличиваем выданное наружу
			this->_state.used += (span->pages * PAGE);
			// Выводим адрес выданной области
			return span->base;
		}
		// Остаток начинается за выдаваемой областью
		rest->base = (span->base + (pages * PAGE));
		// Размер остатка - разница размеров
		rest->pages = (span->pages - pages);
		// Остаток свободен
		rest->released = true;
		// Признак отданного содержимого остаток наследует у делимой области
		rest->purged = span->purged;
		// Отметку времени остаток наследует у делимой области
		rest->stamp = span->stamp;
		// Остаток принадлежит тому же куску
		rest->chunk = span->chunk;
		// Связей в списке свободных у остатка пока нет
		rest->prev = nullptr;
		// Связь со следующей пока пуста
		rest->next = nullptr;
		// Уменьшаем размер выдаваемой области до требуемого
		span->pages = pages;
		// Записываем остаток в указатели куска
		this->mark(rest);
		// Вносим остаток в список свободных
		this->push(rest);
	}
	// Отмечаем область выданной наружу
	span->released = false;
	// Записываем область в указатели куска
	this->mark(span);
	// Если содержимое области было системе отдано
	if(span->purged){
		// Уменьшаем отданное системе
		this->_state.purged -= (span->pages * PAGE);
		// Отмечаем содержимое области присутствующим
		span->purged = false;
	}
	// Увеличиваем число выданных страниц куска
	span->chunk->used += span->pages;
	// Уменьшаем свободное
	this->_state.free -= (span->pages * PAGE);
	// Увеличиваем выданное наружу
	this->_state.used += (span->pages * PAGE);
	// Выводим адрес выданной области
	return span->base;
}
/**
 * @brief Метод поиска области, которой принадлежит адрес
 *
 * @param addr  разбираемый адрес
 * @param begin адрес начала найденной области
 * @param pages размер найденной области в страницах кучи
 * @param live  признак выданной наружу области
 * @return      признак того, что адрес принадлежит куче
 *
 */
bool awh::alloc::Pages::locate(const void * addr, const void ** begin, size_t * pages, bool * live) const noexcept {
	// Если адрес не задан
	if(addr == nullptr)
		// Разбирать нечего
		return false;
	// Приводим адрес к целому виду
	const uintptr_t value = reinterpret_cast <uintptr_t> (addr);
	/**
	 * Перебираем взятые у источника куски
	 *
	 * Перебор допустим: кусков немного, а метод зовётся лишь при разборе адреса сбоя,
	 * то есть однажды за жизнь процесса
	 */
	for(const chunk_t * chunk = this->_chunks; chunk != nullptr; chunk = chunk->next){
		// Приводим начало куска к целому виду
		const uintptr_t base = reinterpret_cast <uintptr_t> (chunk->base);
		// Если адрес лежит вне куска
		if((value < base) || (value >= (base + chunk->size)))
			// Переходим к следующему куску
			continue;
		// Определяем номер страницы, которой принадлежит адрес
		const size_t page = static_cast <size_t> ((value - base) / PAGE);
		// Получаем область, которой принадлежит страница
		const span_t * span = chunk->index[page];
		// Если области у страницы нет
		if(span == nullptr)
			// Адрес принадлежит куче, но области за ним нет
			return false;
		// Если требуется адрес начала области
		if(begin != nullptr)
			// Записываем адрес начала области
			(* begin) = span->base;
		// Если требуется размер области
		if(pages != nullptr)
			// Записываем размер области
			(* pages) = span->pages;
		// Если требуется признак выданной области
		if(live != nullptr)
			// Записываем признак выданной наружу области
			(* live) = (!span->released);
		// Отвечаем, что адрес принадлежит куче
		return true;
	}
	// Адрес куче не принадлежит
	return false;
}
/**
 * @brief Метод определения размера выданной области
 *
 * @param addr адрес выданной области
 * @return     размер области в страницах кучи, либо нуль
 *
 */
size_t awh::alloc::Pages::size(const void * addr) const noexcept {
	// Размер найденной области
	size_t pages = 0;
	// Признак выданной наружу области
	bool live = false;
	// Адрес начала найденной области
	const void * begin = nullptr;
	// Если адрес куче не принадлежит
	if(!this->locate(addr, &begin, &pages, &live))
		// Размера у него нет
		return 0;
	// Если адрес не есть начало выданной наружу области
	if((begin != addr) || !live)
		// Размера у него нет
		return 0;
	// Выводим размер области
	return pages;
}
/**
 * @brief Метод возврата области страниц
 *
 * @param addr адрес возвращаемой области
 * @param now  текущее время в миллисекундах
 * @return     признак возврата области
 *
 */
bool awh::alloc::Pages::free(void * addr, const uint64_t now) noexcept {
	// Если адрес не задан либо куча не заведена
	if((addr == nullptr) || (this->_source == nullptr))
		// Возвращать нечего
		return false;
	// Приводим адрес к целому виду
	const uintptr_t value = reinterpret_cast <uintptr_t> (addr);
	/**
	 * Ищем кусок, которому принадлежит адрес
	 */
	for(chunk_t * chunk = this->_chunks; chunk != nullptr; chunk = chunk->next){
		// Приводим начало куска к целому виду
		const uintptr_t base = reinterpret_cast <uintptr_t> (chunk->base);
		// Если адрес лежит вне куска
		if((value < base) || (value >= (base + chunk->size)))
			// Переходим к следующему куску
			continue;
		// Определяем номер страницы, которой принадлежит адрес
		const size_t page = static_cast <size_t> ((value - base) / PAGE);
		// Получаем область, которой принадлежит страница
		span_t * span = chunk->index[page];
		// Если области нет либо адрес не есть её начало
		if((span == nullptr) || (span->base != reinterpret_cast <uint8_t *> (addr)))
			// Отвечаем отказом: адрес выдан не нами
			return false;
		// Если область уже свободна
		if(span->released)
			// Отвечаем отказом: повторное освобождение
			return false;
		// Отмечаем область свободной
		span->released = true;
		/**
		 * Ставим отметку времени освобождения
		 *
		 * Без неё отсрочка отдачи не работает вовсе: отметка осталась бы нулевой, разница
		 * с текущим временем - заведомо большей отсрочки, и память уходила бы системе
		 * немедленно при любой настройке
		 */
		span->stamp = now;
		// Уменьшаем число выданных страниц куска
		chunk->used -= span->pages;
		// Уменьшаем выданное наружу
		this->_state.used -= (span->pages * PAGE);
		// Увеличиваем свободное
		this->_state.free += (span->pages * PAGE);
		// Сливаем область с соседями по куску
		span = this->merge(span);
		// Вносим область в список свободных
		this->push(span);
		// Отвечаем успехом
		return true;
	}
	// Адрес куче не принадлежит
	return false;
}
/**
 * @brief Метод задания порядка отдачи памяти системе
 *
 * @param delay отсрочка в миллисекундах: -1 - не отдавать вовсе
 * @param block наименьший отдаваемый кусок в байтах
 *
 */
void awh::alloc::Pages::policy(const int64_t delay, const size_t block) noexcept {
	// Запоминаем отсрочку отдачи
	this->_delay = delay;
	// Запоминаем наименьший отдаваемый кусок
	this->_block = block;
}
/**
 * @brief Метод отдачи системе свободной памяти
 *
 * @param now текущее время в миллисекундах
 * @param all отдавать всё, не глядя на отсрочку
 * @return    объём отданной системе памяти в байтах
 *
 */
size_t awh::alloc::Pages::purge(const uint64_t now, const bool all) noexcept {
	// Если куча не заведена
	if(this->_source == nullptr)
		// Отдавать нечего
		return 0;
	// Если отдача памяти системе запрещена и не требуется отдать всё
	if((this->_delay < 0) && !all)
		// Отдавать нечего
		return 0;
	// Объём отданной системе памяти
	size_t result = 0;
	/**
	 * Перебираем все списки свободных областей
	 */
	for(size_t i = 0; i <= (LISTS + 1); i++){
		// Определяем перебираемый список
		span_t * span = ((i <= LISTS) ? this->_lists[i] : this->_large);
		/**
		 * Перебираем области списка
		 */
		while(span != nullptr){
			// Запоминаем следующую область: отдача связей не меняет, но осторожность дешева
			span_t * next = span->next;
			// Определяем размер области в байтах
			const size_t size = (span->pages * PAGE);
			// Если содержимое области уже отдано системе
			if(span->purged){
				// Переходим к следующей области
				span = next;
				// Продолжаем перебор
				continue;
			}
			// Если область меньше наименьшего отдаваемого куска
			if((!all) && (this->_block > 0) && (size < this->_block)){
				// Переходим к следующей области
				span = next;
				// Продолжаем перебор
				continue;
			}
			// Если область освобождена недавно и отдавать всё не требуется
			if((!all) && (this->_delay > 0) && ((now - span->stamp) < static_cast <uint64_t> (this->_delay))){
				// Переходим к следующей области
				span = next;
				// Продолжаем перебор
				continue;
			}
			// Отдаём содержимое области системе
			if(this->_source->purge(span->base, size)){
				// Отмечаем содержимое области отданным
				span->purged = true;
				// Увеличиваем отданное системе
				this->_state.purged += size;
				// Копим объём отданного
				result += size;
			}
			// Переходим к следующей области
			span = next;
		}
	}
	// Выводим объём отданной системе памяти
	return result;
}
/**
 * @brief Метод получения состояния кучи
 *
 * @return состояние кучи
 *
 */
const awh::alloc::Pages::state_t & awh::alloc::Pages::state() const noexcept {
	// Выводим состояние кучи
	return this->_state;
}
