/**
 * @file central.hpp
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
 * \~russian
 * @brief Заголовочный файл центральных списков свободных блоков
 *
 * @section central_decisions Намеренные решения
 *
 * @details <b>Связь свободных блоков лежит в них самих.</b> Отдельная таблица связей
 *          стоила бы памяти на каждый блок и обращения к ней на каждом выделении.
 *          Свободный же блок никем не читается, и первые восемь его байт вольны
 *          держать указатель на следующий: наименьший разряд равен шестнадцати байтам,
 *          места хватает всегда.
 *
 *          <b>Замок у каждого разряда свой, а у кучи общий.</b> Один общий замок
 *          свёл бы весь распределитель к очереди из потоков. Порядок захвата всегда
 *          один - сперва разряд, затем куча, - и обратного порядка нет нигде, оттого
 *          взаимной блокировки не возникает.
 *
 *          <b>Блоки передаются пачками, а не поштучно.</b> Захват замка стоит дороже
 *          самой передачи, оттого поток-локальный кэш забирает и возвращает сразу
 *          связанную цепочку блоков, платя за замок однажды на всю пачку.
 *
 *          <b>Разряд области помечается в самой куче.</b> Освобождение обязано узнать
 *          разряд по одному лишь адресу, и отдельная таблица «адрес - разряд» была бы
 *          вторым учётом того же. Куча же и без того хранит область по каждой её
 *          странице, - метка кладётся туда же.
 *
 * \~english
 * @brief Header file of the central free lists
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_CENTRAL__
#define __AWH_ALLOC_CENTRAL__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "spin.hpp"
#include "pages.hpp"
#include "classes.hpp"
#include "../sys/global.hpp"

/**
 * Если компилятор принадлежит к Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_CENTRAL_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_CENTRAL_INLINE inline __attribute__((always_inline))
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
		 * @brief Класс центральных списков свободных блоков
		 *
		 * \~english
		 * @brief Central free lists class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Central {
			public:
				// Наибольшее число блоков в одной передаваемой пачке
				static constexpr size_t BATCH = 64;
			public:
				/**
				 * \~russian
				 * @brief Состояние центральных списков
				 *
				 * \~english
				 * @brief Central free lists state
				 *
				 */
				typedef struct State {
					// Лежит в центральных списках в байтах
					size_t cached;
					// Выдано кэшам потоков в байтах
					size_t live;
					// Нарезано областей под разряды
					size_t regions;
					/**
					 * @brief Конструктор
					 *
					 */
					State() noexcept : cached(0), live(0), regions(0) {}
				} state_t;
			private:
				/**
				 * @brief Центральный список одного разряда
				 *
				 */
				typedef struct List {
					// Замок разряда
					spin_t lock;
					// Голова списка свободных блоков
					void * free;
					// Число свободных блоков в списке
					size_t count;
					// Число блоков, выданных кэшам потоков
					size_t live;
					// Число нарезанных под разряд областей
					size_t regions;
					/**
					 * @brief Конструктор
					 *
					 */
					List() noexcept : lock(), free(nullptr), count(0), live(0), regions(0) {}
				} list_t;
			private:
				// Страничная куча
				pages_t * _pages;
				// Разряды размеров
				classes_t * _classes;
			public:
				/**
				 * Потолок придержки областей сверх разрядов по умолчанию в байтах
				 *
				 * <b>Нуль: придержка ВЫКЛЮЧЕНА по умолчанию, и это решение замера.</b>
				 * Она ускоряет круг «выдал - освободил» на областях сверх разрядов в
				 * полтора раза (73 563 → 113 074 тысяч действий в секунду), но роняет
				 * рост области перевыдачей вчетверо (18 050 → 3 913): растёт область за
				 * счёт СВОБОДНЫХ соседей, а придержанная для кучи занята. По порогу
				 * скорости обмен убыточен - «крупные блоки» не выигрываются и с нею
				 * (у tcmalloc 115 315), а «рост перевыдачей» был единственным
				 * сценарием, где порог держался на всех системах
				 *
				 * Приложению, не растящему области перевыдачей, придержка даётся
				 * настройкой: обмен этот - его выбор, а не наш
				 */
				// Потолок придержки областей сверх разрядов по умолчанию
				static constexpr size_t DEFAULT_KEPT = 0;
				/**
				 * Наибольший размер придерживаемой области в страницах кучи
				 *
				 * Придержка и рост области НА МЕСТЕ враждебны друг другу: растёт область
				 * за счёт свободных соседей, а придержанная для кучи занята. Придержка
				 * без предела роняла «рост перевыдачей» вчетверо, отбирая единственный
				 * сценарий, где порог держался на всех системах
				 */
				// Наибольший размер придерживаемой области в страницах кучи
				static constexpr size_t KEPT_PAGES = 8;
			private:
				/**
				 * @brief Запись придержанной области сверх разрядов
				 *
				 * @details Кладётся в САМУ придержанную область: та свободна, а держать под
				 *          неё отдельную память значило бы просить память у распределителя
				 *          изнутри его же освобождения
				 *
				 */
				typedef struct Kept {
					// Следующая придержанная область того же размера
					struct Kept * next;
					// Размер области в страницах кучи
					size_t pages;
				} kept_t;
			private:
				// Замок страничной кучи
				spin_t _heap;
				// Центральные списки по разрядам
				list_t _lists[Classes::LIMIT];
			private:
				/**
				 * Придержка освобождённых областей сверх разрядов
				 *
				 * Ниже границы разрядов выдачу держит кэш потока, выше размера куска -
				 * придержка слоя крупных выдач, а МЕЖДУ ними всякое действие ходило в
				 * страничную кучу: поиск области, изъятие из списка, дробление, пометка
				 * всех её страниц - и всё это под замком кучи. Счётчик показывал ровно один
				 * вызов `Pages::alloc` и один `Pages::mark` на каждую выдачу
				 *
				 * Придержанная область куче НЕ возвращается: она остаётся выданной и ждёт
				 * следующего запроса того же размера. Отдаётся - по просьбе отдачи памяти
				 * либо сверх потолка, заданного настройкой; нулевой потолок возвращает
				 * прежнее поведение дословно
				 */
				// Замок придержки, отдельный от замка кучи
				spin_t _reserve;
				// Придержанные области по числу страниц
				kept_t * _kept[Pages::PAGES + 1];
				// Объём придержанного в байтах
				size_t _keptBytes;
				// Потолок придержки в байтах
				size_t _keptCeiling;
			private:
				/**
				 * \~russian
				 * @brief Метод нарезки новой области под разряд
				 *
				 * @note Зовётся из-под замка разряда и захватывает замок кучи сам
				 *
				 * @param index номер разряда
				 * @return      признак нарезки области
				 *
				 * \~english
				 * @brief Method of carving a new region for a class
				 *
				 */
				bool carve(const size_t index) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения центральных списков
				 *
				 * @param pages   страничная куча
				 * @param classes разряды размеров
				 * @return        признак заведения списков
				 *
				 * \~english
				 * @brief Method of initializing the central free lists
				 *
				 */
				bool init(pages_t * pages, classes_t * classes) noexcept;
				/**
				 * \~russian
				 * @brief Метод сброса центральных списков
				 *
				 * @note Память кучи не отдаёт: её отдаёт сама куча при разрушении
				 *
				 * \~english
				 * @brief Method of resetting the central free lists
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод изъятия пачки блоков разряда
				 *
				 * @param index номер разряда
				 * @param head  голова изъятой цепочки блоков
				 * @param tail  хвост изъятой цепочки блоков
				 * @param count требуемое число блоков
				 * @return      действительно изъятое число блоков
				 *
				 * \~english
				 * @brief Method of fetching a batch of blocks of a class
				 *
				 */
				size_t fetch(const size_t index, void ** head, void ** tail, const size_t count) noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата пачки блоков разряда
				 *
				 * @param index номер разряда
				 * @param head  голова возвращаемой цепочки блоков
				 * @param tail  хвост возвращаемой цепочки блоков
				 * @param count число возвращаемых блоков
				 *
				 * \~english
				 * @brief Method of returning a batch of blocks of a class
				 *
				 */
				void back(const size_t index, void * head, void * tail, const size_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выдачи памяти сверх разрядов
				 *
				 * @param size требуемый размер в байтах
				 * @return     адрес выданной памяти либо nullptr
				 *
				 * \~english
				 * @brief Method of allocating memory beyond the size classes
				 *
				 */
				/**
				 * @param served действительно выданный размер в байтах, либо nullptr
				 */
				void * alloc(const size_t size, size_t * served = nullptr) noexcept;
				/**
				 * \~russian
				 * @brief Метод расширения выданной сверх разрядов области на месте
				 *
				 * @note Удаётся не всегда: соседняя область вправе быть занята либо мала.
				 *       Отказ здесь не ошибка - звавший переносит содержимое обычным путём
				 *
				 * @param addr адрес начала расширяемой области
				 * @param size требуемый размер в байтах
				 * @return     признак состоявшегося расширения
				 *
				 * \~english
				 * @brief Method of expanding an allocated region in place
				 *
				 */
				bool expand(void * addr, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод возврата памяти, выданной сверх разрядов
				 *
				 * @param addr адрес возвращаемой памяти
				 * @param now  текущее время в миллисекундах
				 * @return     признак возврата памяти
				 *
				 * \~english
				 * @brief Method of returning memory allocated beyond the size classes
				 *
				 */
				bool free(void * addr, const uint64_t now, const bool keeping = true) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения разряда, которому принадлежит адрес
				 *
				 * @param addr  разбираемый адрес
				 * @param index номер разряда, либо LIMIT если память выдана сверх разрядов
				 * @param begin адрес начала области, которой принадлежит адрес
				 * @param size  размер области в байтах
				 * @return      признак того, что адрес выдан нами
				 *
				 * \~english
				 * @brief Method of determining the class an address belongs to
				 *
				 */
				/**
				 * \~russian
				 * @brief Метод быстрого опознания блока разряда
				 *
				 * @details Отвечает номером разряда одним чтением метки страницы, не
				 *          трогая ни указателей страниц, ни учётной записи области
				 *
				 * @note Отказ означает лишь «быстрым путём не вышло»: разбор идёт полным
				 *       путём через `owner`
				 *
				 * @param addr  разбираемый адрес
				 * @param index номер разряда, которому принадлежит адрес
				 * @param hint  место хранения подсказки поиска
				 * @return      признак опознанного блока разряда
				 *
				 * \~english
				 * @brief Method of quickly identifying a size class block
				 *
				 */
				AWH_CENTRAL_INLINE bool sorted(const void * addr, size_t * index, void ** hint = nullptr) noexcept {
					// Если куча не заведена либо разбирать нечего
					if((this->_pages == nullptr) || (addr == nullptr) || (index == nullptr))
						// Разбирать нечего
						return false;
					// Получаем метку разряда по адресу
					const uint8_t tag = this->_pages->classify(addr, hint);
					// Если метки у страницы нет
					if(tag == 0)
						// Быстрым путём не вышло
						return false;
					// Записываем номер разряда: метка на единицу больше него
					(* index) = static_cast <size_t> (tag - 1);
					// Отвечаем успехом
					return true;
				}
				AWH_CENTRAL_INLINE bool owner(const void * addr, size_t * index, void ** begin, size_t * size, void ** hint = nullptr) noexcept {
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
						 * Замок здесь стоял на пути КАЖДОГО освобождения и обращал его в
						 * очередь: восемь потоков на мелкой выдаче давали 285 наносекунд на
						 * действие против 84 без него, и время на действие РОСЛО с числом
						 * потоков - верный признак очереди
						 *
						 * Читать без замка позволено потому, что живая область неизменна:
						 * дробят и сливают лишь свободные, а у живой ни границы, ни метка не
						 * меняются, пока её не освободят. Таблица же поиска куска сменяется
						 * целой записью, и читатель берёт её одним неделимым обращением
						 *
						 * Исключение одно - РОСТ НА МЕСТЕ: он поглощает свободных соседей и
						 * раздвигает границу живой области. Читателя это не задевает по трём
						 * причинам. Растит область САМ владелец блока, и всякий, кто в тот же
						 * миг спрашивает о ТОМ ЖЕ блоке, состязается с ним и без нас.
						 * Поглощаются лишь СВОБОДНЫЕ соседи, за которыми живых блоков нет, -
						 * стало быть, чужой блок таким ростом не задевается вовсе. А
						 * поглощённые учётные записи возвращаются в оборот лишь после того,
						 * как указатели куска переведены на расширенную область: читатель,
						 * успевший взять запись соседа, читает её, а не переписанную заново
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
			public:
				/**
				 * \~russian
				 * @brief Метод отдачи системе свободной памяти
				 *
				 * @param now текущее время в миллисекундах
				 * @param all отдавать всё, не глядя на отсрочку
				 * @return    объём отданной системе памяти в байтах
				 *
				 * \~english
				 * @brief Method of returning free memory to the system
				 *
				 */
				size_t purge(const uint64_t now, const bool all) noexcept;
				/**
				 * \~russian
				 * @brief Метод задания порядка отдачи памяти системе
				 *
				 * @param delay отсрочка в миллисекундах: -1 - не отдавать вовсе
				 * @param block наименьший отдаваемый кусок в байтах
				 *
				 * \~english
				 * @brief Method of setting the memory release policy
				 *
				 */
				void policy(const int64_t delay, const size_t block) noexcept;
				/**
				 * \~russian
				 * @brief Метод задания потолка взятого у источника
				 *
				 * @param limit потолок в байтах: нуль - без потолка
				 *
				 * \~english
				 * @brief Method of setting the ceiling of memory taken from the source
				 *
				 */
				void ceiling(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод задания потолка придержки областей сверх разрядов
				 *
				 * @note Нуль выключает придержку и отдаёт придержанное куче немедленно
				 *
				 * @param limit потолок придержки в байтах
				 *
				 * \~english
				 * @brief Method of setting the ceiling of the span reserve
				 *
				 */
				void keeper(const size_t limit) noexcept;
				/**
				 * \~russian
				 * @brief Метод занятия области у кучи
				 *
				 * @note Нужен затем, что у систем ELF куча заводится задолго до захвата -
				 *       с первой выдачи стандартной библиотеки; заказанное приложением
				 *       при заведении там применить неоткуда
				 *
				 * @param arena    занимаемая область в байтах
				 * @param confined запрет обращаться к источнику сверх занятого
				 * @return         признак занятия требуемой области
				 *
				 * \~english
				 * @brief Method of occupying an area of the heap
				 *
				 * @param arena    area to occupy in bytes
				 * @param confined ban on going to the source beyond what is occupied
				 * @return         flag of the required area having been occupied
				 *
				 */
				bool occupy(const size_t arena, const bool confined) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод взятия у кучи области с ожиданием изымаемых
				 *
				 * @note Обход отдачи изымает свободные области из списков на время
				 *       обращения к системе, и под заданным потолком кучи выдача упирается
				 *       в пустоту там, где память есть - она изъята и вот-вот вернётся.
				 *       Замерено на OpenBSD: восемь нехваток на восьми потоках, и ВСЕ
				 *       восемь пришлись ровно на изъятые области. Оттого отказ при
				 *       ненулевом числе изъятых - повод подождать, а не отвечать отказом
				 *
				 * @param pages требуемое число страниц кучи
				 * @return      адрес выданной области либо nullptr
				 *
				 * \~english
				 * @brief Method of taking a span from the heap, awaiting withheld ones
				 *
				 * @param pages required number of heap pages
				 * @return      address of the allocated span or nullptr
				 *
				 */
				void * take(const size_t pages, size_t * served = nullptr) noexcept;
				/**
				 * \~russian
				 * @brief Метод взятия области у придержки
				 *
				 * @param pages требуемое число страниц кучи
				 * @return      адрес придержанной области либо nullptr
				 *
				 * \~english
				 * @brief Method of taking a span from the reserve
				 *
				 */
				void * recall(const size_t pages) noexcept;
				/**
				 * \~russian
				 * @brief Метод придержки освобождённой области
				 *
				 * @note Отвечает отказом, когда придерживать не следует: потолок исчерпан,
				 *       область слишком велика либо придержка выключена вовсе. Отказ не
				 *       ошибка - область идёт куче обычным путём
				 *
				 * @param addr  адрес освобождаемой области
				 * @param pages размер области в страницах кучи
				 * @return      признак придержки области
				 *
				 * \~english
				 * @brief Method of reserving a freed span
				 *
				 */
				bool keep(void * addr, const size_t pages) noexcept;
				/**
				 * \~russian
				 * @brief Метод отдачи придержанных областей куче
				 *
				 * @param all отдавать всё, не глядя на потолок
				 * @return    объём отданного куче в байтах
				 *
				 * \~english
				 * @brief Method of returning the reserved spans to the heap
				 *
				 */
				size_t unkeep(const bool all) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения упёртости кучи в потолок
				 *
				 * @return признак упёртости кучи в потолок
				 *
				 * \~english
				 * @brief Method of determining whether the heap has hit its ceiling
				 *
				 */
				bool jammed() noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска области кучи, которой принадлежит адрес
				 *
				 * @note Ходит к куче ПОД ЗАМКОМ, и в том вся суть метода. Прежде разбор
				 *       адреса (`Allocator::resolve`) обращался к куче напрямую, минуя
				 *       замок, - а куча в это же время правила список кусков в `Pages::grow`.
				 *       ThreadSanitizer называл гонку прямо: запись в pages.cpp:457 против
				 *       чтения в pages.cpp:972
				 *
				 * @note Метод зовётся из разбора сбоя обращения, а не из обработчика
				 *       сигнала: замок обработчику брать нельзя
				 *
				 * @param addr  разбираемый адрес
				 * @param begin адрес начала найденной области
				 * @param pages размер найденной области в страницах кучи
				 * @param live  признак выданной наружу области
				 * @return      признак того, что адрес принадлежит куче
				 *
				 * \~english
				 * @brief Method of locating the heap region an address belongs to
				 *
				 */
				bool locate(const void * addr, const void ** begin, size_t * pages, bool * live) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения состояния центральных списков
				 *
				 * @return состояние списков
				 *
				 * \~english
				 * @brief Method of getting the central free lists state
				 *
				 */
				state_t state() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения состояния страничной кучи
				 *
				 * @return состояние кучи
				 *
				 * \~english
				 * @brief Method of getting the page heap state
				 *
				 */
				Pages::state_t heap() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата всех замков перед ветвлением процесса
				 *
				 * \~english
				 * @brief Method of acquiring all locks before forking
				 *
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения всех замков после ветвления процесса
				 *
				 * \~english
				 * @brief Method of releasing all locks after forking
				 *
				 */
				void resume() noexcept;
				/**
				 * \~russian
				 * @brief Метод принудительного освобождения всех замков у потомка
				 *
				 * @note Потомку ветвления достаётся один поток, а замки, захваченные
				 *       прочими потоками родителя, остаются захваченными навсегда
				 *
				 * \~english
				 * @brief Method of forcibly releasing all locks in the forked child
				 *
				 */
				void adopt() noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Central() noexcept;
		} central_t;
	};
};

#endif // __AWH_ALLOC_CENTRAL__
