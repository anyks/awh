/**
 * @file stream.cpp
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
 * @brief Сценарии измерения потоковой работы модуля криптографии — шифрование и расшифровка
 *        порциями разного размера, полный оборот потока и цена вывода ключа
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
 * @brief Внутренние параметры и сценарии бенчмарков потоковой работы
 *
 */
namespace {
	/**
	 * @brief Размер потока сценариев подачи порциями в октетах
	 *
	 */
	static constexpr size_t STREAM_SIZE = (256 * 1024);
	/**
	 * @brief Количество операций сценариев подачи потока порциями
	 *
	 */
	static constexpr size_t STREAM_ROUNDS = 200;
	/**
	 * @brief Количество операций сценария полного оборота потока
	 *
	 */
	static constexpr size_t CYCLE_ROUNDS = 20000;
	/**
	 * @brief Количество операций сценария вывода ключа
	 *
	 * @details Вывод ключа стоит ста тысяч итераций и занимает единицы миллисекунд:
	 *          прогонов нужно немного, иначе один этот сценарий занял бы больше времени,
	 *          чем весь остальной набор
	 *
	 */
	static constexpr size_t DERIVE_ROUNDS = 20;
	/**
	 * @brief Пороги пропускной способности в октетах в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы
	 *
	 */
	static constexpr double ENCODE_CHUNK_THRESHOLD = 1600000000.0;
	static constexpr double ENCODE_BULK_THRESHOLD = 3500000000.0;
	static constexpr double DECODE_CHUNK_THRESHOLD = 90000000.0;
	static constexpr double DECODE_BULK_THRESHOLD = 98000000.0;
	/**
	 * @brief Пороги количества операций в секунду
	 *
	 */
	static constexpr double CYCLE_THRESHOLD = 700000.0;
	static constexpr double DERIVE_THRESHOLD = 80.0;

