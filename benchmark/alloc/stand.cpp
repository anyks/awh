/**
 * @file stand.cpp
 * @date 2026-08-22
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
 * @brief Отдельный стенд замеров распределителя памяти
 *
 * @details Собирается ДВАЖДЫ из одного исходника: с нашим распределителем и с
 *          системным. Разница между сборками одна - какой распределитель обслуживает
 *          выдачу, - и всё прочее в них совпадает дословно. Сличать иначе нельзя:
 *          вторая разница обращает сличение в догадку
 *
 * @note Показатели снимаются ЛУЧШИМ из нескольких прогонов, а не средним: ядра Apple
 *       Silicon двумодальны, и середина внутри прогона смещена целиком
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

/**
 * Если стенд собран с нашим распределителем
 */
#if !defined(AWH_BENCH_SYSTEM)
	/**
	 * Подключаем наши модули
	 */
	#include <alloc/alloc.hpp>
	#include <sys/log.hpp>
	/**
	 * @brief Метод печати сообщения журнала
	 *
	 * @note Журнал стенду не нужен, а связывание его требует: подставляем пустое тело
	 *
	 */
	void awh::Logging::print(std::string_view, flag_t, ...) const noexcept {}
#endif

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	// Число прогонов сценария: наружу идёт лучший
	static constexpr size_t ROUNDS = 5;
	/**
	 * @brief Метод получения времени в микросекундах
	 *
	 * @return время в микросекундах
	 *
	 */
	static uint64_t now() noexcept {
		// Выводим время в микросекундах
		return static_cast <uint64_t> (std::chrono::duration_cast <std::chrono::microseconds> (
			std::chrono::steady_clock::now().time_since_epoch()).count());
	}
	/**
	 * @brief Метод печати показателя сценария
	 *
	 * @param name название сценария
	 * @param ops  число выполненных действий
	 * @param spent затраченное время в микросекундах
	 *
	 */
	static void report(const char * name, const uint64_t ops, const uint64_t spent) noexcept {
		/**
		 * Молчащий сценарий отчитывается ОТКАЗОМ, а не нулём
		 *
		 * Показатель вида «на одно действие» при нуле действий выходит нулём, а ноль
		 * укладывается в любой порог с верхней границей: сценарий, не сделавший ничего,
		 * выходил бы зелёным
		 */
		if((ops == 0) || (spent == 0)){
			// Печатаем отказ сценария
			printf("%-28s НЕДЕЙСТВИТЕЛЬНО (действий: %llu)\n", name, static_cast <unsigned long long> (ops));
			// Выходим из функции
			return;
		}
		// Печатаем показатель сценария
		printf("%-28s %10.2f тыс.оп/с   %8.2f нс/оп\n", name,
		 (static_cast <double> (ops) / static_cast <double> (spent)) * 1000.0,
		 (static_cast <double> (spent) * 1000.0) / static_cast <double> (ops));
	}
	/**
	 * @brief Сценарий: выдача и освобождение блоков одного разряда
	 *
	 * @return затраченное время в микросекундах
	 *
	 */
	static uint64_t fixed() noexcept {
		// Число блоков в круге
		constexpr size_t BATCH = 512;
		// Число кругов
		constexpr size_t LOOPS = 4000;
		// Выданные блоки
		static void * blocks[BATCH];
		// Запоминаем время начала
		const uint64_t begin = ::now();
		/**
		 * Перебираем круги нагрузки
		 */
		for(size_t round = 0; round < LOOPS; round++){
			// Выдаём блоки круга
			for(size_t i = 0; i < BATCH; i++)
				// Выдаём блок
				blocks[i] = ::malloc(64);
			// Освобождаем блоки круга
			for(size_t i = 0; i < BATCH; i++)
				// Освобождаем блок
				::free(blocks[i]);
		}
		// Выводим затраченное время
		return (::now() - begin);
	}
	/**
	 * @brief Сценарий: выдача блоков изменчивого размера
	 *
	 * @return затраченное время в микросекундах
	 *
	 */
	static uint64_t mixed() noexcept {
		// Число блоков в круге
		constexpr size_t BATCH = 512;
		// Число кругов
		constexpr size_t LOOPS = 2000;
		// Выданные блоки
		static void * blocks[BATCH];
		// Запоминаем время начала
		const uint64_t begin = ::now();
		/**
		 * Перебираем круги нагрузки
		 */
		for(size_t round = 0; round < LOOPS; round++){
			// Выдаём блоки круга
			for(size_t i = 0; i < BATCH; i++)
				// Выдаём блок изменчивого размера
				blocks[i] = ::malloc(16 + (((i * 37) + round) % 4080));
			// Освобождаем блоки круга
			for(size_t i = 0; i < BATCH; i++)
				// Освобождаем блок
				::free(blocks[i]);
		}
		// Выводим затраченное время
		return (::now() - begin);
	}
	/**
	 * @brief Сценарий: выдача крупных блоков
	 *
	 * @return затраченное время в микросекундах
	 *
	 */
	static uint64_t large() noexcept {
		// Число блоков в круге
		constexpr size_t BATCH = 64;
		// Число кругов
		constexpr size_t LOOPS = 500;
		// Выданные блоки
		static void * blocks[BATCH];
		// Запоминаем время начала
		const uint64_t begin = ::now();
		/**
		 * Перебираем круги нагрузки
		 */
		for(size_t round = 0; round < LOOPS; round++){
			// Выдаём блоки круга
			for(size_t i = 0; i < BATCH; i++){
				// Выдаём крупный блок
				blocks[i] = ::malloc(64u * 1024u);
				// Трогаем начало блока: невыделенная страница иначе не отводится вовсе
				if(blocks[i] != nullptr)
					// Записываем начало блока
					::memset(blocks[i], 0x5A, 64);
			}
			// Освобождаем блоки круга
			for(size_t i = 0; i < BATCH; i++)
				// Освобождаем блок
				::free(blocks[i]);
		}
		// Выводим затраченное время
		return (::now() - begin);
	}
	/**
	 * @brief Сценарий: удержание множества блоков
	 *
	 * @note Круг «выдал и тут же отдал» ложится в поток-локальный кэш целиком и меряет
	 *       лишь его. Удержание вынуждает распределитель ходить к центральным спискам
	 *       и страничной куче - там и живёт настоящая цена
	 *
	 * @return затраченное время в микросекундах
	 *
	 */
	static uint64_t holding() noexcept {
		// Число удерживаемых блоков
		constexpr size_t HELD = 65536;
		// Число кругов
		constexpr size_t LOOPS = 8;
		// Удерживаемые блоки
		static void * blocks[HELD];
		// Запоминаем время начала
		const uint64_t begin = ::now();
		/**
		 * Перебираем круги нагрузки
		 */
		for(size_t round = 0; round < LOOPS; round++){
			// Выдаём удерживаемые блоки
			for(size_t i = 0; i < HELD; i++)
				// Выдаём блок изменчивого разряда
				blocks[i] = ::malloc(32 + ((i % 8) * 32));
			/**
			 * Освобождаем блоки ВРАЗБИВКУ
			 *
			 * Освобождение подряд возвращает блоки в порядке выдачи и льстит всякому
			 * распределителю: настоящая нагрузка отдаёт их вперемешку
			 */
			for(size_t i = 0; i < HELD; i += 3)
				// Освобождаем блок
				::free(blocks[i]);
			// Освобождаем оставшиеся блоки
			for(size_t i = 1; i < HELD; i += 3)
				// Освобождаем блок
				::free(blocks[i]);
			// Освобождаем последние блоки
			for(size_t i = 2; i < HELD; i += 3)
				// Освобождаем блок
				::free(blocks[i]);
		}
		// Выводим затраченное время
		return (::now() - begin);
	}
	/**
	 * @brief Сценарий: перевыдача с ростом
	 *
	 * @return затраченное время в микросекундах
	 *
	 */
	static uint64_t growth() noexcept {
		// Число кругов
		constexpr size_t LOOPS = 2000;
		// Запоминаем время начала
		const uint64_t begin = ::now();
		/**
		 * Перебираем круги нагрузки
		 */
		for(size_t round = 0; round < LOOPS; round++){
			// Выданная память
			void * block = ::malloc(16);
			/**
			 * Растим блок удвоением
			 */
			for(size_t size = 32; size <= (256u * 1024u); size <<= 1)
				// Перевыдаём блок вдвое большего размера
				block = ::realloc(block, size);
			// Освобождаем блок
			::free(block);
		}
		// Выводим затраченное время
		return (::now() - begin);
	}
	/**
	 * @brief Сценарий: выдача несколькими потоками
	 *
	 * @param threads число потоков нагрузки
	 * @return        затраченное время в микросекундах
	 *
	 */
	static uint64_t threaded(const size_t threads) noexcept {
		// Число блоков в круге
		constexpr size_t BATCH = 256;
		// Число кругов на поток
		constexpr size_t LOOPS = 2000;
		// Потоки нагрузки
		std::vector <std::thread> workers;
		// Отводим место под потоки
		workers.reserve(threads);
		// Запоминаем время начала
		const uint64_t begin = ::now();
		/**
		 * Перебираем потоки нагрузки
		 */
		for(size_t t = 0; t < threads; t++){
			// Заводим поток нагрузки
			workers.emplace_back([]() noexcept {
				// Выданные блоки
				void * blocks[BATCH];
				/**
				 * Перебираем круги нагрузки
				 */
				for(size_t round = 0; round < LOOPS; round++){
					// Выдаём блоки круга
					for(size_t i = 0; i < BATCH; i++)
						// Выдаём блок изменчивого разряда
						blocks[i] = ::malloc(32 + ((i % 16) * 64));
					// Освобождаем блоки круга
					for(size_t i = 0; i < BATCH; i++)
						// Освобождаем блок
						::free(blocks[i]);
				}
			});
		}
		/**
		 * Перебираем потоки нагрузки
		 */
		for(auto & worker : workers)
			// Дожидаемся потока нагрузки
			worker.join();
		// Выводим затраченное время
		return (::now() - begin);
	}
	/**
	 * @brief Сценарий: освобождение ЧУЖИМ потоком
	 *
	 * @note Случай этот - самый неудобный всякому распределителю с кэшами потоков:
	 *       блок, выданный одним потоком, возвращается в кэш другого, и уравнивать их
	 *       приходится через общий слой
	 *
	 * @param threads число пар потоков
	 * @return        затраченное время в микросекундах
	 *
	 */
	static uint64_t crossing(const size_t threads) noexcept {
		// Число блоков в круге
		constexpr size_t BATCH = 4096;
		// Число кругов на пару
		constexpr size_t LOOPS = 64;
		// Потоки нагрузки
		std::vector <std::thread> workers;
		// Переданные блоки
		std::vector <std::vector <void *>> parcels(threads);
		// Отводим место под потоки
		workers.reserve(threads * 2);
		// Запоминаем время начала
		const uint64_t begin = ::now();
		/**
		 * Перебираем пары потоков
		 */
		for(size_t t = 0; t < threads; t++){
			// Получаем место передачи блоков
			std::vector <void *> & parcel = parcels[t];
			// Отводим место под передаваемые блоки
			parcel.resize(BATCH * LOOPS, nullptr);
			// Заводим поток выдачи
			workers.emplace_back([&parcel]() noexcept {
				/**
				 * Перебираем передаваемые блоки
				 */
				for(size_t i = 0; i < parcel.size(); i++)
					// Выдаём блок изменчивого разряда
					parcel[i] = ::malloc(48 + ((i % 8) * 48));
			});
		}
		/**
		 * Перебираем потоки выдачи
		 */
		for(auto & worker : workers)
			// Дожидаемся потока выдачи
			worker.join();
		// Очищаем список потоков
		workers.clear();
		/**
		 * Перебираем пары потоков
		 */
		for(size_t t = 0; t < threads; t++){
			// Получаем место передачи блоков
			std::vector <void *> & parcel = parcels[(t + 1) % threads];
			// Заводим поток освобождения ЧУЖИХ блоков
			workers.emplace_back([&parcel]() noexcept {
				/**
				 * Перебираем переданные блоки
				 */
				for(size_t i = 0; i < parcel.size(); i++)
					// Освобождаем чужой блок
					::free(parcel[i]);
			});
		}
		/**
		 * Перебираем потоки освобождения
		 */
		for(auto & worker : workers)
			// Дожидаемся потока освобождения
			worker.join();
		// Выводим затраченное время
		return (::now() - begin);
	}
	/**
	 * @brief Метод прогона сценария лучшим из нескольких заходов
	 *
	 * @param name название сценария
	 * @param ops  число действий сценария
	 * @param run  тело сценария
	 *
	 */
	static void measure(const char * name, const uint64_t ops, uint64_t (* run)()) noexcept {
		// Лучшее затраченное время
		uint64_t best = 0;
		/**
		 * Перебираем заходы сценария
		 */
		for(size_t i = 0; i < ROUNDS; i++){
			// Прогоняем сценарий
			const uint64_t spent = run();
			// Запоминаем лучшее время
			if((best == 0) || (spent < best))
				// Запоминаем затраченное время
				best = spent;
		}
		// Печатаем показатель сценария
		::report(name, ops, best);
	}
};

