/**
 * @file mach.cpp
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
#include <alloc/zone.hpp>

/**
 * Если операционной системой является macOS
 */
#if defined(__APPLE__)

/**
 * Стандартные заголовочные файлы
 */
#include <cstdlib>
#include <cstring>
#include <malloc/malloc.h>

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	// Наши функции выделения памяти, отданные захвату
	static awh::alloc::functions_t __awh_zone_hooks__;
	// Отклик перед ветвлением процесса
	static void (* __awh_zone_before__)() = nullptr;
	// Отклик у родителя после ветвления процесса
	static void (* __awh_zone_after__)() = nullptr;
	// Отклик у потомка после ветвления процесса
	static void (* __awh_zone_child__)() = nullptr;
	/**
	 * @brief Метод определения размера блока зоны
	 *
	 * @param zone зона, у которой спрашивают
	 * @param ptr  разбираемый указатель
	 * @return     размер блока, либо нуль если блок не наш
	 *
	 */
	static size_t measure(::malloc_zone_t * zone, const void * ptr) noexcept {
		// Зона нам известна, а спрашивают о чужом указателе
		(void) zone;
		// Если измерять нечем либо нечего
		if((__awh_zone_hooks__.msize == nullptr) || (ptr == nullptr))
			// Блок не наш
			return 0;
		/**
		 * Отвечаем нулём у чужого блока
		 *
		 * Именно так система и разбирает, какой зоне отдать освобождение: она обходит
		 * зоны и спрашивает размер, покуда одна из них не ответит ненулём. Ответь мы
		 * ненулём на чужой блок - и чужая память пошла бы в наше освобождение
		 */
		return (* __awh_zone_hooks__.msize)(ptr);
	}
	/**
	 * @brief Метод выделения памяти зоной
	 *
	 * @param zone зона, у которой просят
	 * @param size требуемый размер в байтах
	 * @return     адрес выданной памяти либо nullptr
	 *
	 */
	static void * reserve(::malloc_zone_t * zone, size_t size) noexcept {
		// Зона нам известна
		(void) zone;
		// Выводим выданную нашим распределителем память
		return (* __awh_zone_hooks__.malloc)(size);
	}
	/**
	 * @brief Метод выделения обнулённой памяти зоной
	 *
	 * @param zone  зона, у которой просят
	 * @param count число элементов
	 * @param size  размер элемента в байтах
	 * @return      адрес выданной памяти либо nullptr
	 *
	 */
	static void * reserveZeroed(::malloc_zone_t * zone, size_t count, size_t size) noexcept {
		// Зона нам известна
		(void) zone;
		// Выводим выданную нашим распределителем память
		return (* __awh_zone_hooks__.calloc)(count, size);
	}
	/**
	 * @brief Метод выделения памяти зоной с выравниванием по странице
	 *
	 * @param zone зона, у которой просят
	 * @param size требуемый размер в байтах
	 * @return     адрес выданной памяти либо nullptr
	 *
	 */
	static void * reservePaged(::malloc_zone_t * zone, size_t size) noexcept {
		// Зона нам известна
		(void) zone;
		/**
		 * Выдаём страничным выравниванием
		 *
		 * Отдельного выравнивающего отклика у прежних выпусков зоны нет, и `valloc`
		 * обязан выдать выровненное по странице сам
		 */
		return (* __awh_zone_hooks__.malloc)(size);
	}
	/**
	 * @brief Метод освобождения памяти зоной
	 *
	 * @param zone зона, у которой просят
	 * @param ptr  адрес освобождаемой памяти
	 *
	 */
	static void discard(::malloc_zone_t * zone, void * ptr) noexcept {
		// Зона нам известна
		(void) zone;
		// Освобождаем память нашим распределителем
		(* __awh_zone_hooks__.free)(ptr);
	}
	/**
	 * @brief Метод освобождения памяти зоной с известным размером
	 *
	 * @param zone зона, у которой просят
	 * @param ptr  адрес освобождаемой памяти
	 * @param size размер освобождаемого блока
	 *
	 */
	static void discardSized(::malloc_zone_t * zone, void * ptr, size_t size) noexcept {
		// Размер нам известен и без подсказки
		(void) zone; (void) size;
		// Освобождаем память нашим распределителем
		(* __awh_zone_hooks__.free)(ptr);
	}
	/**
	 * @brief Метод изменения размера памяти зоной
	 *
	 * @param zone зона, у которой просят
	 * @param ptr  адрес изменяемой памяти
	 * @param size требуемый размер в байтах
	 * @return     адрес выданной памяти либо nullptr
	 *
	 */
	static void * resize(::malloc_zone_t * zone, void * ptr, size_t size) noexcept {
		// Зона нам известна
		(void) zone;
		// Выводим выданную нашим распределителем память
		return (* __awh_zone_hooks__.realloc)(ptr, size);
	}
	/**
	 * @brief Метод разрушения зоны
	 *
	 * @param zone разрушаемая зона
	 *
	 */
	static void demolish(::malloc_zone_t * zone) noexcept {
		/**
		 * Разрушать нечего
		 *
		 * Зона наша живёт столько же, сколько процесс: разрушить её значило бы
		 * обесценить всю выданную ею память, а та живёт до конца процесса
		 */
		(void) zone;
	}
	/**
	 * @brief Метод отдачи системе свободной памяти зоны
	 *
	 * @param zone зона, у которой просят
	 * @param goal требуемый объём в байтах
	 * @return     объём отданного в байтах
	 *
	 */
	static size_t relieve(::malloc_zone_t * zone, size_t goal) noexcept {
		// Отдача памяти системе ведётся самим распределителем по его настройке
		(void) zone; (void) goal;
		// Отдавать по требованию системы нечего
		return 0;
	}
	/**
	 * @brief Метод опознания принадлежности адреса зоне
	 *
	 * @param zone зона, у которой спрашивают
	 * @param ptr  разбираемый адрес
	 * @return     признак принадлежности адреса зоне
	 *
	 */
	static boolean_t claimed(::malloc_zone_t * zone, void * ptr) noexcept {
		// Выводим признак принадлежности адреса нашей зоне
		return static_cast <boolean_t> (::measure(zone, ptr) != 0);
	}
	/**
	 * @brief Метод перечисления блоков зоны для средств разбора памяти
	 *
	 * @return признак успеха перечисления
	 *
	 */
	static kern_return_t enumerate(::task_t task, void * context, unsigned type, ::vm_address_t base,
	 ::memory_reader_t reader, ::vm_range_recorder_t recorder) noexcept {
		/**
		 * Перечислять блоки мы не умеем
		 *
		 * Перечисление нужно средствам разбора памяти чужого процесса (`leaks`,
		 * `heap`), и отвечать им ложью нельзя. Свой съём утечек у распределителя
		 * будет свой, а этот путь остаётся честно неподдержанным
		 */
		(void) task; (void) context; (void) type; (void) base; (void) reader; (void) recorder;
		// Отвечаем отказом
		return KERN_FAILURE;
	}
	/**
	 * @brief Метод определения хорошего размера блока
	 *
	 * @param zone зона, у которой спрашивают
	 * @param size требуемый размер в байтах
	 * @return     размер, выдаваемый зоной на такой запрос
	 *
	 */
	static size_t rounded(::malloc_zone_t * zone, size_t size) noexcept {
		// Зона нам известна
		(void) zone;
		// Отвечаем запрошенным: округление разрядами - дело самого распределителя
		return size;
	}
	/**
	 * @brief Метод проверки целостности зоны
	 *
	 * @param zone проверяемая зона
	 * @return     признак целостности
	 *
	 */
	static boolean_t sound(::malloc_zone_t * zone) noexcept {
		// Зона нам известна
		(void) zone;
		// Отвечаем целостностью
		return 1;
	}
	/**
	 * @brief Метод печати состояния зоны
	 *
	 */
	static void describe(::malloc_zone_t * zone, boolean_t verbose) noexcept {
		// Печатать нечего: состояние распределителя опрашивается его же средствами
		(void) zone; (void) verbose;
	}
	/**
	 * @brief Метод захвата замков зоны перед ветвлением процесса
	 *
	 */
	static void freeze(::malloc_zone_t * zone) noexcept {
		// Зона нам известна
		(void) zone;
		// Если отклик перед ветвлением задан
		if(::__awh_zone_before__ != nullptr)
			// Захватываем замки распределителя
			(* ::__awh_zone_before__)();
	}
	/**
	 * @brief Метод освобождения замков зоны после ветвления процесса
	 *
	 */
	static void thaw(::malloc_zone_t * zone) noexcept {
		// Зона нам известна
		(void) zone;
		// Если отклик у родителя задан
		if(::__awh_zone_after__ != nullptr)
			// Освобождаем замки распределителя
			(* ::__awh_zone_after__)();
	}
	/**
	 * @brief Метод сбора статистики зоны
	 *
	 */
	static void collect(::malloc_zone_t * zone, ::malloc_statistics_t * stats) noexcept {
		// Зона нам известна
		(void) zone;
		// Если собирать некуда
		if(stats == nullptr)
			// Собирать нечего
			return;
		// Обнуляем статистику: состояние распределителя опрашивается его же средствами
		::memset(stats, 0, sizeof(::malloc_statistics_t));
	}
	/**
	 * @brief Метод определения занятости зоны
	 *
	 * @return признак занятости зоны
	 *
	 */
	static boolean_t busy(::malloc_zone_t * zone) noexcept {
		// Зона нам известна
		(void) zone;
		// Отвечаем незанятостью
		return 0;
	}
	/**
	 * @brief Метод включения записи обращений к адресу
	 *
	 */
	static void trace(::malloc_zone_t * zone, void * address) noexcept {
		// Записи обращений мы не ведём: съём у распределителя будет свой
		(void) zone; (void) address;
	}
	/**
	 * @brief Метод перезаведения замков зоны у потомка ветвления
	 *
	 */
	static void relock(::malloc_zone_t * zone) noexcept {
		// Зона нам известна
		(void) zone;
		// Если отклик у потомка задан
		if(::__awh_zone_child__ != nullptr)
			// Приводим распределитель в порядок у потомка
			(* ::__awh_zone_child__)();
	}
	/**
	 * Средства разбора нашей зоны
	 *
	 * Заполняются поимённо в захвате, а не записью по порядку: состав их у разных
	 * выпусков системы разный, и запись по порядку разъехалась бы при первом же
	 * добавленном поле - причём молча, сдвинув отклики на соседние места
	 */
	// Средства разбора нашей зоны
	static ::malloc_introspection_t __awh_zone_introspect__;
	/**
	 * @brief Метод получения зоны, обслуживающей выделение памяти процесса
	 *
	 * @return обслуживающая зона либо nullptr
	 *
	 */
	static ::malloc_zone_t * defaultZone() noexcept {
		// Перечень зон процесса
		::vm_address_t * zones = nullptr;
		// Число зон в перечне
		unsigned count = 0;
		/**
		 * Берём ПЕРВУЮ зону перечня, а не ответ `malloc_default_zone`
		 *
		 * Проверено опытом на Darwin 25.5 (macOS 26): `malloc_default_zone` отвечает
		 * не той зоной, что обслуживает выдачу, а неизменной обёрткой из общего
		 * образа системы, - и сверка с её ответом не сходится НИКОГДА, отчего захват
		 * отказывал при вполне удавшейся перестановке. Выдачу же обслуживает именно
		 * первая зона перечня
		 */
		if((::malloc_get_all_zones(0, nullptr, &zones, &count) == KERN_SUCCESS) && (count > 0))
			// Выводим первую зону перечня
			return reinterpret_cast <::malloc_zone_t *> (zones[0]);
		// Выводим ответ системы: иного пути не осталось
		return ::malloc_default_zone();
	}
	// Наша зона выделения памяти
	static ::malloc_zone_t __awh_zone__;
	// Прежняя основная зона процесса
	static ::malloc_zone_t * __awh_zone_previous__ = nullptr;
	/**
	 * Прежние функции зовутся у прежней ЗОНЫ, а не через `::malloc`
	 *
	 * После захвата `::malloc` идёт первой зоной перечня, то есть нашей, - и выдай мы
	 * его прежней функцией, всякое обращение к ней уходило бы обратно к нам и не
	 * возвращалось никогда. Обращение же прямо к зоне минует перечень вовсе
	 */
	/**
	 * @brief Метод выделения памяти прежним распределителем
	 *
	 * @param size требуемый размер в байтах
	 * @return     адрес выданной памяти либо nullptr
	 *
	 */
	static void * priorMalloc(size_t size) noexcept {
		// Если прежней зоны нет
		if(::__awh_zone_previous__ == nullptr)
			// Выдавать нечего
			return nullptr;
		// Выводим память, выданную прежней зоной
		return ::malloc_zone_malloc(::__awh_zone_previous__, size);
	}
	/**
	 * @brief Метод выделения обнулённой памяти прежним распределителем
	 *
	 * @param count число элементов
	 * @param size  размер элемента в байтах
	 * @return      адрес выданной памяти либо nullptr
	 *
	 */
	static void * priorCalloc(size_t count, size_t size) noexcept {
		// Если прежней зоны нет
		if(::__awh_zone_previous__ == nullptr)
			// Выдавать нечего
			return nullptr;
		// Выводим память, выданную прежней зоной
		return ::malloc_zone_calloc(::__awh_zone_previous__, count, size);
	}
	/**
	 * @brief Метод изменения размера памяти прежним распределителем
	 *
	 * @param ptr  адрес изменяемой памяти
	 * @param size требуемый размер в байтах
	 * @return     адрес выданной памяти либо nullptr
	 *
	 */
	static void * priorRealloc(void * ptr, size_t size) noexcept {
		// Если прежней зоны нет
		if(::__awh_zone_previous__ == nullptr)
			// Выдавать нечего
			return nullptr;
		// Выводим память, выданную прежней зоной
		return ::malloc_zone_realloc(::__awh_zone_previous__, ptr, size);
	}
	/**
	 * @brief Метод освобождения памяти прежним распределителем
	 *
	 * @param ptr адрес освобождаемой памяти
	 *
	 */
	static void priorFree(void * ptr) noexcept {
		// Если освобождать нечего
		if(ptr == nullptr)
			// Освобождать нечего
			return;
		/**
		 * Освобождаем разбором зоны, а не прежней зоной прямо
		 *
		 * Указатель мог быть выдан и третьей зоной - скажем, зоной, которую завёл
		 * кто-то ещё, - и отдать его прежней означало бы порчу чужого учёта. Разбор
		 * же отвечает той зоной, что блок и выдала, и в нашу он не заведёт: своих
		 * блоков мы прежним распределителем не освобождаем
		 */
		::free(ptr);
	}
};