	/**
	 * @brief Функция прогона потока шифрования порциями указанного размера
	 *
	 * @param chunk размер порции подачи в октетах
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t sealing(const size_t chunk) noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Накопитель размеров шифротекстов
		uint64_t summary = 0;
		// Буфер накопления шифротекста потока
		string encoded;
		// Отводим память под шифротекст потока заранее
		encoded.reserve(STREAM_SIZE + 64);
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(STREAM_ROUNDS, STREAM_SIZE, [&]() noexcept {
			// Выполняем очистку буфера накопления шифротекста
			encoded.clear();
			// Выполняем заведение потока шифрования
			crypto.initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
			/**
			 * Выполняем подачу потока порциями
			 */
			for(size_t offset = 0; offset < STREAM_SIZE; offset += chunk)
				// Выполняем шифрование очередной порции потока
				encoded.append(crypto.encrypt <string> (data.data() + offset, chunk));
			// Выполняем завершение потока шифрования
			crypto.finalize(encoded);
			// Накапливаем размер шифротекста потока
			summary += encoded.size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона потока расшифровки порциями указанного размера
	 *
	 * @details Расшифровка в режиме с проверкой подлинности удерживает последние октеты
	 *          потока: имитовставка стоит в самом конце шифротекста, а какая порция
	 *          окажется последней, до завершения работы неизвестно. Удержание стоит
	 *          лишнего прохода копирования по всему шифротексту, и сличение расшифровки
	 *          с шифрованием - единственный способ узнать, во сколько этот проход обходится
	 *
	 * @param chunk размер порции подачи в октетах
	 * @return      итоги прогона сценария
	 *
	 */
	static outcome_t opening(const size_t chunk) noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Буфер шифротекста потока
		string sealed;
		// Выполняем заведение потока шифрования
		crypto.initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
		/**
		 * Выполняем подачу потока порциями
		 */
		for(size_t offset = 0; offset < STREAM_SIZE; offset += chunk)
			// Выполняем шифрование очередной порции потока
			sealed.append(crypto.encrypt <string> (data.data() + offset, chunk));
		// Выполняем завершение потока шифрования
		crypto.finalize(sealed);
		// Накопитель размеров открытых текстов
		uint64_t summary = 0;
		// Буфер накопления открытого текста потока
		string decoded;
		// Отводим память под открытый текст потока заранее
		decoded.reserve(STREAM_SIZE + 64);
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(STREAM_ROUNDS, STREAM_SIZE, [&]() noexcept {
			// Выполняем очистку буфера накопления открытого текста
			decoded.clear();
			// Выполняем заведение потока расшифровки
			crypto.initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
			/**
			 * Выполняем подачу шифротекста порциями
			 */
			for(size_t offset = 0; offset < sealed.size(); offset += chunk)
				// Выполняем расшифровку очередной порции потока
				decoded.append(crypto.decrypt <string> (sealed.data() + offset, ((sealed.size() - offset) < chunk ? (sealed.size() - offset) : chunk)));
			// Выполняем завершение потока расшифровки
			crypto.finalize(decoded);
			// Накапливаем размер открытого текста потока
			summary += decoded.size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария полного оборота потока
	 *
	 * @details Заведение потока, одна порция и завершение - самый короткий поток, какой
	 *          бывает. Показатель отражает стоимость самого оборота, а не шифрования:
	 *          на нём видно, выводится ли ключ заново при всяком заведении
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t cycling() noexcept {
		// Получаем эталонный объект криптографии
		awh::crypto_t & crypto = engine();
		// Получаем эталонный буфер данных
		const vector <uint8_t> & data = buffer();
		// Накопитель размеров шифротекстов
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(CYCLE_ROUNDS, SHORT_SIZE, [&]() noexcept {
			// Выполняем заведение потока шифрования
			crypto.initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
			// Выполняем шифрование единственной порции потока
			string encoded = crypto.encrypt <string> (data.data(), SHORT_SIZE);
			// Выполняем завершение потока шифрования
			crypto.finalize(encoded);
			// Накапливаем размер шифротекста потока
			summary += encoded.size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария вывода ключа шифрования
	 *
	 * @details Ключ выводится из пароля и соли за сто тысяч итераций, и цена эта взята
	 *          намеренно - она защищает от перебора пароля. Замер показывает её прямо,
	 *          чтобы вызывающая сторона знала, чего стоит смена пароля либо соли, и не
	 *          принимала удержание ключа за само собой разумеющееся
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t deriving() noexcept {
		// Объект криптографии сценария вывода ключа
		awh::crypto_t crypto(framework(), logger());
		// Устанавливаем пароль шифрования
		crypto.password("benchmark password");
		// Устанавливаем режим блочного шифрования с проверкой подлинности
		crypto.mode(awh::crypto_t::mode_t::GCM);
		// Порядковый номер соли вывода ключа
		size_t index = 0;
		// Накопитель признаков выполненного вывода ключа
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(DERIVE_ROUNDS, 0, [&]() noexcept {
			/**
			 * Соль меняется на всяком обороте: смена соли сбрасывает стейт и заставляет
			 * вывести ключ заново - иначе он удерживался бы и замерялось бы удержание
			 */
			// Устанавливаем новую соль вывода ключа
			crypto.salt(string("benchmark salt ") + to_string(index++));
			// Выполняем заведение потока шифрования с накоплением признака
			summary += static_cast <uint64_t> (crypto.initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона потока шифрования порциями
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & sealedChunk() noexcept {
		// Итоги прогона потока шифрования порциями
		static const outcome_t result = ::sealing(CHUNK_SIZE);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона потока шифрования крупными порциями
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & sealedBulk() noexcept {
		// Итоги прогона потока шифрования крупными порциями
		static const outcome_t result = ::sealing(BULK_SIZE);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона потока расшифровки порциями
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & openedChunk() noexcept {
		// Итоги прогона потока расшифровки порциями
		static const outcome_t result = ::opening(CHUNK_SIZE);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона потока расшифровки крупными порциями
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & openedBulk() noexcept {
		// Итоги прогона потока расшифровки крупными порциями
		static const outcome_t result = ::opening(BULK_SIZE);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона полного оборота потока
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & cycled() noexcept {
		// Итоги прогона полного оборота потока
		static const outcome_t result = ::cycling();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона вывода ключа шифрования
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & derived() noexcept {
		// Итоги прогона вывода ключа шифрования
		static const outcome_t result = ::deriving();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии потока шифрования порциями
	AWH_CRYPTO_SCENARIO(EncodeChunk, ::sealedChunk)
	// Объявляем сценарии потока шифрования крупными порциями
	AWH_CRYPTO_SCENARIO(EncodeBulk, ::sealedBulk)
	// Объявляем сценарии потока расшифровки порциями
	AWH_CRYPTO_SCENARIO(DecodeChunk, ::openedChunk)
	// Объявляем сценарии потока расшифровки крупными порциями
	AWH_CRYPTO_SCENARIO(DecodeBulk, ::openedBulk)
	// Объявляем сценарии полного оборота потока
	AWH_CRYPTO_SCENARIO(Cycle, ::cycled)
	// Объявляем сценарии вывода ключа шифрования
	AWH_CRYPTO_SCENARIO(Derive, ::derived)

	// Регистрируем сценарий пропускной способности потока шифрования порциями
	static const bool gEncodeChunk = awh::benchmark::add(
		"crypto/stream/encode-1k", "октетов/с", ENCODE_CHUNK_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesEncodeChunk
	);
	// Регистрируем сценарий пропускной способности потока шифрования крупными порциями
	static const bool gEncodeBulk = awh::benchmark::add(
		"crypto/stream/encode-64k", "октетов/с", ENCODE_BULK_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesEncodeBulk
	);
	// Регистрируем сценарий пропускной способности потока расшифровки порциями
	static const bool gDecodeChunk = awh::benchmark::add(
		"crypto/stream/decode-1k", "октетов/с", DECODE_CHUNK_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesDecodeChunk
	);
	// Регистрируем сценарий пропускной способности потока расшифровки крупными порциями
	static const bool gDecodeBulk = awh::benchmark::add(
		"crypto/stream/decode-64k", "октетов/с", DECODE_BULK_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bytesDecodeBulk
	);
	// Регистрируем сценарий скорости полного оборота потока
	static const bool gCycle = awh::benchmark::add(
		"crypto/stream/cycle", "оборотов/с", CYCLE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedCycle
	);
	// Регистрируем сценарий скорости вывода ключа шифрования
	static const bool gDerive = awh::benchmark::add(
		"crypto/stream/derive", "выводов/с", DERIVE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedDerive
	);
};
