/**
 * @file keys.cpp
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
 * @brief Сценарии измерения работ с ключами RSA модуля криптографии — шифрование
 *        и расшифровка ключом, выработка и проверка подписи
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
 * @brief Внутренние параметры и сценарии бенчмарков работ с ключами RSA
 *
 */
namespace {
	/**
	 * @brief Размер сообщения сценариев работ с ключом RSA в октетах
	 *
	 * @details Ключом RSA шифруются лишь короткие сообщения: у ключа в две тысячи
	 *          разрядов предел равен ста девяноста октетам. Размер взят с запасом
	 *          под этим пределом
	 *
	 */
	static constexpr size_t MESSAGE_SIZE = 128;
	/**
	 * @brief Количество операций сценариев работ с ключом RSA
	 *
	 * @details Работы с ключом на порядки дороже симметричного шифрования: прогонов
	 *          нужно немного, иначе эти сценарии заняли бы больше времени, чем весь
	 *          остальной набор
	 *
	 */
	static constexpr size_t KEY_ROUNDS = 2000;
	/**
	 * @brief Пороги количества операций в секунду
	 *
	 * @details Пороги сняты НАСТОЯЩЕЙ выпускной сборкой проекта - со своим
	 *          BoringSSL, а не с системным OpenSSL, - по дну девяти систем
	 *          с двукратным запасом к нему: они ловят регрессию в разы, а не колебания
	 *          планировщика операционной системы и не разницу самих машин. Отладочная
	 *          сборка мерой не служит - на потоковой расшифровке она врёт на порядок
	 *
	 */
	static constexpr double SEAL_THRESHOLD = 14000.0;
	static constexpr double OPEN_THRESHOLD = 430.0;
	static constexpr double SIGN_THRESHOLD = 480.0;
	static constexpr double VERIFY_THRESHOLD = 14000.0;

	/**
	 * @brief Функция получения объекта криптографии с заведённым ключом RSA
	 *
	 * @details Ключ вырабатывается единожды: выработка его стоит дороже всех замеров
	 *          вместе взятых и к измеряемым работам отношения не имеет
	 *
	 * @return объект криптографии с заведённым ключом RSA
	 *
	 */
	static awh::crypto_t & keyed() noexcept {
		// Объект криптографии сценариев работ с ключом RSA
		static awh::crypto_t result(framework(), logger());
		// Признак выполненной выработки ключа RSA
		static const bool ready = result.generatePrivateKeyRSA(2048);
		// Снимаем предупреждение о неиспользуемом признаке выработки
		(void) ready;
		// Выводим объект криптографии с заведённым ключом RSA
		return result;
	}
	/**
	 * @brief Функция получения эталонного сообщения сценариев работ с ключом RSA
	 *
	 * @return эталонное сообщение сценариев работ с ключом RSA
	 *
	 */
	static const vector <uint8_t> & message() noexcept {
		// Эталонное сообщение сценариев работ с ключом RSA
		static const vector <uint8_t> result(buffer().begin(), buffer().begin() + MESSAGE_SIZE);
		// Выводим эталонное сообщение
		return result;
	}

