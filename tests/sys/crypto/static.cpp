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
	ASSERT_TRUE(this->_crypto->finalize(encoded));
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
	ASSERT_TRUE(this->_crypto->finalize(decoded));
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

/**
 * @brief Тест неповторяемости шифротекста на одних и тех же данных
 *
 * @details Вектор инициализации выводился из пароля вместе с ключом, отчего
 *          повторное шифрование теми же паролем и солью давало ту же гамму:
 *          два сообщения, сложенные по модулю два, выдавали друг друга без
 *          всякого ключа. Вектор берётся случайным на каждое сообщение
 *
 */
TEST_F(CryptoFixture, RandomVectorCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	/**
	 * Выполняем перебор режимов блочного шифрования
	 */
	for(const awh::crypto_t::mode_t mode : {awh::crypto_t::mode_t::GCM, awh::crypto_t::mode_t::CFB}){
		// Устанавливаем режим блочного шифрования
		this->_crypto->mode(mode);
		// Набор сформированных шифротекстов
		std::unordered_set <std::string> results;
		/**
		 * Выполняем перебор шифрований одного и того же текста
		 */
		for(uint32_t i = 0; i < 64; i++){
			// Выполняем шифрование текста
			const std::string result = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
			// Проверяем выполнение шифрования текста
			ASSERT_FALSE(result.empty());
			// Проверяем обратимость шифрования текста
			ASSERT_EQ(this->_crypto->decrypt <std::string> (result, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), text);
			// Добавляем сформированный шифротекст
			results.emplace(result);
		}
		// Проверяем неповторяемость шифротекста на одних и тех же данных
		EXPECT_EQ(results.size(), static_cast <size_t> (64));
	}
}

/**
 * @brief Тест обнаружения подделки шифротекста
 *
 * @details Режим с проверкой подлинности подделку обнаруживает, режим
 *          гаммирования — нет, и это его объявленное свойство, а не дефект
 *
 */
