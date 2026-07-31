/**
 * @file: static.cpp
 * @date: 2026-01-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты модуля криптографии — проверка создания и сброса объекта модуля,
 *        а также корректности симметричного шифрования и расшифровки данных, вычисления хешей и кодирования в Base64
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "crypto.hpp"

/**
 * @brief Тест создания объекта шифрования
 *
 */
TEST_F(CryptoFixture, CreateCryptoTest){
	// Проверяем, что объект шифрования создан
	ASSERT_TRUE(this->_crypto != nullptr);
	// Сбрасываем объект шифрования
	this->_crypto.reset();
	// Проверяем, что объект шифрования сброшен
	ASSERT_TRUE(this->_crypto == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта шифрования
 *
 */
TEST_F(CryptoFixture, ResetAndCreateCryptoTest){
	// Проверяем, что объект шифрования создан
	ASSERT_TRUE(this->_crypto != nullptr);
	// Сбрасываем объект шифрования
	this->_crypto.reset();
	// Проверяем, что объект шифрования сброшен
	ASSERT_TRUE(this->_crypto == nullptr);
	// Создаём объект шифрования заново
	this->_crypto = std::make_unique <awh::crypto_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект шифрования создан
	ASSERT_TRUE(this->_crypto != nullptr);
}

/**
 * @brief Тест повторного создания объекта шифрования
 *
 */
TEST_F(CryptoFixture, ReCreateCryptoTest){
	// Проверяем, что объект шифрования создан
	ASSERT_TRUE(this->_crypto != nullptr);
	// Создаём объект шифрования заново
	this->_crypto = std::make_unique <awh::crypto_t> (this->_fmk.get(), this->_log.get());
	// Отключаем потокобезопасность
	this->_crypto->threadSafety(false);
	// Проверяем, что объект шифрования создан
	ASSERT_TRUE(this->_crypto != nullptr);
}

/**
 * @brief Тест отказа шифрования при незаданном типе шифрования
 *
 * @details Разрядность AES256 равна 256, и в младший октет она не умещается.
 *          Отбор типа шифрования шёл по младшему октету, отчего метка AES256
 *          совпадала с меткой незаданного шифрования, и работа без шифрования
 *          молча уходила в ветвь AES
 *
 */
TEST_F(CryptoFixture, CipherNotSetCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Выполняем шифрование при незаданном типе шифрования
	const std::string none = this->_crypto->encrypt <std::string> (text);
	// Проверяем отказ шифрования при незаданном типе шифрования
	EXPECT_TRUE(none.empty());
	// Выполняем расшифровку при незаданном типе шифрования
	EXPECT_TRUE(this->_crypto->decrypt <std::string> (text).empty());
	/**
	 * Выполняем перебор всех разрядностей шифрования
	 */
	for(const awh::crypto_t::cipher_t cipher : {awh::crypto_t::cipher_t::AES128, awh::crypto_t::cipher_t::AES192, awh::crypto_t::cipher_t::AES256}){
		// Выполняем шифрование текста
		const std::string result = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, cipher);
		// Проверяем выполнение шифрования текста
		ASSERT_FALSE(result.empty());
		// Проверяем отличие результата шифрования от отказа
		ASSERT_NE(result, none);
		// Проверяем обратимость шифрования текста
		ASSERT_EQ(this->_crypto->decrypt <std::string> (result, awh::crypto_t::hash_t::SHA256, cipher), text);
	}
}

/**
 * @brief Тест повторной инициализации контекста потокового шифрования
 *
 * @details Заведённый прежде контекст отменял всякую следующую инициализацию,
 *          и сменить направление либо разрядность шифрования после первого
 *          раза было нечем
 *
 */
TEST_F(CryptoFixture, ReInitializeCryptoTest){
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Проверяем первую инициализацию контекста шифрования
	EXPECT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем повторную инициализацию контекста шифрования
	EXPECT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем смену направления работы потокового шифрования
	EXPECT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем смену разрядности потокового шифрования
	EXPECT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES128));
}

/**
 * @brief Тест отказа инициализации при незаданном направлении работы
 *
 * @details Контекст оставался заведённым и неинициализированным, и всякая
 *          следующая попытка видела его заведённым и отвечала отказом
 *          безвозвратно
 *
 */
