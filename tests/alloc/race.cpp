/**
 * @file race.cpp
 * @date 2026-09-29
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
 * @brief Щуп гонок поток-локальных кэшей под ThreadSanitizer
 *
 * @details Набор проверок распределителя гонок назвать не может по устройству: у macOS
 *          под любым надзирателем зону забирает он сам и выдачу ведём уже не мы, а у
 *          систем ELF наш перехват валит TSan при заведении его перехватчиков. Щуп этот
 *          обходит оба препятствия разом - перехвата в нём нет вовсе, а кэши, центральные
 *          списки и страничная куча заводятся им самим как обычные объекты и собираются
 *          ПОД надзором. Память самого щупа раздаёт надзиратель, а блоки кэшей - наша
 *          страничная куча.
 *
 *          Обстановка повторяет боевую: работники берут и отдают блоки своих кэшей, пока
 *          чужие потоки правят потолок, опрашивают расход и просят кэши опустошиться, а
 *          сами работники завершаются и заводятся вновь, переиспользуя кэши ушедших.
 *
 *          Поля кэша, какие трогает чужой поток (`_bytes`, `_limit`, `_kept`, `_tally`,
 *          `_yield`), однажды уже были обычными, и гонки назвал TSan. Щуп сторожит, чтобы
 *          так не стало снова. Способность отказать доказана подменой каждого из пяти
 *          полей по очереди обычным полем того же вида: всякий раз надзиратель назвал
 *          гонки, а без подмены - ни одной (замер 29.09.2026, macOS).
 *
 *          Поля `Caches::_limit` и `Caches::_budget` щуп не сторожит, и не может: все
 *          обращения к ним идут под замком общего списка кэшей, и гонки на них нет при
 *          любом их виде. Подмена их обычными полями надзирателем не замечается - и
 *          верно.
 *
 * @note Собирается и запускается стендом tests/alloc/race.sh, в набор проверок НЕ
 *       входит: набору нужен перехват, щупу - его отсутствие
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Наши модули
 */
#include <alloc/source.hpp>
#include <alloc/pages.hpp>
#include <alloc/classes.hpp>
#include <alloc/central.hpp>
#include <alloc/cache.hpp>

/**
 * Стандартные модули
 */
#include <atomic>
#include <thread>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdint>

/**
 * Число работников одного поколения
 */
static constexpr size_t WORKERS = 4;
/**
 * Число поколений работников
 *
 * Работники завершаются и заводятся вновь: кэши ушедших не разрушаются, а достаются
 * новым потокам, и переход этот - сам по себе место возможной гонки
 */
static constexpr size_t GENERATIONS = 3;
/**
 * Число оборотов работника
 */
static constexpr size_t ROUNDS = 20000;
/**
 * Число блоков, удерживаемых работником за оборот
 */
static constexpr size_t HELD = 16;
/**
 * Потолки кэша, между которыми мечется правящий поток
 *
 * Нуль означает «по усмотрению модуля» и ведёт другим путём - раздачей доли общего
 * запаса, - оттого он в обороте тоже
 */
static constexpr size_t LIMITS[] = {(64u * 1024u), 0u, (4u * 1024u * 1024u), (8u * 1024u)};
/**
 * Порог накопления учёта занятого
 *
 * Берётся тот же, что у распределителя: накопленное сверх него кэш отдаёт наружу
 */
static constexpr int64_t TALLY = (256u * 1024u);

/**
 * Устройство, заводимое щупом
 *
 * Лежит в неизменяемых данных, как и у самого распределителя: кэши завершившихся
 * потоков отпускаются откликом ключа, и устройство обязано их пережить
 */
static awh::alloc::SystemSource source;
static awh::alloc::pages_t pages;
static awh::alloc::classes_t classes;
static awh::alloc::central_t central;
static awh::alloc::caches_t caches;

/**
 * Число несостоявшихся выдач
 */
static std::atomic <size_t> refused(0);
/**
 * Число испорченных блоков
 */
static std::atomic <size_t> spoiled(0);
/**
 * Число оборотов, прошедших мимо кэша
 */
static std::atomic <size_t> bypassed(0);

/**
 * @brief Работа одного работника
 *
 * @param number номер работника
 *
 */
static void work(const size_t number) noexcept {
	// Удерживаемые блоки
	void * held[HELD];
	// Разряды удерживаемых блоков
	size_t index[HELD];
	// Размеры удерживаемых блоков
	size_t size[HELD];
	/**
	 * Перебираем обороты работника
	 */
	for(size_t round = 0; round < ROUNDS; round++){
		/**
		 * Кэш спрашиваем на КАЖДОМ обороте
		 *
		 * Так делает и сам распределитель: кэш потока берётся при каждой выдаче, и
		 * чтение его места - часть того же пути
		 */
		awh::alloc::cache_t * cache = caches.local();
		// Если кэша нет
		if(cache == nullptr){
			// Учитываем оборот, прошедший мимо кэша
			bypassed.fetch_add(1, std::memory_order_relaxed);
			// Переходим к следующему обороту
			continue;
		}
		/**
		 * Берём блоки разных разрядов
		 */
		for(size_t i = 0; i < HELD; i++){
			// Получаем размер блока
			size[i] = (16u + (((round + i + number) % 64u) * 16u));
			// Получаем разряд блока
			index[i] = classes.index(size[i]);
			// Берём блок у кэша
			held[i] = cache->alloc(index[i]);
			// Если блок не выдан
			if(held[i] == nullptr){
				// Учитываем несостоявшуюся выдачу
				refused.fetch_add(1, std::memory_order_relaxed);
				// Продолжать с пустым блоком нечем
				continue;
			}
			// Засеваем блок меткой работника
			::memset(held[i], static_cast <int> ((number + round + i) & 0xFF), size[i]);
			/**
			 * Копим учёт занятого, как копит его распределитель на каждой выдаче
			 *
			 * Накопленное читает чужой поток - опрос расхода, - и без этого вызова
			 * поле учёта оставалось бы вне охвата щупа
			 */
			cache->tally(static_cast <int64_t> (size[i]), TALLY);
		}
		/**
		 * Сверяем и отдаём блоки
		 */
		for(size_t i = 0; i < HELD; i++){
			// Если блок не выдавался
			if(held[i] == nullptr)
				// Отдавать нечего
				continue;
			// Получаем метку блока
			const uint8_t mark = static_cast <uint8_t> ((number + round + i) & 0xFF);
			// Если метка блока испорчена
			if((static_cast <uint8_t *> (held[i])[0] != mark) || (static_cast <uint8_t *> (held[i])[size[i] - 1] != mark))
				// Учитываем испорченный блок
				spoiled.fetch_add(1, std::memory_order_relaxed);
			// Снимаем блок с учёта занятого
			cache->tally(-static_cast <int64_t> (size[i]), TALLY);
			// Отдаём блок кэшу
			cache->free(index[i], held[i]);
		}
	}
}

