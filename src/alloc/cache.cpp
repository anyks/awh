/**
 * @file cache.cpp
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
#include <alloc/cache.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <new>
#include <cstring>

/**
 * Если операционной системой является MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Стандартные заголовочные файлы
	 */
	#include <windows.h>
/**
 * Если операционной системой является Unix
 */
#else
	/**
	 * Стандартные заголовочные файлы
	 */
	#include <pthread.h>
#endif

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
		// Следующий свободный блок
		void * result = nullptr;
		// Читаем указатель побайтовым копированием: выравнивания указателя мы не обещали
		::memcpy(&result, block, sizeof(void *));
		// Выводим следующий свободный блок
		return result;
	}
	/**
	 * @brief Метод записи указателя на следующий свободный блок
	 *
	 * @param block      изменяемый свободный блок
	 * @param subsequent записываемый следующий свободный блок
	 *
	 */
	static void following(void * block, void * subsequent) noexcept {
		// Записываем указатель побайтовым копированием
		::memcpy(block, &subsequent, sizeof(void *));
	}
	/**
	 * Кэш потока хранится ключом системы, а НЕ в `thread_local`
	 *
	 * Поток-локальное место заводится лениво, при первом обращении потока, и заведение
	 * это само обращается за памятью - к тому самому malloc, из-под которого мы его и
	 * трогаем. Возвратность выходит бесконечной и валит стек на первом же выделении в
	 * новом потоке. Проверено опытом на macOS: срыв стека внутри `_tlv_get_addr`.
	 * Ключ же системы заведён заранее и ничего не выделяет
	 */
};

/**
 * @brief Конструктор
 *
 */
awh::alloc::Cache::Cache() noexcept :
 _owner(nullptr), _central(nullptr), _classes(nullptr), _bytes(0), _limit(LIMIT), _tally(0), _hint(nullptr), _next(nullptr), _busy(false) {
	// Обнуляем списки свободных блоков
	::memset(this->_lists, 0, sizeof(this->_lists));
}
/**
 * @brief Метод заведения кэша
 *
 * @param central центральные списки
 * @param classes разряды размеров
 * @param limit   предел кэша в байтах
 * @return        признак заведения кэша
 *
 */