TEST_F(CryptoFixture, InitializeWithoutDirectionCryptoTest){
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Проверяем отказ инициализации при незаданном направлении работы
	EXPECT_FALSE(this->_crypto->initialize(awh::crypto_t::event_t::NONE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем, что отказ не отменил дальнейшую работу
	EXPECT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
}

/**
 * @brief Тест отказа инициализации при незаданном пароле шифрования
 *
 */
TEST_F(CryptoFixture, InitializeWithoutPasswordCryptoTest){
	// Проверяем отказ инициализации при незаданном пароле шифрования
	EXPECT_FALSE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
}

/**
 * @brief Тест сверки направления работы потокового шифрования
 *
 * @details Довод направления в потоковом режиме не читался вовсе, и расшифровка
 *          поверх контекста шифрования молча шифровала ещё раз
 *
 */
TEST_F(CryptoFixture, StreamDirectionCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Выполняем инициализацию контекста шифрования
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем отказ расшифровки поверх контекста шифрования
	EXPECT_TRUE(this->_crypto->decrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
	// Проверяем работу шифрования в заданном направлении
	EXPECT_FALSE(this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
}

/**
 * @brief Тест обратимости потокового шифрования
 *
 */
TEST_F(CryptoFixture, StreamRoundTripCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!! Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Выполняем инициализацию контекста шифрования
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Зашифрованный текст
	std::string encoded;
	/**
	 * Выполняем передачу текста в потоковое шифрование порциями
	 */
	for(size_t offset = 0; offset < text.size(); offset += 16)
		// Выполняем шифрование очередной порции текста
		encoded.append(this->_crypto->encrypt <std::string> (text.data() + offset, ((text.size() - offset) < 16 ? (text.size() - offset) : 16), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));

	// Выполняем завершение потокового шифрования
	this->_crypto->finalize(encoded);
	// Проверяем отличие зашифрованного текста от исходного
	ASSERT_NE(encoded, text);
	// Выполняем инициализацию контекста расшифровки
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Расшифрованный текст
	std::string decoded;
	/**
	 * Выполняем передачу зашифрованного текста в потоковую расшифровку порциями
	 */
	for(size_t offset = 0; offset < encoded.size(); offset += 16)
		// Выполняем расшифровку очередной порции текста
		decoded.append(this->_crypto->decrypt <std::string> (encoded.data() + offset, ((encoded.size() - offset) < 16 ? (encoded.size() - offset) : 16), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));

	// Выполняем завершение потоковой расшифровки
	this->_crypto->finalize(decoded);
	// Проверяем обратимость потокового шифрования
	EXPECT_EQ(decoded, text);
}

/**
 * @brief Тест смены пароля поверх заведённого контекста шифрования
 *
 * @details Стейт сбрасывался присвоением заново созданного объекта, а контекст
 *          шифрования присвоением не освобождается: смена пароля после начала
 *          потокового шифрования теряла контекст безвозвратно
 *
 */
TEST_F(CryptoFixture, ResetStateCryptoTest){
	/**
	 * Выполняем перебор смен параметров шифрования
	 */
	for(uint32_t i = 0; i < 64; i++){
		// Устанавливаем пароль шифрования
		this->_crypto->password("password" + std::to_string(i));
		// Устанавливаем соль шифрования
		this->_crypto->salt("salt" + std::to_string(i));
		// Выполняем инициализацию контекста шифрования
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	}
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Проверяем работу шифрования после смены параметров
	EXPECT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
}

/**
 * @brief Тест повторной генерации приватного ключа RSA
 *
 * @details Новый ключ записывался поверх прежнего без его освобождения
 *
 */
TEST_F(CryptoFixture, RegenerateKeyCryptoTest){
	/**
	 * Выполняем перебор генераций приватного ключа RSA
	 */
	for(uint32_t i = 0; i < 4; i++)
		// Проверяем генерацию приватного ключа RSA
		ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));

	// Проверяем получение приватного ключа RSA
	EXPECT_FALSE(this->_crypto->getPrivateKeyRSA().empty());
}

/**
 * @brief Тест отказа подписи при неподдерживаемом типе хэш-суммы
 *
 */
TEST_F(CryptoFixture, SignUnsupportedHashCryptoTest){
	// Буфер данных для подписи
	const std::vector <uint8_t> buffer = {0x01, 0x02, 0x03, 0x04};
	// Буфер результата подписи
	std::vector <uint8_t> result;
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Выполняем подпись данных с неподдерживаемым типом хэш-суммы
	this->_crypto->signWithPrivateKey(buffer, awh::crypto_t::hash_t::NONE, result);
	// Проверяем отказ подписи при неподдерживаемом типе хэш-суммы
	EXPECT_TRUE(result.empty());
	// Выполняем подпись данных с поддерживаемым типом хэш-суммы
	this->_crypto->signWithPrivateKey(buffer, awh::crypto_t::hash_t::SHA256, result);
	// Проверяем выполнение подписи данных
	EXPECT_FALSE(result.empty());
	// Проверяем отказ проверки подписи при неподдерживаемом типе хэш-суммы
	EXPECT_FALSE(this->_crypto->verifyWithPublicKey(buffer, result, awh::crypto_t::hash_t::NONE));
}
