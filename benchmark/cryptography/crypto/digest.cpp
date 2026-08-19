/**
 * @file digest.cpp
 * @date 2026-08-01
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
 * @brief Сценарии измерения хэширования и выработки имитовставки модуля криптографии —
 *        суммы разной разрядности и работы с ключом на данных разного размера
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл бенчмарков модуля криптографии
 */
#include "crypto.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён бенчмарков модуля криптографии
 */
using namespace awh::benchmark::crypto;

/**
 * @brief Внутренние параметры и сценарии бенчмарков хэширования
 *
 */
namespace {
	/**
	 * @brief Количество операций сценариев короткого сообщения
	 *
	 */
	static constexpr size_t SHORT_ROUNDS = 200000;
	/**
	 * @brief Количество операций сценариев порции потока
	 *
	 */
	static constexpr size_t CHUNK_ROUNDS = 100000;
	/**
	 * @brief Пороги пропускной способности в октетах в секунду
	 *
	 * @details Пороги сняты выпускной сборкой по самому медленному из восьми стендов
	 *          с двукратным запасом к нему: они ловят регрессию в разы, а не колебания
	 *          планировщика операционной системы и не разницу самих машин. Отладочная
	 *          сборка мерой не служит - на потоковой расшифровке она врёт на порядок
	 *
	 */
	static constexpr double SHORT_THRESHOLD = 16000000.0;
	static constexpr double CHUNK_THRESHOLD = 68000000.0;
	static constexpr double HMAC_THRESHOLD = 46000000.0;
	/**
	 * @brief Порог количества выделений памяти на одну операцию
	 *
	 * @details Ограничение сверху. Хэш-сумма выводится шестнадцатеричной записью, и
	 *          отведений у неё не менее двух: под промежуточные значения и под саму
	 *          запись, вдвое длиннее двоичного вида
	 *
	 */
	static constexpr double DIGEST_ALLOCATIONS = 2.0;

	/**
	 * @brief Функция прогона сценария хэширования короткого сообщения
	 *
	 * @details На коротком сообщении измеряется не пропускная способность суммы, а
	 *          стоимость самого вызова вместе с отведением памяти под запись
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t hashingShort() noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Текст короткого сообщения
		const string text(reinterpret_cast <const char *> (data.data()), SHORT_SIZE);
		// Накопитель размеров хэш-сумм
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(SHORT_ROUNDS, SHORT_SIZE, [&]() noexcept {
			// Выполняем хэширование короткого сообщения с накоплением размера
			summary += crypto.hash <string> (text, awh::crypto_t::hash_t::SHA256).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария хэширования порции потока
	 *
	 * @details На порции такого размера стоимость вызова уже не главенствует, и
	 *          показатель отражает работу самой суммы. Запись при этом выводится
	 *          шестнадцатеричной, то есть вдвое длиннее двоичного вида: сличение с
	 *          пропускной способностью шифрования показывает цену этого выбора
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t hashingChunk() noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Текст порции потока
		const string text(reinterpret_cast <const char *> (data.data()), CHUNK_SIZE);
		// Накопитель размеров хэш-сумм
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHUNK_ROUNDS, CHUNK_SIZE, [&]() noexcept {
			// Выполняем хэширование порции потока с накоплением размера
			summary += crypto.hash <string> (text, awh::crypto_t::hash_t::SHA256).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария выработки имитовставки
	 *
	 * @details Имитовставка вырабатывается сложением двух хэш-сумм с ключом, и сличение
	 *          с обычным хэшированием на тех же данных показывает цену ключа
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t signing() noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Текст порции потока
		const string text(reinterpret_cast <const char *> (data.data()), CHUNK_SIZE);
		// Ключ выработки имитовставки
		const string key = "benchmark hmac key";
		// Накопитель размеров имитовставок
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHUNK_ROUNDS, CHUNK_SIZE, [&]() noexcept {
			// Выполняем выработку имитовставки с накоплением размера
			summary += crypto.hmac <string> (key, text, awh::crypto_t::hash_t::SHA256).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона хэширования короткого сообщения
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & hashedShort() noexcept {
		// Итоги прогона хэширования короткого сообщения
		static const outcome_t result = ::hashingShort();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона хэширования порции потока
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & hashedChunk() noexcept {
		// Итоги прогона хэширования порции потока
		static const outcome_t result = ::hashingChunk();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона выработки имитовставки
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & signed_() noexcept {
		// Итоги прогона выработки имитовставки
		static const outcome_t result = ::signing();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии хэширования короткого сообщения
	AWH_CRYPTO_SCENARIO(HashShort, ::hashedShort)
	// Объявляем сценарии хэширования порции потока
	AWH_CRYPTO_SCENARIO(HashChunk, ::hashedChunk)
	// Объявляем сценарии выработки имитовставки
	AWH_CRYPTO_SCENARIO(Hmac, ::signed_)

	// Регистрируем сценарий пропускной способности хэширования короткого сообщения
	static const bool gHashShort = awh::benchmark::add(
		"crypto/digest/sha256-64", "октетов/с", SHORT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesHashShort
	);
	// Регистрируем сценарий выделений памяти на хэширование короткого сообщения
	static const bool gMemoryHashShort = awh::benchmark::add(
		"crypto/digest/sha256-64/allocations", "выделений", DIGEST_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryHashShort
	);
	// Регистрируем сценарий пропускной способности хэширования порции потока
	static const bool gHashChunk = awh::benchmark::add(
		"crypto/digest/sha256-1k", "октетов/с", CHUNK_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesHashChunk
	);
	// Регистрируем сценарий выделений памяти на хэширование порции потока
	static const bool gMemoryHashChunk = awh::benchmark::add(
		"crypto/digest/sha256-1k/allocations", "выделений", DIGEST_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryHashChunk
	);
	// Регистрируем сценарий пропускной способности выработки имитовставки
	static const bool gHmac = awh::benchmark::add(
		"crypto/digest/hmac-1k", "октетов/с", HMAC_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesHmac
	);
	// Регистрируем сценарий выделений памяти на выработку имитовставки
	static const bool gMemoryHmac = awh::benchmark::add(
		"crypto/digest/hmac-1k/allocations", "выделений", DIGEST_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryHmac
	);
};
