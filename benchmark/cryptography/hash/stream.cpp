/**
 * @file: stream.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения потокового хэширования — пропускная способность при подаче данных
 *        порциями разного размера, стоимость передачи одной порции и формирования результата,
 *        а также учёт выделений памяти потокового режима
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков модуля хэширования
 */
#include "hash.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков модуля хэширования
 */
using namespace awh::benchmark::hash;

/**
 * @brief Внутренние параметры и сценарии бенчмарков потокового хэширования
 *
 */
namespace {
	/**
	 * @brief Размер порции данных сценария подачи мелкими порциями в октетах
	 *
	 * @details Порция меньше блока движка: каждая такая подача попадает в буфер
	 *          неполного блока и обрабатывается только по его заполнению
	 *
	 */
	static constexpr size_t SMALL_CHUNK = 17;
	/**
	 * @brief Размер порции данных сценария подачи порциями сетевого пакета в октетах
	 *
	 */
	static constexpr size_t PACKET_CHUNK = 1500;
	/**
	 * @brief Размер порции данных сценария подачи крупными порциями в октетах
	 *
	 */
	static constexpr size_t LARGE_CHUNK = 65536;
	/**
	 * @brief Количество операций сценария подачи мелкими порциями
	 *
	 */
	static constexpr size_t SMALL_ROUNDS = 20;
	/**
	 * @brief Количество операций сценария подачи порциями сетевого пакета
	 *
	 */
	static constexpr size_t PACKET_ROUNDS = 200;
	/**
	 * @brief Количество операций сценария подачи крупными порциями
	 *
	 */
	static constexpr size_t LARGE_ROUNDS = 2000;
	/**
	 * @brief Количество операций сценария стоимости передачи одной порции
	 *
	 */
	static constexpr size_t UPDATE_ROUNDS = 3000000;
	/**
	 * @brief Количество операций сценария формирования результата
	 *
	 */
	static constexpr size_t DIGEST_ROUNDS = 3000000;
	/**
	 * @brief Количество операций сценария учёта выделений памяти
	 *
	 */
	static constexpr size_t ALLOCATION_ROUNDS = 200;
	/**
	 * @brief Порог пропускной способности подачи мелкими порциями в октетах в секунду
	 *
	 * @details Показатель ловит появление лишнего копирования в буфер неполного
	 *          блока: порция в семнадцать октетов копируется в него целиком, и
	 *          копирование здесь сопоставимо по стоимости с самим хэшированием
	 *
	 */
	static constexpr double SMALL_THRESHOLD = 235000000.0;
	/**
	 * @brief Порог пропускной способности подачи порциями сетевого пакета в октетах в секунду
	 *
	 */
	static constexpr double PACKET_THRESHOLD = 440000000.0;
	/**
	 * @brief Порог пропускной способности подачи крупными порциями в октетах в секунду
	 *
	 * @details Крупная порция обрабатывается блоками прямо из буфера вызывающего,
	 *          минуя буфер неполного блока, поэтому показатель обязан быть близок
	 *          к пропускной способности одноразового хэширования
	 *
	 */
	static constexpr double LARGE_THRESHOLD = 450000000.0;
	/**
	 * @brief Порог скорости передачи одной порции в операциях в секунду
	 *
	 */
	static constexpr double UPDATE_THRESHOLD = 5800000.0;
	/**
	 * @brief Порог скорости формирования результата в операциях в секунду
	 *
	 * @details Формирование результата состояние потокового хэширования не
	 *          изменяет, поэтому выполняется по копии состояния: показатель
	 *          ловит появление в этом пути работы, пропорциональной объёму
	 *          уже обработанных данных
	 *
	 */
	static constexpr double DIGEST_THRESHOLD = 3400000.0;
	/**
	 * @brief Порог количества выделений памяти на один прогон потокового хэширования
	 *
	 * @details Ограничение сверху: буфер неполного блока размещается в самом
	 *          объекте хэширования, поэтому потоковая обработка буфера любого
	 *          размера не выделяет память ни разу
	 *
	 */
	static constexpr double ALLOCATIONS_THRESHOLD = 0.01;

