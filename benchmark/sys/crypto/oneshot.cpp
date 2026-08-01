/**
 * @file: oneshot.cpp
 * @date: 2026-08-01
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения разовой работы модуля криптографии — шифрование и расшифровка
 *        одним вызовом в обоих режимах блочного шифрования и кодирование BASE64
 *
 * @copyright: Copyright © 2026
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
 * @brief Внутренние параметры и сценарии бенчмарков разовой работы
 *
 */
namespace {
	/**
	 * @brief Количество операций сценариев короткого сообщения
	 *
	 */
	static constexpr size_t SHORT_ROUNDS = 100000;
	/**
	 * @brief Количество операций сценариев порции потока
	 *
	 */
	static constexpr size_t CHUNK_ROUNDS = 50000;
	/**
	 * @brief Пороги пропускной способности в октетах в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы
	 *
	 */
	static constexpr double SHORT_GCM_THRESHOLD = 60000000.0;
	static constexpr double SHORT_CFB_THRESHOLD = 65000000.0;
	static constexpr double CHUNK_GCM_THRESHOLD = 800000000.0;
	static constexpr double DECODE_GCM_THRESHOLD = 1200000000.0;
	static constexpr double BASE64_THRESHOLD = 340000000.0;
	/**
	 * @brief Порог количества выделений памяти на одну операцию
	 *
	 * @details Ограничение сверху, заданное по выпускной сборке. У прочих подмодулей
	 *          набора показатель этот от сборки не зависит, а здесь зависит: работа
	 *          отводит память не только сама, но и внутри библиотеки криптографии - на
	 *          контекст шифра, - и учёт этих отведений между сборками расходится.
	 *          Измерено: разовое шифрование даёт три отведения на операцию в выпускной
	 *          сборке и одно в отладочной, кодирование BASE64 - шесть и одно. Порог
	 *          взят по большему из двух: он обязан держаться в обеих
	 *
	 */
	static constexpr double SHORT_ALLOCATIONS = 3.0;
	static constexpr double CHUNK_ALLOCATIONS = 3.0;
	static constexpr double BASE64_ALLOCATIONS = 6.0;