/**
 * @brief Главная функция стенда
 *
 * @return код возврата
 *
 */
int main(){
	/**
	 * Если стенд собран с нашим распределителем
	 */
	#if !defined(AWH_BENCH_SYSTEM)
		// Настройки распределителя памяти
		awh::alloc::options_t options;
		// Захватываем выдачу памяти процесса
		if(!awh::alloc::Allocator::capture(options, nullptr)){
			// Печатаем отказ захвата
			printf("ЗАХВАТ НЕ СОСТОЯЛСЯ: замерять нечего\n");
			// Выходим с признаком отказа
			return 1;
		}
		// Печатаем название распределителя
		printf("распределитель: НАШ (awh::alloc)\n\n");
	/**
	 * Если стенд собран с системным распределителем
	 */
	#else
		// Печатаем название распределителя
		printf("распределитель: СИСТЕМНЫЙ\n\n");
	#endif
	// Число ядер машины
	const size_t cores = ((std::thread::hardware_concurrency() > 0) ? std::thread::hardware_concurrency() : 4);
	// Печатаем заголовок таблицы
	printf("%-28s %-22s %s\n", "сценарий", "полоса", "цена действия");
	// Снимаем показатель выдачи блоков одного разряда
	::measure("один разряд", (512ull * 4000ull * 2ull), &::fixed);
	// Снимаем показатель выдачи блоков изменчивого размера
	::measure("изменчивый размер", (512ull * 2000ull * 2ull), &::mixed);
	// Снимаем показатель выдачи крупных блоков
	::measure("крупные блоки", (64ull * 500ull * 2ull), &::large);
	// Снимаем показатель удержания множества блоков
	::measure("удержание вразбивку", (65536ull * 8ull * 2ull), &::holding);
	// Снимаем показатель перевыдачи с ростом
	::measure("рост перевыдачей", (2000ull * 15ull), &::growth);
	// Лучшее время выдачи несколькими потоками
	uint64_t best = 0;
	/**
	 * Перебираем заходы сценария потоков
	 */
	for(size_t i = 0; i < ROUNDS; i++){
		// Прогоняем сценарий потоков
		const uint64_t spent = ::threaded(cores);
		// Запоминаем лучшее время
		if((best == 0) || (spent < best))
			// Запоминаем затраченное время
			best = spent;
	}
	// Печатаем показатель выдачи несколькими потоками
	::report("потоки по числу ядер", (256ull * 2000ull * 2ull * cores), best);
	// Обнуляем лучшее время
	best = 0;
	/**
	 * Перебираем заходы сценария передачи блоков
	 */
	for(size_t i = 0; i < ROUNDS; i++){
		// Прогоняем сценарий передачи блоков
		const uint64_t spent = ::crossing(4);
		// Запоминаем лучшее время
		if((best == 0) || (spent < best))
			// Запоминаем затраченное время
			best = spent;
	}
	// Печатаем показатель освобождения чужим потоком
	::report("освобождение чужим", (4096ull * 64ull * 2ull * 4ull), best);
	// Печатаем число ядер машины
	printf("\nядер: %zu, заходов на сценарий: %zu (наружу идёт лучший)\n", cores, ROUNDS);
	// Выводим успешный код возврата
	return 0;
}