	/**
	 * @brief Функция прогона сценария потокового хэширования
	 *
	 * @param rounds количество выполняемых операций
	 * @param chunk  размер порции подаваемых данных в октетах
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t streaming(const size_t rounds, const size_t chunk) noexcept {
		// Получаем буфер данных для хэширования
		const vector <uint8_t> & data = buffer();
		// Накопитель результата операций
		uint64_t accumulator = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(rounds, data.size(), [&]() noexcept {
			// Создаём объект потокового хэширования
			awh::hash_t hash;
			/**
			 * Выполняем передачу данных в потоковое хэширование порциями
			 */
			for(size_t offset = 0; offset < data.size(); offset += chunk)
				// Выполняем добавление очередной порции данных
				hash.update(data.data() + offset, ((data.size() - offset) < chunk ? (data.size() - offset) : chunk));

			// Накапливаем результат потокового хэширования
			accumulator += hash.digest <uint64_t> ();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария подачи мелкими порциями
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t small() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = streaming(SMALL_ROUNDS, SMALL_CHUNK);
		// Устанавливаем измеренное значение
		result.value = perBytes(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария подачи порциями сетевого пакета
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t packet() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = streaming(PACKET_ROUNDS, PACKET_CHUNK);
		// Устанавливаем измеренное значение
		result.value = perBytes(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария подачи крупными порциями
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t large() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = streaming(LARGE_ROUNDS, LARGE_CHUNK);
		// Устанавливаем измеренное значение
		result.value = perBytes(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария стоимости передачи одной порции
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t update() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Получаем буфер данных для хэширования
		const uint8_t * data = buffer().data();
		// Создаём объект потокового хэширования
		awh::hash_t hash;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(UPDATE_ROUNDS, BLOCK_SIZE, [&]() noexcept {
			// Выполняем добавление очередной порции данных
			hash.update(data, BLOCK_SIZE);
		});
		// Накапливаем контрольную сумму прогона
		checksum() += hash.digest <uint64_t> ();
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария формирования результата
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t digest() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Создаём объект потокового хэширования
		awh::hash_t hash;
		// Накопитель результата операций
		uint64_t accumulator = 0;
		// Выполняем добавление данных в потоковое хэширование
		hash.update(buffer().data(), MEMORY_PAGE_SIZE);
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = measure(DIGEST_ROUNDS, MEMORY_PAGE_SIZE, [&]() noexcept {
			// Выполняем формирование результата потокового хэширования
			accumulator += hash.digest <uint64_t> ();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += accumulator;
		// Устанавливаем измеренное значение
		result.value = perSecond(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция прогона сценария учёта выделений памяти
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t allocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Выполняем прогон измеряемой операции
		const outcome_t outcome = streaming(ALLOCATION_ROUNDS, PACKET_CHUNK);
		// Устанавливаем измеренное значение
		result.value = perOperation(outcome);
		// Устанавливаем сведения о прогоне
		result.details = details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий подачи мелкими порциями
	static const bool gSmall = awh::benchmark::add(
		"hash/stream/chunk-17", "октетов/с", SMALL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::small
	);
	// Регистрируем сценарий подачи порциями сетевого пакета
	static const bool gPacket = awh::benchmark::add(
		"hash/stream/chunk-1500", "октетов/с", PACKET_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::packet
	);
	// Регистрируем сценарий подачи крупными порциями
	static const bool gLarge = awh::benchmark::add(
		"hash/stream/chunk-64k", "октетов/с", LARGE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::large
	);
	// Регистрируем сценарий стоимости передачи одной порции
	static const bool gUpdate = awh::benchmark::add(
		"hash/stream/update-64", "операций/с", UPDATE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::update
	);
	// Регистрируем сценарий формирования результата
	static const bool gDigest = awh::benchmark::add(
		"hash/stream/digest", "операций/с", DIGEST_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::digest
	);
	// Регистрируем сценарий учёта выделений памяти
	static const bool gAllocations = awh::benchmark::add(
		"hash/stream/allocations-per-run", "выделений", ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::allocations
	);
};
