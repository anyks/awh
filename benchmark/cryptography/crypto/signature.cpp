/**
 * @file: signature.cpp
 * @date: 2026-08-18
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения видов подписи — выработка и проверка подписи схемами
 *        Ed25519, ECDSA P-256 и RSA, а также выработка отпечатка открытого ключа
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
 * @brief Внутренние параметры и сценарии бенчмарков видов подписи
 *
 */
namespace {
	/**
	 * @brief Размер подписываемого сообщения в октетах
	 *
	 * @details Работы, ради которых виды подписи и заведены, подписывают корень дерева
	 *          свёрток - три десятка октетов, а не поток. Замер потому идёт на коротком
	 *          сообщении: он показывает цену самой схемы, а не пропускную способность
	 *          хэширования.
	 *
	 *          Замер этот доводом за вид подписи по умолчанию не служит - см. 5а.17 в
	 *          записях решений: у работы с деревом свёрток подпись одна на фиксацию, и
	 *          тонет она в стоимости самой фиксации при любой схеме
	 *
	 */
	static constexpr size_t DIGEST_SIZE = 32;
	/**
	 * @brief Количество операций сценариев быстрых схем
	 *
	 */
	static constexpr size_t FAST_ROUNDS = 2000;
	/**
	 * @brief Количество операций сценариев выработки подписи RSA
	 *
	 * @details Подпись RSA вырабатывается закрытым ключом и стоит на два порядка дороже
	 *          прочих схем: прогонов ей нужно меньше, иначе один этот сценарий занял бы
	 *          больше времени, чем весь остальной набор
	 *
	 */
	static constexpr size_t SLOW_ROUNDS = 200;
	/**
	 * @brief Пороги количества операций в секунду
	 *
	 * @details Пороги сняты НАСТОЯЩЕЙ выпускной сборкой проекта - со своим
	 *          BoringSSL, а не с системным OpenSSL, - по дну девяти систем
	 *          с двукратным запасом к нему. Запас берётся к стенду, а не к рабочей
	 *          машине: она быстрее самого медленного стенда впятеро и более, и порог,
	 *          посчитанный от неё, валил бы прогон на стенде при полностью исправном
	 *          модуле
	 *
	 */
	static constexpr double ED25519_SIGN_THRESHOLD = 14000.0;
	static constexpr double ED25519_VERIFY_THRESHOLD = 6400.0;
	static constexpr double ECDSA_SIGN_THRESHOLD = 15000.0;
	static constexpr double ECDSA_VERIFY_THRESHOLD = 5300.0;
	static constexpr double RSA_SIGN_THRESHOLD = 470.0;
	static constexpr double RSA_VERIFY_THRESHOLD = 15000.0;
	/**
	 * Пороги схемы ГОСТ Р 34.10-2012
	 *
	 * @details Схема считается своими силами на общей длинной арифметике, оттого
	 *          медленнее прочих на два порядка. Пороги взяты по самому медленному
	 *          стенду, как и у прочих сценариев набора
	 */
	/**
	 * Порог пропускной способности хэш-функции ГОСТ Р 34.11-2012
	 *
	 * @details Сценарии подписи её не меряют вовсе - они работают с 32 октетами, и
	 *          замедление хэша в них не видно. Свод преобразования дал 88 МБ/с против
	 *          1,9 МБ/с у поразрядного счёта (5б.12), а вложение подстановки Pi в свод
	 *          подняло его ещё вдвое (5б.18); порог стоит стражем обеих правок. Самый
	 *          медленный стенд - Solaris, 71,9 МБ/с против 209 у рабочей машины
	 */
	static constexpr double STREEBOG_THRESHOLD = 34.0;
	/**
	 * Пороги схемы на 256 разрядов взяты по самому медленному стенду - Solaris, где
	 * схема даёт 1900 подписей в секунду, - а не по рабочей машине с её 8500: порог,
	 * посчитанный от машины, валил бы замер на стенде при исправном модуле (5б.13)
	 */
	static constexpr double GOST_SIGN_THRESHOLD = 1100.0;
	static constexpr double GOST_VERIFY_THRESHOLD = 260.0;
	/**
	 * Пороги схемы на 512 разрядов
	 *
	 * @details Счёт поля идёт вдвое более широкими числами, а умножение растёт от
	 *          ширины квадратично, оттого схема эта дороже схемы на 256 разрядов
	 *          примерно вшестеро; пороги взяты по самому медленному стенду - Ubuntu,
	 *          где схема даёт 325 подписей и 77 проверок в секунду
	 */
	static constexpr double GOST512_SIGN_THRESHOLD = 160.0;
	static constexpr double GOST512_VERIFY_THRESHOLD = 39.0;
	/**
	 * Порог отпечатка держится не машиной, а разновидностью библиотеки криптографии:
	 * каноническая запись открытого ключа вырабатывается вызовом i2d_PUBKEY, и у
	 * OpenSSL 3.0 он всякий раз ищет кодировщик через поставщика. Рабочая машина
	 * (OpenSSL 3.6) даёт 5,9 миллиона отпечатков в секунду, стенды с OpenSSL 3.0 -
	 * от семи тысяч, то есть в восемьсот раз меньше при разнице машин в восемь раз
	 */
	static constexpr double FINGERPRINT_THRESHOLD = 720000.0;
	/**
	 * @brief Функция получения объекта криптографии со связкой ключей подписи
	 *
	 * @details Ключи всех видов вырабатываются единожды и служат всем сценариям: выработка
	 *          ключа к измеряемой работе отношения не имеет, а у RSA стоит дороже всего
	 *          прогона
	 *
	 * @return объект криптографии со связкой ключей подписи
	 *
	 */
	static awh::crypto_t & keyring() noexcept {
		// Объект криптографии со связкой ключей подписи
		static awh::crypto_t result(framework(), logger());
		// Признак выполненного заведения связки ключей
		static const bool ready = [&]() noexcept -> bool {
			// Выполняем выработку ключа Ed25519
			result.generateKey("ed25519", awh::crypto_t::signature_t::ED25519);
			// Выполняем выработку ключа ECDSA
			result.generateKey("ecdsa", awh::crypto_t::signature_t::ECDSA);
			// Выполняем выработку ключа RSA
			result.generateKey("rsa", awh::crypto_t::signature_t::RSA);
			// Выполняем выработку ключа ГОСТ Р 34.10-2012 на 256 разрядов
			result.generateKey("gost", awh::crypto_t::signature_t::GOST);
			// Выполняем выработку ключа ГОСТ Р 34.10-2012 на 512 разрядов
			result.generateKey("gost512", awh::crypto_t::signature_t::GOST512);
			// Выводим признак выполненного заведения
			return true;
		}();
		// Снимаем предупреждение о неиспользуемом признаке заведения
		(void) ready;
		// Выводим объект криптографии со связкой ключей подписи
		return result;
	}
	/**
	 * @brief Функция получения подписываемого сообщения
	 *
	 * @return подписываемое сообщение
	 *
	 */
	static const vector <uint8_t> & digest() noexcept {
		// Подписываемое сообщение
		static const vector <uint8_t> result(buffer().begin(), buffer().begin() + DIGEST_SIZE);
		// Выводим подписываемое сообщение
		return result;
	}
	/**
	 * @brief Функция прогона сценария выработки подписи
	 *
	 * @param name   имя ключа в связке
	 * @param hash   тип хэш-суммы, схеме подписи отвечающий
	 * @param rounds количество выполняемых операций
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t signing(const char * name, const awh::crypto_t::hash_t hash, const size_t rounds) noexcept {
		// Получаем объект криптографии со связкой ключей подписи
		awh::crypto_t & crypto = keyring();
		// Получаем подписываемое сообщение
		const vector <uint8_t> & data = digest();
		// Буфер подписи
		vector <uint8_t> signature;
		// Накопитель размеров подписей
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(rounds, DIGEST_SIZE, [&]() noexcept {
			// Выполняем выработку подписи сообщения
			crypto.sign(name, data.data(), data.size(), hash, signature);
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
	 * @param name   имя ключа в связке
	 * @param hash   тип хэш-суммы, схеме подписи отвечающий
	 * @param rounds количество выполняемых операций
	 * @return       итоги прогона сценария
	 *
	 */
	static outcome_t verifying(const char * name, const awh::crypto_t::hash_t hash, const size_t rounds) noexcept {
		// Получаем объект криптографии со связкой ключей подписи
		awh::crypto_t & crypto = keyring();
		// Получаем подписываемое сообщение
		const vector <uint8_t> & data = digest();
		// Буфер подписи
		vector <uint8_t> signature;
		// Выполняем выработку подписи сообщения
		crypto.sign(name, data.data(), data.size(), hash, signature);
		// Накопитель признаков проверки подписи
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(rounds, DIGEST_SIZE, [&]() noexcept {
			// Выполняем проверку подписи сообщения с накоплением признака
			summary += static_cast <uint64_t> (crypto.verify(name, data.data(), data.size(), signature, hash));
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция прогона сценария выработки отпечатка открытого ключа
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static outcome_t printing() noexcept {
		// Получаем объект криптографии со связкой ключей подписи
		awh::crypto_t & crypto = keyring();
		// Накопитель размеров отпечатков
		uint64_t summary = 0;
		// Выполняем прогон измеряемой операции
		const outcome_t result = measure(FAST_ROUNDS, 0, [&]() noexcept {
			// Выполняем выработку отпечатка открытого ключа
			summary += crypto.fingerprint <vector <uint8_t>> ("ed25519").size();
		});
		// Накапливаем контрольную сумму прогона
		checksum() += summary;
		// Выводим итоги прогона сценария
		return result;
	}

	/**
	 * @brief Функция получения итогов прогона выработки подписи Ed25519
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & signedPure() noexcept {
		// Итоги прогона выработки подписи Ed25519
		static const outcome_t result = ::signing("ed25519", awh::crypto_t::hash_t::NONE, FAST_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона проверки подписи Ed25519
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & verifiedPure() noexcept {
		// Итоги прогона проверки подписи Ed25519
		static const outcome_t result = ::verifying("ed25519", awh::crypto_t::hash_t::NONE, FAST_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона выработки подписи ECDSA
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & signedCurve() noexcept {
		// Итоги прогона выработки подписи ECDSA
		static const outcome_t result = ::signing("ecdsa", awh::crypto_t::hash_t::SHA256, FAST_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона проверки подписи ECDSA
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & verifiedCurve() noexcept {
		// Итоги прогона проверки подписи ECDSA
		static const outcome_t result = ::verifying("ecdsa", awh::crypto_t::hash_t::SHA256, FAST_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона хэш-функции ГОСТ Р 34.11-2012
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & hashedGOST() noexcept {
		// Итоги прогона хэш-функции ГОСТ
		static const outcome_t result = [&]() noexcept -> outcome_t {
			// Получаем объект криптографии со связкой ключей подписи
			awh::crypto_t & crypto = keyring();
			// Размер подаваемого содержимого
			static constexpr size_t VOLUME = (1024 * 1024);
			// Подаваемое содержимое
			const vector <uint8_t> content(VOLUME, 0x5A);
			// Буфер подписи
			vector <uint8_t> signature;
			// Выполняем прогон измеряемой операции
			return measure(20, VOLUME, [&]() noexcept {
				// Выполняем заведение потока выработки подписи
				crypto.signInitialize("gost", awh::crypto_t::hash_t::NONE);
				// Выполняем подачу содержимого в поток
				crypto.signUpdate(content.data(), content.size());
				// Выполняем завершение потока выработки подписи
				crypto.signFinalize(signature);
			});
		}();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона выработки подписи ГОСТ Р 34.10-2012
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & signedGOST() noexcept {
		// Итоги прогона выработки подписи ГОСТ
		static const outcome_t result = ::signing("gost", awh::crypto_t::hash_t::NONE, SLOW_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона проверки подписи ГОСТ Р 34.10-2012
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & verifiedGOST() noexcept {
		// Итоги прогона проверки подписи ГОСТ
		static const outcome_t result = ::verifying("gost", awh::crypto_t::hash_t::NONE, SLOW_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона выработки подписи ГОСТ на 512 разрядов
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & signedGOST512() noexcept {
		// Итоги прогона выработки подписи ГОСТ на 512 разрядов
		static const outcome_t result = ::signing("gost512", awh::crypto_t::hash_t::NONE, SLOW_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона проверки подписи ГОСТ на 512 разрядов
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & verifiedGOST512() noexcept {
		// Итоги прогона проверки подписи ГОСТ на 512 разрядов
		static const outcome_t result = ::verifying("gost512", awh::crypto_t::hash_t::NONE, SLOW_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона выработки подписи RSA
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & signedRSA() noexcept {
		// Итоги прогона выработки подписи RSA
		static const outcome_t result = ::signing("rsa", awh::crypto_t::hash_t::SHA256, SLOW_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона проверки подписи RSA
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & verifiedRSA() noexcept {
		// Итоги прогона проверки подписи RSA
		static const outcome_t result = ::verifying("rsa", awh::crypto_t::hash_t::SHA256, FAST_ROUNDS);
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция получения итогов прогона выработки отпечатка
	 *
	 * @return итоги прогона сценария
	 *
	 */
	static const outcome_t & printed() noexcept {
		// Итоги прогона выработки отпечатка
		static const outcome_t result = ::printing();
		// Выводим итоги прогона сценария
		return result;
	}

	// Объявляем сценарии выработки подписи Ed25519
	AWH_CRYPTO_SCENARIO(SignPure, ::signedPure)
	// Объявляем сценарии проверки подписи Ed25519
	AWH_CRYPTO_SCENARIO(VerifyPure, ::verifiedPure)
	// Объявляем сценарии выработки подписи ECDSA
	AWH_CRYPTO_SCENARIO(SignCurve, ::signedCurve)
	// Объявляем сценарии проверки подписи ECDSA
	AWH_CRYPTO_SCENARIO(VerifyCurve, ::verifiedCurve)
	// Объявляем сценарии выработки подписи RSA
	AWH_CRYPTO_SCENARIO(SignRSA, ::signedRSA)
	// Объявляем сценарии проверки подписи RSA
	AWH_CRYPTO_SCENARIO(VerifyRSA, ::verifiedRSA)
	// Объявляем сценарии хэш-функции ГОСТ Р 34.11-2012
	AWH_CRYPTO_SCENARIO(HashGOST, ::hashedGOST)
	// Объявляем сценарии выработки подписи ГОСТ Р 34.10-2012
	AWH_CRYPTO_SCENARIO(SignGOST, ::signedGOST)
	// Объявляем сценарии проверки подписи ГОСТ Р 34.10-2012
	AWH_CRYPTO_SCENARIO(VerifyGOST, ::verifiedGOST)
	// Сценарий выработки подписи ГОСТ на 512 разрядов
	AWH_CRYPTO_SCENARIO(SignGOST512, ::signedGOST512)
	// Сценарий проверки подписи ГОСТ на 512 разрядов
	AWH_CRYPTO_SCENARIO(VerifyGOST512, ::verifiedGOST512)
	// Объявляем сценарии выработки отпечатка открытого ключа
	AWH_CRYPTO_SCENARIO(Print, ::printed)

	// Регистрируем сценарий скорости выработки подписи Ed25519
	static const bool gSignPure = awh::benchmark::add(
		"crypto/signature/ed25519-sign", "подписей/с", ED25519_SIGN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedSignPure
	);
	// Регистрируем сценарий скорости проверки подписи Ed25519
	static const bool gVerifyPure = awh::benchmark::add(
		"crypto/signature/ed25519-verify", "проверок/с", ED25519_VERIFY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedVerifyPure
	);
	// Регистрируем сценарий скорости выработки подписи ECDSA
	static const bool gSignCurve = awh::benchmark::add(
		"crypto/signature/ecdsa-sign", "подписей/с", ECDSA_SIGN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedSignCurve
	);
	// Регистрируем сценарий скорости проверки подписи ECDSA
	static const bool gVerifyCurve = awh::benchmark::add(
		"crypto/signature/ecdsa-verify", "проверок/с", ECDSA_VERIFY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedVerifyCurve
	);
	// Регистрируем сценарий скорости выработки подписи RSA
	static const bool gSignRSA = awh::benchmark::add(
		"crypto/signature/rsa-sign", "подписей/с", RSA_SIGN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedSignRSA
	);
	// Регистрируем сценарий скорости проверки подписи RSA
	static const bool gVerifyRSA = awh::benchmark::add(
		"crypto/signature/rsa-verify", "проверок/с", RSA_VERIFY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedVerifyRSA
	);
	// Регистрируем сценарий пропускной способности хэш-функции ГОСТ Р 34.11-2012
	static const bool gHashGost = awh::benchmark::add(
		"crypto/signature/gost-hash", "МБ/с", STREEBOG_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedHashGOST
	);
	// Регистрируем сценарий скорости выработки подписи ГОСТ Р 34.10-2012
	static const bool gSignGost = awh::benchmark::add(
		"crypto/signature/gost-sign", "подписей/с", GOST_SIGN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedSignGOST
	);
	// Регистрируем сценарий скорости проверки подписи ГОСТ Р 34.10-2012
	static const bool gVerifyGost = awh::benchmark::add(
		"crypto/signature/gost-verify", "проверок/с", GOST_VERIFY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedVerifyGOST
	);
	// Регистрируем сценарий скорости выработки подписи ГОСТ на 512 разрядов
	static const bool gSignGost512 = awh::benchmark::add(
		"crypto/signature/gost512-sign", "подписей/с", GOST512_SIGN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedSignGOST512
	);
	// Регистрируем сценарий скорости проверки подписи ГОСТ на 512 разрядов
	static const bool gVerifyGost512 = awh::benchmark::add(
		"crypto/signature/gost512-verify", "проверок/с", GOST512_VERIFY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedVerifyGOST512
	);
	// Регистрируем сценарий скорости выработки отпечатка открытого ключа
	static const bool gPrint = awh::benchmark::add(
		"crypto/signature/fingerprint", "отпечатков/с", FINGERPRINT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedPrint
	);
};