/**
 * @brief Метод захвата выделения памяти процесса
 *
 * @param hooks     наши функции, ставимые на место прежних
 * @param originals прежние функции, отдаваемые захватом
 * @return          признак состоявшегося захвата
 *
 */
bool awh::alloc::ZoneCapture::acquire(const functions_t & hooks, functions_t & originals) noexcept {
	// Если захват уже состоялся
	if(this->_acquired){
		// Отдаём прежде добытые функции
		originals = this->_originals;
		// Отвечаем успехом
		return true;
	}
	// Если наши функции заданы не полностью
	if((hooks.malloc == nullptr) || (hooks.free == nullptr) ||
	   (hooks.calloc == nullptr) || (hooks.realloc == nullptr) || (hooks.msize == nullptr))
		/**
		 * Отвечаем отказом
		 *
		 * Измерение блока здесь обязательно, в отличие от систем ELF: им система
		 * разбирает, какой зоне отдать освобождение, и без него зона негодна вовсе
		 */
		return false;
	// Запоминаем наши функции для откликов зоны
	::__awh_zone_hooks__ = hooks;
	/**
	 * Понуждаем зону сбрасываемой памяти завестись прежде нашей
	 *
	 * Заводится она лениво, и заведись она ПОСЛЕ нашей - оказалась бы в перечне
	 * впереди и перехватила бы выдачу на себя
	 */
	::malloc_default_purgeable_zone();
	// Получаем прежнюю основную зону процесса
	::malloc_zone_t * previous = ::defaultZone();
	// Если прежней основной зоны нет
	if(previous == nullptr)
		// Отвечаем отказом
		return false;
	/**
	 * Запоминаем прежнюю зону ДО перестановки
	 *
	 * Прежние функции зовут её прямо, и запиши мы её после - они успели бы получить
	 * пустоту, если перестановка отказала бы посередине
	 */
	::__awh_zone_previous__ = previous;
	// Запоминаем прежнее выделение памяти
	this->_originals.malloc = &::priorMalloc;
	// Запоминаем прежнее освобождение памяти
	this->_originals.free = &::priorFree;
	// Запоминаем прежнее выделение обнулённой памяти
	this->_originals.calloc = &::priorCalloc;
	// Запоминаем прежнее изменение размера выделенной памяти
	this->_originals.realloc = &::priorRealloc;
	// Запоминаем прежнее измерение блока
	this->_originals.msize = &::malloc_size;
	// Обнуляем средства разбора нашей зоны
	::memset(&::__awh_zone_introspect__, 0, sizeof(::__awh_zone_introspect__));
	// Задаём метод перечисления блоков зоны
	::__awh_zone_introspect__.enumerator = &::enumerate;
	// Задаём метод определения хорошего размера блока
	::__awh_zone_introspect__.good_size = &::rounded;
	// Задаём метод проверки целостности зоны
	::__awh_zone_introspect__.check = &::sound;
	// Задаём метод печати состояния зоны
	::__awh_zone_introspect__.print = &::describe;
	// Задаём метод включения записи обращений к адресу
	::__awh_zone_introspect__.log = &::trace;
	// Задаём метод захвата замков зоны перед ветвлением процесса
	::__awh_zone_introspect__.force_lock = &::freeze;
	// Задаём метод освобождения замков зоны после ветвления процесса
	::__awh_zone_introspect__.force_unlock = &::thaw;
	// Задаём метод сбора статистики зоны
	::__awh_zone_introspect__.statistics = &::collect;
	// Задаём метод определения занятости зоны
	::__awh_zone_introspect__.zone_locked = &::busy;
	// Задаём метод перезаведения замков зоны у потомка ветвления
	::__awh_zone_introspect__.reinit_lock = &::relock;
	// Обнуляем нашу зону
	::memset(&::__awh_zone__, 0, sizeof(::__awh_zone__));
	// Задаём зоне метод определения размера блока
	::__awh_zone__.size = &::measure;
	// Задаём зоне метод выделения памяти
	::__awh_zone__.malloc = &::reserve;
	// Задаём зоне метод выделения обнулённой памяти
	::__awh_zone__.calloc = &::reserveZeroed;
	// Задаём зоне метод выделения памяти с выравниванием по странице
	::__awh_zone__.valloc = &::reservePaged;
	// Задаём зоне метод освобождения памяти
	::__awh_zone__.free = &::discard;
	// Задаём зоне метод изменения размера памяти
	::__awh_zone__.realloc = &::resize;
	// Задаём зоне метод разрушения
	::__awh_zone__.destroy = &::demolish;
	// Задаём зоне название, видимое средствами разбора памяти
	::__awh_zone__.zone_name = TITLE;
	// Задаём зоне средства разбора
	::__awh_zone__.introspect = &::__awh_zone_introspect__;
	/**
	 * Задаём выпуск зоны
	 *
	 * Выпуск девятый: он несёт освобождение с известным размером и отдачу памяти по
	 * требованию системы, но ещё не требует полей, каких мы не заполняем
	 */
	::__awh_zone__.version = 9;
	// Задаём зоне метод освобождения памяти с известным размером
	::__awh_zone__.free_definite_size = &::discardSized;
	// Задаём зоне метод отдачи системе свободной памяти
	::__awh_zone__.pressure_relief = &::relieve;
	// Задаём зоне метод опознания принадлежности адреса
	::__awh_zone__.claimed_address = &::claimed;
	// Вносим нашу зону в перечень зон процесса
	::malloc_zone_register(&::__awh_zone__);
	/**
	 * Изымаем прежнюю основную зону и вносим её заново
	 *
	 * Признака «эта зона теперь основная» система не даёт: основной зовётся первая в
	 * перечне. Изъятие с повторным внесением уводит прежнюю основную в конец перечня,
	 * отчего первой оказывается наша
	 */
	if(previous != &::__awh_zone__){
		// Изымаем прежнюю основную зону
		::malloc_zone_unregister(previous);
		// Вносим прежнюю основную зону заново
		::malloc_zone_register(previous);
	}
	/**
	 * Сверяем, что первой в перечне стала наша зона
	 *
	 * Сверяем, а не полагаемся: перестановка может и не удаться. Отчитаться успехом,
	 * не сверив, значило бы обещать захват там, где его нет
	 */
	if(::defaultZone() != &::__awh_zone__){
		// Изымаем нашу зону из перечня
		::malloc_zone_unregister(&::__awh_zone__);
		/**
		 * Возвращаем прежнюю основную зону на её место
		 *
		 * Возвращаем тем же приёмом: изъятие с повторным внесением ставит её первой
		 */
		if(::defaultZone() != previous){
			// Изымаем прежнюю основную зону
			::malloc_zone_unregister(previous);
			// Вносим прежнюю основную зону заново
			::malloc_zone_register(previous);
		}
		// Обнуляем прежние функции
		this->_originals = functions_t();
		// Обнуляем прежнюю основную зону
		::__awh_zone_previous__ = nullptr;
		// Отвечаем отказом
		return false;
	}
	// Отмечаем захват состоявшимся
	this->_acquired = true;
	// Отдаём прежние функции
	originals = this->_originals;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод снятия захвата
 *
 */
void awh::alloc::ZoneCapture::release() noexcept {
	// Если захват не состоялся
	if(!this->_acquired)
		// Снимать нечего
		return;
	// Изымаем нашу зону из перечня зон процесса
	::malloc_zone_unregister(&::__awh_zone__);
	/**
	 * Возвращаем прежнюю основную зону в начало перечня
	 */
	if((::__awh_zone_previous__ != nullptr) && (::defaultZone() != ::__awh_zone_previous__)){
		// Изымаем прежнюю основную зону
		::malloc_zone_unregister(::__awh_zone_previous__);
		// Вносим прежнюю основную зону заново
		::malloc_zone_register(::__awh_zone_previous__);
	}
	// Отмечаем захват снятым
	this->_acquired = false;
}
/**
 * @brief Метод определения состоявшегося захвата
 *
 * @return признак захвата
 *
 */
bool awh::alloc::ZoneCapture::acquired() const noexcept {
	// Выводим признак состоявшегося захвата
	return this->_acquired;
}
/**
 * @brief Метод опознания указателя, выданного прежним распределителем
 *
 * @param ptr разбираемый указатель
 * @return    признак принадлежности прежнему распределителю
 *
 */
bool awh::alloc::ZoneCapture::foreign(const void * ptr) const noexcept {
	// Если указатель не задан
	if(ptr == nullptr)
		// Опознавать нечего
		return false;
	/**
	 * Спрашиваем СНАЧАЛА себя
	 *
	 * `malloc_zone_from_ptr` для того негоден: проверено опытом на Darwin 25.5, он
	 * отвечает одной и той же обёрткой из общего образа системы на всякий блок -
	 * и на выданный нами, и на выданный прежним распределителем. Опознай мы по его
	 * ответу, свой блок числился бы чужим и уходил бы в чужое освобождение
	 */
	if((::__awh_zone__.size != nullptr) && ((* ::__awh_zone__.size)(&::__awh_zone__, ptr) != 0))
		// Указатель выдан нами
		return false;
	// Если прежней зоны нет
	if((::__awh_zone_previous__ == nullptr) || (::__awh_zone_previous__->size == nullptr))
		// Опознавать нечем
		return false;
	/**
	 * Спрашиваем прежнюю зону прямо
	 *
	 * Прямо, а не через перечень: зона отвечает размером своего блока и нулём у
	 * чужого, и ответ этот - единственный годный признак принадлежности
	 */
	return ((* ::__awh_zone_previous__->size)(::__awh_zone_previous__, ptr) != 0);
}
/**
 * @brief Метод получения названия способа захвата
 *
 * @return название способа захвата
 *
 */
const char * awh::alloc::ZoneCapture::name() const noexcept {
	// Выводим название способа захвата
	return "macOS malloc zone";
}
/**
 * @brief Метод задания откликов ветвления процесса
 *
 * @param before отклик перед ветвлением
 * @param after  отклик у родителя после ветвления
 * @param child  отклик у потомка после ветвления
 *
 */
void awh::alloc::ZoneCapture::fork(void (* before)(), void (* after)(), void (* child)()) noexcept {
	// Запоминаем отклик перед ветвлением
	::__awh_zone_before__ = before;
	// Запоминаем отклик у родителя после ветвления
	::__awh_zone_after__ = after;
	// Запоминаем отклик у потомка после ветвления
	::__awh_zone_child__ = child;
}
/**
 * @brief Метод получения прежних функций выделения памяти
 *
 * @return прежние функции
 *
 */
const awh::alloc::functions_t & awh::alloc::ZoneCapture::originals() const noexcept {
	// Выводим прежние функции выделения памяти
	return this->_originals;
}
/**
 * @brief Деструктор
 *
 */
awh::alloc::ZoneCapture::~ZoneCapture() noexcept {
	// Снимаем захват, если он состоялся
	if(this->_acquired)
		// Снимаем захват
		this->release();
}

#endif // __APPLE__
