/**
 * @file alloc.cpp
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
#include <alloc/alloc.hpp>
#include <alloc/guard.hpp>
#include <alloc/profile.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <thread>
#include <chrono>

/**
 * Наши модули
 */
#include <alloc/cache.hpp>
#include <alloc/central.hpp>
#include <alloc/classes.hpp>
#include <alloc/pages.hpp>
#include <alloc/spin.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <new>
#include <atomic>
#include <cstring>
#include <chrono>

/**
 * Ветвление процесса есть не у всех систем
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#include <pthread.h>
	#include <unistd.h>
#else
	#include <windows.h>
#endif

/**
 * Подключаем захват, свой у каждого приёма
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <alloc/pe.hpp>
#elif defined(__APPLE__)
	#include <alloc/zone.hpp>
#else
	#include <alloc/elf.hpp>
#endif

/**
 * @brief Пространство имён внутреннего устройства распределителя
 *
 */
namespace {
	/**
	 * Признак готовности распределителя
	 *
	 * Заводится он ЛЕНИВО, при первом же выделении, а не при захвате: у систем ELF
	 * наши имена заслоняют прежние с самого начала процесса, и первое выделение
	 * случается задолго до того, как приложение доберётся до `capture`
	 */
	// Распределитель не заведён вовсе
	static constexpr int32_t STAGE_COLD = 0;
	// Распределитель заводится прямо сейчас
	static constexpr int32_t STAGE_WARM = 1;
	// Распределитель заведён и готов
	static constexpr int32_t STAGE_READY = 2;
	// Ступень готовности распределителя
	static std::atomic <int32_t> stage(STAGE_COLD);
	/**
	 * Область первоначальной выдачи
	 *
	 * Пока распределитель заводится, выделять память ему нечем, а обращения к нему
	 * идут: их шлёт и сама библиотека времени исполнения, и заведение ключа
	 * завершения потока. Область эта невелика, лежит в неизменяемых данных и никогда
	 * не освобождается - на заведение того довольно
	 */
	// Размер области первоначальной выдачи в байтах
	static constexpr size_t BOOTSTRAP = (256u * 1024u);
	// Область первоначальной выдачи
	static uint8_t bootstrap[BOOTSTRAP];
	// Занято в области первоначальной выдачи
	static std::atomic <size_t> bootstrapUsed(0);
	/**
	 * Устройство распределителя
	 *
	 * Лежит в неизменяемых данных и заводится размещающим созданием, а не как
	 * обычный объект: обычный завёлся бы порядком заведения неизменяемых данных, а
	 * первое обращение к распределителю случается прежде того
	 */
	typedef struct Machinery {
		// Источник страниц по умолчанию
		awh::alloc::SystemSource system;
		// Страничная куча
		awh::alloc::pages_t pages;
		// Разряды размеров
		awh::alloc::classes_t classes;
		// Центральные списки
		awh::alloc::central_t central;
		// Поток-локальные кэши
		awh::alloc::caches_t caches;
		// Заслоны выборочной выдачи
		awh::alloc::guard_t guard;
		// Карантин освобождённой памяти
		awh::alloc::quarantine_t quarantine;
		// Съём стека вызовов
		awh::alloc::trace_t trace;
		// Учёт мест выдачи памяти
		awh::alloc::profile_t profile;
		/**
		 * @brief Конструктор
		 *
		 */
		Machinery() noexcept :
		 system(), pages(), classes(), central(), caches(), guard(), quarantine(),
		 trace(), profile() {}
	} machinery_t;
	// Место под устройство распределителя
	alignas(16) static uint8_t space[sizeof(machinery_t)];
	// Устройство распределителя
	static machinery_t * machinery = nullptr;
	// Подменённый источник страниц
	static awh::alloc::source_t * substitute = nullptr;
	// Действующие настройки распределителя
	static awh::alloc::options_t settings;
	// Прежние функции выделения памяти
	static awh::alloc::functions_t originals;
	// Признак состоявшегося захвата
	static std::atomic <bool> seized(false);
	/**
	 * Номер процесса, которому принадлежит заведённый распределитель
	 *
	 * Нужен запасным путём опознания ветвления. `pthread_atfork` не срабатывает, если
	 * `fork` позвали в обход обёртки библиотеки времени исполнения - прямым системным
	 * вызовом или из обработчика сигнала, - и замки, захваченные не пережившими
	 * ветвление потоками, остаются захваченными навсегда. Сличение же номера процесса
	 * отвечает правду всегда: у потомка он иной
	 */
	// Номер процесса, которому принадлежит распределитель
	static std::atomic <int64_t> holder(0);
	/**
	 * Признак выданных когда-либо заслонённых блоков
	 *
	 * Держится здесь, а не спрашивается у заслонов: спросить их значит взять замок, а
	 * стоит этот вопрос на пути КАЖДОГО освобождения - и при выключенных заслонах тоже.
	 * Признак же читается одним обращением к памяти и при лжи снимает вопрос вовсе
	 *
	 * Взведённый, он НЕ снимается никогда. Считать живые заслонённые блоки было бы
	 * дешевле, но неверно: освобождённая заслонённая область остаётся нашей до самой
	 * отдачи системе, и перестань мы спрашивать заслоны при нуле живых - повторное
	 * освобождение ушло бы прежнему распределителю. Проверено опытом: щуп валился с
	 * кодом 134 ровно на этом
	 */
	// Признак выданных когда-либо заслонённых блоков
	static std::atomic <bool> guarded(false);
	// Занято прикладным кодом прямо сейчас
	static std::atomic <size_t> occupied(0);
	// Наибольшее занятое за время работы
	static std::atomic <size_t> summit(0);
	// Объект журнала
	static const awh::log_t * journal = nullptr;
	/**
	 * Поток доклада
	 *
	 * Отклики зовутся ОТДЕЛЬНЫМ потоком, а не из выдачи памяти: позови мы отклик прямо
	 * из malloc - тот волен обратиться за памятью, и обращение вернулось бы в ту же
	 * выдачу. Поток же опрашивает состояние размеренно и зовёт отклики у себя
	 *
	 * Ждёт он выдержкой, а не условной переменной: подача сигнала берёт мьютекс, а
	 * подавать его пришлось бы ИЗНУТРИ выдачи памяти - то есть завести там ровно тот
	 * мьютекс, ради отсутствия которого писался свой замок
	 */
	// Длина кольца докладов о крупных выдачах
	static constexpr size_t REPORTS = 1024;
	// Выдержка между опросами потока доклада в миллисекундах
	static constexpr int64_t TICK = 50;
	/**
	 * @brief Доклад о крупной выдаче
	 *
	 */
	typedef struct Report {
		// Адрес выданного блока
		const void * block;
		// Затребованный размер блока в байтах
		size_t size;
	} report_t;
	// Замок кольца докладов
	static awh::alloc::spin_t reportLock;
	// Кольцо докладов о крупных выдачах
	static report_t reports[REPORTS];
	// Место записи очередного доклада
	static size_t reportHead = 0;
	// Место изъятия старейшего доклада
	static size_t reportTail = 0;
	// Число докладов в кольце
	static size_t reportHeld = 0;
	// Число докладов, потерянных из-за переполнения кольца
	static std::atomic <size_t> reportLost(0);
	// Замок откликов доклада
	static awh::alloc::spin_t callbackLock;
	// Отклик на крупное выделение
	static std::function <void (const void *, const size_t)> largeCallback;
	// Отклик на достижение потолка кучи
	static std::function <void (const size_t)> limitCallback;
	// Признак работы потока доклада
	static std::atomic <bool> reporting(false);
	// Поток доклада
	static std::thread * reporter = nullptr;
	// Число блоков, испорченных записью после освобождения, доложенное прежде
	static size_t spoiledSeen = 0;
	/**
	 * Способ захвата, свой у каждой семьи систем
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Захват переписыванием входа функций
		static awh::alloc::PECapture seizure;
	#elif defined(__APPLE__)
		// Захват зоной
		static awh::alloc::ZoneCapture seizure;
	#else
		// Захват подменой имён
		static awh::alloc::ELFCapture seizure;
	#endif
	/**
	 * @brief Метод получения текущего времени в миллисекундах
	 *
	 * @return текущее время в миллисекундах
	 *
	 */
	static uint64_t now() noexcept {
		// Выводим текущее время в миллисекундах
		return static_cast <uint64_t> (std::chrono::duration_cast <std::chrono::milliseconds> (
		 std::chrono::steady_clock::now().time_since_epoch()).count());
	}
	/**
	 * @brief Метод выдачи памяти из области первоначальной выдачи
	 *
	 * @param size требуемый размер в байтах
	 * @return     адрес выданной памяти либо nullptr
	 *
	 */
	static void * initial(const size_t size) noexcept {
		// Приводим требуемый размер к границе выравнивания
		const size_t need = ((size + (awh::alloc::Classes::ALIGN - 1)) & ~static_cast <size_t> (awh::alloc::Classes::ALIGN - 1));
		/**
		 * Занимаем место в области сдвигом счётчика
		 *
		 * Сдвигом без замка: обращения сюда идут и из нескольких потоков сразу, а
		 * замок здесь был бы замком поверх замка заведения
		 */
		const size_t taken = bootstrapUsed.fetch_add(need, std::memory_order_relaxed);
		// Если места в области не осталось
		if((taken + need) > BOOTSTRAP)
			// Выдавать нечего
			return nullptr;
		// Выводим занятое место
		return (bootstrap + taken);
	}
	/**
	 * @brief Метод определения принадлежности адреса области первоначальной выдачи
	 *
	 * @param ptr разбираемый адрес
	 * @return    признак принадлежности области
	 *
	 */
	static bool preliminary(const void * ptr) noexcept {
		// Выводим признак принадлежности адреса области первоначальной выдачи
		return ((ptr >= bootstrap) && (ptr < (bootstrap + BOOTSTRAP)));
	}
	/**
	 * @brief Метод получения номера текущего процесса
	 *
	 * @return номер текущего процесса
	 *
	 */
	static int64_t identity() noexcept {
		/**
		 * Если операционной системой является MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выводим номер текущего процесса
			return static_cast <int64_t> (::GetCurrentProcessId());
		/**
		 * Если операционной системой является Unix
		 */
		#else
			// Выводим номер текущего процесса
			return static_cast <int64_t> (::getpid());
		#endif
	}
	/**
	 * @brief Метод разбора заклинившего замка
	 *
	 * @note Зовётся замком, не отпущенным тысячу уступок времени подряд. Обыкновенно
	 *       сюда не доходят вовсе: состязание потоков разрешается на первых же
	 *       оборотах. Оттого обращение к системе за номером процесса здесь ничего не
	 *       стоит - на обычном пути выделения его нет
	 *
	 */
	static void jammed() noexcept {
		// Если распределитель не заведён
		if(stage.load(std::memory_order_acquire) != STAGE_READY)
			// Разбирать нечего
			return;
		// Получаем номер текущего процесса
		const int64_t current = identity();
		// Читаем номер процесса, которому принадлежит распределитель
		int64_t owner = holder.load(std::memory_order_acquire);
		// Если процесс не сменился
		if((owner == 0) || (owner == current))
			/**
			 * Разбирать нечего
			 *
			 * Замок держит живой поток этого же процесса, и отпускать его силой нельзя:
			 * состояние под ним недостроено, а долгое удержание - не повод его рвать
			 */
			return;
		/**
		 * Занимаем разбор за собою
		 *
		 * Заклинивший замок обнаружат разом все кружащие потоки, а приводить в порядок
		 * распределитель обязан один: второй застал бы его посреди приведения
		 */
		if(!holder.compare_exchange_strong(owner, current, std::memory_order_acq_rel, std::memory_order_acquire))
			// Разбирает кто-то другой
			return;
		/**
		 * Приводим распределитель в порядок как у потомка ветвления
		 *
		 * Именно им потомок и оказался: о ветвлении нам не сообщили, а замки достались
		 * захваченными от потоков, каких в этом процессе нет
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Освобождаем замки центральных списков и кучи
			machinery->central.adopt();
			// Приводим в порядок кэши прочих потоков родителя
			machinery->caches.adopt();
			// Приводим в порядок замок заслонов
			machinery->guard.adopt();
			// Приводим в порядок замок карантина
			machinery->quarantine.adopt();
			// Приводим в порядок замок учёта мест выдачи
			machinery->profile.adopt();
			// Приводим в порядок замки доклада
			reportLock.reset();
			// Приводим в порядок замок откликов доклада
			callbackLock.reset();
		#endif
	}
	/**
	 * Отклики ветвления процесса
	 *
	 * Порядок захвата у распределителя один: кэши, затем центральные списки, затем
	 * куча. Отпускание идёт обратным порядком, а у потомка - тем же, что и отпускание:
	 * кэши отдают блоки центральным спискам, и отпусти мы кэши первыми, отдача ушла бы
	 * за замком, который списки ещё держат
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		/**
		 * @brief Метод захвата замков распределителя перед ветвлением процесса
		 *
		 */
		static void beforeFork() noexcept {
			// Если распределитель не заведён
			if(stage.load(std::memory_order_acquire) != STAGE_READY)
				// Захватывать нечего
				return;
			// Захватываем замок управляющего кэшами
			machinery->caches.prepare();
			// Захватываем замки центральных списков и кучи
			machinery->central.prepare();
			// Захватываем замок заслонов
			machinery->guard.prepare();
			// Захватываем замок карантина
			machinery->quarantine.prepare();
			// Захватываем замок учёта мест выдачи
			machinery->profile.prepare();
			// Захватываем замок кольца докладов
			reportLock.acquire();
		}
		/**
		 * @brief Метод освобождения замков распределителя у родителя после ветвления
		 *
		 */
		static void afterFork() noexcept {
			// Если распределитель не заведён
			if(stage.load(std::memory_order_acquire) != STAGE_READY)
				// Освобождать нечего
				return;
			// Освобождаем замок кольца докладов
			reportLock.release();
			// Освобождаем замок учёта мест выдачи
			machinery->profile.resume();
			// Освобождаем замок карантина
			machinery->quarantine.resume();
			// Освобождаем замок заслонов
			machinery->guard.resume();
			// Освобождаем замки центральных списков и кучи
			machinery->central.resume();
			// Освобождаем замок управляющего кэшами
			machinery->caches.resume();
		}
		/**
		 * @brief Метод приведения распределителя в порядок у потомка ветвления
		 *
		 */
		static void childFork() noexcept {
			// Если распределитель не заведён
			if(stage.load(std::memory_order_acquire) != STAGE_READY)
				// Приводить нечего
				return;
			// Запоминаем номер процесса потомка
			holder.store(identity(), std::memory_order_release);
			// Освобождаем замки центральных списков и кучи
			machinery->central.adopt();
			// Приводим в порядок кэши прочих потоков родителя
			machinery->caches.adopt();
			// Приводим в порядок замок заслонов
			machinery->guard.adopt();
			// Приводим в порядок замок карантина
			machinery->quarantine.adopt();
			// Приводим в порядок замок учёта мест выдачи
			machinery->profile.adopt();
			/**
			 * Потока доклада у потомка ветвления НЕТ
			 *
			 * Ветвление переносит лишь звавший поток, а прочие остаются у родителя.
			 * Отмечаем доклад остановленным и забываем поток, не трогая его: снести его
			 * значило бы тронуть состояние потока, какого в этом процессе нет. Приложению,
			 * которому доклад нужен и у потомка, довольно поставить отклик заново
			 */
			// Отмечаем поток доклада остановленным
			reporting.store(false, std::memory_order_release);
			// Забываем поток доклада
			reporter = nullptr;
			// Замок кольца докладов приводим в порядок
			reportLock.reset();
			// Замок откликов доклада приводим в порядок
			callbackLock.reset();
		}
	#endif
	/**
	 * @brief Метод заведения распределителя
	 *
	 * @return признак готовности распределителя
	 *
	 */
	static bool prepare() noexcept {
		// Если распределитель уже готов
		if(stage.load(std::memory_order_acquire) == STAGE_READY)
			// Заводить нечего
			return true;
		// Ожидаемая ступень готовности: незаведённый распределитель
		int32_t expected = STAGE_COLD;
		/**
		 * Беремся заводить распределитель, если за него не взялся никто
		 *
		 * Ни замка, ни опознания потока. Опознавать поток нечем: `pthread_self` у
		 * FreeBSD САМ зовёт `calloc` при первом обращении, и опознание уходило бы в
		 * бесконечную возвратность через наше же выделение - проверено съёмом стека,
		 * тот состоял из повторяющейся тройки calloc-prepare-pthread_self.
		 * Поток-локальное место негодно по той же причине: его ленивое заведение тоже
		 * обращается за памятью (проверено на macOS - срыв стека внутри `_tlv_get_addr`).
		 *
		 * Оттого отказ здесь означает разом и «заводит кто-то другой», и «мы сами
		 * внутри заведения»: оба случая обслуживаются одинаково - областью
		 * первоначальной выдачи, и различать их незачем
		 */
		if(!stage.compare_exchange_strong(expected, STAGE_WARM, std::memory_order_acq_rel, std::memory_order_acquire))
			// Заводить нечем: выдача пойдёт из области первоначальной выдачи
			return false;
		// Заводим устройство распределителя на отведённом месте
		machinery = new (space) machinery_t();
		// Определяем источник страниц
		awh::alloc::source_t * source = ((substitute != nullptr) ? substitute : &machinery->system);
		// Строим разряды размеров
		const bool ranked = (machinery->classes.init() > 0);
		// Заводим страничную кучу
		const bool heaped = machinery->pages.init(source, settings.arena, settings.confined);
		// Задаём куче порядок отдачи памяти системе
		machinery->pages.policy(settings.purgeDelay, settings.purgeBlock);
		// Задаём куче потолок взятого у источника
		machinery->pages.ceiling(settings.heapLimit);
		// Заводим центральные списки
		const bool listed = machinery->central.init(&machinery->pages, &machinery->classes);
		// Заводим управляющего поток-локальными кэшами
		const bool cached = machinery->caches.init(&machinery->central, &machinery->classes);
		// Если потолок поток-локальных кэшей задан
		if(settings.cacheLimit > 0)
			// Задаём потолок поток-локальных кэшей
			machinery->caches.limit(settings.cacheLimit);
		/**
		 * Заводим заслоны и карантин
		 *
		 * Берут они страницы прямо у источника, минуя страничную кучу: заслон ставится
		 * страницей системы, а куча раздаёт свои логические из общих кусков - запрет
		 * посреди куска накрыл бы соседние выдачи
		 */
		// Заводим заслоны выборочной выдачи
		machinery->guard.init(source);
		// Задаём заслонам долю выборки
		machinery->guard.rate(settings.guardRate);
		// Заводим карантин освобождённой памяти
		machinery->quarantine.init(source, settings.quarantine);
		// Задаём карантину засев удерживаемой памяти
		machinery->quarantine.junk(settings.fill == awh::alloc::fill_t::JUNK);
		/**
		 * Заводим съём стека и учёт мест выдачи
		 *
		 * Съём прогревается прямо здесь, вне всякой выдачи: первое обращение к раскрутке
		 * у части систем само просит памяти
		 */
		// Заводим съём стека вызовов
		machinery->trace.init();
		// Заводим учёт мест выдачи памяти
		machinery->profile.init(source, &machinery->trace);
		// Задаём учёту долю выборки
		machinery->profile.rate(settings.profileRate);
		/**
		 * Заводим отклики ветвления процесса
		 *
		 * Заводим здесь, а не при захвате: у систем ELF наши имена заслоняют прежние с
		 * начала процесса, и ветвление может случиться прежде, чем приложение доберётся
		 * до захвата
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Ветвления у MS Windows нет вовсе
		#elif defined(__APPLE__)
			/**
			 * У macOS отклики отдаются ЗОНЕ, а не `pthread_atfork`
			 *
			 * Система зовёт отклики зоны раньше всяких `pthread_atfork`. Заведи мы их
			 * через `pthread_atfork`, они оказались бы позже откликов самой библиотеки
			 * времени исполнения: та обратилась бы у потомка за памятью, наткнулась на
			 * замок, захваченный перед ветвлением, и встала бы навсегда. Проверено
			 * опытом: потомок вставал, не дойдя до нашего отклика вовсе
			 */
			// Отдаём отклики ветвления зоне
			awh::alloc::ZoneCapture::fork(&beforeFork, &afterFork, &childFork);
		#else
			// Заводим отклики ветвления процесса
			::pthread_atfork(&beforeFork, &afterFork, &childFork);
		#endif
		// Если завести распределитель не вышло
		if(!(ranked && heaped && listed && cached)){
			// Отмечаем распределитель незаведённым
			stage.store(STAGE_COLD, std::memory_order_release);
			// Отвечаем отказом
			return false;
		}
		// Запоминаем номер процесса, которому принадлежит распределитель
		holder.store(identity(), std::memory_order_release);
		/**
		 * Вешаем разбор ветвления на заклинивший замок
		 *
		 * Запасной путь к откликам ветвления, а не замена им: те срабатывают вовремя и
		 * приводят распределитель в порядок прежде первого же выделения, а этот -
		 * лишь когда о ветвлении не сообщили вовсе
		 */
		awh::alloc::Spin::onStuck(&jammed);
		// Отмечаем распределитель готовым
		stage.store(STAGE_READY, std::memory_order_release);
		// Отвечаем готовностью
		return true;
	}
	/**
	 * @brief Метод учёта выданного прикладному коду
	 *
	 * @param size объём выданного в байтах
	 *
	 */
	static void account(const size_t size) noexcept {
		// Увеличиваем занятое прикладным кодом
		const size_t taken = (occupied.fetch_add(size, std::memory_order_relaxed) + size);
		// Читаем наибольшее занятое за время работы
		size_t highest = summit.load(std::memory_order_relaxed);
		/**
		 * Поднимаем наибольшее занятое, если оно перекрыто
		 */
		while(taken > highest){
			// Пробуем поднять наибольшее занятое
			if(summit.compare_exchange_weak(highest, taken, std::memory_order_relaxed, std::memory_order_relaxed))
				// Наибольшее занятое поднято
				break;
		}
	}
	/**
	 * @brief Метод постановки доклада о крупной выдаче
	 *
	 * @note Зовётся ИЗНУТРИ выдачи памяти: ни отклика, ни журнала, ни памяти здесь
	 *       трогать нельзя - только запись в кольцо под своим замком
	 *
	 * @param ptr  адрес выданного блока
	 * @param size затребованный размер блока в байтах
	 *
	 */
	static void announce(const void * ptr, const size_t size) noexcept {
		// Захватываем замок кольца докладов
		awh::alloc::hold_t hold(reportLock);
		// Если кольцо заполнено целиком
		if(reportHeld >= REPORTS){
			/**
			 * Считаем потерянное, а не вытесняем старейшее
			 *
			 * Доклад о крупной выдаче нужен, чтобы найти виновника; вытесняя старейшие,
			 * мы теряли бы как раз первые - те, что случились прежде затопления
			 */
			reportLost.fetch_add(1, std::memory_order_relaxed);
			// Докладывать больше некуда
			return;
		}
		// Записываем адрес выданного блока
		reports[reportHead].block = ptr;
		// Записываем затребованный размер блока
		reports[reportHead].size = size;
		// Сдвигаем место записи очередного доклада
		reportHead = ((reportHead + 1) % REPORTS);
		// Увеличиваем число докладов в кольце
		reportHeld++;
	}
	/**
	 * @brief Метод работы потока доклада
	 *
	 */
	static void deliver() noexcept {
		/**
		 * Опрашиваем состояние, пока поток доклада работает
		 */
		while(reporting.load(std::memory_order_acquire)){
			// Отклик на крупное выделение
			std::function <void (const void *, const size_t)> large;
			// Отклик на достижение потолка кучи
			std::function <void (const size_t)> limit;
			{
				// Захватываем замок откликов доклада
				awh::alloc::hold_t hold(callbackLock);
				// Забираем отклик на крупное выделение
				large = largeCallback;
				// Забираем отклик на достижение потолка кучи
				limit = limitCallback;
			}
			/**
			 * Разбираем кольцо докладов о крупных выдачах
			 */
			for(;;){
				// Очередной доклад о крупной выдаче
				report_t report;
				{
					// Захватываем замок кольца докладов
					awh::alloc::hold_t hold(reportLock);
					// Если кольцо пусто
					if(reportHeld == 0)
						// Разбирать больше нечего
						break;
					// Забираем старейший доклад
					report = reports[reportTail];
					// Сдвигаем место изъятия старейшего доклада
					reportTail = ((reportTail + 1) % REPORTS);
					// Уменьшаем число докладов в кольце
					reportHeld--;
				}
				// Если отклик на крупное выделение задан
				if(large)
					// Докладываем о крупной выдаче
					large(report.block, report.size);
			}
			// Если распределитель заведён
			if(stage.load(std::memory_order_acquire) == STAGE_READY){
				// Если куча упёрлась в потолок
				if(machinery->central.jammed() && limit)
					// Докладываем о достижении потолка кучи
					limit(machinery->pages.state().total);
				/**
				 * Докладываем о записи по освобождённому блоку
				 *
				 * Отклика на то в договоре нет, и заводить его отдельно незачем: находка
				 * эта - сообщение о дефекте прикладного кода, а место таких сообщений в
				 * журнале. Ловит её карантин, а докладывает поток - изнутри освобождения
				 * запись в журнал сама пошла бы за памятью
				 */
				// Получаем состояние карантина
				const awh::alloc::Quarantine::state_t quarantine = machinery->quarantine.state();
				// Если испорченных блоков прибавилось
				if((quarantine.spoiled > spoiledSeen) && (journal != nullptr)){
					// Запоминаем число доложенных испорченных блоков
					spoiledSeen = quarantine.spoiled;
					// Докладываем о записи по освобождённому блоку
					journal->print("Memory was written after being freed: %zu block(s), first at %p offset %zu", awh::log_t::flag_t::CRITICAL, quarantine.spoiled, quarantine.culprit, quarantine.offset);
				}
			}
			// Ждём до следующего опроса
			std::this_thread::sleep_for(std::chrono::milliseconds(TICK));
		}
	}
	/**
	 * @brief Метод заведения потока доклада
	 *
	 * @note Звать ВНЕ выдачи памяти: заведение потока само за памятью и обращается
	 *
	 */
	static void awaken() noexcept {
		// Если поток доклада уже заведён
		if(reporting.load(std::memory_order_acquire))
			// Заводить нечего
			return;
		// Отмечаем поток доклада работающим
		reporting.store(true, std::memory_order_release);
		// Заводим поток доклада
		reporter = new std::thread(&deliver);
	}
	/**
	 * @brief Метод взятия выданного блока под учёт места выдачи
	 *
	 * @note Стек, снимаемый учётом, начинается с того, кто позвал взятие: сколько
	 *       ближних уровней придётся на сам распределитель, зависит от подстановки у
	 *       собирателя, и обрезать их числом наперёд значило бы обрезать наугад
	 *
	 * @param ptr  адрес выданного блока
	 * @param size затребованный размер блока в байтах
	 *
	 */
	static void enrol(void * ptr, const size_t size) noexcept {
		// Если выдавать было нечего
		if(ptr == nullptr)
			// Учитывать нечего
			return;
		// Если выдача перекрыла порог доклада о крупном выделении
		if((settings.reportLarge > 0) && (size >= settings.reportLarge))
			// Ставим доклад о крупной выдаче
			announce(ptr, size);
		// Если выданный блок под выборку учёта не попал
		if(!machinery->profile.wanted())
			// Учитывать нечего
			return;
		// Берём выданный блок под учёт места выдачи
		machinery->profile.enroll(ptr, size, now(), 0);
	}
	/**
	 * @brief Метод выдачи памяти
	 *
	 * @param size требуемый размер в байтах
	 * @return     адрес выданной памяти либо nullptr
	 *
	 */
	static void * reserve(const size_t size) noexcept {
		// Если требуемый размер не задан
		if(size == 0)
			/**
			 * Выдаём наименьший блок, а не пустоту
			 *
			 * Стандарт позволяет и то и другое, но выданный на нулевой запрос
			 * указатель обязан быть отличим от неудачи, а пустота от неё неотличима
			 */
			return reserve(1);
		// Если распределитель не готов
		if(!prepare())
			// Выдаём из области первоначальной выдачи
			return initial(size);
		/**
		 * Выдаём блок под заслонами, если выборка взяла эту выдачу
		 *
		 * Отказ заслонов отказом выдачи НЕ является: заслоны выборочны и всегда могут
		 * не состояться - у источника нет памяти, источник не умеет закрывать страницы.
		 * Выдача при этом идёт обычным путём
		 */
		if(machinery->guard.wanted(size)){
			// Выдаём блок под заслонами
			void * result = machinery->guard.alloc(size);
			// Если блок под заслонами выдан
			if(result != nullptr){
				// Отмечаем заслонённые блоки выданными
				guarded.store(true, std::memory_order_relaxed);
				// Учитываем выданное прикладному коду
				account(size);
				// Берём выданный блок под учёт места выдачи
				enrol(result, size);
				// Выводим выданный блок
				return result;
			}
		}
		// Определяем разряд требуемого размера
		const size_t index = machinery->classes.index(size);
		// Если размер обслуживается разрядами
		if(index < machinery->classes.count()){
			// Получаем кэш текущего потока
			awh::alloc::cache_t * cache = machinery->caches.local();
			// Если кэш текущего потока заведён
			if(cache != nullptr){
				// Выдаём блок разряда из кэша
				void * result = cache->alloc(index);
				// Если блок выдан
				if(result != nullptr){
					// Учитываем выданное прикладному коду
					account(machinery->classes.size(index));
					// Берём выданный блок под учёт места выдачи
					enrol(result, size);
					// Выводим выданный блок
					return result;
				}
			}
			// Отвечаем отказом: разряд есть, а памяти под него нет
			return nullptr;
		}
		// Выдаём область страниц под запрос сверх разрядов
		void * result = machinery->central.alloc(size);
		// Если область выдана
		if(result != nullptr){
			// Учитываем выданное прикладному коду
			account(((size + (awh::alloc::Pages::PAGE - 1)) / awh::alloc::Pages::PAGE) * awh::alloc::Pages::PAGE);
			// Берём выданную область под учёт места выдачи
			enrol(result, size);
		}
		// Выводим выданную область
		return result;
	}
	/**
	 * @brief Метод выдачи памяти с требуемым выравниванием
	 *
	 * @param alignment требуемое выравнивание в байтах
	 * @param size      требуемый размер в байтах
	 * @return          адрес выданной памяти либо nullptr
	 *
	 */
	static void * align(const size_t alignment, const size_t size) noexcept {
		/**
		 * Если требуемое выравнивание не превышает выравнивания разрядов
		 *
		 * Всякий выданный нами блок выровнен по шестнадцати байтам: того требует язык
		 * от обычной выдачи, и просить меньшего незачем
		 */
		if(alignment <= awh::alloc::Classes::ALIGN)
			// Выдаём память обычным путём
			return reserve(size);
		// Если распределитель не готов
		if(!prepare()){
			/**
			 * Выдаём из области первоначальной выдачи с запасом на выравнивание
			 *
			 * Память эта не освобождается вовсе, оттого пропуск перед выровненным
			 * адресом теряется безвозвратно - но области той четверть мегабайта на весь
			 * век процесса, и выравнивающих запросов до заведения считанные единицы
			 */
			uint8_t * block = reinterpret_cast <uint8_t *> (initial(size + alignment));
			// Если память не выдана
			if(block == nullptr)
				// Выдавать нечего
				return nullptr;
			// Определяем пропуск до выровненного адреса
			const size_t shift = (reinterpret_cast <uintptr_t> (block) % alignment);
			// Выводим выровненный адрес
			return (block + ((shift == 0) ? 0 : (alignment - shift)));
		}
		/**
		 * Если требуемое выравнивание не превышает страницы кучи
		 *
		 * Области сверх разрядов выдаются страничной кучей, а начала её страниц выровнены
		 * по размеру страницы: кусок берётся у источника выровненным по своему размеру, а
		 * страницы нарезаются от начала куска. Оттого выдача сверх разрядов выровнена
		 * всегда, и добиваться выравнивания приведением адреса не требуется
		 */
		if(alignment <= awh::alloc::Pages::PAGE){
			// Требуемый размер, заведомо выходящий за разряды
			size_t need = size;
			// Если размер обслуживается разрядами
			if(need <= awh::alloc::Classes::MAXIMUM)
				// Поднимаем размер за пределы разрядов
				need = (awh::alloc::Classes::MAXIMUM + 1);
			// Выдаём область страниц
			void * result = machinery->central.alloc(need);
			// Если область выдана
			if(result != nullptr)
				// Учитываем выданное прикладному коду
				account(((need + (awh::alloc::Pages::PAGE - 1)) / awh::alloc::Pages::PAGE) * awh::alloc::Pages::PAGE);
			// Выводим выданную область
			return result;
		}
		/**
		 * Отвечаем отказом
		 *
		 * Выравнивание сверх страницы кучи нам не по силам: куски у источника выровнены
		 * по своему размеру, но начала областей внутри них - лишь по странице. Звавший
		 * волен обратиться к прежнему распределителю - и обращается
		 */
		return nullptr;
	}
	/**
	 * @brief Метод определения размера выданного блока
	 *
	 * @param ptr   разбираемый адрес
	 * @param index номер разряда, либо LIMIT у памяти сверх разрядов
	 * @return      размер блока в байтах, либо нуль если блок не наш
	 *
	 */
	static size_t measure(const void * ptr, size_t * index) noexcept {
		// Если разбирать нечего
		if(ptr == nullptr)
			// Разбирать нечего
			return 0;
		// Если адрес лежит в области первоначальной выдачи
		if(preliminary(ptr))
			/**
			 * Отвечаем нулём
			 *
			 * Размера блоков той области мы не помним: она заводится прежде всякого
			 * учёта и живёт до конца процесса, а измерять в ней нечего
			 */
			return 0;
		// Если распределитель не готов
		if(stage.load(std::memory_order_acquire) != STAGE_READY)
			// Разбирать нечем
			return 0;
		// Если заслонённые блоки выдавались
		if(guarded.load(std::memory_order_relaxed)){
			// Размер заслонённого блока в байтах
			size_t served = 0;
			// Если адрес принадлежит заслонённому блоку
			if(machinery->guard.owner(ptr, &served)){
				// Если требуется номер разряда
				if(index != nullptr)
					// Отмечаем блок лежащим вне разрядов
					(* index) = awh::alloc::Classes::LIMIT;
				// Выводим затребованный размер заслонённого блока
				return served;
			}
		}
		// Номер разряда, которому принадлежит адрес
		size_t which = 0;
		// Адрес начала области
		void * begin = nullptr;
		// Размер области в байтах
		size_t size = 0;
		// Если адрес нам не принадлежит
		if(!machinery->central.owner(ptr, &which, &begin, &size))
			// Блок не наш
			return 0;
		// Если требуется номер разряда
		if(index != nullptr)
			// Записываем номер разряда
			(* index) = which;
		// Если адрес принадлежит области разряда
		if(which < machinery->classes.count())
			// Выводим размер блока разряда
			return machinery->classes.size(which);
		// Выводим размер области сверх разрядов
		return size;
	}
	/**
	 * @brief Метод возврата блока слоям распределителя
	 *
	 * @param ptr   адрес возвращаемого блока
	 * @param index номер разряда, которому принадлежит блок
	 *
	 */
	static void recycle(void * ptr, const size_t index) noexcept {
		// Если блок принадлежит области разряда
		if(index < machinery->classes.count()){
			// Получаем кэш текущего потока
			awh::alloc::cache_t * cache = machinery->caches.local();
			// Если кэш текущего потока заведён
			if(cache != nullptr){
				// Возвращаем блок в кэш текущего потока
				cache->free(index, ptr);
				// Возвращать больше нечего
				return;
			}
		}
		// Возвращаем куче область, выданную сверх разрядов
		machinery->central.free(ptr, now());
	}
	/**
	 * @brief Метод освобождения памяти
	 *
	 * @param ptr адрес освобождаемой памяти
	 *
	 */
	static void discard(void * ptr) noexcept {
		// Если освобождать нечего
		if(ptr == nullptr)
			// Освобождать нечего
			return;
		/**
		 * Память области первоначальной выдачи не освобождается вовсе
		 *
		 * Она невелика, выдаётся однажды при заведении и живёт до конца процесса:
		 * вести ей учёт значило бы завести второй распределитель ради четверти
		 * мегабайта
		 */
		if(preliminary(ptr))
			// Освобождать нечего
			return;
		// Если распределитель не готов
		if(stage.load(std::memory_order_acquire) != STAGE_READY){
			// Если прежнее освобождение памяти известно
			if(originals.free != nullptr)
				// Отдаём память прежнему распределителю
				(* originals.free)(ptr);
			// Освобождать больше нечем
			return;
		}
		/**
		 * Освобождаем заслонённый блок
		 *
		 * Спрашиваем заслоны лишь когда заслонённые блоки есть: при выключенных
		 * заслонах вопрос этот стоил бы замка на каждом освобождении
		 */
		if(guarded.load(std::memory_order_relaxed)){
			// Признак принадлежности адреса заслонам
			bool mine = false;
			// Освобождаем заслонённый блок
			const size_t served = machinery->guard.free(ptr, &mine);
			/**
			 * Если адрес принадлежит заслонам
			 *
			 * Нулевой размер при нашем адресе означает повторное освобождение: блок
			 * закрыт прежде. Отдавать слоям нечего, но и прежнему распределителю такой
			 * указатель нести НЕЛЬЗЯ - тот принял бы за свой чужой ему адрес
			 */
			if(mine){
				/**
				 * Учёт правим лишь при настоящем освобождении
				 *
				 * Повторное отдаёт нулевой размер, и вычитай мы его - занятое прикладным
				 * кодом уехало бы вниз на пустом месте
				 */
				if(served > 0){
					// Если учёт мест выдачи ведётся
					if(machinery->profile.tracking())
						// Снимаем блок с учёта места выдачи
						machinery->profile.expel(ptr);
					// Уменьшаем занятое прикладным кодом
					occupied.fetch_sub(((occupied.load(std::memory_order_relaxed) < served) ? occupied.load(std::memory_order_relaxed) : served), std::memory_order_relaxed);
				}
				/**
				 * Освобождать больше нечего
				 *
				 * Область заслонённого блока закрыта целиком и слоям не возвращается:
				 * обращение по освобождённому адресу обязано валить, а не доставаться
				 * следующей выдаче
				 */
				return;
			}
		}
		// Номер разряда, которому принадлежит адрес
		size_t index = awh::alloc::Classes::LIMIT;
		// Определяем размер освобождаемого блока
		const size_t size = measure(ptr, &index);
		// Если блок нам не принадлежит
		if(size == 0){
			/**
			 * Отдаём память прежнему распределителю
			 *
			 * Часть памяти процесса выдана до захвата, а часть - путями, каких мы не
			 * заслонили: выравнивающей выдачей библиотеки времени исполнения, скажем.
			 * Освобождать её обязан тот, кто выдавал
			 */
			if(originals.free != nullptr)
				// Отдаём память прежнему распределителю
				(* originals.free)(ptr);
			// Освобождать больше нечем
			return;
		}
		// Если учёт мест выдачи ведётся
		if(machinery->profile.tracking())
			// Снимаем блок с учёта места выдачи
			machinery->profile.expel(ptr);
		// Уменьшаем занятое прикладным кодом
		occupied.fetch_sub(((occupied.load(std::memory_order_relaxed) < size) ? occupied.load(std::memory_order_relaxed) : size), std::memory_order_relaxed);
		/**
		 * Удерживаем блок карантином, если тот заведён
		 *
		 * Карантин засевает удерживаемое сам, оттого засев здесь идёт лишь тем блокам,
		 * каких он не принял: непомерным ему по размеру да пришедшим к полному кольцу
		 */
		if((settings.quarantine > 0) && machinery->quarantine.hold(ptr, size, index)){
			// Размер вытесненного из карантина блока
			size_t served = 0;
			// Номер разряда вытесненного из карантина блока
			size_t which = 0;
			// Адрес вытесненного из карантина блока
			void * exiled = nullptr;
			/**
			 * Возвращаем слоям вытесненное карантином
			 *
			 * Возвращает их принявший, а не сам карантин: тот слоёв не знает вовсе и
			 * знать не должен - иначе очередь освобождённого оказалась бы вплетена в
			 * выдачу памяти
			 */
			while((exiled = machinery->quarantine.release(&served, &which)) != nullptr)
				// Возвращаем слоям вытесненный блок
				recycle(exiled, which);
			// Освобождать больше нечего
			return;
		}
		// Если требуется засевать освобождаемое
		if(settings.fill == awh::alloc::fill_t::JUNK)
			// Засеваем освобождаемое заметным образом
			::memset(ptr, 0xDE, size);
		// Возвращаем блок слоям распределителя
		recycle(ptr, index);
	}
};

/**
 * Подставные функции, ставимые на место прежних
 *
 * У систем ELF они обязаны зваться именами библиотеки времени исполнения - в этом и
 * состоит приём подмены. У macOS и MS Windows имена свои: там захват идёт зоной и
 * переписыванием входа, а имена наших функций никого не заслоняют
 *
 * ВАЖНО: файл этот обязан собираться с `-fno-builtin-malloc -fno-builtin-calloc
 * -fno-builtin-realloc -fno-builtin-free`, иначе собиратель узнаёт в нашем `calloc`
 * пару «выделение плюс обнуление» и подменяет её вызовом... `calloc`, то есть себя
 */
#if defined(_WIN32) || defined(_WIN64) || defined(__APPLE__)
	#define AWH_ALLOC_HOOK(name) __awh_alloc_##name##__
	#define AWH_ALLOC_LINKAGE static
#else
	#define AWH_ALLOC_HOOK(name) name
	#define AWH_ALLOC_LINKAGE extern "C"
#endif

/**
 * @brief Метод выделения памяти
 *
 * @param size требуемый размер в байтах
 * @return     адрес выданной памяти либо nullptr
 *
 */
AWH_ALLOC_LINKAGE void * AWH_ALLOC_HOOK(malloc)(size_t size) {
	// Выдаём память требуемого размера
	void * result = ::reserve(size);
	// Если требуется обнулять выдаваемое
	if((result != nullptr) && (::settings.fill == awh::alloc::fill_t::ZERO))
		// Обнуляем выдаваемое
		::memset(result, 0, size);
	// Выводим выданную память
	return result;
}
/**
 * @brief Метод освобождения памяти
 *
 * @param ptr адрес освобождаемой памяти
 *
 */
AWH_ALLOC_LINKAGE void AWH_ALLOC_HOOK(free)(void * ptr) {
	// Освобождаем память
	::discard(ptr);
}
/**
 * @brief Метод выделения обнулённой памяти
 *
 * @param count число элементов
 * @param size  размер элемента в байтах
 * @return      адрес выданной памяти либо nullptr
 *
 */
AWH_ALLOC_LINKAGE void * AWH_ALLOC_HOOK(calloc)(size_t count, size_t size) {
	// Если считать нечего
	if((count == 0) || (size == 0))
		// Выдаём наименьший блок
		return ::reserve(1);
	/**
	 * Сверяем произведение на переполнение
	 *
	 * Без сверки запрос вида (2^33, 2^33) дал бы нулевой объём, выдача удалась бы, а
	 * запись по нему ушла бы далеко за конец блока
	 */
	if(count > (static_cast <size_t> (-1) / size))
		// Отвечаем отказом
		return nullptr;
	// Определяем требуемый объём
	const size_t need = (count * size);
	// Выдаём память требуемого объёма
	void * result = ::reserve(need);
	// Если память выдана
	if(result != nullptr)
		// Обнуляем выданное
		::memset(result, 0, need);
	// Выводим выданную память
	return result;
}
/**
 * @brief Метод изменения размера выделенной памяти
 *
 * @param ptr  адрес изменяемой памяти
 * @param size требуемый размер в байтах
 * @return     адрес выданной памяти либо nullptr
 *
 */
AWH_ALLOC_LINKAGE void * AWH_ALLOC_HOOK(realloc)(void * ptr, size_t size) {
	// Если изменять нечего
	if(ptr == nullptr)
		// Выдаём память требуемого размера
		return ::reserve(size);
	// Если требуемый размер не задан
	if(size == 0){
		// Освобождаем память
		::discard(ptr);
		// Выдавать нечего
		return nullptr;
	}
	// Определяем размер прежнего блока
	const size_t before = ::measure(ptr, nullptr);
	/**
	 * Если прежний блок не наш, отдаём изменение прежнему распределителю
	 *
	 * Переносить его содержимое к себе нельзя: длины его мы не знаем, а прочесть
	 * лишнее значило бы уйти за конец чужого блока
	 */
	if(before == 0){
		// Если прежнее изменение размера известно
		if(::originals.realloc != nullptr)
			// Отдаём изменение размера прежнему распределителю
			return (* ::originals.realloc)(ptr, size);
		// Отвечаем отказом
		return nullptr;
	}
	/**
	 * Оставляем блок на месте, если требуемое в него помещается
	 *
	 * Помещается оно часто: разряды округляют размер вверх, и рост в пределах
	 * округления переносить незачем
	 */
	if(size <= before)
		// Выводим прежний блок
		return ptr;
	// Выдаём память требуемого размера
	void * result = ::reserve(size);
	// Если память не выдана
	if(result == nullptr)
		// Отвечаем отказом: прежний блок при этом остаётся годным
		return nullptr;
	// Переносим содержимое прежнего блока
	::memcpy(result, ptr, before);
	// Освобождаем прежний блок
	::discard(ptr);
	// Выводим выданную память
	return result;
}
/**
 * @brief Метод определения размера выделенного блока
 *
 * @param ptr разбираемый адрес
 * @return    размер блока в байтах, либо нуль если блок не наш
 *
 */
static size_t AWH_ALLOC_HOOK(msize)(const void * ptr) {
	// Выводим размер выданного блока
	return ::measure(ptr, nullptr);
}
/**
 * @brief Метод выделения памяти с требуемым выравниванием
 *
 * @param alignment требуемое выравнивание в байтах
 * @param size      требуемый размер в байтах
 * @return          адрес выданной памяти либо nullptr
 *
 */
/**
 * Имя выравнивающей выдачи своё у ВСЕХ систем, а не только у macOS и MS Windows
 *
 * У систем ELF `memalign` объявлен библиотекой времени исполнения, и наше определение
 * с внутренним связыванием спорит с тем объявлением: собиратели Alpine и Solaris валят
 * сборку словами «объявлено extern, а затем static». Имени этого мы там и не заслоняем:
 * выравнивающая выдача идёт своим путём внутри библиотеки и нашего malloc не зовёт, а
 * зовут наш отклик лишь зона macOS да переписывание входа у MS Windows - и оба зовут
 * его указателем, имени не спрашивая
 */
static void * __awh_alloc_memalign__(size_t alignment, size_t size) {
	// Если выравнивание не задано либо не является степенью двойки
	if((alignment == 0) || ((alignment & (alignment - 1)) != 0))
		// Выравнивать нечем
		return nullptr;
	// Выдаём память с требуемым выравниванием
	void * result = ::align(alignment, size);
	// Если требуется обнулять выдаваемое
	if((result != nullptr) && (::settings.fill == awh::alloc::fill_t::ZERO))
		// Обнуляем выданную память
		::memset(result, 0, size);
	// Выводим выданную память
	return result;
}

/**
 * @brief Метод захвата выделения памяти процесса
 *
 * @param options настройки распределителя
 * @param log     объект журнала
 * @return        признак состоявшегося захвата
 *
 */
bool awh::alloc::Allocator::capture(const options_t & options, const log_t * log) noexcept {
	// Запоминаем объект журнала
	::journal = log;
	/**
	 * Запоминаем настройки ПРЕЖДЕ заведения
	 *
	 * Заведение читает и размер занимаемой области, и запрет обращаться к системе, и
	 * порядок отдачи памяти: запиши мы настройки после, распределитель завёлся бы по
	 * умолчанию, а заданное приложением не подействовало бы вовсе
	 */
	::settings = options;
	// Заводим распределитель, если он ещё не заведён
	if(!::prepare())
		// Отвечаем отказом
		return false;
	// Если захват уже состоялся
	if(::seized.load(std::memory_order_acquire))
		// Отвечаем успехом
		return true;
	// Наши функции, ставимые на место прежних
	functions_t hooks;
	// Задаём выделение памяти
	hooks.malloc = &AWH_ALLOC_HOOK(malloc);
	// Задаём освобождение памяти
	hooks.free = &AWH_ALLOC_HOOK(free);
	// Задаём выделение обнулённой памяти
	hooks.calloc = &AWH_ALLOC_HOOK(calloc);
	// Задаём изменение размера выделенной памяти
	hooks.realloc = &AWH_ALLOC_HOOK(realloc);
	// Задаём определение размера выделенного блока
	hooks.msize = &AWH_ALLOC_HOOK(msize);
	// Задаём выделение памяти с требуемым выравниванием
	hooks.memalign = &__awh_alloc_memalign__;
	// Захватываем выделение памяти процесса
	if(!::seizure.acquire(hooks, ::originals)){
		// Если объект журнала задан
		if(::journal != nullptr)
			// Записываем отказ захвата в журнал
			::journal->print("Memory allocation capture failed: %s", log_t::flag_t::WARNING, ::seizure.name());
		// Отвечаем отказом
		return false;
	}
	// Отмечаем захват состоявшимся
	::seized.store(true, std::memory_order_release);
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод снятия захвата выделения памяти процесса
 *
 */
void awh::alloc::Allocator::surrender() noexcept {
	// Если захват не состоялся
	if(!::seized.load(std::memory_order_acquire))
		// Снимать нечего
		return;
	/**
	 * Гасим поток доклада прежде снятия захвата
	 *
	 * Поток этот ходит в состояние распределителя и зовёт отклики приложения: оставь мы
	 * его жить после снятия - он звал бы отклики о распределителе, каким уже никто не
	 * пользуется
	 */
	if(::reporting.load(std::memory_order_acquire)){
		// Отмечаем поток доклада остановленным
		::reporting.store(false, std::memory_order_release);
		// Если поток доклада заведён
		if(::reporter != nullptr){
			// Дожидаемся завершения потока доклада
			::reporter->join();
			// Сносим поток доклада
			delete ::reporter;
			// Забываем поток доклада
			::reporter = nullptr;
		}
	}
	// Снимаем захват выделения памяти процесса
	::seizure.release();
	// Отмечаем захват снятым
	::seized.store(false, std::memory_order_release);
}
/**
 * @brief Метод определения захваченности выделения памяти процесса
 *
 * @return признак захвата
 *
 */
bool awh::alloc::Allocator::captured() noexcept {
	// Выводим признак состоявшегося захвата
	return ::seized.load(std::memory_order_acquire);
}
/**
 * @brief Метод получения действующих настроек
 *
 * @return действующие настройки
 *
 */
const awh::alloc::options_t & awh::alloc::Allocator::options() noexcept {
	// Выводим действующие настройки
	return ::settings;
}
/**
 * @brief Метод изменения настроек в работе
 *
 * @param options требуемые настройки
 *
 */
void awh::alloc::Allocator::options(const options_t & options) noexcept {
	/**
	 * Размер занимаемой области и запрет обращаться к системе не меняем
	 *
	 * Они действуют лишь при заведении: занять область задним числом ещё можно было
	 * бы, а вот вернуть уже занятое приложению, её не просившему, - нельзя
	 */
	// Запоминаем размер занимаемой области прежним
	const size_t arena = ::settings.arena;
	// Запоминаем запрет обращаться к системе прежним
	const bool confined = ::settings.confined;
	// Запоминаем требуемые настройки
	::settings = options;
	// Возвращаем размер занимаемой области
	::settings.arena = arena;
	// Возвращаем запрет обращаться к системе
	::settings.confined = confined;
	// Если распределитель заведён
	if(::stage.load(std::memory_order_acquire) == STAGE_READY){
		// Задаём куче порядок отдачи памяти системе
		::machinery->central.policy(::settings.purgeDelay, ::settings.purgeBlock);
		// Задаём куче потолок взятого у источника
		::machinery->central.ceiling(::settings.heapLimit);
		// Если потолок поток-локальных кэшей задан
		if(::settings.cacheLimit > 0)
			// Задаём потолок поток-локальных кэшей
			::machinery->caches.limit(::settings.cacheLimit);
		// Задаём заслонам долю выборки
		::machinery->guard.rate(::settings.guardRate);
		/**
		 * Задаём карантину потолок объёма
		 *
		 * Кольцо карантина заведено при заведении распределителя и в работе не растёт:
		 * потолок сверх заказанного при заведении упрётся в число мест кольца, и
		 * карантин окажется короче заказанного - но верным останется
		 */
		::machinery->quarantine.limit(::settings.quarantine);
		// Задаём карантину засев удерживаемой памяти
		::machinery->quarantine.junk(::settings.fill == fill_t::JUNK);
		// Задаём учёту мест выдачи долю выборки
		::machinery->profile.rate(::settings.profileRate);
	}
}
/**
 * @brief Метод опроса расхода памяти
 *
 * @param property требуемое свойство
 * @return         величина свойства в байтах
 *
 */
size_t awh::alloc::Allocator::property(const property_t property) noexcept {
	// Если распределитель не заведён
	if(::stage.load(std::memory_order_acquire) != STAGE_READY)
		// Опрашивать нечего
		return 0;
	/**
	 * Определяем требуемое свойство
	 */
	switch(static_cast <uint8_t> (property)){
		// Если требуется занятое прикладным кодом прямо сейчас
		case static_cast <uint8_t> (property_t::ALLOCATED):
			// Выводим занятое прикладным кодом
			return ::occupied.load(std::memory_order_relaxed);
		// Если требуется наибольшее занятое за время работы
		case static_cast <uint8_t> (property_t::PEAK):
			// Выводим наибольшее занятое
			return ::summit.load(std::memory_order_relaxed);
		// Если требуется взятое у системы под кучу
		case static_cast <uint8_t> (property_t::HEAP):
			// Выводим взятое у системы
			return ::machinery->central.heap().total;
		// Если требуется свободное в поток-локальных кэшах
		case static_cast <uint8_t> (property_t::CACHED):
			// Выводим свободное в кэшах
			return ::machinery->caches.cached(nullptr);
		// Если требуется свободное в страничной куче
		case static_cast <uint8_t> (property_t::PAGEFREE): {
			// Получаем состояние страничной кучи
			const Pages::state_t heap = ::machinery->central.heap();
			// Выводим свободное в куче, но системе не отданное
			return ((heap.free > heap.purged) ? (heap.free - heap.purged) : 0);
		}
		// Если требуется отданное системе обратно
		case static_cast <uint8_t> (property_t::UNMAPPED):
			// Выводим отданное системе
			return ::machinery->central.heap().purged;
	}
	// Опрашивать нечего
	return 0;
}
/**
 * @brief Метод возврата системе свободной памяти
 *
 * @return объём возвращённой системе памяти в байтах
 *
 */
size_t awh::alloc::Allocator::purge() noexcept {
	// Если распределитель не заведён
	if(::stage.load(std::memory_order_acquire) != STAGE_READY)
		// Возвращать нечего
		return 0;
	/**
	 * Опустошаем поток-локальные кэши ПРЕЖДЕ отдачи
	 *
	 * Блоки, лежащие в кэшах, для кучи заняты, и куча не отдала бы системе ни
	 * страницы, покуда в области разряда лежит хоть один занятый блок
	 */
	::machinery->caches.flush();
	// Выводим объём отданной системе памяти
	return ::machinery->central.purge(::now(), true);
}
/**
 * @brief Метод подмены источника страниц
 *
 * @param source требуемый источник страниц
 * @return       признак состоявшейся подмены
 *
 */
bool awh::alloc::Allocator::source(source_t * source) noexcept {
	// Если распределитель уже заведён
	if(::stage.load(std::memory_order_acquire) != STAGE_COLD)
		/**
		 * Отвечаем отказом
		 *
		 * Заменять источник у кучи, уже раздавшей страницы, значило бы отдавать их
		 * потом не тому, кто выдавал
		 */
		return false;
	// Запоминаем требуемый источник страниц
	::substitute = source;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод получения действующего источника страниц
 *
 * @return действующий источник страниц
 *
 */
awh::alloc::source_t * awh::alloc::Allocator::source() noexcept {
	// Если источник подменён
	if(::substitute != nullptr)
		// Выводим подменённый источник страниц
		return ::substitute;
	// Если распределитель не заведён
	if(::stage.load(std::memory_order_acquire) != STAGE_READY)
		// Действующего источника ещё нет
		return nullptr;
	// Выводим источник страниц по умолчанию
	return &::machinery->system;
}
/**
 * @brief Метод разбора адреса обращения
 *
 * @param addr разбираемый адрес
 * @return     сведения о разобранном адресе
 *
 */
awh::alloc::region_t awh::alloc::Allocator::resolve(const void * addr) noexcept {
	// Сведения о разобранном адресе
	region_t result;
	// Если разбирать нечего
	if(addr == nullptr){
		// Отмечаем адрес нулевой страницей
		result.origin = origin_t::NULLPAGE;
		// Выводим сведения о разобранном адресе
		return result;
	}
	/**
	 * Отмечаем нулевую страницу отдельно
	 *
	 * Разыменование нулевого указателя со смещением - самый частый сбой обращения, и
	 * отличить его от обращения к чужой памяти важнее всего прочего
	 */
	if(reinterpret_cast <uintptr_t> (addr) < static_cast <uintptr_t> (Pages::PAGE)){
		// Отмечаем адрес нулевой страницей
		result.origin = origin_t::NULLPAGE;
		// Выводим сведения о разобранном адресе
		return result;
	}
	// Если распределитель не заведён
	if(::stage.load(std::memory_order_acquire) != STAGE_READY)
		// Выводим сведения о разобранном адресе
		return result;
	/**
	 * Спрашиваем заслоны прежде кучи
	 *
	 * Заслонённые блоки лежат в стороне от кучи, взяты прямо у источника, и куче о них
	 * неизвестно ничего: спроси мы её первой - она честно ответила бы «память чужая»
	 */
	{
		// Адрес начала заслонённого блока
		const void * block = nullptr;
		// Затребованный размер заслонённого блока
		size_t size = 0;
		// Смещение разбираемого адреса от начала блока
		ptrdiff_t offset = 0;
		// Признак освобождённого блока
		bool sealed = false;
		// Если адрес принадлежит заслонённой области
		if(::machinery->guard.resolve(addr, &block, &size, &offset, &sealed)){
			// Записываем адрес начала блока
			result.begin = block;
			// Записываем размер блока
			result.size = size;
			// Записываем смещение разбираемого адреса от начала блока
			result.offset = offset;
			/**
			 * Определяем, чем оказался адрес
			 *
			 * Заслон на то и ставится, чтобы край обращения был известен точно:
			 * смещение ниже нуля означает недобор, смещение за концом - переполнение,
			 * а закрытая область - обращение по освобождённому блоку
			 */
			// Если адрес лежит перед началом блока
			if(offset < 0)
				// Отмечаем адрес недобором
				result.origin = origin_t::UNDERRUN;
			// Если адрес лежит за концом блока
			else if(offset >= static_cast <ptrdiff_t> (size))
				// Отмечаем адрес переполнением
				result.origin = origin_t::OVERRUN;
			// Отмечаем блок живым либо освобождённым
			else result.origin = (sealed ? origin_t::FREED : origin_t::LIVE);
			// Выводим сведения о разобранном адресе
			return result;
		}
	}
	/**
	 * Спрашиваем карантин прежде кучи
	 *
	 * Удерживаемый карантином блок куче принадлежит по-прежнему, и та ответила бы о
	 * нём «область жива»: слоям он ещё не возвращён. Освобождённым его числит один
	 * только карантин
	 */
	{
		// Адрес начала удерживаемого блока
		const void * block = nullptr;
		// Размер удерживаемого блока
		size_t size = 0;
		// Если адрес удерживается карантином
		if(::machinery->quarantine.held(addr, &block, &size)){
			// Записываем адрес начала блока
			result.begin = block;
			// Записываем размер блока
			result.size = size;
			// Записываем смещение разбираемого адреса от начала блока
			result.offset = (reinterpret_cast <const uint8_t *> (addr) - reinterpret_cast <const uint8_t *> (block));
			// Отмечаем блок освобождённым
			result.origin = origin_t::FREED;
			// Выводим сведения о разобранном адресе
			return result;
		}
	}
	// Адрес начала области
	const void * begin = nullptr;
	// Размер области в страницах кучи
	size_t pages = 0;
	// Признак выданной наружу области
	bool live = false;
	// Если адрес куче не принадлежит
	if(!::machinery->pages.locate(addr, &begin, &pages, &live)){
		// Отмечаем память нам не принадлежащей
		result.origin = origin_t::FOREIGN;
		// Выводим сведения о разобранном адресе
		return result;
	}
	// Записываем адрес начала области
	result.begin = begin;
	// Записываем размер области
	result.size = (pages * Pages::PAGE);
	// Отмечаем область живой либо освобождённой
	result.origin = (live ? origin_t::LIVE : origin_t::FREED);
	/**
	 * Уточняем ответ до блока разряда
	 *
	 * Куча знает лишь области своих страниц, а прикладному коду выдан блок внутри
	 * области - и сказать «адрес в области на четыре килобайта» значит не сказать
	 * почти ничего. Блоки разряда одинаковы, оттого начало нужного берётся делением
	 *
	 * Живость блока при этом остаётся живостью ОБЛАСТИ: свободный блок лежит в списках
	 * без всякой отметки, а перебор списков и кэшей всех потоков у самого сбоя
	 * ненадёжен вдвойне. Освобождённый блок опознаёт карантин, спрошенный выше
	 */
	if(live){
		// Номер разряда, которому принадлежит адрес
		size_t which = 0;
		// Адрес начала области разряда
		void * region = nullptr;
		// Размер области разряда в байтах
		size_t size = 0;
		// Если адрес принадлежит области разряда
		if(::machinery->central.owner(addr, &which, &region, &size) && (which < ::machinery->classes.count())){
			// Определяем размер блока разряда
			const size_t block = ::machinery->classes.size(which);
			// Определяем смещение разбираемого адреса от начала области
			const size_t shift = (reinterpret_cast <const uint8_t *> (addr) - reinterpret_cast <const uint8_t *> (region));
			// Записываем адрес начала блока
			result.begin = (reinterpret_cast <const uint8_t *> (region) + ((shift / block) * block));
			// Записываем размер блока
			result.size = block;
		}
	}
	// Записываем смещение разбираемого адреса от начала блока
	result.offset = (reinterpret_cast <const uint8_t *> (addr) - reinterpret_cast <const uint8_t *> (result.begin));
	// Выводим сведения о разобранном адресе
	return result;
}
/**
 * @brief Метод перебора удерживаемой прикладным кодом памяти
 *
 * @param callback отклик перебора
 * @return         число перебранных блоков
 *
 */
size_t awh::alloc::Allocator::holdings(function <bool (const holding_t &)> callback) noexcept {
	// Если распределитель не заведён либо перебирать нечем
	if((::stage.load(std::memory_order_acquire) != STAGE_READY) || (callback == nullptr))
		// Перебирать нечего
		return 0;
	/**
	 * Отдаём отклик перебору через предмет, а не замыканием
	 *
	 * Учёт мест выдачи о `std::function` не знает и знать не должен: он лежит слоем
	 * ниже и стандартной библиотекой не пользуется вовсе - та ходит за памятью в нас же
	 */
	return ::machinery->profile.walk([](const holding_t & holding, void * context) noexcept -> bool {
		// Выводим решение отклика перебора
		return (* reinterpret_cast <function <bool (const holding_t &)> *> (context))(holding);
	}, &callback);
}
/**
 * @brief Метод обращения адреса стека вызовов в имя
 *
 * @param frame  разбираемый адрес
 * @param symbol сведения о разобранном адресе
 * @return       признак разбора адреса
 *
 */
bool awh::alloc::Allocator::symbol(const void * frame, symbol_t & symbol) noexcept {
	// Если распределитель не заведён
	if(::stage.load(std::memory_order_acquire) != STAGE_READY)
		// Разбирать нечем
		return false;
	// Выводим результат обращения адреса стека в имя
	return ::machinery->trace.resolve(frame, symbol);
}
/**
 * @brief Метод установки отклика на крупное выделение
 *
 * @param callback функция обратного вызова
 *
 */
void awh::alloc::Allocator::onLarge(function <void (const void *, const size_t)> callback) noexcept {
	{
		// Захватываем замок откликов доклада
		hold_t hold(::callbackLock);
		// Запоминаем отклик на крупное выделение
		::largeCallback = callback;
	}
	/**
	 * Заводим поток доклада
	 *
	 * Заводим здесь, а не при захвате: поток нужен лишь тому, кто отклики поставил, а
	 * заведение его само обращается за памятью - изнутри выдачи того делать нельзя
	 */
	::awaken();
}
/**
 * @brief Метод установки отклика на достижение потолка кучи
 *
 * @param callback функция обратного вызова
 *
 */
void awh::alloc::Allocator::onLimit(function <void (const size_t)> callback) noexcept {
	{
		// Захватываем замок откликов доклада
		hold_t hold(::callbackLock);
		// Запоминаем отклик на достижение потолка кучи
		::limitCallback = callback;
	}
	// Заводим поток доклада
	::awaken();
}