TEST_F(CryptoFixture, TamperCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!! Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Выполняем шифрование текста
	const std::string result = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем выполнение шифрования текста
	ASSERT_FALSE(result.empty());
	/**
	 * Выполняем перебор всех октетов шифротекста
	 */
	for(size_t i = 0; i < result.size(); i++){
		// Формируем поддельный шифротекст
		std::string tampered = result;
		// Выполняем изменение очередного октета шифротекста
		tampered[i] = static_cast <char> (tampered[i] ^ 0x01);
		// Проверяем обнаружение подделки шифротекста
		ASSERT_TRUE(this->_crypto->decrypt <std::string> (tampered, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
	}
	// Проверяем обнаружение усечения шифротекста
	EXPECT_TRUE(this->_crypto->decrypt <std::string> (result.substr(0, result.size() - 1), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
	// Проверяем отказ расшифровки шифротекста, вектора инициализации не несущего
	EXPECT_TRUE(this->_crypto->decrypt <std::string> (result.substr(0, 4), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
}

/**
 * @brief Тест длины шифротекста в разных режимах блочного шифрования
 *
 */
TEST_F(CryptoFixture, CiphertextLengthCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Проверяем установленный режим блочного шифрования
	EXPECT_EQ(static_cast <uint8_t> (this->_crypto->mode()), static_cast <uint8_t> (awh::crypto_t::mode_t::GCM));
	// Проверяем длину шифротекста режима с проверкой подлинности
	EXPECT_EQ(this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).size(), (text.size() + 12 + 16));
	// Устанавливаем режим блочного шифрования гаммированием
	this->_crypto->mode(awh::crypto_t::mode_t::CFB);
	// Проверяем длину шифротекста режима гаммирования
	EXPECT_EQ(this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).size(), (text.size() + 16));
	// Устанавливаем режим блочного шифрования незаданным
	this->_crypto->mode(awh::crypto_t::mode_t::NONE);
	// Проверяем отказ шифрования при незаданном режиме блочного шифрования
	EXPECT_TRUE(this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
}

/**
 * @brief Тест потокового шифрования при разных размерах порции
 *
 * @details Вектор инициализации выписывается в начало потока и вычитывается
 *          из его начала, а имитовставка стоит в самом конце шифротекста,
 *          поэтому последние октеты потока удерживаются до его завершения.
 *          Ни то, ни другое от размера порции зависеть не должно
 *
 */
TEST_F(CryptoFixture, StreamChunkCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!! Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	/**
	 * Выполняем перебор режимов блочного шифрования
	 */
	for(const awh::crypto_t::mode_t mode : {awh::crypto_t::mode_t::GCM, awh::crypto_t::mode_t::CFB}){
		// Устанавливаем режим блочного шифрования
		this->_crypto->mode(mode);
		/**
		 * Выполняем перебор размеров порции потокового шифрования
		 */
		for(size_t chunk = 1; chunk <= 40; chunk++){
			// Зашифрованный текст
			std::string encoded;
			// Выполняем инициализацию контекста шифрования
			ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
			/**
			 * Выполняем передачу текста в потоковое шифрование порциями
			 */
			for(size_t offset = 0; offset < text.size(); offset += chunk)
				// Выполняем шифрование очередной порции текста
				encoded.append(this->_crypto->encrypt <std::string> (text.data() + offset, ((text.size() - offset) < chunk ? (text.size() - offset) : chunk)));

			// Выполняем завершение потокового шифрования
			ASSERT_TRUE(this->_crypto->finalize(encoded));
			// Проверяем отличие зашифрованного текста от исходного
			ASSERT_NE(encoded, text);
			// Расшифрованный текст
			std::string decoded;
			// Выполняем инициализацию контекста расшифровки
			ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
			/**
			 * Выполняем передачу зашифрованного текста в потоковую расшифровку порциями
			 */
			for(size_t offset = 0; offset < encoded.size(); offset += chunk)
				// Выполняем расшифровку очередной порции текста
				decoded.append(this->_crypto->decrypt <std::string> (encoded.data() + offset, ((encoded.size() - offset) < chunk ? (encoded.size() - offset) : chunk)));

			// Выполняем завершение потоковой расшифровки
			ASSERT_TRUE(this->_crypto->finalize(decoded));
			// Проверяем обратимость потокового шифрования
			ASSERT_EQ(decoded, text);
		}
	}
}

/**
 * @brief Тест обнаружения подделки в потоковом режиме
 *
 */
TEST_F(CryptoFixture, StreamTamperCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!! Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Зашифрованный текст
	std::string encoded;
	// Выполняем инициализацию контекста шифрования
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем шифрование текста
	encoded.append(this->_crypto->encrypt <std::string> (text));
	// Выполняем завершение потокового шифрования
	ASSERT_TRUE(this->_crypto->finalize(encoded));
	// Выполняем изменение октета шифротекста
	encoded[encoded.size() / 2] = static_cast <char> (encoded[encoded.size() / 2] ^ 0x01);
	// Расшифрованный текст
	std::string decoded;
	// Выполняем инициализацию контекста расшифровки
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем расшифровку поддельного шифротекста
	decoded.append(this->_crypto->decrypt <std::string> (encoded));
	// Проверяем обнаружение подделки при завершении потоковой расшифровки
	EXPECT_FALSE(this->_crypto->finalize(decoded));
	// Проверяем очистку результата расшифровки поддельного шифротекста
	EXPECT_TRUE(decoded.empty());
}

/**
 * @brief Тест потокового шифрования сообщения, порций не имеющего
 *
 * @details Вектор инициализации выписывается первой же порцией выхода, и поток
 *          без единой порции оставался без вектора: расшифровать его было
 *          нечем, хотя имитовставка в него попадала
 *
 */
TEST_F(CryptoFixture, StreamEmptyCryptoTest){
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	/**
	 * Выполняем перебор режимов блочного шифрования
	 */
	for(const awh::crypto_t::mode_t mode : {awh::crypto_t::mode_t::GCM, awh::crypto_t::mode_t::CFB}){
		// Устанавливаем режим блочного шифрования
		this->_crypto->mode(mode);
		// Зашифрованный текст
		std::string encoded;
		// Выполняем инициализацию контекста шифрования
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
		// Выполняем завершение потокового шифрования, порций не подавая
		ASSERT_TRUE(this->_crypto->finalize(encoded));
		// Проверяем наличие вектора инициализации в шифротексте
		ASSERT_FALSE(encoded.empty());
		// Расшифрованный текст
		std::string decoded = encoded;
		// Выполняем инициализацию контекста расшифровки
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
		// Выполняем расшифровку шифротекста
		decoded = this->_crypto->decrypt <std::string> (encoded);
		// Проверяем обратимость потокового шифрования сообщения, порций не имеющего
		ASSERT_TRUE(this->_crypto->finalize(decoded));
		// Проверяем пустоту расшифрованного сообщения
		EXPECT_TRUE(decoded.empty());
	}
}

/**
 * @brief Тест отказа доводов вызова, расходящихся с заведённым потоком
 *
 * @details Разрядность и тип хэш-суммы задаются при инициализации потока и
 *          живут в самом контексте. Прежде доводы вызова здесь молча
 *          отбрасывались — работа думала, что шифрует одной разрядностью,
 *          а шифровала другой
 *
 */
TEST_F(CryptoFixture, StreamCipherMismatchCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Выполняем инициализацию контекста шифрования
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем отказ шифрования при расхождении разрядности с заведённым потоком
	EXPECT_TRUE(this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES128).empty());
	// Проверяем отказ шифрования при расхождении типа хэш-суммы с заведённым потоком
	EXPECT_TRUE(this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA512, awh::crypto_t::cipher_t::AES256).empty());
	// Проверяем работу шифрования при совпадении доводов вызова с заведённым потоком
	EXPECT_FALSE(this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
	// Проверяем работу шифрования при доводах вызова, заданных потоком
	EXPECT_FALSE(this->_crypto->encrypt <std::string> (text).empty());
}

/**
 * @brief Тест отказа подписи и шифрования RSA без ключа
 *
 * @details Буфер результата отводился и заполнялся нулями до самой работы, и
 *          при отказе в нём оставались нули: работа, судящая об удаче по его
 *          непустоте, приняла бы их за готовый результат
 *
 */
TEST_F(CryptoFixture, EmptyResultOnFailureCryptoTest){
	// Буфер данных для работы
	const std::vector <uint8_t> buffer = {0x01, 0x02, 0x03, 0x04};
	// Буфер результата работы
	std::vector <uint8_t> result;
	// Выполняем шифрование данных без заведённого ключа
	this->_crypto->encryptWithPublicKey(buffer, result);
	// Проверяем пустоту результата шифрования без заведённого ключа
	EXPECT_TRUE(result.empty());
	// Выполняем расшифровку данных без заведённого ключа
	this->_crypto->decryptWithPrivateKey(buffer, result);
	// Проверяем пустоту результата расшифровки без заведённого ключа
	EXPECT_TRUE(result.empty());
	// Выполняем подпись данных без заведённого ключа
	this->_crypto->signWithPrivateKey(buffer, awh::crypto_t::hash_t::SHA256, result);
	// Проверяем пустоту результата подписи без заведённого ключа
	EXPECT_TRUE(result.empty());
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Выполняем расшифровку данных, шифротекстом не являющихся
	this->_crypto->decryptWithPrivateKey(buffer, result);
	// Проверяем пустоту результата расшифровки данных, шифротекстом не являющихся
	EXPECT_TRUE(result.empty());
}

/**
 * @brief Тест схемы дополнения подписи RSA
 *
 * @details Вероятностная схема при каждой подписи берёт новую соль, поэтому
 *          подписи одних и тех же данных различны, тогда как схема PKCS#1 v1.5
 *          детерминирована. Подпись, сделанная одной схемой, другой схемой
 *          проверку не проходит
 *
 */
TEST_F(CryptoFixture, PaddingCryptoTest){
	// Буфер данных для подписи
	const std::vector <uint8_t> buffer = {0x01, 0x02, 0x03, 0x04, 0x05};
	// Буфер результата подписи
	std::vector <uint8_t> result;
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Проверяем схему дополнения подписи по умолчанию
	EXPECT_EQ(static_cast <uint8_t> (this->_crypto->padding()), static_cast <uint8_t> (awh::crypto_t::padding_t::PSS));
	// Набор сформированных подписей
	std::unordered_set <std::string> results;
	/**
	 * Выполняем перебор подписей одних и тех же данных вероятностной схемой
	 */
	for(uint32_t i = 0; i < 8; i++){
		// Выполняем подпись данных
		this->_crypto->signWithPrivateKey(buffer, awh::crypto_t::hash_t::SHA256, result);
		// Проверяем выполнение подписи данных
		ASSERT_FALSE(result.empty());
		// Добавляем сформированную подпись
		results.emplace(result.begin(), result.end());
	}
	// Проверяем вероятностность схемы дополнения подписи
	EXPECT_EQ(results.size(), static_cast <size_t> (8));
	// Проверяем проверку подписи вероятностной схемой
	EXPECT_TRUE(this->_crypto->verifyWithPublicKey(buffer, result, awh::crypto_t::hash_t::SHA256));
	// Устанавливаем схему дополнения подписи PKCS#1 v1.5
	this->_crypto->padding(awh::crypto_t::padding_t::PKCS1);
	// Очищаем набор сформированных подписей
	results.clear();
	/**
	 * Выполняем перебор подписей одних и тех же данных схемой PKCS#1 v1.5
	 */
	for(uint32_t i = 0; i < 8; i++){
		// Выполняем подпись данных
		this->_crypto->signWithPrivateKey(buffer, awh::crypto_t::hash_t::SHA256, result);
		// Проверяем выполнение подписи данных
		ASSERT_FALSE(result.empty());
		// Добавляем сформированную подпись
		results.emplace(result.begin(), result.end());
	}
	// Проверяем детерминированность схемы дополнения подписи PKCS#1 v1.5
	EXPECT_EQ(results.size(), static_cast <size_t> (1));
	// Проверяем проверку подписи схемой PKCS#1 v1.5
	EXPECT_TRUE(this->_crypto->verifyWithPublicKey(buffer, result, awh::crypto_t::hash_t::SHA256));
	// Устанавливаем схему дополнения подписи вероятностную
	this->_crypto->padding(awh::crypto_t::padding_t::PSS);
	// Проверяем отказ проверки подписи, сделанной иной схемой дополнения
	EXPECT_FALSE(this->_crypto->verifyWithPublicKey(buffer, result, awh::crypto_t::hash_t::SHA256));
	// Устанавливаем схему дополнения подписи незаданной
	this->_crypto->padding(awh::crypto_t::padding_t::NONE);
	// Выполняем подпись данных при незаданной схеме дополнения
	this->_crypto->signWithPrivateKey(buffer, awh::crypto_t::hash_t::SHA256, result);
	// Проверяем отказ подписи при незаданной схеме дополнения
	EXPECT_TRUE(result.empty());
}

/**
 * @brief Тест разделения пароля защиты ключа и пароля шифрования данных
 *
 * @details Поле было одно на оба назначения, и приватный ключ выписывался под
 *          тем же паролем, которым шифруются данные — утрата одного означала
 *          утрату и другого
 *
 */
TEST_F(CryptoFixture, KeyPasswordCryptoTest){
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Устанавливаем пароль шифрования данных
	this->_crypto->password("data-password");
	// Получаем приватный ключ RSA при незаданном пароле его защиты
	const std::string opened = this->_crypto->getPrivateKeyRSA();
	// Проверяем получение приватного ключа RSA
	ASSERT_FALSE(opened.empty());
	// Проверяем, что пароль шифрования данных ключ не защищает
	EXPECT_EQ(opened.find("ENCRYPTED"), std::string::npos);
	// Устанавливаем пароль защиты приватного ключа RSA
	this->_crypto->passwordRSA("key-password");
	// Получаем приватный ключ RSA при заданном пароле его защиты
	const std::string sealed = this->_crypto->getPrivateKeyRSA();
	// Проверяем получение приватного ключа RSA
	ASSERT_FALSE(sealed.empty());
	// Проверяем защиту приватного ключа паролем его защиты
	EXPECT_NE(sealed.find("ENCRYPTED"), std::string::npos);
	// Проверяем вычитывание защищённого приватного ключа
	EXPECT_TRUE(this->_crypto->setPrivateKeyRSA(sealed));
}

/**
 * @brief Тест разового шифрования сообщения, октетов не имеющего
 *
 * @details Отказ по одному лишь нулевому размеру расходил разовую работу с
 *          потоковой: поток пустое сообщение принимал, а разовая работа
 *          возвращала пустоту, неотличимую от отказа. У пустого сообщения
 *          есть шифротекст — вектор инициализации и имитовставка
 *
 */
TEST_F(CryptoFixture, EmptyMessageCryptoTest){
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	/**
	 * Выполняем перебор режимов блочного шифрования
	 */
	for(const awh::crypto_t::mode_t mode : {awh::crypto_t::mode_t::GCM, awh::crypto_t::mode_t::CFB}){
		// Устанавливаем режим блочного шифрования
		this->_crypto->mode(mode);
		// Определяем размер имитовставки режима блочного шифрования
		const size_t tagsize = ((mode == awh::crypto_t::mode_t::GCM) ? 16 : 0);
		// Определяем размер вектора инициализации режима блочного шифрования
		const size_t ivsize = ((mode == awh::crypto_t::mode_t::GCM) ? 12 : 16);
		// Выполняем шифрование сообщения, октетов не имеющего
		const std::string encoded = this->_crypto->encrypt <std::string> ("", 0, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
		// Проверяем длину шифротекста сообщения, октетов не имеющего
		ASSERT_EQ(encoded.size(), (ivsize + tagsize));
		// Выполняем расшифровку шифротекста
		const std::string decoded = this->_crypto->decrypt <std::string> (encoded.data(), encoded.size(), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
		// Проверяем пустоту расшифрованного сообщения
		EXPECT_TRUE(decoded.empty());
		/**
		 * Проверяем отлов подделки шифротекста сообщения, октетов не имеющего
		 */
		if(tagsize > 0){
			// Копируем шифротекст для подделки
			std::string tampered = encoded;
			// Выполняем подделку последнего октета имитовставки
			tampered.back() = static_cast <char> (tampered.back() ^ 0x01);
			// Проверяем отказ расшифровки поддельного шифротекста
			EXPECT_TRUE(this->_crypto->decrypt <std::string> (tampered.data(), tampered.size(), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
		}
	}
}

/**
 * @brief Тест отказа нулевого количества итераций вывода ключа
 *
 * @details Нулевое количество итераций уходило в отказ OpenSSL без указания
 *          на настоящую причину, а прежде установленное значение при этом
 *          терялось
 *
 */
TEST_F(CryptoFixture, RoundsCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Выполняем шифрование текста
	const std::string encoded = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем выполнение шифрования
	ASSERT_FALSE(encoded.empty());
	// Устанавливаем нулевое количество итераций вывода ключа
	this->_crypto->roundAES(0);
	// Проверяем, что прежде установленное количество итераций сохранено
	EXPECT_EQ(this->_crypto->decrypt <std::string> (encoded.data(), encoded.size(), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), text);
}

/**
 * @brief Тест отказа разбора негодного BASE64
 *
 * @details Ветвь BASE64 доходила до общего успешного выхода при любом исходе,
 *          и негодная запись была неотличима от разбора в пустоту
 *
 */
TEST_F(CryptoFixture, Base64FailureCryptoTest){
	// Проверяем обратимость кодирования BASE64
	EXPECT_EQ(this->_crypto->decrypt <std::string> (this->_crypto->encrypt <std::string> (std::string("Anyks"), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::BASE64), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::BASE64), "Anyks");
	// Проверяем кодирование сообщения, октетов не имеющего
	EXPECT_TRUE(this->_crypto->encrypt <std::string> ("", 0, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::BASE64).empty());
	// Проверяем разбор записи, октетов не имеющей
	EXPECT_TRUE(this->_crypto->decrypt <std::string> ("", 0, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::BASE64).empty());
	// Проверяем отказ разбора записи, алфавиту BASE64 не принадлежащей
	EXPECT_TRUE(this->_crypto->decrypt <std::string> (std::string("!!!!"), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::BASE64).empty());
}

/**
 * @brief Тест вычитывания защищённого приватного ключа RSA из файла
 *
 * @details Путь для MS Windows пароль защиты ключа не передавал вовсе, и
 *          защищённый ключ на нём не открывался
 *
 */
TEST_F(CryptoFixture, KeyFileCryptoTest){
	// Путь к файлу приватного ключа
	const std::string path = "./sealed_private_key.pem";
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Устанавливаем пароль защиты приватного ключа RSA
	this->_crypto->passwordRSA("key-password");
	// Выполняем выписывание приватного ключа RSA в файл
	ASSERT_TRUE(this->_crypto->savePrivateKeyRSA(path));
	// Выполняем вычитывание приватного ключа RSA из файла
	EXPECT_TRUE(this->_crypto->loadPrivateKeyRSA(path));
	// Удаляем файл приватного ключа
	::remove(path.c_str());
}

/**
 * @brief Тест отказа буфера, предел разрядности библиотеки криптографии превышающего
 *
 * @details Приведение размера к знаковому 32-битному числу молча обрезало буфер,
 *          и работа выдавала шифротекст части поданных данных за шифротекст всех
 *
 */
TEST_F(CryptoFixture, BufferLimitCryptoTest){
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Небольшой буфер, поданный с размером свыше предела разрядности
	const std::string text = "Anyks Framework";
	// Проверяем отказ шифрования буфера, предел разрядности превышающего
	EXPECT_TRUE(this->_crypto->encrypt <std::string> (text.data(), (static_cast <size_t> (INT32_MAX) + 1), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
}

/**
 * @brief Тест вычитывания приватного ключа RSA из обозрения, нулём не оканчивающегося
 *
 * @details Обозрение строки завершающего нуля не обещает, и подача его указателя
 *          в работу с файлами открывала бы не тот файл либо уводила чтение за
 *          границу обозреваемого
 *
 */
TEST_F(CryptoFixture, PathViewCryptoTest){
	// Строка, в которой путь к файлу нулём не оканчивается
	const std::string storage = "./view_private_key.pemXXXXXX";
	// Обозрение пути к файлу, завершающего нуля не имеющее
	const std::string_view path(storage.data(), storage.size() - 6);
	// Имя файла приватного ключа, завершающим нулём оканчивающееся
	const std::string filename(path);
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Выполняем выписывание приватного ключа RSA в файл по имени, нулём оканчивающемуся
	ASSERT_TRUE(this->_crypto->savePrivateKeyRSA(filename));
	// Выполняем вычитывание приватного ключа RSA по обозрению, нулём не оканчивающемуся
	EXPECT_TRUE(this->_crypto->loadPrivateKeyRSA(path));
	// Удаляем файл приватного ключа
	::remove(filename.c_str());
}

/**
 * @brief Тест отказа завершения потока при недочитанном векторе инициализации
 *
 * @details Контекст расшифровки заводится лишь по вычитывании вектора из начала
 *          потока, и завершение шло по контексту, ключом не наделённому
 *
 */
TEST_F(CryptoFixture, StreamShortVectorCryptoTest){
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	/**
	 * Выполняем перебор режимов блочного шифрования
	 */
	for(const awh::crypto_t::mode_t mode : {awh::crypto_t::mode_t::GCM, awh::crypto_t::mode_t::CFB}){
		// Устанавливаем режим блочного шифрования
		this->_crypto->mode(mode);
		// Выполняем инициализацию контекста расшифровки
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
		// Расшифрованный текст
		std::string decoded;
		// Подаём в поток часть вектора инициализации
		decoded = this->_crypto->decrypt <std::string> (std::string(4, '\0'));
		// Проверяем отсутствие выхода на неполном векторе инициализации
		EXPECT_TRUE(decoded.empty());
		// Проверяем отказ завершения потока при недочитанном векторе инициализации
		EXPECT_FALSE(this->_crypto->finalize(decoded));
	}
}

/**
 * @brief Тест отказа выработки ключа RSA недостаточной разрядности
 *
 * @details Разрядность в глубине выработки не проверялась вовсе, и ключ короче
 *          двух тысяч разрядов стойкости не имел
 *
 */
TEST_F(CryptoFixture, KeySizeCryptoTest){
	// Проверяем отказ выработки ключа RSA недостаточной разрядности
	EXPECT_FALSE(this->_crypto->generatePrivateKeyRSA(512));
	// Проверяем отказ выработки ключа RSA недостаточной разрядности
	EXPECT_FALSE(this->_crypto->generatePrivateKeyRSA(1024));
	// Проверяем выработку ключа RSA при незаданной разрядности
	EXPECT_TRUE(this->_crypto->generatePrivateKeyRSA(0));
	// Проверяем выработку ключа RSA достаточной разрядности
	EXPECT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
}

/**
 * @brief Тест отказа выписывания ключа шифром, защите ключа не подходящим
 *
 * @details Тип шифрования, разбору не знакомый, молча подменялся наибольшей
 *          разрядностью: работа думала, что ключ защищён тем шифром, который
 *          она назвала
 *
 */
TEST_F(CryptoFixture, KeyCipherCryptoTest){
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Устанавливаем пароль защиты приватного ключа RSA
	this->_crypto->passwordRSA("key-password");
	// Проверяем выписывание ключа шифрованием, защите ключа подходящим
	EXPECT_FALSE(this->_crypto->getPrivateKeyRSA(awh::crypto_t::cipher_t::AES128).empty());
	// Проверяем выписывание ключа шифрованием, защите ключа подходящим
	EXPECT_FALSE(this->_crypto->getPrivateKeyRSA(awh::crypto_t::cipher_t::AES192).empty());
	// Проверяем выписывание ключа шифрованием, защите ключа подходящим
	EXPECT_FALSE(this->_crypto->getPrivateKeyRSA(awh::crypto_t::cipher_t::AES256).empty());
	// Проверяем выписывание ключа при незаданном шифровании
	EXPECT_FALSE(this->_crypto->getPrivateKeyRSA().empty());
	// Проверяем отказ выписывания ключа шифрованием, защите ключа не подходящим
	EXPECT_TRUE(this->_crypto->getPrivateKeyRSA(awh::crypto_t::cipher_t::BASE64).empty());
	/**
	 * Итог снимается с объекта BIO только по удавшейся выписке: отказ записи
	 * оставлял в объекте недописанную часть, и она уходила наружу непустым итогом
	 */
	// Получаем приватный ключ RSA после отказа выписывания
	const std::string sealed = this->_crypto->getPrivateKeyRSA(awh::crypto_t::cipher_t::AES256);
	// Проверяем получение приватного ключа RSA целиком
	ASSERT_FALSE(sealed.empty());
	// Проверяем целость полученного приватного ключа RSA
	EXPECT_NE(sealed.find("-----END ENCRYPTED PRIVATE KEY-----"), std::string::npos);
	// Проверяем вычитывание полученного приватного ключа RSA
	EXPECT_TRUE(this->_crypto->setPrivateKeyRSA(sealed));
	// Путь к файлу приватного ключа
	const std::string path = "./cipher_private_key.pem";
	// Проверяем отказ выписывания ключа в файл шифрованием, защите ключа не подходящим
	EXPECT_FALSE(this->_crypto->savePrivateKeyRSA(path, awh::crypto_t::cipher_t::BASE64));
	// Удаляем файл приватного ключа
	::remove(path.c_str());
}

/**
 * @brief Тест затирания открытого текста при отказе разовой расшифровки
 *
 * @details Отказ работы буфер лишь очищал, тогда как очистка содержимого не
 *          гасит: подделка шифротекста выдавала открытый текст в кучу
 *
 */
TEST_F(CryptoFixture, WipeCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Выполняем шифрование текста
	const std::string encoded = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем выполнение шифрования
	ASSERT_FALSE(encoded.empty());
	// Формируем поддельный шифротекст
	std::string tampered = encoded;
	// Выполняем подделку последнего октета имитовставки
	tampered.back() = static_cast <char> (tampered.back() ^ 0x01);
	// Проверяем отказ расшифровки поддельного шифротекста
	EXPECT_TRUE(this->_crypto->decrypt <std::string> (tampered.data(), tampered.size(), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
	// Проверяем обратимость шифрования подлинного шифротекста
	EXPECT_EQ(this->_crypto->decrypt <std::string> (encoded.data(), encoded.size(), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), text);
}

/**
 * @brief Тест признака работы у разовой работы
 *
 * @details Наружу уходил один лишь буфер, а пустой буфер отказом не является:
 *          расшифровка сообщения, октетов не имеющего, даёт пустой открытый
 *          текст — удача и отказ выглядели одинаково
 *
 */
TEST_F(CryptoFixture, OutcomeCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Буфер шифротекста
	std::string encoded;
	// Проверяем признак работы при шифровании текста
	ASSERT_TRUE(this->_crypto->encrypt <std::string> (text.data(), text.size(), encoded, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем наличие шифротекста
	ASSERT_FALSE(encoded.empty());
	// Буфер открытого текста
	std::string decoded;
	// Проверяем признак работы при расшифровке шифротекста
	ASSERT_TRUE(this->_crypto->decrypt <std::string> (encoded.data(), encoded.size(), decoded, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем обратимость шифрования
	EXPECT_EQ(decoded, text);
	/**
	 * Пустое сообщение: буфер пуст и при удаче, и при отказе, — различает их
	 * один лишь признак работы
	 */
	// Буфер шифротекста сообщения, октетов не имеющего
	std::string empty;
	// Проверяем признак работы при шифровании сообщения, октетов не имеющего
	ASSERT_TRUE(this->_crypto->encrypt <std::string> ("", 0, empty, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Буфер открытого текста сообщения, октетов не имеющего
	std::string opened;
	// Проверяем признак работы при расшифровке сообщения, октетов не имеющего
	EXPECT_TRUE(this->_crypto->decrypt <std::string> (empty.data(), empty.size(), opened, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем пустоту открытого текста при удавшейся расшифровке
	EXPECT_TRUE(opened.empty());
	// Формируем поддельный шифротекст сообщения, октетов не имеющего
	std::string tampered = empty;
	// Выполняем подделку последнего октета имитовставки
	tampered.back() = static_cast <char> (tampered.back() ^ 0x01);
	// Проверяем отказ расшифровки поддельного шифротекста
	EXPECT_FALSE(this->_crypto->decrypt <std::string> (tampered.data(), tampered.size(), opened, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем пустоту открытого текста при отказе расшифровки
	EXPECT_TRUE(opened.empty());
	// Проверяем отказ работы при незаданном типе шифрования
	EXPECT_FALSE(this->_crypto->encrypt <std::string> (text.data(), text.size(), encoded, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::NONE));
}

/**
 * @brief Тест сброса состояния при отказе вывода ключа
 *
 * @details Метки разрядности и хэш-суммы оставались от прежнего вывода, а ключ
 *          к этой поре был уже отведён и заполнен нулями: следующий вызов с
 *          прежними метками счёл бы ключ готовым и зашифровал бы нулевым ключом
 *
 */
TEST_F(CryptoFixture, KeyDerivationFailureCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Выполняем шифрование текста, выводя ключ по первой хэш-сумме
	const std::string first = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем выполнение шифрования
	ASSERT_FALSE(first.empty());
	/**
	 * Вызов с разбору не знакомой хэш-суммой отменяет вывод ключа уже после
	 * отведения самого ключа — ровно то состояние, ради которого тест и написан
	 */
	// Выполняем шифрование текста хэш-суммой, разбору не знакомой
	EXPECT_TRUE(this->_crypto->encrypt <std::string> (text, static_cast <awh::crypto_t::hash_t> (0xFE), awh::crypto_t::cipher_t::AES256).empty());
	// Выполняем шифрование текста прежней хэш-суммой
	const std::string second = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем выполнение шифрования
	ASSERT_FALSE(second.empty());
	// Проверяем обратимость шифрования, выполненного после отказа вывода ключа
	EXPECT_EQ(this->_crypto->decrypt <std::string> (second.data(), second.size(), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), text);
	/**
	 * Ключ выведен заново, а не взят нулевым: шифротекст, снятый прежним ключом,
	 * расшифровывается тем же паролем и той же солью
	 */
	// Проверяем обратимость шифрования, выполненного до отказа вывода ключа
	EXPECT_EQ(this->_crypto->decrypt <std::string> (first.data(), first.size(), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), text);
}