	/**
	 * @brief Функция прогона сценария шифрования ключом RSA
	 *
	 * @details Шифрование ведётся открытым ключом, у которого показатель степени мал,
	 *          и потому оно на порядок дешевле расшифровки закрытым
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t sealing() noexcept {
		// Получаем объект криптографии с заведённым ключом RSA
		awh::crypto_t & crypto = keyed();
		// Получаем эталонное сообщение
		const vector <uint8_t> & data = message();
		// Буфер шифротекста
		vector <uint8_t> sealed;
		// Накопитель размеров шифротекстов
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(KEY_ROUNDS, MESSAGE_SIZE, [&]() noexcept {
			// Выполняем шифрование сообщения ключом RSA
			crypto.encryptWithPublicKey(data, sealed);
			// Накапливаем размер шифротекста
			summary += sealed.size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария расшифровки ключом RSA
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t opening() noexcept {
		// Получаем объект криптографии с заведённым ключом RSA
		awh::crypto_t & crypto = keyed();
		// Получаем эталонное сообщение
		const vector <uint8_t> & data = message();
		// Буфер шифротекста
		vector <uint8_t> sealed;
		// Выполняем шифрование сообщения ключом RSA
		crypto.encryptWithPublicKey(data, sealed);
		// Буфер открытого текста
		vector <uint8_t> opened;
		// Накопитель размеров открытых текстов
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(KEY_ROUNDS, MESSAGE_SIZE, [&]() noexcept {
			// Выполняем расшифровку сообщения ключом RSA
			crypto.decryptWithPrivateKey(sealed, opened);
			// Накапливаем размер открытого текста
			summary += opened.size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария выработки подписи
	 *
	 * @details Подпись вырабатывается закрытым ключом и потому стоит наравне с
	 *          расшифровкой, а проверяется открытым - наравне с шифрованием
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t signing() noexcept {
		// Получаем объект криптографии с заведённым ключом RSA
		awh::crypto_t & crypto = keyed();
		// Получаем эталонное сообщение
		const vector <uint8_t> & data = message();
		// Буфер подписи
		vector <uint8_t> signature;
		// Накопитель размеров подписей
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(KEY_ROUNDS, MESSAGE_SIZE, [&]() noexcept {
			// Выполняем выработку подписи сообщения
			crypto.signWithPrivateKey(data, awh::crypto_t::hash_t::SHA256, signature);
			// Накапливаем размер подписи
			summary += signature.size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария проверки подписи
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t verifying() noexcept {
		// Получаем объект криптографии с заведённым ключом RSA
		awh::crypto_t & crypto = keyed();
		// Получаем эталонное сообщение
		const vector <uint8_t> & data = message();
		// Буфер подписи
		vector <uint8_t> signature;
		// Выполняем выработку подписи сообщения
		crypto.signWithPrivateKey(data, awh::crypto_t::hash_t::SHA256, signature);
		// Накопитель признаков проверки подписи
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(KEY_ROUNDS, MESSAGE_SIZE, [&]() noexcept {
			// Выполняем проверку подписи сообщения с накоплением признака
			summary += static_cast <uint64_t> (crypto.verifyWithPublicKey(data, signature, awh::crypto_t::hash_t::SHA256));
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона шифрования ключом RSA
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & sealed() noexcept {
		// Итоги прогона шифрования ключом RSA
		static const outcome_t result = ::sealing();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона расшифровки ключом RSA
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & opened() noexcept {
		// Итоги прогона расшифровки ключом RSA
		static const outcome_t result = ::opening();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона выработки подписи
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & signed_() noexcept {
		// Итоги прогона выработки подписи
		static const outcome_t result = ::signing();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона проверки подписи
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & verified() noexcept {
		// Итоги прогона проверки подписи
		static const outcome_t result = ::verifying();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии шифрования ключом RSA
	AWH_CRYPTO_SCENARIO(Seal, ::sealed)
	// Объявляем сценарии расшифровки ключом RSA
	AWH_CRYPTO_SCENARIO(Open, ::opened)
	// Объявляем сценарии выработки подписи
	AWH_CRYPTO_SCENARIO(Sign, ::signed_)
	// Объявляем сценарии проверки подписи
	AWH_CRYPTO_SCENARIO(Verify, ::verified)

	// Регистрируем сценарий скорости шифрования ключом RSA
	static const bool gSeal = awh::benchmark::add(
		"crypto/keys/encrypt", "шифрований/с", SEAL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedSeal
	);
	// Регистрируем сценарий скорости расшифровки ключом RSA
	static const bool gOpen = awh::benchmark::add(
		"crypto/keys/decrypt", "расшифровок/с", OPEN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedOpen
	);
	// Регистрируем сценарий скорости выработки подписи
	static const bool gSign = awh::benchmark::add(
		"crypto/keys/sign", "подписей/с", SIGN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedSign
	);
	// Регистрируем сценарий скорости проверки подписи
	static const bool gVerify = awh::benchmark::add(
		"crypto/keys/verify", "проверок/с", VERIFY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedVerify
	);
};
