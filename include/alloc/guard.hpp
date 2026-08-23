/**
 * @file guard.hpp
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
 * @brief Заголовочный файл заслонов и карантина — выдача части блоков между
 *        закрытыми страницами и удержание освобождённого от повторной выдачи
 *
 * @section guard_decisions Намеренные решения
 *
 * @details <b>Заслонённые блоки берутся прямо у источника, а не у страничной кучи.</b>
 *          Куча нарезает куски по четыре мегабайта и раздаёт из них логические
 *          страницы по восемь килобайт; заслон же ставится страницей системы, и
 *          запрет посреди куска накрыл бы соседние выдачи, к дефекту касательства не
 *          имеющие.
 *
 *          <b>Заслона два, а не один.</b> Один задний заслон ловит переполнение, но
 *          недобор перед началом блока оставляет незамеченным - его пришлось бы ловить
 *          сличением образца при освобождении, то есть много позже точки дефекта.
 *          Два заслона ловят оба края немедленно, ценою двух страниц на блок; выдача
 *          эта и без того выборочная.
 *
 *          <b>Блок прижимается к заднему заслону.</b> Переполнение на единственный
 *          байт обязано валить обращение, а не попадать в пропуск выравнивания.
 *          Пропуск неизбежен, но лежит он перед блоком, где недобор ловится передним
 *          заслоном.
 *
 *          <b>Освобождённый заслонённый блок закрывается целиком, а не отдаётся.</b>
 *          Отдай мы его системе сразу - адрес достался бы следующей выдаче, и
 *          обращение по висячему указателю прошло бы молча. Закрытая же область валит
 *          обращение в точке дефекта и разбором адреса опознаётся освобождённой.
 *
 *          <b>Удержание закрытых областей ограничено ДВУМЯ границами.</b> Числом - ради
 *          места под адреса у мелких блоков, и объёмом - ради самой памяти у крупных.
 *          Одной лишь границы по числу мало: под заслонами выдаётся до мегабайта, и
 *          четыре тысячи областей держали бы до четырёх гигабайт. Замерено на кругах
 *          нагрузки: 201 мегабайт удержанного при НУЛЕВОМ занятом, причём рост встал не
 *          оттого, что память вернулась, а оттого, что счёт областей упёрся в предел.
 *
 *          <b>Карантин ведётся кольцом записей, а не связью внутри блоков.</b> Связь
 *          внутри блока заняла бы первые байты, и засев освобождённого проверить было
 *          бы нечем: как раз затёртые связью байты и портит запись по висячему
 *          указателю чаще всего. Кольцо же лежит в стороне и блок не трогает вовсе.
 *
 *          <b>Живость блока разряда известна лишь через карантин.</b> Свободный блок
 *          лежит в списках без всякой отметки, а перебор списков под замками у самого
 *          сбоя ненадёжен вдвойне. Оттого разбор отвечает FREED, когда блок найден в
 *          карантине, и LIVE, когда область жива, а в карантине блока нет. С
 *          выключенным карантином второе не значит ничего - о чём и сказано настройкой.
 *
 * \~english
 * @brief Header file of guard pages and quarantine — issuing part of the blocks
 *        between protected pages and holding freed memory back from reuse
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_GUARD__
#define __AWH_ALLOC_GUARD__

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "spin.hpp"
#include "source.hpp"
#include "../sys/global.hpp"

/**
 * Если компилятор принадлежит к Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_GUARD_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_GUARD_INLINE inline __attribute__((always_inline))
#endif

/**
 * @brief Пространство имён фреймворка
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён распределителя памяти
	 *
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Класс заслонов
		 *
		 * \~english
		 * @brief Guard pages class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Guard {
			public:
				// Начальная длина таблицы поиска заслонённого блока в местах
				static constexpr size_t TABLE = 256;
				// Наибольший размер, выдаваемый под заслонами, в байтах
				static constexpr size_t MAXIMUM = (1u * 1024u * 1024u);
				/**
				 * Наибольшее число закрытых областей, удерживаемых от отдачи
				 *
				 * Закрытая область не отдаётся системе затем, чтобы обращение по
				 * освобождённому адресу валило, а не проходило молча. Удерживать их без
				 * счёта нельзя: у долгой работы кончится не память, так место под адреса
				 */
				static constexpr size_t SEALED = 4096;
				/**
				 * Наибольший ОБЪЁМ закрытых областей, удерживаемых от отдачи
				 *
				 * Счёт областей объёма не ограничивает: под заслонами выдаётся до
				 * мегабайта, и четыре тысячи закрытых областей держали бы до четырёх
				 * гигабайт - закрытых, но взятых у системы и однажды тронутых. Замерено
				 * щупом на кругах настройка-нагрузка-выключение: на блоках до
				 * шестнадцати килобайт заслоны держали 201 мегабайт при НУЛЕВОМ занятом,
				 * и рост встал не оттого, что память вернулась, а оттого, что счёт
				 * областей упёрся в предел
				 *
				 * Границы нужны обе: счёт бережёт место под адреса у мелких блоков,
				 * объём - саму память у крупных
				 */
				static constexpr size_t RETAINED = (64u * 1024u * 1024u);
			public:
				/**
				 * \~russian
				 * @brief Состояние заслонов
				 *
				 * \~english
				 * @brief Guard pages state
				 *
				 */
				typedef struct State {
					// Число живых заслонённых блоков
					size_t live;
					// Число закрытых заслонённых блоков, ожидающих отдачи
					size_t sealed;
					// Взято у источника под заслонённые блоки в байтах
					size_t taken;
					// Выдано прикладному коду под заслонами в байтах
					size_t given;
					/**
					 * @brief Конструктор
					 *
					 */
					State() noexcept : live(0), sealed(0), taken(0), given(0) {}
				} state_t;
			private:
				/**
				 * @brief Учётная запись заслонённого блока
				 *
				 */
				typedef struct Record {
					// Адрес выданного прикладному коду блока
					uint8_t * block;
					// Адрес начала взятой у источника области
					uint8_t * base;
					// Размер взятой у источника области в байтах
					size_t span;
					// Затребованный прикладным кодом размер в байтах
					size_t size;
					// Признак закрытой области, ожидающей отдачи
					bool sealed;
					// Следующая закрытая область в очереди на отдачу
					struct Record * queue;
					/**
					 * @brief Конструктор
					 *
					 */
					Record() noexcept :
					 block(nullptr), base(nullptr), span(0), size(0), sealed(false), queue(nullptr) {}
				} record_t;
			private:
				// Источник страниц
				source_t * _source;
				// Замок заслонов
				spin_t _lock;
				/**
				 * Таблица поиска заслонённого блока
				 *
				 * Открытая адресация с удвоением: заслонённых блоков немного по самому
				 * устройству выборки, а искать их приходится на каждом освобождении -
				 * перебор списка для того негоден
				 */
				// Таблица поиска заслонённого блока по адресу
				record_t ** _table;
				// Длина таблицы поиска в местах
				size_t _length;
				// Число записей, внесённых в таблицу
				size_t _enrolled;
				// Текущий кусок памяти под учётные записи
				uint8_t * _meta;
				// Остаток текущего куска памяти под учётные записи
				size_t _metaLeft;
				// Список повторно используемых учётных записей
				record_t * _spare;
				// Начало очереди закрытых областей, старейшая из них
				record_t * _oldest;
				// Конец очереди закрытых областей, младшая из них
				record_t * _newest;
				// Объём закрытых областей, удерживаемых от отдачи, в байтах
				size_t _sealedBytes;
				// Доля выборки: одна выдача из скольких
				std::atomic <size_t> _rate;
				// Счётчик выдач для выборки
				std::atomic <size_t> _counter;
				// Состояние заслонов
				state_t _state;
			private:
				/**
				 * Метка снесённого места таблицы поиска
				 *
				 * Открытая адресация не терпит пустоты на месте снесённой записи: перебор
				 * мест идёт подряд и остановился бы на ней, не дойдя до записи, попавшей
				 * дальше по перебору. Оттого снесённое место метится особым знаком -
				 * занятым для перебора и свободным для записи
				 */
				static record_t * const _tomb;
			private:
				/**
				 * @brief Метод выдачи памяти под учётную запись
				 *
				 * @return адрес выданной памяти либо nullptr
				 *
				 */
				void * meta() noexcept;
				/**
				 * @brief Метод перестроения таблицы поиска
				 *
				 * @param length требуемая длина таблицы в местах
				 * @return       признак перестроения таблицы
				 *
				 */
				bool rehash(const size_t length) noexcept;
				/**
				 * @brief Метод внесения записи в таблицу поиска
				 *
				 * @param record вносимая запись
				 * @return       признак внесения записи
				 *
				 */
				bool enroll(record_t * record) noexcept;
				/**
				 * @brief Метод поиска записи по адресу блока
				 *
				 * @param addr разбираемый адрес
				 * @param exact признак поиска ровно по началу блока
				 * @return      найденная запись либо nullptr
				 *
				 */
				record_t * lookup(const void * addr, const bool exact) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения заслонов
				 *
				 * @param source источник страниц
				 * @return       признак заведения заслонов
				 *
				 * \~english
				 * @brief Method of initializing the guard pages
				 *
				 */
				bool init(source_t * source) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия заслонов
				 *
				 * \~english
				 * @brief Method of shutting down the guard pages
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения надобности заслона очередной выдаче
				 *
				 * @note Метод меняет счётчик выборки и оттого зовётся ровно однажды на
				 *       выдачу; повторный вопрос о том же запросе ответит иначе
				 *
				 * @param size требуемый размер в байтах
				 * @return     признак надобности заслона
				 *
				 * \~english
				 * @brief Method of determining whether the next allocation needs a guard
				 *
				 */
				/**
				 * \~russian
				 * @brief Метод выборки выдачи под заслоны
				 *
				 * @note Холодный хвост `wanted`: сюда приходят лишь при включённых заслонах
				 *
				 * @param size требуемый размер в байтах
				 * @return     признак того, что выборка взяла эту выдачу
				 *
				 * \~english
				 * @brief Method of sampling an allocation for guarding
				 *
				 */
				bool sampled(const size_t size) noexcept;
				AWH_GUARD_INLINE bool wanted(const size_t size) noexcept {
					/**
					 * Отвечаем отказом, не заходя в файл кода
					 *
					 * Заслоны выключены у подавляющего большинства приложений - и вопрос этот стоит
					 * на пути КАЖДОЙ выдачи. Профиль `perf` на Debian отдавал методу
					 * 1.8 % времени, и то была цена вызова: работы здесь одно нестрогое
					 * чтение
					 */
					// Если заслоны выключены
					if(this->_rate.load(std::memory_order_relaxed) == 0)
						// Заслон не нужен
						return false;
					// Уходим холодным путём: заслоны включены, идёт выборка
					return this->sampled(size);
				}
				/**
				 * \~russian
				 * @brief Метод выдачи заслонённого блока
				 *
				 * @param size требуемый размер в байтах
				 * @return     адрес выданного блока либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating a guarded block
				 *
				 */
				void * alloc(const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения заслонённого блока
				 *
				 * @note Область при этом закрывается целиком и системе не отдаётся:
				 *       обращение по освобождённому адресу обязано валить, а не проходить
				 *
				 * @note Признак принадлежности отдаётся ОТДЕЛЬНО от размера: повторное
				 *       освобождение заслонённого блока даёт нулевой размер при вполне
				 *       нашем адресе, и не различай мы этих случаев - такой указатель ушёл
				 *       бы прежнему распределителю, то есть чужой куче
				 *
				 * @param ptr  адрес освобождаемого блока
				 * @param mine признак принадлежности адреса заслонам
				 * @return     затребованный размер блока в байтах, либо нуль
				 *
				 * \~english
				 * @brief Method of freeing a guarded block
				 *
				 */
				size_t free(void * ptr, bool * mine) noexcept;
				/**
				 * \~russian
				 * @brief Метод определения принадлежности адреса заслонам
				 *
				 * @param ptr  разбираемый адрес
				 * @param size размер блока, если он определён
				 * @return     признак принадлежности адреса заслонам
				 *
				 * \~english
				 * @brief Method of determining whether the address belongs to the guards
				 *
				 */
				bool owner(const void * ptr, size_t * size) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора адреса обращения
				 *
				 * @param addr   разбираемый адрес
				 * @param begin  адрес начала блока, если он определён
				 * @param size   размер блока, если он определён
				 * @param offset смещение разбираемого адреса от начала блока
				 * @param sealed признак освобождённого блока
				 * @return       признак принадлежности адреса заслонённой области
				 *
				 * \~english
				 * @brief Method of resolving an access address
				 *
				 */
				bool resolve(const void * addr, const void ** begin, size_t * size, ptrdiff_t * offset, bool * sealed) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод задания доли выборки
				 *
				 * @param rate одна выдача из скольких: нуль - заслоны выключены
				 *
				 * \~english
				 * @brief Method of setting the sampling rate
				 *
				 */
				void rate(const size_t rate) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения состояния заслонов
				 *
				 * @return состояние заслонов
				 *
				 * \~english
				 * @brief Method of getting the guard pages state
				 *
				 */
				state_t state() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата замка перед ветвлением процесса
				 *
				 * \~english
				 * @brief Method of acquiring the lock before process forking
				 *
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод отпускания замка после ветвления процесса
				 *
				 * \~english
				 * @brief Method of releasing the lock after process forking
				 *
				 */
				void resume() noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения замка в порядок у потомка ветвления
				 *
				 * \~english
				 * @brief Method of resetting the lock in the forked child
				 *
				 */
				void adopt() noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Guard() noexcept;
		} guard_t;
		/**
		 * \~russian
		 * @brief Класс карантина освобождённой памяти
		 *
		 * \~english
		 * @brief Freed memory quarantine class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Quarantine {
			public:
				// Знак засева освобождаемой памяти
				static constexpr uint8_t JUNK = 0xDE;
				// Наименьшее число мест в кольце карантина
				static constexpr size_t MINIMUM = 256;
				// Наибольшее число мест в кольце карантина
				static constexpr size_t LIMIT = 65536;
				// Доля объёма карантина, приходящаяся на одно место кольца, в байтах
				static constexpr size_t SHARE = 512;
			public:
				/**
				 * \~russian
				 * @brief Состояние карантина
				 *
				 * \~english
				 * @brief Quarantine state
				 *
				 */
				typedef struct State {
					// Число блоков, удерживаемых карантином
					size_t held;
					// Объём удерживаемой карантином памяти в байтах
					size_t bytes;
					// Число блоков, прошедших карантин за время работы
					size_t passed;
					// Число блоков, испорченных записью после освобождения
					size_t spoiled;
					// Адрес блока, испорченного записью после освобождения, первого по счёту
					const void * culprit;
					// Смещение первой порчи от начала испорченного блока
					size_t offset;
					/**
					 * @brief Конструктор
					 *
					 */
					State() noexcept :
					 held(0), bytes(0), passed(0), spoiled(0), culprit(nullptr), offset(0) {}
				} state_t;
			private:
				/**
				 * @brief Место кольца карантина
				 *
				 */
				typedef struct Slot {
					// Адрес удерживаемого блока
					void * block;
					// Размер удерживаемого блока в байтах
					size_t size;
					// Номер разряда, которому принадлежит блок
					size_t index;
				} slot_t;
			private:
				// Источник страниц
				source_t * _source;
				// Замок карантина
				spin_t _lock;
				// Кольцо удерживаемых блоков
				slot_t * _ring;
				// Размер кольца в местах
				size_t _length;
				// Размер взятой под кольцо области в байтах
				size_t _region;
				// Место записи очередного блока
				size_t _head;
				// Место изъятия старейшего блока
				size_t _tail;
				// Число удерживаемых блоков
				size_t _held;
				// Объём удерживаемой памяти в байтах
				size_t _bytes;
				// Потолок объёма карантина в байтах
				size_t _limit;
				// Признак засева удерживаемой памяти
				bool _junk;
				// Состояние карантина
				state_t _state;
			private:
				/**
				 * @brief Метод сличения засева удерживаемого блока
				 *
				 * @param block разбираемый блок
				 * @param size  размер блока в байтах
				 * @return      признак сохранности засева
				 *
				 */
				bool intact(const void * block, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения карантина
				 *
				 * @param source источник страниц
				 * @param limit  потолок объёма карантина в байтах: нуль - карантин выключен
				 * @return       признак заведения карантина
				 *
				 * \~english
				 * @brief Method of initializing the quarantine
				 *
				 */
				bool init(source_t * source, const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия карантина
				 *
				 * @note Удерживаемые блоки при этом НЕ возвращаются: возвращать их
				 *       некуда - слои, которым они принадлежат, к этому мигу уже сняты
				 *
				 * \~english
				 * @brief Method of shutting down the quarantine
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод удержания освобождённого блока
				 *
				 * @note Вытеснением карантин НЕ занимается: принявший блок волен
				 *       переполниться, и звать `release` до опустошения обязан тот, кто
				 *       знает, куда возвращать вытесненное
				 *
				 * @param ptr    адрес удерживаемого блока
				 * @param size   размер удерживаемого блока в байтах
				 * @param index  номер разряда, которому принадлежит блок
				 * @return       признак принятия блока карантином
				 *
				 * \~english
				 * @brief Method of holding a freed block
				 *
				 */
				bool hold(void * ptr, const size_t size, const size_t index) noexcept;
				/**
				 * \~russian
				 * @brief Метод изъятия старейшего удерживаемого блока
				 *
				 * @note Блок изымается лишь когда карантин переполнен объёмом либо
				 *       числом мест; пока он вмещается - метод отвечает пустотой
				 *
				 * @param size  размер изъятого блока в байтах
				 * @param index номер разряда изъятого блока
				 * @return      адрес изъятого блока либо nullptr
				 *
				 * \~english
				 * @brief Method of taking out the oldest held block
				 *
				 */
				void * release(size_t * size, size_t * index) noexcept;
				/**
				 * \~russian
				 * @brief Метод определения удержания адреса карантином
				 *
				 * @param addr   разбираемый адрес
				 * @param begin  адрес начала блока, если он определён
				 * @param size   размер блока, если он определён
				 * @return       признак удержания адреса карантином
				 *
				 * \~english
				 * @brief Method of determining whether the address is held by the quarantine
				 *
				 */
				bool held(const void * addr, const void ** begin, size_t * size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод задания потолка объёма карантина
				 *
				 * @param limit потолок объёма в байтах: нуль - карантин выключен
				 *
				 * \~english
				 * @brief Method of setting the quarantine volume ceiling
				 *
				 */
				void limit(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод задания засева удерживаемой памяти
				 *
				 * @param junk признак засева
				 *
				 * \~english
				 * @brief Method of setting the junk filling of held memory
				 *
				 */
				void junk(const bool junk) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения состояния карантина
				 *
				 * @return состояние карантина
				 *
				 * \~english
				 * @brief Method of getting the quarantine state
				 *
				 */
				state_t state() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата замка перед ветвлением процесса
				 *
				 * \~english
				 * @brief Method of acquiring the lock before process forking
				 *
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод отпускания замка после ветвления процесса
				 *
				 * \~english
				 * @brief Method of releasing the lock after process forking
				 *
				 */
				void resume() noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения замка в порядок у потомка ветвления
				 *
				 * \~english
				 * @brief Method of resetting the lock in the forked child
				 *
				 */
				void adopt() noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Quarantine() noexcept;
		} quarantine_t;
	};
};

#endif // __AWH_ALLOC_GUARD__