	/**
	 * @brief Функция прогона сценария разового шифрования короткого сообщения
	 *
	 * @details Сообщение размером с сетевой кадр: на нём измеряется не пропускная
	 *          способность шифра, а стоимость самого вызова - отведение буфера
	 *          результата, выработка вектора инициализации и снятие имитовставки.
	 *          Сличение с порцией большего размера показывает, сколько стоит подача
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t sealingShort() noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Накопитель размеров шифротекстов
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(SHORT_ROUNDS, SHORT_SIZE, [&]() noexcept {
			// Выполняем шифрование короткого сообщения с накоплением размера
			summary += crypto.encrypt <string> (data.data(), SHORT_SIZE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария разового шифрования гаммированием
	 *
	 * @details Тот же размер сообщения, что и у режима с проверкой подлинности: сличение
	 *          показывает цену самой проверки - выработку имитовставки и её дописывание
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t sealingGamma() noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Устанавливаем режим блочного шифрования гаммированием
		crypto.mode(awh::crypto_t::mode_t::CFB);
		// Накопитель размеров шифротекстов
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(SHORT_ROUNDS, SHORT_SIZE, [&]() noexcept {
			// Выполняем шифрование короткого сообщения с накоплением размера
			summary += crypto.encrypt <string> (data.data(), SHORT_SIZE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).size();
		});
		// Возвращаем режим блочного шифрования с проверкой подлинности
		crypto.mode(awh::crypto_t::mode_t::GCM);
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария разового шифрования порции потока
	 *
	 * @details На порции такого размера стоимость вызова уже не главенствует, и
	 *          показатель отражает работу шифра
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t sealingChunk() noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Накопитель размеров шифротекстов
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHUNK_ROUNDS, CHUNK_SIZE, [&]() noexcept {
			// Выполняем шифрование порции потока с накоплением размера
			summary += crypto.encrypt <string> (data.data(), CHUNK_SIZE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария разовой расшифровки порции потока
	 *
	 * @details Расшифровка в режиме с проверкой подлинности сверяет имитовставку, и
	 *          сличение с шифрованием показывает цену сверки
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t opening() noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Выполняем шифрование порции потока
		const string sealed = crypto.encrypt <string> (data.data(), CHUNK_SIZE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
		// Накопитель размеров открытых текстов
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHUNK_ROUNDS, CHUNK_SIZE, [&]() noexcept {
			// Выполняем расшифровку порции потока с накоплением размера
			summary += crypto.decrypt <string> (sealed.data(), sealed.size(), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария кодирования BASE64
	 *
	 * @details Кодирование выполняется цепочкой объектов библиотеки криптографии, а не
	 *          табличным кодировщиком. Замер даёт цену этого выбора: без него судить о
	 *          том, окупится ли свой кодировщик, нечем
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t encoding() noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Накопитель размеров записей BASE64
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CHUNK_ROUNDS, CHUNK_SIZE, [&]() noexcept {
			// Выполняем кодирование порции потока с накоплением размера
			summary += crypto.encrypt <string> (data.data(), CHUNK_SIZE, awh::crypto_t::hash_t::NONE, awh::crypto_t::cipher_t::BASE64).size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона сценария шифрования короткого сообщения
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & sealedShort() noexcept {
		// Итоги прогона сценария шифрования короткого сообщения
		static const outcome_t result = ::sealingShort();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария шифрования гаммированием
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & sealedGamma() noexcept {
		// Итоги прогона сценария шифрования гаммированием
		static const outcome_t result = ::sealingGamma();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария шифрования порции потока
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & sealedChunk() noexcept {
		// Итоги прогона сценария шифрования порции потока
		static const outcome_t result = ::sealingChunk();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария расшифровки порции потока
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & opened() noexcept {
		// Итоги прогона сценария расшифровки порции потока
		static const outcome_t result = ::opening();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона сценария кодирования BASE64
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & encoded() noexcept {
		// Итоги прогона сценария кодирования BASE64
		static const outcome_t result = ::encoding();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии шифрования короткого сообщения
	AWH_CRYPTO_SCENARIO(ShortGCM, ::sealedShort)
	// Объявляем сценарии шифрования гаммированием
	AWH_CRYPTO_SCENARIO(ShortCFB, ::sealedGamma)
	// Объявляем сценарии шифрования порции потока
	AWH_CRYPTO_SCENARIO(ChunkGCM, ::sealedChunk)
	// Объявляем сценарии расшифровки порции потока
	AWH_CRYPTO_SCENARIO(DecodeGCM, ::opened)
	// Объявляем сценарии кодирования BASE64
	AWH_CRYPTO_SCENARIO(Base64, ::encoded)

	// Регистрируем сценарий пропускной способности шифрования короткого сообщения
	static const bool gShortGCM = awh::benchmark::add(
		"crypto/oneshot/gcm-64", "октетов/с", SHORT_GCM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesShortGCM
	);
	// Регистрируем сценарий выделений памяти на шифрование короткого сообщения
	static const bool gMemoryShortGCM = awh::benchmark::add(
		"crypto/oneshot/gcm-64/allocations", "выделений", SHORT_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryShortGCM
	);
	// Регистрируем сценарий пропускной способности шифрования гаммированием
	static const bool gShortCFB = awh::benchmark::add(
		"crypto/oneshot/cfb-64", "октетов/с", SHORT_CFB_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesShortCFB
	);
	// Регистрируем сценарий выделений памяти на шифрование гаммированием
	static const bool gMemoryShortCFB = awh::benchmark::add(
		"crypto/oneshot/cfb-64/allocations", "выделений", SHORT_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryShortCFB
	);
	// Регистрируем сценарий пропускной способности шифрования порции потока
	static const bool gChunkGCM = awh::benchmark::add(
		"crypto/oneshot/gcm-1k", "октетов/с", CHUNK_GCM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesChunkGCM
	);
	// Регистрируем сценарий выделений памяти на шифрование порции потока
	static const bool gMemoryChunkGCM = awh::benchmark::add(
		"crypto/oneshot/gcm-1k/allocations", "выделений", CHUNK_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryChunkGCM
	);
	// Регистрируем сценарий пропускной способности расшифровки порции потока
	static const bool gDecodeGCM = awh::benchmark::add(
		"crypto/oneshot/gcm-1k-decode", "октетов/с", DECODE_GCM_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesDecodeGCM
	);
	// Регистрируем сценарий выделений памяти на расшифровку порции потока
	static const bool gMemoryDecodeGCM = awh::benchmark::add(
		"crypto/oneshot/gcm-1k-decode/allocations", "выделений", CHUNK_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryDecodeGCM
	);
	// Регистрируем сценарий пропускной способности кодирования BASE64
	static const bool gBase64 = awh::benchmark::add(
		"crypto/oneshot/base64-1k", "октетов/с", BASE64_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesBase64
	);
	// Регистрируем сценарий выделений памяти на кодирование BASE64
	static const bool gMemoryBase64 = awh::benchmark::add(
		"crypto/oneshot/base64-1k/allocations", "выделений", BASE64_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryBase64
	);
};