bool awh::alloc::Cache::init(central_t * central, classes_t * classes, const size_t limit) noexcept {
	// Если центральные списки либо разряды не заданы
	if((central == nullptr) || (classes == nullptr))
		// Отвечаем отказом
		return false;
	// Запоминаем центральные списки
	this->_central = central;
	// Запоминаем разряды размеров
	this->_classes = classes;
	// Запоминаем предел кэша
	this->_limit = ((limit > 0) ? limit : LIMIT);
	// Лежащего в кэше пока нет
	this->_bytes = 0;
	/**
	 * Накопленное НЕ обнуляем
	 *
	 * Кэш заводится и на переиспользовании после завершения потока, а блоки, выданные
	 * прежним потоком и до сих пор живые, числятся занятыми по-прежнему. Обнули мы
	 * накопленное - занятое прикладным кодом уехало бы вниз на пустом месте
	 */
	// Обнуляем списки свободных блоков
	::memset(this->_lists, 0, sizeof(this->_lists));
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод накопления занятого прикладным кодом
 *
 * @param delta изменение занятого в байтах
 * @param batch величина, по достижении которой накопленное отдаётся наружу
 * @return      отдаваемое наружу накопленное, либо нуль
 *
 */
int64_t awh::alloc::Cache::tally(const int64_t delta, const int64_t batch) noexcept {
	// Накапливаем изменение занятого
	this->_tally += delta;
	// Если накопленное не дотянуло до величины отдачи
	if((this->_tally < batch) && (this->_tally > -batch))
		// Отдавать нечего
		return 0;
	// Запоминаем накопленное
	const int64_t result = this->_tally;
	// Обнуляем накопленное: оно уходит наружу
	this->_tally = 0;
	// Выводим отдаваемое наружу накопленное
	return result;
}
/**
 * @brief Метод получения накопленного, но не отданного наружу
 *
 * @return накопленное этим потоком в байтах
 *
 */
int64_t awh::alloc::Cache::pending() const noexcept {
	// Выводим накопленное этим потоком
	return this->_tally;
}
/**
 * @brief Метод получения места под подсказку поиска куска
 *
 * @return место под подсказку поиска
 *
 */
void ** awh::alloc::Cache::hint() noexcept {
	// Выводим место под подсказку поиска
	return &this->_hint;
}
/**
 * @brief Метод возврата пачки блоков разряда центральным спискам
 *
 * @param index номер разряда
 * @param count требуемое число возвращаемых блоков
 * @return      действительно возвращённое число блоков
 *
 */
size_t awh::alloc::Cache::drain(const size_t index, const size_t count) noexcept {
	// Если возвращать нечего
	if((count == 0) || (this->_lists[index].free == nullptr))
		// Возвращать нечего
		return 0;
	// Число действительно возвращаемых блоков
	size_t result = 0;
	// Голова возвращаемой цепочки
	void * head = this->_lists[index].free;
	// Хвост возвращаемой цепочки
	void * tail = head;
	/**
	 * Снимаем с кэша требуемое число блоков
	 */
	while(result < count){
		// Увеличиваем число снятых блоков
		result++;
		// Если требуемое число блоков набрано
		if(result == count)
			// Прекращаем набор: хвостом остаётся текущий блок
			break;
		// Получаем следующий свободный блок
		void * subsequent = ::following(tail);
		// Если следующего блока нет
		if(subsequent == nullptr)
			// Прекращаем набор: кэш исчерпан
			break;
		// Хвостом становится следующий блок
		tail = subsequent;
	}
	// Головой кэша становится блок, следующий за хвостом возвращаемой цепочки
	this->_lists[index].free = ::following(tail);
	// Обрываем цепочку у хвоста
	::following(tail, nullptr);
	// Уменьшаем число блоков в кэше
	this->_lists[index].count -= result;
	// Уменьшаем лежащее в кэше
	this->_bytes -= (result * this->_classes->size(index));
	// Возвращаем цепочку центральным спискам
	this->_central->back(index, head, tail, result);
	// Выводим число возвращённых блоков
	return result;
}
/**
 * @brief Метод отдачи всех блоков кэша центральным спискам
 *
 */
void awh::alloc::Cache::flush() noexcept {
	// Если кэш не заведён
	if((this->_central == nullptr) || (this->_classes == nullptr))
		// Отдавать нечего
		return;
	/**
	 * Отдаём блоки всех разрядов
	 */
	for(size_t i = 0; i < Classes::LIMIT; i++){
		// Отдаём все блоки очередного разряда
		while(this->_lists[i].count > 0){
			// Если отдать не вышло
			if(this->drain(i, this->_lists[i].count) == 0)
				// Прекращаем отдачу разряда
				break;
		}
	}
}
/**
 * @brief Метод отвязки кэша от завершившегося потока
 *
 */
void awh::alloc::Cache::release() noexcept {
	// Если управляющего у кэша нет
	if(this->_owner == nullptr)
		// Отвязывать некому
		return;
	// Отвязываем кэш у заведшего его управляющего
	this->_owner->retire(this);
}
/**
 * @brief Метод выдачи блока разряда
 *
 * @param index номер разряда
 * @return      адрес выданного блока либо nullptr
 *
 */
void * awh::alloc::Cache::alloc(const size_t index) noexcept {
	// Если кэш не заведён либо разряд неведом
	if((this->_classes == nullptr) || (index >= this->_classes->count()))
		// Выдавать нечего
		return nullptr;
	// Если свободных блоков разряда в кэше не осталось
	if(this->_lists[index].free == nullptr){
		// Голова изымаемой у центральных списков цепочки
		void * head = nullptr;
		// Хвост изымаемой цепочки
		void * tail = nullptr;
		/**
		 * Забираем у центральных списков пачку блоков
		 *
		 * Пачку, а не блок: захват замка стоит дороже самой передачи, и платить за
		 * него поштучно значило бы свести кэш к обёртке над центральными списками
		 */
		const size_t taken = this->_central->fetch(index, &head, &tail, Central::BATCH);
		// Если пачку взять не вышло
		if(taken == 0)
			// Выдавать нечего
			return nullptr;
		// Головой кэша становится голова взятой цепочки
		this->_lists[index].free = head;
		// Увеличиваем число блоков в кэше
		this->_lists[index].count += taken;
		// Увеличиваем лежащее в кэше
		this->_bytes += (taken * this->_classes->size(index));
	}
	// Снимаем блок с головы списка
	void * result = this->_lists[index].free;
	// Головой списка становится следующий блок
	this->_lists[index].free = ::following(result);
	// Уменьшаем число блоков в кэше
	this->_lists[index].count--;
	// Уменьшаем лежащее в кэше
	this->_bytes -= this->_classes->size(index);
	// Выводим выданный блок
	return result;
}
/**
 * @brief Метод возврата блока разряда
 *
 * @param index номер разряда
 * @param addr  адрес возвращаемого блока
 *
 */
void awh::alloc::Cache::free(const size_t index, void * addr) noexcept {
	// Если кэш не заведён, разряд неведом либо блок не задан
	if((this->_classes == nullptr) || (index >= this->_classes->count()) || (addr == nullptr))
		// Возвращать нечего
		return;
	// Связываем возвращаемый блок с прежней головой списка
	::following(addr, this->_lists[index].free);
	// Головой списка становится возвращаемый блок
	this->_lists[index].free = addr;
	// Увеличиваем число блоков в кэше
	this->_lists[index].count++;
	// Увеличиваем лежащее в кэше
	this->_bytes += this->_classes->size(index);
	/**
	 * Отдаём излишек центральным спискам, если предел кэша перебран
	 *
	 * Отдаём пачками и лишь пока предел перебран: отдача всего разом обнулила бы
	 * кэш и следующее же выделение снова пошло бы за замком
	 */
	while(this->_bytes > this->_limit){
		// Число отданного за оборот
		size_t given = 0;
		/**
		 * Перебираем разряды, отдавая по пачке
		 */
		for(size_t i = 0; (i < Classes::LIMIT) && (this->_bytes > this->_limit); i++){
			// Если в разряде блоков нет
			if(this->_lists[i].count == 0)
				// Переходим к следующему разряду
				continue;
			// Отдаём пачку блоков разряда
			given += this->drain(i, ((this->_lists[i].count < Central::BATCH) ? this->_lists[i].count : Central::BATCH));
		}
		// Если отдать не вышло вовсе
		if(given == 0)
			// Прекращаем отдачу: иначе оборот повторялся бы вечно
			break;
	}
}
/**
 * @brief Метод задания предела кэша
 *
 * @param limit предел кэша в байтах
 *
 */
void awh::alloc::Cache::limit(const size_t limit) noexcept {
	// Запоминаем предел кэша
	this->_limit = ((limit > 0) ? limit : LIMIT);
}
/**
 * @brief Метод получения объёма лежащего в кэше
 *
 * @return объём лежащего в кэше в байтах
 *
 */
size_t awh::alloc::Cache::bytes() const noexcept {
	// Выводим объём лежащего в кэше
	return this->_bytes;
}

/**
 * Ключ завершения потока
 *
 * Заводится однажды на процесс: отклик его зовётся системой при завершении потока и
 * получает кэш, блоки которого некому больше выдавать
 */
#if defined(_WIN32) || defined(_WIN64)
	// Место хранения кэша, отслеживаемое системой
	static DWORD __awh_alloc_slot__ = FLS_OUT_OF_INDEXES;
#else
	// Ключ хранения кэша, отслеживаемый системой
	static pthread_key_t __awh_alloc_key__;
#endif

/**
 * @brief Метод получения кэша текущего потока
 *
 * @return кэш текущего потока либо nullptr
 *
 */
static awh::alloc::cache_t * attached() noexcept {
	/**
	 * Если операционной системой является MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Если место хранения не заведено
		if(__awh_alloc_slot__ == FLS_OUT_OF_INDEXES)
			// Кэша нет
			return nullptr;
		// Выводим кэш из места хранения потока
		return reinterpret_cast <awh::alloc::cache_t *> (::FlsGetValue(__awh_alloc_slot__));
	/**
	 * Если операционной системой является Unix
	 */
	#else
		// Выводим кэш из ключа хранения потока
		return reinterpret_cast <awh::alloc::cache_t *> (::pthread_getspecific(__awh_alloc_key__));
	#endif
}
/**
 * @brief Метод привязки кэша к текущему потоку
 *
 * @param cache привязываемый кэш
 *
 */
static void attach(awh::alloc::cache_t * cache) noexcept {
	/**
	 * Если операционной системой является MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Если место хранения не заведено
		if(__awh_alloc_slot__ == FLS_OUT_OF_INDEXES)
			// Привязывать некуда
			return;
		// Записываем кэш в место хранения потока
		::FlsSetValue(__awh_alloc_slot__, cache);
	/**
	 * Если операционной системой является Unix
	 */
	#else
		// Записываем кэш в ключ хранения потока
		::pthread_setspecific(__awh_alloc_key__, cache);
	#endif
}
/**
 * @brief Отклик завершения потока
 *
 * @param value кэш завершившегося потока
 *
 */
#if defined(_WIN32) || defined(_WIN64)
static void WINAPI __awh_alloc_finish__(void * value) noexcept {
#else
static void __awh_alloc_finish__(void * value) noexcept {
#endif
	// Если кэша нет
	if(value == nullptr)
		// Отпускать нечего
		return;
	// Приводим значение к кэшу
	awh::alloc::cache_t * cache = reinterpret_cast <awh::alloc::cache_t *> (value);
	// Отвязываем кэш от завершившегося потока
	cache->release();
}

/**
 * @brief Конструктор
 *
 */
awh::alloc::Caches::Caches() noexcept :
 _central(nullptr), _classes(nullptr), _lock(), _caches(nullptr), _count(0), _limit(Cache::LIMIT), _keyed(false) {}
/**
 * @brief Метод заведения управляющего кэшами
 *
 * @param central центральные списки
 * @param classes разряды размеров
 * @return        признак заведения
 *
 */
bool awh::alloc::Caches::init(central_t * central, classes_t * classes) noexcept {
	// Если центральные списки либо разряды не заданы
	if((central == nullptr) || (classes == nullptr))
		// Отвечаем отказом
		return false;
	// Запоминаем центральные списки
	this->_central = central;
	// Запоминаем разряды размеров
	this->_classes = classes;
	// Если ключ завершения потока ещё не заведён
	if(!this->_keyed){
		/**
		 * Если операционной системой является MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Заводим место хранения с откликом завершения потока
			__awh_alloc_slot__ = ::FlsAlloc(&__awh_alloc_finish__);
			// Запоминаем признак заведённости ключа
			this->_keyed = (__awh_alloc_slot__ != FLS_OUT_OF_INDEXES);
		/**
		 * Если операционной системой является Unix
		 */
		#else
			// Заводим ключ хранения с откликом завершения потока
			this->_keyed = (::pthread_key_create(&__awh_alloc_key__, &__awh_alloc_finish__) == 0);
		#endif
		// Если ключ завести не вышло
		if(!this->_keyed)
			/**
			 * Отвечаем отказом
			 *
			 * Без ключа кэш завершившегося потока остался бы держать блоки навсегда, а
			 * распределитель, теряющий память на каждом потоке, негоден вовсе
			 */
			return false;
	}
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод заведения кэша текущему потоку
 *
 * @return заведённый кэш либо nullptr
 *
 */
awh::alloc::cache_t * awh::alloc::Caches::create() noexcept {
	// Заводимый кэш
	cache_t * result = nullptr;
	/**
	 * Ищем свободный кэш среди заведённых
	 */
	{
		// Захватываем замок общего списка кэшей
		hold_t hold(this->_lock);
		/**
		 * Перебираем заведённые кэши
		 */
		for(cache_t * cache = this->_caches; cache != nullptr; cache = cache->_next){
			// Если кэш никем не занят
			if(!cache->_busy){
				// Отмечаем кэш занятым
				cache->_busy = true;
				// Запоминаем найденный кэш
				result = cache;
				// Прекращаем перебор
				break;
			}
		}
	}
	// Если свободного кэша не нашлось
	if(result == nullptr){
		/**
		 * Берём память под кэш у центральных списков, а не у malloc
		 *
		 * Кэш заводится из-под перехваченного malloc, и обращение за памятью к нему же
		 * ушло бы в бесконечную возвратность
		 */
		void * memory = this->_central->alloc(sizeof(cache_t));
		// Если память под кэш не выдана
		if(memory == nullptr)
			// Отвечаем отказом
			return nullptr;
		// Заводим кэш на выданной памяти
		result = new (memory) Cache();
		// Запоминаем управляющего в самом кэше
		result->_owner = this;
		// Отмечаем кэш занятым
		result->_busy = true;
		// Захватываем замок общего списка кэшей
		hold_t hold(this->_lock);
		// Вносим кэш в общий список
		result->_next = this->_caches;
		// Запоминаем кэш как первый
		this->_caches = result;
		// Увеличиваем число заведённых кэшей
		this->_count++;
	}
	// Заводим кэш
	if(!result->init(this->_central, this->_classes, this->_limit)){
		// Отмечаем кэш свободным
		result->_busy = false;
		// Отвечаем отказом
		return nullptr;
	}
	// Выводим заведённый кэш
	return result;
}
/**
 * @brief Метод получения кэша текущего потока
 *
 * @return кэш текущего потока либо nullptr
 *
 */
awh::alloc::cache_t * awh::alloc::Caches::local() noexcept {
	// Получаем кэш текущего потока
	cache_t * attached = ::attached();
	// Если кэш текущего потока уже заведён
	if(attached != nullptr)
		// Выводим кэш текущего потока
		return attached;
	// Если управляющий не заведён
	if((this->_central == nullptr) || !this->_keyed)
		// Выдавать нечего
		return nullptr;
	// Заводим кэш текущему потоку
	cache_t * result = this->create();
	// Если кэш завести не вышло
	if(result == nullptr)
		// Выдавать нечего
		return nullptr;
	/**
	 * Отдаём кэш системе на отслеживание завершения потока
	 *
	 * Тем же ключом он и хранится: второго места под него не заводится
	 */
	::attach(result);
	// Выводим заведённый кэш
	return result;
}
/**
 * @brief Метод отвязки кэша от завершившегося потока
 *
 * @param cache отвязываемый кэш
 *
 */
void awh::alloc::Caches::retire(cache_t * cache) noexcept {
	// Если кэш не задан
	if(cache == nullptr)
		// Отвязывать нечего
		return;
	// Отдаём блоки кэша центральным спискам
	cache->flush();
	// Захватываем замок общего списка кэшей
	hold_t hold(this->_lock);
	/**
	 * Отмечаем кэш свободным, а не отдаём его память
	 *
	 * Число живущих разом потоков обыкновенно устойчиво, и возврат памяти кэша с новым
	 * заведением на каждом потоке был бы работой впустую
	 */
	cache->_busy = false;
}
/**
 * @brief Метод задания предела кэша потока
 *
 * @param limit предел кэша в байтах
 *
 */
void awh::alloc::Caches::limit(const size_t limit) noexcept {
	// Захватываем замок общего списка кэшей
	hold_t hold(this->_lock);
	// Запоминаем предел кэша потока
	this->_limit = ((limit > 0) ? limit : Cache::LIMIT);
	/**
	 * Задаём предел уже заведённым кэшам
	 */
	for(cache_t * cache = this->_caches; cache != nullptr; cache = cache->_next)
		// Задаём предел очередному кэшу
		cache->limit(this->_limit);
}
/**
 * @brief Метод получения объёма, лежащего во всех кэшах
 *
 * @param count число заведённых кэшей
 * @return      объём лежащего в кэшах в байтах
 *
 */
size_t awh::alloc::Caches::cached(size_t * count) noexcept {
	// Объём лежащего в кэшах
	size_t result = 0;
	// Захватываем замок общего списка кэшей
	hold_t hold(this->_lock);
	/**
	 * Перебираем заведённые кэши
	 */
	for(cache_t * cache = this->_caches; cache != nullptr; cache = cache->_next)
		// Увеличиваем объём лежащего в кэшах
		result += cache->bytes();
	// Если требуется число заведённых кэшей
	if(count != nullptr)
		// Записываем число заведённых кэшей
		(* count) = this->_count;
	// Выводим объём лежащего в кэшах
	return result;
}
/**
 * @brief Метод получения накопленного кэшами, но не отданного наружу
 *
 * @return накопленное всеми кэшами в байтах
 *
 */
int64_t awh::alloc::Caches::pending() noexcept {
	// Накопленное всеми кэшами
	int64_t result = 0;
	// Захватываем замок общего списка кэшей
	hold_t hold(this->_lock);
	/**
	 * Перебираем заведённые кэши
	 *
	 * Перебираются ВСЕ, включая кэши завершившихся потоков: те не разрушаются, а
	 * переиспользуются, и накопленное в них остаётся верным
	 */
	for(cache_t * cache = this->_caches; cache != nullptr; cache = cache->_next)
		// Увеличиваем накопленное всеми кэшами
		result += cache->pending();
	// Выводим накопленное всеми кэшами
	return result;
}
/**
 * @brief Метод отдачи центральным спискам блоков всех кэшей
 *
 * @return объём отданного в байтах
 *
 */
size_t awh::alloc::Caches::flush() noexcept {
	// Объём отданного
	size_t result = 0;
	// Захватываем замок общего списка кэшей
	hold_t hold(this->_lock);
	/**
	 * Перебираем заведённые кэши
	 */
	for(cache_t * cache = this->_caches; cache != nullptr; cache = cache->_next){
		// Запоминаем лежащее в кэше прежде отдачи
		const size_t before = cache->bytes();
		// Отдаём блоки кэша центральным спискам
		cache->flush();
		// Увеличиваем объём отданного
		result += (before - cache->bytes());
	}
	// Выводим объём отданного
	return result;
}
/**
 * @brief Метод разрушения управляющего кэшами
 *
 */
void awh::alloc::Caches::destroy() noexcept {
	// Отдаём центральным спискам блоки всех кэшей
	this->flush();
	/**
	 * Снимаем ключ завершения потока ПРЕЖДЕ отдачи памяти кэшей
	 *
	 * Память кэшей взята у страничной кучи, и та отдаёт её системе при своём
	 * разрушении. Оставь мы ключ, отклик завершения потока обратился бы к уже
	 * отданной памяти - у Windows он зовётся и при завершении самого процесса, и
	 * щуп там валился с кодом 139 при вполне сошедшихся наблюдениях. Снятие ключа
	 * откликов не зовёт ни у Windows, ни у Unix - на то оно и снятие
	 */
	if(this->_keyed){
		/**
		 * Отвязываем кэш от текущего потока ПРЕЖДЕ снятия ключа
		 *
		 * Обращение к снятому ключу - ошибка, и NetBSD её не прощает: там
		 * `pthread_setspecific` по ключу без отклика завершения валит процесс
		 * утверждением `pthread__tsd_destructors[key] != NULL` внутри libpthread.
		 * Прежде отвязка стояла ниже, за снятием ключа, и щуп кэшей там валился с
		 * кодом 134 у самого последнего наблюдения
		 */
		::attach(nullptr);
		/**
		 * Если операционной системой является MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Снимаем место хранения кэша
			::FlsFree(__awh_alloc_slot__);
			// Обнуляем место хранения кэша
			__awh_alloc_slot__ = FLS_OUT_OF_INDEXES;
		/**
		 * Если операционной системой является Unix
		 */
		#else
			// Снимаем ключ хранения кэша
			::pthread_key_delete(__awh_alloc_key__);
		#endif
		// Отмечаем ключ снятым
		this->_keyed = false;
	}
	// Захватываем замок общего списка кэшей
	hold_t hold(this->_lock);
	/**
	 * Обнуляем общий список кэшей
	 *
	 * Память самих кэшей не отдаём: она взята у страничной кучи, а та отдаёт всё
	 * взятое при своём разрушении - отдавать её порознь значило бы вести учёт учёта
	 */
	this->_caches = nullptr;
	// Обнуляем число заведённых кэшей
	this->_count = 0;
	// Обнуляем разряды размеров
	this->_classes = nullptr;
	// Обнуляем центральные списки
	this->_central = nullptr;
}
/**
 * @brief Метод захвата замка перед ветвлением процесса
 *
 */
void awh::alloc::Caches::prepare() noexcept {
	// Захватываем замок общего списка кэшей
	this->_lock.acquire();
}
/**
 * @brief Метод освобождения замка после ветвления процесса
 *
 */
void awh::alloc::Caches::resume() noexcept {
	// Освобождаем замок общего списка кэшей
	this->_lock.release();
}
/**
 * @brief Метод приведения кэшей в порядок у потомка ветвления
 *
 */
void awh::alloc::Caches::adopt() noexcept {
	// Освобождаем замок, не глядя на его состояние
	this->_lock.reset();
	/**
	 * Отдаём центральным спискам блоки кэшей прочих потоков
	 *
	 * Потомку достался один поток, а кэши прочих потоков родителя держат блоки, выдать
	 * которые больше некому: без отдачи они были бы потеряны навсегда
	 */
	for(cache_t * cache = this->_caches; cache != nullptr; cache = cache->_next){
		// Если кэш принадлежит текущему потоку
		if(cache == ::attached())
			// Оставляем его как есть: этот поток жив
			continue;
		// Отдаём блоки кэша центральным спискам
		cache->flush();
		// Отмечаем кэш свободным
		cache->_busy = false;
	}
}