/**
 * @brief Главная функция щупа
 *
 * @return код возврата
 *
 */
int main() noexcept {
	// Если разряды размеров не построены
	if(classes.init(0) == 0){
		// Сообщаем об отказе
		::fprintf(stderr, "ОТКАЗ: разряды размеров не построены\n");
		// Выходим с признаком отказа
		return 1;
	}
	// Если устройство распределителя не заведено
	if(!pages.init(&source, 0, false) || !central.init(&pages, &classes) || !caches.init(&central, &classes) || !caches.arm()){
		// Сообщаем об отказе
		::fprintf(stderr, "ОТКАЗ: устройство распределителя не заведено\n");
		// Выходим с признаком отказа
		return 1;
	}
	// Признак работы чужих потоков
	std::atomic <bool> running(true);
	/**
	 * Правящий поток: переставляет потолок кэшей
	 */
	std::thread setter([&running]() noexcept {
		// Номер очередного потолка
		size_t step = 0;
		// Пока работники трудятся
		while(running.load(std::memory_order_relaxed))
			// Задаём очередной потолок
			caches.limit(LIMITS[step++ % (sizeof(LIMITS) / sizeof(LIMITS[0]))]);
	});
	/**
	 * Опрашивающий поток: складывает расход по всем кэшам
	 */
	std::thread reader([&running]() noexcept {
		// Число заведённых кэшей
		size_t count = 0;
		// Сумма ответов, чтобы опрос не выбросил собиратель
		volatile size_t sink = 0;
		// Пока работники трудятся
		while(running.load(std::memory_order_relaxed)){
			// Складываем лежащее в кэшах
			sink = (sink + caches.cached(&count));
			// Складываем накопленное кэшами
			sink = (sink + static_cast <size_t> (caches.pending()));
		}
	});
	/**
	 * Опустошающий поток: просит чужие кэши отдать блоки
	 *
	 * Сам кэша он не заводит - и просьба уходит хозяевам всех кэшей до единого
	 */
	std::thread flusher([&running]() noexcept {
		// Пока работники трудятся
		while(running.load(std::memory_order_relaxed)){
			// Просим кэши опустошиться
			caches.flush();
			// Уступаем время работникам
			std::this_thread::yield();
		}
	});
	/**
	 * Перебираем поколения работников
	 */
	for(size_t generation = 0; generation < GENERATIONS; generation++){
		// Работники поколения
		std::vector <std::thread> workers;
		// Отводим место под работников
		workers.reserve(WORKERS);
		// Заводим работников
		for(size_t i = 0; i < WORKERS; i++)
			// Заводим очередного работника
			workers.emplace_back(work, (generation * WORKERS) + i);
		// Дожидаемся работников
		for(std::thread & worker : workers)
			// Дожидаемся очередного работника
			worker.join();
	}
	// Останавливаем чужие потоки
	running.store(false, std::memory_order_relaxed);
	// Дожидаемся правящего потока
	setter.join();
	// Дожидаемся опрашивающего потока
	reader.join();
	// Дожидаемся опустошающего потока
	flusher.join();
	// Число заведённых кэшей
	size_t count = 0;
	// Получаем лежащее в кэшах
	const size_t cached = caches.cached(&count);
	// Сообщаем итог
	::printf("кэшей: %zu, лежит: %zu, мимо кэша: %zu, отказов: %zu, порчи: %zu\n", count, cached, bypassed.load(), refused.load(), spoiled.load());
	/**
	 * Кэшей не может быть больше, чем работников одного поколения
	 *
	 * Кэши ушедших потоков переиспользуются, и заведение нового на каждом поколении
	 * значило бы, что отпускание кэша потерялось
	 */
	const bool reused = (count <= WORKERS);
	// Если кэши не переиспользовались
	if(!reused)
		// Сообщаем об отказе
		::fprintf(stderr, "ОТКАЗ: кэшей %zu при %zu работниках - кэши ушедших потоков не переиспользованы\n", count, WORKERS);
	// Если хоть один оборот прошёл мимо кэша
	if(bypassed.load() > 0)
		// Сообщаем об отказе
		::fprintf(stderr, "ОТКАЗ: оборотов мимо кэша %zu - щуп кэши не проверил\n", bypassed.load());
	// Выводим итог
	return (((refused.load() == 0) && (spoiled.load() == 0) && (bypassed.load() == 0) && reused) ? 0 : 1);
}
