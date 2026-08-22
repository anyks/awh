/**
 * @file speed.cpp
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
 * @brief Сценарии измерения распределителя памяти — выдача и освобождение блоков
 *        разрядами, крупными кусками, вразбивку, перевыдачей и несколькими потоками
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков
 */
#include "../main.hpp"

/**
 * Подключаем наши модули
 */
#include <alloc/alloc.hpp>

/**
 * Стандартные модули
 */
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние параметры и сценарии бенчмарков распределителя памяти
 *
 */
namespace {
	/**
	 * @brief Пороги пропускной способности в действиях в секунду
	 *
	 * @details Назначены по САМОЙ МЕДЛЕННОЙ из пройденных машин с запасом около
	 *          половины: порог, снятый на быстрой, отказывает на медленной без всякой
	 *          регрессии. Снятые показатели — в COMPARISON.md
	 *
	 */
	static constexpr double FIXED_THRESHOLD = 12000000.0;
	static constexpr double MIXED_THRESHOLD = 11000000.0;
	/**
	 * @brief Порог выдачи крупных блоков
	 *
	 * @details Взят по NetBSD и OpenBSD: там выдача крупного блока идёт к системе и
	 *          стоит 1,5-1,7 микросекунды против 120 наносекунд у macOS - разница в
	 *          тринадцать раз. Порог, снятый по быстрым машинам, отказывал бы там без
	 *          всякой регрессии
	 *
	 */
	static constexpr double LARGE_THRESHOLD = 400000.0;
	static constexpr double HOLDING_THRESHOLD = 6000000.0;
	static constexpr double GROWTH_THRESHOLD = 300000.0;
	/**
	 * @brief Порог многопоточной нагрузки
	 *
	 * @details Взят с особенным запасом: показатель этот от числа ядер зависит сильнее
	 *          прочих, а машины стендов расходятся от восьми ядер до сорока
	 *
	 */
	static constexpr double THREADED_THRESHOLD = 40000000.0;
	static constexpr double CROSSING_THRESHOLD = 10000000.0;
	/**
	 * @brief Метод захвата выдачи памяти процесса
	 *
	 * @details Захват берётся ОДИН на процесс и здесь же: у систем ELF наш `malloc`
	 *          стоит в двоичном файле с самого связывания, а у macOS выдача до захвата
	 *          идёт зоной системы - замер тогда снимал бы ЧУЖОЙ распределитель и
	 *          отчитывался бы о нашем
	 *
	 * @return признак состоявшегося захвата
	 *
	 */
	static bool captured() noexcept {
		// Признак состоявшегося захвата
		static const bool outcome = []() noexcept -> bool {
			// Настройки распределителя памяти
			awh::alloc::options_t options;
			// Захватываем выдачу памяти процесса
			return awh::alloc::Allocator::capture(options, nullptr);
		}();
		// Выводим признак состоявшегося захвата
		return outcome;
	}
	/**
	 * @brief Метод получения времени в микросекундах
	 *
	 * @return время в микросекундах
	 *
	 */
	static uint64_t stamp() noexcept {
		// Выводим время в микросекундах
		return static_cast <uint64_t> (chrono::duration_cast <chrono::microseconds> (
			chrono::steady_clock::now().time_since_epoch()).count());
	}
	/**
	 * @brief Метод оформления итога замера
	 *
	 * @param ops   число выполненных действий
	 * @param spent затраченное время в микросекундах
	 * @return      итог замера
	 *
	 */
	static awh::benchmark::result_t outcome(const uint64_t ops, const uint64_t spent) noexcept {
		// Итог замера
		awh::benchmark::result_t result;
		// Если захват выдачи памяти не состоялся
		if(!::captured()){
			/**
			 * Замер не выполнялся вовсе
			 *
			 * Под санитайзерами захват состояться не может: они подменяют выдачу памяти
			 * собою. Это не удача и не провал - мерить попросту нечего
			 */
			result.skipped = true;
			// Записываем причину, по которой замер не выполнялся
			result.reason = "захват выдачи памяти процесса не состоялся";
			// Выводим итог замера
			return result;
		}
		/**
		 * Молчащий сценарий отчитывается НЕДЕЙСТВИТЕЛЬНЫМ, а не нулём
		 *
		 * Ноль укладывается в любой порог с верхней границей, и сценарий, не сделавший
		 * ничего, выходил бы зелёным
		 */
		if((ops == 0) || (spent == 0)){
			// Отмечаем замер недействительным
			result.invalid = true;
			// Выводим итог замера
			return result;
		}
		// Записываем измеренную пропускную способность
		result.value = ((static_cast <double> (ops) * 1000000.0) / static_cast <double> (spent));
		// Записываем цену одного действия
		result.details = ("цена действия: " + to_string((static_cast <double> (spent) * 1000.0) / static_cast <double> (ops)) + " нс");
		// Выводим итог замера
		return result;
	}
	/**
	 * @brief Сценарий выдачи блоков одного разряда
	 *
	 * @return итог замера
	 *
	 */
	static awh::benchmark::result_t uniform() noexcept {
		// Если захват выдачи памяти не состоялся
		if(!::captured())
			// Мерить нечего
			return ::outcome(0, 0);
		// Число блоков в круге
		constexpr size_t BATCH = 512;
		// Число кругов
		constexpr size_t LOOPS = 2000;
		// Выданные блоки
		static void * blocks[BATCH];
		// Запоминаем время начала
		const uint64_t begin = ::stamp();
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
		// Выводим итог замера
		return ::outcome((BATCH * LOOPS * 2), (::stamp() - begin));
	}
	/**
	 * @brief Сценарий выдачи блоков изменчивого размера
	 *
	 * @return итог замера
	 *
	 */
	static awh::benchmark::result_t mixed() noexcept {
		// Если захват выдачи памяти не состоялся
		if(!::captured())
			// Мерить нечего
			return ::outcome(0, 0);
		// Число блоков в круге
		constexpr size_t BATCH = 512;
		// Число кругов
		constexpr size_t LOOPS = 1000;
		// Выданные блоки
		static void * blocks[BATCH];
		// Запоминаем время начала
		const uint64_t begin = ::stamp();
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
		// Выводим итог замера
		return ::outcome((BATCH * LOOPS * 2), (::stamp() - begin));
	}
	/**
	 * @brief Сценарий выдачи крупных блоков
	 *
	 * @return итог замера
	 *
	 */
	static awh::benchmark::result_t bulky() noexcept {
		// Если захват выдачи памяти не состоялся
		if(!::captured())
			// Мерить нечего
			return ::outcome(0, 0);
		// Число блоков в круге
		constexpr size_t BATCH = 64;
		// Число кругов
		constexpr size_t LOOPS = 250;
		// Выданные блоки
		static void * blocks[BATCH];
		// Запоминаем время начала
		const uint64_t begin = ::stamp();
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
		// Выводим итог замера
		return ::outcome((BATCH * LOOPS * 2), (::stamp() - begin));
	}
	/**
	 * @brief Сценарий удержания множества блоков с освобождением вразбивку
	 *
	 * @details Круг «выдал и тут же отдал» ложится в поток-локальный кэш целиком и
	 *          меряет лишь его. Удержание вынуждает распределитель ходить к центральным
	 *          спискам и страничной куче - там и живёт настоящая цена
	 *
	 * @return итог замера
	 *
	 */
	static awh::benchmark::result_t holding() noexcept {
		// Если захват выдачи памяти не состоялся
		if(!::captured())
			// Мерить нечего
			return ::outcome(0, 0);
		// Число удерживаемых блоков
		constexpr size_t HELD = 65536;
		// Число кругов
		constexpr size_t LOOPS = 4;
		// Удерживаемые блоки
		static void * blocks[HELD];
		// Запоминаем время начала
		const uint64_t begin = ::stamp();
		/**
		 * Перебираем круги нагрузки
		 */
		for(size_t round = 0; round < LOOPS; round++){
			// Выдаём удерживаемые блоки
			for(size_t i = 0; i < HELD; i++)
				// Выдаём блок изменчивого разряда
				blocks[i] = ::malloc(32 + ((i % 8) * 32));
			/**
			 * Освобождаем блоки вразбивку
			 *
			 * Освобождение подряд возвращает блоки в порядке выдачи и льстит всякому
			 * распределителю: настоящая нагрузка отдаёт их вперемешку
			 */
			for(size_t step = 0; step < 3; step++){
				// Перебираем блоки с шагом
				for(size_t i = step; i < HELD; i += 3)
					// Освобождаем блок
					::free(blocks[i]);
			}
		}
		// Выводим итог замера
		return ::outcome((HELD * LOOPS * 2), (::stamp() - begin));
	}
	/**
	 * @brief Сценарий перевыдачи с ростом
	 *
	 * @return итог замера
	 *
	 */
	static awh::benchmark::result_t growth() noexcept {
		// Если захват выдачи памяти не состоялся
		if(!::captured())
			// Мерить нечего
			return ::outcome(0, 0);
		// Число кругов
		constexpr size_t LOOPS = 1000;
		// Запоминаем время начала
		const uint64_t begin = ::stamp();
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
		// Выводим итог замера
		return ::outcome((LOOPS * 15), (::stamp() - begin));
	}
	/**
	 * @brief Сценарий выдачи несколькими потоками
	 *
	 * @return итог замера
	 *
	 */
	static awh::benchmark::result_t threaded() noexcept {
		// Если захват выдачи памяти не состоялся
		if(!::captured())
			// Мерить нечего
			return ::outcome(0, 0);
		// Число блоков в круге
		constexpr size_t BATCH = 256;
		// Число кругов на поток
		constexpr size_t LOOPS = 1000;
		// Число потоков нагрузки
		const size_t count = ((thread::hardware_concurrency() > 0) ? thread::hardware_concurrency() : 4);
		// Потоки нагрузки
		vector <thread> workers;
		// Отводим место под потоки
		workers.reserve(count);
		// Запоминаем время начала
		const uint64_t begin = ::stamp();
		/**
		 * Перебираем потоки нагрузки
		 */
		for(size_t t = 0; t < count; t++){
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
		// Выводим итог замера
		return ::outcome((BATCH * LOOPS * 2 * count), (::stamp() - begin));
	}
	/**
	 * @brief Сценарий освобождения блока чужим потоком
	 *
	 * @details Случай этот - самый неудобный всякому распределителю с кэшами потоков:
	 *          блок, выданный одним потоком, возвращается в кэш другого
	 *
	 * @return итог замера
	 *
	 */
	static awh::benchmark::result_t crossing() noexcept {
		// Если захват выдачи памяти не состоялся
		if(!::captured())
			// Мерить нечего
			return ::outcome(0, 0);
		// Число блоков на поток
		constexpr size_t BATCH = 65536;
		// Число пар потоков
		constexpr size_t PAIRS = 4;
		// Переданные блоки
		vector <vector <void *>> parcels(PAIRS);
		// Потоки нагрузки
		vector <thread> workers;
		// Отводим место под потоки
		workers.reserve(PAIRS);
		// Запоминаем время начала
		const uint64_t begin = ::stamp();
		/**
		 * Перебираем пары потоков
		 */
		for(size_t t = 0; t < PAIRS; t++){
			// Получаем место передачи блоков
			vector <void *> & parcel = parcels[t];
			// Отводим место под передаваемые блоки
			parcel.resize(BATCH, nullptr);
			// Заводим поток выдачи
			workers.emplace_back([&parcel]() noexcept {
				// Перебираем передаваемые блоки
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
		for(size_t t = 0; t < PAIRS; t++){
			// Получаем место передачи блоков соседа
			vector <void *> & parcel = parcels[(t + 1) % PAIRS];
			// Заводим поток освобождения чужих блоков
			workers.emplace_back([&parcel]() noexcept {
				// Перебираем переданные блоки
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
		// Выводим итог замера
		return ::outcome((BATCH * 2 * PAIRS), (::stamp() - begin));
	}

	// Регистрируем сценарий выдачи блоков одного разряда
	static const bool gFixed = awh::benchmark::add(
		"alloc/speed/fixed", "действий/с", FIXED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::uniform
	);
	// Регистрируем сценарий выдачи блоков изменчивого размера
	static const bool gMixed = awh::benchmark::add(
		"alloc/speed/mixed", "действий/с", MIXED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::mixed
	);
	// Регистрируем сценарий выдачи крупных блоков
	static const bool gLarge = awh::benchmark::add(
		"alloc/speed/large", "действий/с", LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bulky
	);
	// Регистрируем сценарий удержания множества блоков
	static const bool gHolding = awh::benchmark::add(
		"alloc/speed/holding", "действий/с", HOLDING_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::holding
	);
	// Регистрируем сценарий перевыдачи с ростом
	static const bool gGrowth = awh::benchmark::add(
		"alloc/speed/growth", "действий/с", GROWTH_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::growth
	);
	// Регистрируем сценарий выдачи несколькими потоками
	static const bool gThreaded = awh::benchmark::add(
		"alloc/speed/threaded", "действий/с", THREADED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::threaded
	);
	// Регистрируем сценарий освобождения блока чужим потоком
	static const bool gCrossing = awh::benchmark::add(
		"alloc/speed/crossing", "действий/с", CROSSING_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::crossing
	);
};
