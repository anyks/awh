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
 * Заголовочный файл очереди ошибок библиотеки криптографии
 */
#include <openssl/err.h>

/**
 * Стандартный заголовочный файл работы с файлами
 */
#include <chrono>
#include <fstream>

/**
 * Стандартный заголовочный файл примет файла
 */
#include <sys/stat.h>

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
	/**
	 * Отбор шифра сличается с одним лишь счётчиком Галуа, и значение, ни одному
	 * из режимов не отвечающее, молча уходило в гаммирование - в работу без
	 * проверки подлинности, которой вызывающий не просил
	 */
	// Устанавливаем режим блочного шифрования, разбору не знакомый
	this->_crypto->mode(static_cast <awh::crypto_t::mode_t> (0xFE));
	// Проверяем отказ шифрования при режиме блочного шифрования, разбору не знакомом
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
	// Запоминаем годную подпись схемой PKCS#1 v1.5
	const std::vector <uint8_t> signature = result;
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
	/**
	 * Отбор схемы вёлся сличением с одной лишь вероятностной, и всякое иное
	 * значение молча уходило дополнением PKCS#1 - схемой, доказанной стойкости
	 * не имеющей и выбираемой лишь явно
	 */
	// Устанавливаем схему дополнения подписи, разбору не знакомую
	this->_crypto->padding(static_cast <awh::crypto_t::padding_t> (0xFE));
	// Выполняем подпись данных схемой дополнения, разбору не знакомой
	this->_crypto->signWithPrivateKey(buffer, awh::crypto_t::hash_t::SHA256, result);
	// Проверяем отказ подписи схемой дополнения, разбору не знакомой
	EXPECT_TRUE(result.empty());
	// Проверяем отказ проверки подписи схемой дополнения, разбору не знакомой
	EXPECT_FALSE(this->_crypto->verifyWithPublicKey(buffer, signature, awh::crypto_t::hash_t::SHA256));
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
	/**
	 * Количество итераций приводится к знаковому 32-битному числу, и большее
	 * обращалось бы в отрицательное — работа задала бы перебору не цену, а отказ
	 */
	// Устанавливаем количество итераций, предел разрядности превышающее
	this->_crypto->roundAES(static_cast <uint32_t> (INT32_MAX) + 1);
	// Проверяем, что прежде установленное количество итераций сохранено
	EXPECT_EQ(this->_crypto->decrypt <std::string> (encoded.data(), encoded.size(), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), text);
	/**
	 * Наибольшее пригодное количество итераций испытанием не проверяется: оно
	 * честно отработало бы два миллиарда итераций, а это две минуты на прогон
	 */
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
 * @brief Тест единственности записи отказа BASE64 в лог
 *
 * @details Причину отказа писал разбор цепочки объектов ввода-вывода, а обёртка
 *          добавляла свою запись: отказ BASE64 попадал в лог дважды, тогда как
 *          решение 4.12 обещает единственную запись. У шифрования AES записей
 *          двое намеренно (7.4): там первая называет причину
 *
 */
TEST_F(CryptoFixture, Base64SingleRecordCryptoTest){
	// Количество записей отказа, полученных из лога
	size_t records = 0;
	// Подписываемся на получение логов
	this->_log->subscribe([&records](const awh::log_t::flag_t flag, std::string_view text) noexcept -> void {
		// Снимаем предупреждения о неиспользуемых параметрах
		(void) flag;
		(void) text;
		// Наращиваем количество полученных записей
		records++;
	});
	// Устанавливаем отложенный режим логов, консоль набора не засоряя
	this->_log->mode({awh::log_t::mode_t::DEFERRED});
	// Проверяем отказ разбора записи, алфавиту BASE64 не принадлежащей
	EXPECT_TRUE(this->_crypto->decrypt <std::string> (std::string("!!!!"), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::BASE64).empty());
	// Проверяем, что отказ записан в лог единожды
	EXPECT_EQ(records, static_cast <size_t> (1));
	// Снимаем режимы логов
	this->_log->mode({awh::log_t::mode_t::NONE});
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

/**
 * @brief Тест признака работы у работ с ключом RSA
 *
 * @details Работы выводили пустоту и при удаче, и при отказе: пустой открытый
 *          текст после расшифровки ключом RSA от отказа было не отличить
 *
 */
TEST_F(CryptoFixture, KeyOutcomeCryptoTest){
	// Буфер данных для работы
	const std::vector <uint8_t> data = {0x41, 0x4E, 0x59, 0x4B, 0x53};
	// Буфер шифротекста
	std::vector <uint8_t> sealed;
	// Проверяем отказ работы при незаведённом ключе RSA
	EXPECT_FALSE(this->_crypto->encryptWithPublicKey(data, sealed));
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Проверяем признак работы при шифровании ключом RSA
	ASSERT_TRUE(this->_crypto->encryptWithPublicKey(data, sealed));
	// Проверяем наличие шифротекста
	ASSERT_FALSE(sealed.empty());
	// Буфер открытого текста
	std::vector <uint8_t> opened;
	// Проверяем признак работы при расшифровке ключом RSA
	ASSERT_TRUE(this->_crypto->decryptWithPrivateKey(sealed, opened));
	// Проверяем обратимость шифрования ключом RSA
	EXPECT_EQ(opened, data);
	// Формируем поддельный шифротекст
	std::vector <uint8_t> tampered = sealed;
	// Выполняем подделку октета шифротекста
	tampered[tampered.size() / 2] = static_cast <uint8_t> (tampered[tampered.size() / 2] ^ 0x01);
	// Проверяем отказ расшифровки поддельного шифротекста
	EXPECT_FALSE(this->_crypto->decryptWithPrivateKey(tampered, opened));
	// Проверяем пустоту открытого текста при отказе расшифровки
	EXPECT_TRUE(opened.empty());
	// Буфер подписи
	std::vector <uint8_t> signature;
	// Проверяем признак работы при подписи данных
	ASSERT_TRUE(this->_crypto->signWithPrivateKey(data, awh::crypto_t::hash_t::SHA256, signature));
	// Проверяем проверку подписи
	EXPECT_TRUE(this->_crypto->verifyWithPublicKey(data, signature, awh::crypto_t::hash_t::SHA256));
	// Проверяем отказ подписи хэш-суммой, разбору не знакомой
	EXPECT_FALSE(this->_crypto->signWithPrivateKey(data, static_cast <awh::crypto_t::hash_t> (0xFE), signature));
	// Проверяем пустоту подписи при отказе работы
	EXPECT_TRUE(signature.empty());
	/**
	 * Сообщение, октетов не имеющее, работой принимается наравне с работой по
	 * симметричному ключу (4.9): пустота сообщения — не отсутствие сообщения
	 */
	// Буфер сообщения, октетов не имеющего
	const std::vector <uint8_t> empty;
	// Буфер шифротекста сообщения, октетов не имеющего
	std::vector <uint8_t> sealedEmpty;
	// Проверяем шифрование сообщения, октетов не имеющего
	ASSERT_TRUE(this->_crypto->encryptWithPublicKey(empty, sealedEmpty));
	// Проверяем наличие шифротекста
	ASSERT_FALSE(sealedEmpty.empty());
	// Буфер открытого текста сообщения, октетов не имеющего
	std::vector <uint8_t> openedEmpty;
	// Проверяем расшифровку сообщения, октетов не имеющего
	EXPECT_TRUE(this->_crypto->decryptWithPrivateKey(sealedEmpty, openedEmpty));
	// Проверяем пустоту открытого текста при удавшейся расшифровке
	EXPECT_TRUE(openedEmpty.empty());
	// Буфер подписи сообщения, октетов не имеющего
	std::vector <uint8_t> signatureEmpty;
	// Проверяем подпись сообщения, октетов не имеющего
	ASSERT_TRUE(this->_crypto->signWithPrivateKey(empty, awh::crypto_t::hash_t::SHA256, signatureEmpty));
	// Проверяем проверку подписи сообщения, октетов не имеющего
	EXPECT_TRUE(this->_crypto->verifyWithPublicKey(empty, signatureEmpty, awh::crypto_t::hash_t::SHA256));
	// Проверяем отказ работы при отсутствующем буфере с заявленным размером
	EXPECT_FALSE(this->_crypto->encryptWithPublicKey(nullptr, 16, sealedEmpty));
	/**
	 * Дополнение OAEP отводит под себя две хэш-суммы и ещё два октета: у ключа
	 * в две тысячи сорок восемь разрядов под сообщение остаётся 190 октетов
	 */
	// Сообщение наибольшего пригодного размера
	const std::vector <uint8_t> largest(190, 0x41);
	// Буфер шифротекста сообщения наибольшего пригодного размера
	std::vector <uint8_t> sealedLargest;
	// Проверяем шифрование сообщения наибольшего пригодного размера
	EXPECT_TRUE(this->_crypto->encryptWithPublicKey(largest, sealedLargest));
	// Сообщение, предел шифрования ключом RSA превышающее
	const std::vector <uint8_t> oversized(191, 0x41);
	// Буфер шифротекста сообщения, предел превышающего
	std::vector <uint8_t> sealedOversized;
	// Проверяем отказ шифрования сообщения, предел превышающего
	EXPECT_FALSE(this->_crypto->encryptWithPublicKey(oversized, sealedOversized));
	// Проверяем пустоту шифротекста при отказе работы
	EXPECT_TRUE(sealedOversized.empty());
}

/**
 * @brief Тест завершения потока поверх непустого буфера
 *
 * @details Вектор инициализации вставлялся в начало поданного буфера, а хвост
 *          завершения дописывался за прежним его содержимым — шифротекст
 *          выходил с разорванной серединой
 *
 */
TEST_F(CryptoFixture, StreamAppendCryptoTest){
	// Содержимое, лежащее в буфере до работы завершения
	const std::string header = "AWH";
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
		// Буфер, в котором уже лежит содержимое вызывающего
		std::string encoded = header;
		// Выполняем инициализацию контекста шифрования
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
		// Выполняем завершение потокового шифрования, порций не подавая
		ASSERT_TRUE(this->_crypto->finalize(encoded));
		// Проверяем сохранность прежнего содержимого буфера в его начале
		ASSERT_EQ(encoded.compare(0, header.size(), header), 0);
		// Снимаем шифротекст, прежнее содержимое буфера отбрасывая
		const std::string sealed = encoded.substr(header.size());
		// Проверяем наличие шифротекста
		ASSERT_FALSE(sealed.empty());
		// Расшифрованный текст
		std::string decoded;
		// Выполняем инициализацию контекста расшифровки
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
		// Выполняем расшифровку шифротекста
		decoded = this->_crypto->decrypt <std::string> (sealed);
		// Проверяем обратимость потокового шифрования поверх непустого буфера
		ASSERT_TRUE(this->_crypto->finalize(decoded));
		// Проверяем пустоту расшифрованного сообщения
		EXPECT_TRUE(decoded.empty());
	}
}

/**
 * @brief Тест независимости работы с BASE64 от заведённого потока
 *
 * @details Перехват ошибок сбрасывал состояние по всякому пути, а работа с
 *          BASE64 состояния не касается вовсе — сбой её сносил бы заведённый
 *          поток заодно
 *
 */
TEST_F(CryptoFixture, Base64OverStreamCryptoTest){
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
	// Выполняем инициализацию контекста шифрования
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем шифрование первой порции потока
	std::string encoded = this->_crypto->encrypt <std::string> (text);
	// Выполняем работу с BASE64 поверх заведённого потока
	const std::string digest = this->_crypto->encrypt <std::string> (std::string("Anyks"), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::BASE64);
	// Проверяем выполнение работы с BASE64
	ASSERT_FALSE(digest.empty());
	// Проверяем обратимость работы с BASE64
	EXPECT_EQ(this->_crypto->decrypt <std::string> (digest, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::BASE64), "Anyks");
	// Выполняем шифрование второй порции потока
	encoded.append(this->_crypto->encrypt <std::string> (text));
	// Проверяем, что заведённый поток работой с BASE64 не снесён
	ASSERT_TRUE(this->_crypto->finalize(encoded));
	// Расшифрованный текст
	std::string decoded;
	// Выполняем инициализацию контекста расшифровки
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем расшифровку шифротекста
	decoded = this->_crypto->decrypt <std::string> (encoded);
	// Проверяем завершение потоковой расшифровки
	ASSERT_TRUE(this->_crypto->finalize(decoded));
	// Проверяем обратимость потокового шифрования
	EXPECT_EQ(decoded, (text + text));
}

/**
 * @brief Тест отказа хэширования при незаданном типе хэш-суммы
 *
 * @details Работа выводила пустоту молча, и отличить её от пустого итога было
 *          нечем: подпись ключом тот же случай называла прямо, хэширование нет
 *
 */
TEST_F(CryptoFixture, UnknownHashCryptoTest){
	// Текст для хэширования
	const std::string text = "Anyks Framework";
	// Ключ подписи
	const std::string key = "secret";
	// Проверяем хэширование известным типом хэш-суммы
	EXPECT_FALSE(this->_crypto->hash <std::string> (text, awh::crypto_t::hash_t::SHA256).empty());
	// Проверяем подпись известным типом хэш-суммы
	EXPECT_FALSE(this->_crypto->hmac <std::string> (key, text, awh::crypto_t::hash_t::SHA256).empty());
	// Проверяем отказ хэширования при незаданном типе хэш-суммы
	EXPECT_TRUE(this->_crypto->hash <std::string> (text, awh::crypto_t::hash_t::NONE).empty());
	// Проверяем отказ подписи при незаданном типе хэш-суммы
	EXPECT_TRUE(this->_crypto->hmac <std::string> (key, text, awh::crypto_t::hash_t::NONE).empty());
	// Проверяем отказ хэширования типом хэш-суммы, разбору не знакомым
	EXPECT_TRUE(this->_crypto->hash <std::string> (text, static_cast <awh::crypto_t::hash_t> (0xFE)).empty());
	// Проверяем отказ подписи типом хэш-суммы, разбору не знакомым
	EXPECT_TRUE(this->_crypto->hmac <std::string> (key, text, static_cast <awh::crypto_t::hash_t> (0xFE)).empty());
}

/**
 * @brief Тест разрядности ввозимого ключа RSA
 *
 * @details Выработка ключа отвергает разрядность ниже двух тысяч, а ввод принимал
 *          всякую: слабый ключ из файла доходил до шифрования и подписи. Приватный
 *          ключ - свой, и отвергается наравне с выработкой; открытый - чужой, и
 *          его разрядность оглашается предупреждением, а решение оставлено
 *          вызывающему
 *
 */
TEST_F(CryptoFixture, ImportKeyStrengthCryptoTest){
	// Приватный ключ RSA разрядностью в тысячу двадцать четыре
	const std::string privateKey =
		"-----BEGIN PRIVATE KEY-----\n"
		"MIICeQIBADANBgkqhkiG9w0BAQEFAASCAmMwggJfAgEAAoGBAMNRiIIFp4wi7mkq\n"
		"+hU5LqNNPRSDDaY6OpUJdyPycauoW7QLV0cCoii8pv3OEAhj5ru4TYXIWiHMex2L\n"
		"EvOY7s4CeM4/iWqL/eYwyjeqdfP1xIopFK6eAmcuVnKXSnx8WoAsNw0q4SivbCtQ\n"
		"57+ESZsuTWhC40lfmsHZW6k9BNshAgMBAAECgYEAlKsO2MktCwHbrrlDubvYv/we\n"
		"repDDW/s/1xBD1+PHjX790Nan3Zlr9RI149trLU9/0z91QL3eBqI66fcOQcDXP1n\n"
		"8rSZE7CLA72aPyHuA5BSjKBtRbOgtNyO2GWsUlWouCVgXFUYBCopFvAaysD8Mmye\n"
		"MJtOFONWlRW5S5xA6nECQQD3VyMMLk44UePSBdUfKAjsXpV9U8UARk7C5QNU9+T0\n"
		"SSEgkFERM7/Dm/KYyfBPGa1QnVENds6gCNnVpAInVfpfAkEAyigiZtcd1mtDgEyP\n"
		"uXxSyrdyDxO1x38Arus9mZ93w96jZEl4EHVsr+ME7ABbre+jwc7ldmvCkTNlG7j1\n"
		"qBAafwJBAMniuPu3TCdSSCdklVmx/t6YMWKznogj2yPfdAHFuX7ftgdzZIgq+ip6\n"
		"vuCRa/HUno+/aKoZwHwF3XAxR4S9+/cCQQDFVeQvC3I+6robtaDe6bNP2z7l5NGf\n"
		"iiQ6m7uoCHi6pMxOi0E+n8GW+D7HuZnE8paiC7sGnC5z2v2p0CVNB1s1AkEAlRyc\n"
		"FOLMKEguotYBC6GK33KrXfpyzzvJUmHfZWkGDktVcLygz9WqXEFYoeJL/SxG4DSB\n"
		"in18lhsNIWYfXpED0A==\n"
		"-----END PRIVATE KEY-----";
	// Открытый ключ RSA той же разрядности
	const std::string publicKey =
		"-----BEGIN PUBLIC KEY-----\n"
		"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDDUYiCBaeMIu5pKvoVOS6jTT0U\n"
		"gw2mOjqVCXcj8nGrqFu0C1dHAqIovKb9zhAIY+a7uE2FyFohzHsdixLzmO7OAnjO\n"
		"P4lqi/3mMMo3qnXz9cSKKRSungJnLlZyl0p8fFqALDcNKuEor2wrUOe/hEmbLk1o\n"
		"QuNJX5rB2VupPQTbIQIDAQAB\n"
		"-----END PUBLIC KEY-----";
	// Количество предупреждений, полученных из лога
	size_t records = 0;
	// Подписываемся на получение логов
	this->_log->subscribe([&records](const awh::log_t::flag_t flag, std::string_view text) noexcept -> void {
		// Снимаем предупреждение о неиспользуемом параметре
		(void) text;
		// Если получено предупреждение
		if(flag == awh::log_t::flag_t::WARNING)
			// Наращиваем количество полученных предупреждений
			records++;
	});
	// Устанавливаем отложенный режим логов, консоль набора не засоряя
	this->_log->mode({awh::log_t::mode_t::DEFERRED});
	// Проверяем приём приватного ключа недостаточной разрядности
	EXPECT_TRUE(this->_crypto->setPrivateKeyRSA(privateKey));
	// Проверяем, что разрядность приватного ключа оглашена
	EXPECT_EQ(records, static_cast <size_t> (1));
	/**
	 * Ключ недостаточной разрядности работать обязан: им расшифровывают старые
	 * данные и проверяют давние подписи, и отказ на вводе лишил бы вызывающего
	 * работы, которую тот в состоянии выполнить (5.20)
	 */
	// Сообщение подписи
	const std::vector <uint8_t> text = {0x41, 0x4E, 0x59, 0x4B, 0x53};
	// Буфер подписи
	std::vector <uint8_t> signature;
	// Проверяем, что ввезённый ключ работает
	EXPECT_TRUE(this->_crypto->signWithPrivateKey(text, awh::crypto_t::hash_t::SHA256, signature));
	// Проверяем приём открытого ключа недостаточной разрядности
	EXPECT_TRUE(this->_crypto->setPublicKeyRSA(publicKey));
	// Проверяем, что разрядность открытого ключа оглашена
	EXPECT_EQ(records, static_cast <size_t> (2));
	// Проверяем, что ввезённым открытым ключом подпись проверяется
	EXPECT_TRUE(this->_crypto->verifyWithPublicKey(text, signature, awh::crypto_t::hash_t::SHA256));
	// Выполняем генерацию приватного ключа RSA годной разрядности
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Проверяем, что разрядность годного ключа не оглашается
	EXPECT_EQ(records, static_cast <size_t> (2));
	// Снимаем режимы логов
	this->_log->mode({awh::log_t::mode_t::NONE});
}

/**
 * @brief Тест сохранности прежнего файла ключа при отказе выписывания
 *
 * @details Разбор типа шифрования шёл после открытия файла, а открытие усекает
 *          прежний файл сразу: вызов с негодным типом уничтожал годный ключ,
 *          на диске лежавший, и работы при этом не начинал. Ключ выписывается
 *          теперь в отдельный файл и ставится на место переименованием
 *
 */
TEST_F(CryptoFixture, KeyFileSurvivesFailureCryptoTest){
	// Путь к файлу приватного ключа
	const std::string path = "./survives_private_key.pem";
	// Путь к файлу открытого ключа
	const std::string publicPath = "./survives_public_key.pem";
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Устанавливаем пароль защиты приватного ключа RSA
	this->_crypto->passwordRSA("key-password");
	// Выполняем выписывание годного приватного ключа RSA в файл
	ASSERT_TRUE(this->_crypto->savePrivateKeyRSA(path, awh::crypto_t::cipher_t::AES256));
	// Открываем файл годного приватного ключа на чтение
	std::ifstream source(path, std::ios::binary);
	// Проверяем что файл открыт
	ASSERT_TRUE(source.is_open());
	// Вычитываем содержимое файла годного приватного ключа
	const std::string original((std::istreambuf_iterator <char> (source)), std::istreambuf_iterator <char> ());
	// Закрываем файл годного приватного ключа
	source.close();
	// Проверяем что годный приватный ключ выписан целиком
	ASSERT_NE(original.find("-----END ENCRYPTED PRIVATE KEY-----"), std::string::npos);
	// Проверяем отказ выписывания ключа шифрованием, защите ключа не подходящим
	EXPECT_FALSE(this->_crypto->savePrivateKeyRSA(path, awh::crypto_t::cipher_t::BASE64));
	// Открываем файл приватного ключа на чтение после отказа
	std::ifstream target(path, std::ios::binary);
	// Проверяем что прежний файл на месте
	ASSERT_TRUE(target.is_open());
	// Вычитываем содержимое файла приватного ключа после отказа
	const std::string survived((std::istreambuf_iterator <char> (target)), std::istreambuf_iterator <char> ());
	// Закрываем файл приватного ключа
	target.close();
	// Проверяем что прежний ключ отказом не тронут
	EXPECT_EQ(survived, original);
	/**
	 * Отдельный файл после работы остаться не должен ни при удаче, ни при отказе
	 */
	// Приметы отдельного файла выписывания
	struct stat attributes;
	// Проверяем что отдельного файла выписывания не осталось
	EXPECT_NE(::stat((path + ".tmp").c_str(), &attributes), 0);
	// Выполняем выписывание открытого ключа RSA в файл
	EXPECT_TRUE(this->_crypto->savePublicKeyRSA(publicPath));
	// Проверяем что отдельного файла выписывания открытого ключа не осталось
	EXPECT_NE(::stat((publicPath + ".tmp").c_str(), &attributes), 0);
	// Удаляем файл открытого ключа
	::remove(publicPath.c_str());
	// Удаляем файл приватного ключа
	::remove(path.c_str());
}

/**
 * @brief Тест пароля защиты ключа RSA, нулевой октет содержащего
 *
 * @details Выписка ключа берёт пароль с указанием длины, а вычитывание шло с
 *          умолчательным разбором, берущим пароль до первого нулевого октета:
 *          пароль «a\0b» уходил в файл целиком, а обратно подавался как «a», и
 *          ключ, только что выписанный, тем же объектом не открывался
 *
 */
TEST_F(CryptoFixture, KeyPasswordZeroCryptoTest){
	// Путь к файлу приватного ключа
	const std::string path = "./zeroed_private_key.pem";
	// Пароль защиты ключа, нулевой октет содержащий
	const std::string password("a\0b", 3);
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Устанавливаем пароль защиты приватного ключа RSA
	this->_crypto->passwordRSA(password);
	// Выполняем выписывание приватного ключа RSA в файл
	ASSERT_TRUE(this->_crypto->savePrivateKeyRSA(path));
	// Проверяем вычитывание приватного ключа RSA из файла
	EXPECT_TRUE(this->_crypto->loadPrivateKeyRSA(path));
	// Получаем запись приватного ключа RSA под защитой того же пароля
	const std::string sealed = this->_crypto->getPrivateKeyRSA();
	// Проверяем что запись ключа получена
	ASSERT_FALSE(sealed.empty());
	// Проверяем ввод записи приватного ключа RSA под тем же паролем
	EXPECT_TRUE(this->_crypto->setPrivateKeyRSA(sealed));
	// Удаляем файл приватного ключа
	::remove(path.c_str());
}

/**
 * @brief Тест пароля защиты ключа RSA, предел разбора превышающего
 *
 * @details Выписка ключа берёт пароль с указанием длины, а буфер выдачи пароля
 *          при вычитывании отведён библиотекой по своей мерке: пароль длиннее
 *          выдавался обрезанным, и ключ, только что выписанный, тем же объектом
 *          не открывался
 *
 */
TEST_F(CryptoFixture, KeyPasswordLongCryptoTest){
	// Путь к файлу приватного ключа
	const std::string path = "./long_private_key.pem";
	// Пароль защиты ключа предельной длины
	const std::string bounded(1024, 'a');
	// Пароль защиты ключа, предел выдачи превышающий
	const std::string oversized(1025, 'b');
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Устанавливаем пароль защиты приватного ключа RSA предельной длины
	this->_crypto->passwordRSA(bounded);
	// Выполняем выписывание приватного ключа RSA в файл
	ASSERT_TRUE(this->_crypto->savePrivateKeyRSA(path));
	// Проверяем вычитывание приватного ключа RSA под паролем предельной длины
	EXPECT_TRUE(this->_crypto->loadPrivateKeyRSA(path));
	/**
	 * Пароль длиннее предела выдачи отвергается установкой: объект остаётся с
	 * прежним паролем, и ключ, им защищённый, открывается по-прежнему
	 */
	// Устанавливаем пароль защиты приватного ключа, предел выдачи превышающий
	this->_crypto->passwordRSA(oversized);
	// Проверяем вычитывание приватного ключа RSA прежним паролем
	EXPECT_TRUE(this->_crypto->loadPrivateKeyRSA(path));
	// Удаляем файл приватного ключа
	::remove(path.c_str());
}

/**
 * @brief Тест неповторимости вектора инициализации при удержании ключа
 *
 * @details Ключ удерживается между потоками и между разовыми работами, а вектор
 *          инициализации обязан быть новым на всякое сообщение: в режиме с
 *          проверкой подлинности повтор вектора на одном ключе выдаёт открытый
 *          текст обоих сообщений
 *
 */
TEST_F(CryptoFixture, VectorUniquenessCryptoTest){
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
	// Набор собранных векторов инициализации
	std::unordered_set <std::string> vectors;
	/**
	 * Выполняем перебор оборотов потокового шифрования на удержанном ключе
	 */
	for(uint16_t i = 0; i < 16; i++){
		// Выполняем инициализацию контекста шифрования
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
		// Выполняем шифрование порции потока
		std::string encoded = this->_crypto->encrypt <std::string> (text);
		// Выполняем завершение потока шифрования
		ASSERT_TRUE(this->_crypto->finalize(encoded));
		// Проверяем что шифротекст вектор инициализации несёт
		ASSERT_GE(encoded.size(), static_cast <size_t> (12));
		// Собираем вектор инициализации из начала шифротекста
		vectors.emplace(encoded.substr(0, 12));
	}
	// Проверяем неповторимость векторов инициализации потоков
	EXPECT_EQ(vectors.size(), static_cast <size_t> (16));
	// Очищаем набор собранных векторов инициализации
	vectors.clear();
	/**
	 * Выполняем перебор разовых работ на удержанном ключе
	 */
	for(uint16_t i = 0; i < 16; i++){
		// Выполняем разовое шифрование сообщения
		const std::string encoded = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
		// Проверяем что шифротекст вектор инициализации несёт
		ASSERT_GE(encoded.size(), static_cast <size_t> (12));
		// Собираем вектор инициализации из начала шифротекста
		vectors.emplace(encoded.substr(0, 12));
	}
	// Проверяем неповторимость векторов инициализации разовых работ
	EXPECT_EQ(vectors.size(), static_cast <size_t> (16));
}

/**
 * @brief Тест потоковой работы порциями по одному октету
 *
 * @details Вектор инициализации и имитовставка приходят разорванными по порциям,
 *          и накопитель обязан собрать их из разрозненных октетов
 *
 */
TEST_F(CryptoFixture, ByteChunkStreamCryptoTest){
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
	for(auto & mode : {awh::crypto_t::mode_t::GCM, awh::crypto_t::mode_t::CFB}){
		// Устанавливаем режим блочного шифрования
		this->_crypto->mode(mode);
		// Выполняем инициализацию контекста шифрования
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256)) << "mode = " << static_cast <uint16_t> (mode);
		// Результат потокового шифрования
		std::string encoded;
		/**
		 * Выполняем подачу текста по одному октету
		 */
		for(size_t i = 0; i < text.size(); i++){
			// Буфер выхода порции
			std::string part;
			// Выполняем шифрование порции в один октет
			ASSERT_TRUE(this->_crypto->encrypt <std::string> (text.data() + i, 1, part)) << "mode = " << static_cast <uint16_t> (mode);
			// Дописываем выход порции в результат
			encoded.append(part);
		}
		// Выполняем завершение потока шифрования
		ASSERT_TRUE(this->_crypto->finalize(encoded)) << "mode = " << static_cast <uint16_t> (mode);
		// Выполняем инициализацию контекста расшифровки
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256)) << "mode = " << static_cast <uint16_t> (mode);
		// Результат потоковой расшифровки
		std::string decoded;
		/**
		 * Выполняем подачу шифротекста по одному октету
		 */
		for(size_t i = 0; i < encoded.size(); i++){
			// Буфер выхода порции
			std::string part;
			// Выполняем расшифровку порции в один октет
			ASSERT_TRUE(this->_crypto->decrypt <std::string> (encoded.data() + i, 1, part)) << "mode = " << static_cast <uint16_t> (mode);
			// Дописываем выход порции в результат
			decoded.append(part);
		}
		// Выполняем завершение потока расшифровки
		ASSERT_TRUE(this->_crypto->finalize(decoded)) << "mode = " << static_cast <uint16_t> (mode);
		// Проверяем обратимость потоковой работы порциями по одному октету
		EXPECT_EQ(decoded, text) << "mode = " << static_cast <uint16_t> (mode);
	}
}

/**
 * @brief Тест согласия видов шифротекста разовой работы и потока
 *
 * @details Шифротекст, собранный разовой работой, обязан разбираться потоком, и
 *          наоборот: устройство его одно - вектор инициализации в начале,
 *          имитовставка в конце
 *
 */
TEST_F(CryptoFixture, FormatContractCryptoTest){
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
	for(auto & mode : {awh::crypto_t::mode_t::GCM, awh::crypto_t::mode_t::CFB}){
		// Устанавливаем режим блочного шифрования
		this->_crypto->mode(mode);
		// Выполняем разовое шифрование сообщения
		const std::string oneshot = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
		// Проверяем выполнение разового шифрования
		ASSERT_FALSE(oneshot.empty()) << "mode = " << static_cast <uint16_t> (mode);
		// Выполняем инициализацию контекста расшифровки
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
		// Результат потоковой расшифровки
		std::string streamed;
		// Выполняем расшифровку шифротекста разовой работы потоком
		ASSERT_TRUE(this->_crypto->decrypt <std::string> (oneshot.data(), oneshot.size(), streamed)) << "mode = " << static_cast <uint16_t> (mode);
		// Выполняем завершение потока расшифровки
		ASSERT_TRUE(this->_crypto->finalize(streamed)) << "mode = " << static_cast <uint16_t> (mode);
		// Проверяем разбор потоком шифротекста разовой работы
		EXPECT_EQ(streamed, text) << "mode = " << static_cast <uint16_t> (mode);
		// Выполняем инициализацию контекста шифрования
		ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
		// Результат потокового шифрования
		std::string encoded = this->_crypto->encrypt <std::string> (text);
		// Выполняем завершение потока шифрования
		ASSERT_TRUE(this->_crypto->finalize(encoded)) << "mode = " << static_cast <uint16_t> (mode);
		// Проверяем разбор разовой работой шифротекста потока
		EXPECT_EQ(this->_crypto->decrypt <std::string> (encoded, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), text) << "mode = " << static_cast <uint16_t> (mode);
	}
}

/**
 * @brief Тест обратимости на размерах вокруг границ блока шифра
 *
 * @details Размеры, кратные блоку шифра и соседние с ними, ловят ошибки отведения
 *          выходного буфера и укорочения его до настоящей длины выхода. Перебор
 *          идёт по обоим режимам, всем разрядностям и обоим путям - разовому и
 *          потоковому
 *
 */
TEST_F(CryptoFixture, BoundarySizesCryptoTest){
	// Набор проверяемых размеров сообщения
	const size_t sizes[] = {0, 1, 2, 3, 15, 16, 17, 31, 32, 33, 63, 64, 65, 4095, 4096, 4097};
	// Набор проверяемых разрядностей шифрования
	const awh::crypto_t::cipher_t ciphers[] = {
		awh::crypto_t::cipher_t::AES128,
		awh::crypto_t::cipher_t::AES192,
		awh::crypto_t::cipher_t::AES256
	};
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	/**
	 * Выполняем перебор режимов блочного шифрования
	 */
	for(auto & mode : {awh::crypto_t::mode_t::GCM, awh::crypto_t::mode_t::CFB}){
		// Устанавливаем режим блочного шифрования
		this->_crypto->mode(mode);
		/**
		 * Выполняем перебор разрядностей шифрования
		 */
		for(auto & cipher : ciphers){
			/**
			 * Выполняем перебор размеров сообщения
			 */
			for(auto & size : sizes){
				// Собираем сообщение проверяемого размера
				const std::string text(size, 'A');
				// Выполняем разовое шифрование сообщения
				const std::string oneshot = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, cipher);
				// Проверяем обратимость разовой работы
				EXPECT_EQ(this->_crypto->decrypt <std::string> (oneshot, awh::crypto_t::hash_t::SHA256, cipher), text)
					<< "mode = " << static_cast <uint16_t> (mode) << ", cipher = " << static_cast <uint16_t> (cipher) << ", size = " << size;
				// Выполняем инициализацию контекста шифрования
				ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, cipher));
				// Результат потокового шифрования
				std::string encoded = this->_crypto->encrypt <std::string> (text);
				// Выполняем завершение потока шифрования
				ASSERT_TRUE(this->_crypto->finalize(encoded));
				// Выполняем инициализацию контекста расшифровки
				ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, cipher));
				// Результат потоковой расшифровки
				std::string decoded = this->_crypto->decrypt <std::string> (encoded);
				// Выполняем завершение потока расшифровки
				ASSERT_TRUE(this->_crypto->finalize(decoded));
				// Проверяем обратимость потоковой работы
				EXPECT_EQ(decoded, text)
					<< "mode = " << static_cast <uint16_t> (mode) << ", cipher = " << static_cast <uint16_t> (cipher) << ", size = " << size;
				// Проверяем разбор разовой работой шифротекста потока
				EXPECT_EQ(this->_crypto->decrypt <std::string> (encoded, awh::crypto_t::hash_t::SHA256, cipher), text)
					<< "mode = " << static_cast <uint16_t> (mode) << ", cipher = " << static_cast <uint16_t> (cipher) << ", size = " << size;
			}
		}
	}
}

/**
 * @brief Тест прав файла приватного ключа RSA
 *
 * @details Файл заводился обычным открытием, и права его брались у маски создания:
 *          приватный ключ ложился на диск доступным для чтения всякому в системе
 *
 */
TEST_F(CryptoFixture, KeyFileRightsCryptoTest){
	// Путь к файлу приватного ключа
	const std::string path = "./rights_private_key.pem";
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Выполняем выписывание приватного ключа RSA в файл
	ASSERT_TRUE(this->_crypto->savePrivateKeyRSA(path));
	// Приметы файла приватного ключа
	struct stat attributes;
	// Проверяем что приметы файла сняты
	ASSERT_EQ(::stat(path.c_str(), &attributes), 0);
	// Проверяем что права файла даны одному лишь владельцу
	EXPECT_EQ(static_cast <uint32_t> (attributes.st_mode & 0777), static_cast <uint32_t> (0600));
	// Удаляем файл приватного ключа
	::remove(path.c_str());
	/**
	 * Права, поданные открытию, берутся им лишь при заведении нового файла:
	 * перезапись прежнего усекает содержимое, а права оставляет как есть
	 */
	// Открываем файл на запись прежде выписывания ключа
	std::ofstream file(path, std::ios::binary);
	// Проверяем что файл открыт
	ASSERT_TRUE(file.is_open());
	// Выписываем в файл нечто, ключом не являющееся
	file << "anyks";
	// Закрываем файл
	file.close();
	// Даём файлу права на чтение всякому в системе
	ASSERT_EQ(::chmod(path.c_str(), 0644), 0);
	// Выполняем выписывание приватного ключа RSA поверх прежнего файла
	ASSERT_TRUE(this->_crypto->savePrivateKeyRSA(path));
	// Проверяем что приметы перезаписанного файла сняты
	ASSERT_EQ(::stat(path.c_str(), &attributes), 0);
	// Проверяем что права перезаписанного файла даны одному лишь владельцу
	EXPECT_EQ(static_cast <uint32_t> (attributes.st_mode & 0777), static_cast <uint32_t> (0600));
	// Удаляем файл приватного ключа
	::remove(path.c_str());
}

/**
 * @brief Тест единственности записи отказа схемы дополнения подписи
 *
 * @details Проверка подписи ступени заведения разносит порознь (5.25), а выработка
 *          сводила их в одно условие: отказ схемы дополнения писал свою причину и
 *          общую «Digest signature init failed» следом, подменяя названное
 *
 */
TEST_F(CryptoFixture, SignPaddingRecordCryptoTest){
	// Сообщение подписи
	const std::vector <uint8_t> text = {0x41, 0x4E, 0x59, 0x4B, 0x53};
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Устанавливаем схему дополнения подписи незаданной
	this->_crypto->padding(awh::crypto_t::padding_t::NONE);
	// Количество записей отказа, полученных из лога
	size_t records = 0;
	// Подписываемся на получение логов
	this->_log->subscribe([&records](const awh::log_t::flag_t flag, std::string_view text) noexcept -> void {
		// Снимаем предупреждения о неиспользуемых параметрах
		(void) flag;
		(void) text;
		// Наращиваем количество полученных записей
		records++;
	});
	// Устанавливаем отложенный режим логов, консоль набора не засоряя
	this->_log->mode({awh::log_t::mode_t::DEFERRED});
	// Буфер подписи
	std::vector <uint8_t> signature;
	// Проверяем отказ выработки подписи при незаданной схеме дополнения
	EXPECT_FALSE(this->_crypto->signWithPrivateKey(text, awh::crypto_t::hash_t::SHA256, signature));
	// Проверяем, что отказ записан в лог единожды
	EXPECT_EQ(records, static_cast <size_t> (1));
	// Снимаем режимы логов
	this->_log->mode({awh::log_t::mode_t::NONE});
}

/**
 * @brief Тест выдачи хэш-суммы и имитовставки двоичным видом
 *
 * @details Вид записи выбора не имел: итог всегда выписывался шестнадцатеричной
 *          записью, и двоичный буфер получал не саму сумму, а её запись знаками
 *          ASCII. Подпись сообщений по RFC 9421 кодирует BASE64 саму имитовставку,
 *          и подпись выходила чужим работам не принимаемой. Сличается с эталоном
 *          RFC 4231 - имитовставкой SHA-256 на ключе «key» и известном сообщении
 *
 */
TEST_F(CryptoFixture, RawFormatCryptoTest){
	// Сообщение эталона
	const std::string text = "The quick brown fox jumps over the lazy dog";
	// Ключ подписи эталона
	const std::string key = "key";
	// Шестнадцатеричная запись эталонной имитовставки
	const std::string expected = "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8";
	// Проверяем, что по умолчанию выдаётся шестнадцатеричная запись эталона
	EXPECT_EQ(this->_crypto->hmac <std::string> (key, text, awh::crypto_t::hash_t::SHA256), expected);
	// Получаем имитовставку двоичным видом
	const std::vector <uint8_t> digest = this->_crypto->hmac <std::vector <uint8_t>> (key, text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::format_t::RAW);
	// Проверяем, что двоичный вид вдвое короче шестнадцатеричной записи
	ASSERT_EQ(digest.size(), static_cast <size_t> (32));
	// Буфер шестнадцатеричной записи полученного двоичного вида
	std::string actual = "";
	/**
	 * Выполняем перебор всех октетов двоичного вида
	 */
	for(size_t i = 0; i < digest.size(); i++){
		// Буфер записи одного октета
		char octet[3] = {0};
		// Формируем шестнадцатеричную запись октета
		::snprintf(octet, sizeof(octet), "%02x", digest.at(i));
		// Дописываем запись октета в буфер
		actual.append(octet);
	}
	// Проверяем, что двоичный вид отвечает эталону
	EXPECT_EQ(actual, expected);
	// Проверяем, что двоичным видом выдаётся и хэш-сумма без ключа
	EXPECT_EQ(this->_crypto->hash <std::vector <uint8_t>> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::format_t::RAW).size(), static_cast <size_t> (32));
	// Проверяем, что по умолчанию хэш-сумма выдаётся шестнадцатеричной записью
	EXPECT_EQ(this->_crypto->hash <std::vector <uint8_t>> (text, awh::crypto_t::hash_t::SHA256).size(), static_cast <size_t> (64));
	/**
	 * Выбор вида записи шёл сличением с одной лишь шестнадцатеричной, и значение,
	 * ни одному из видов не отвечающее, молча выдавалось бы двоичным
	 */
	// Проверяем отказ хэширования видом записи, разбору не знакомым
	EXPECT_TRUE(this->_crypto->hash <std::string> (text, awh::crypto_t::hash_t::SHA256, static_cast <awh::crypto_t::format_t> (0xFE)).empty());
	// Проверяем отказ выработки имитовставки видом записи, разбору не знакомым
	EXPECT_TRUE(this->_crypto->hmac <std::string> (key, text, awh::crypto_t::hash_t::SHA256, static_cast <awh::crypto_t::format_t> (0xFE)).empty());
}

/**
 * @brief Тест отказа шифрования ключом RSA, под дополнение слишком коротким
 *
 * @details Ввод ключа со стороны разрядность его не проверяет - в отличие от генерации,
 *          отвергающей ключ короче двух тысяч разрядов. Предел сообщения считается
 *          вычитанием дополнения из длины ключа, и у ключа короче шестидесяти шести
 *          октетов разность, считаемая беззнаковой, обращалась в число огромное: предел
 *          переставал отвергать что бы то ни было, и отказ приходил из глубины
 *          библиотеки. Тест закрепляет отказ на коротком ключе и работоспособность
 *          объекта после него
 *
 */
TEST_F(CryptoFixture, KeyShortCryptoTest){
	// Буфер данных для работы
	const std::vector <uint8_t> data = {0x41, 0x4E, 0x59, 0x4B, 0x53};
	// Буфер шифротекста
	std::vector <uint8_t> sealed;
	// Открытый ключ RSA разрядностью в пятьсот двенадцать разрядов
	const std::string key =
		"-----BEGIN PUBLIC KEY-----\n"
		"MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAMk9wBK+qvVfzPAltyHTRHEA6wTTSdr0\n"
		"qFG+d9RE7cFSql5IEG0uAaDoROuEOasosU4lnCOztIG00lEXltpY48MCAwEAAQ==\n"
		"-----END PUBLIC KEY-----\n";
	// Проверяем принятие ключа RSA, разрядность которого не проверяется
	ASSERT_TRUE(this->_crypto->setPublicKeyRSA(key));
	// Проверяем отказ шифрования ключом RSA, под дополнение слишком коротким
	EXPECT_FALSE(this->_crypto->encryptWithPublicKey(data, sealed));
	// Проверяем пустоту шифротекста при отказе шифрования
	EXPECT_TRUE(sealed.empty());
	/**
	 * Отказ объект не портит: ключ годной разрядности принимается следом и работает
	 */
	// Выполняем генерацию приватного ключа RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
	// Проверяем признак работы при шифровании ключом RSA годной разрядности
	ASSERT_TRUE(this->_crypto->encryptWithPublicKey(data, sealed));
	// Проверяем наличие шифротекста
	EXPECT_FALSE(sealed.empty());
}

/**
 * @brief Тест зависимости шифротекста от количества итераций вывода ключа
 *
 * @details Ключ выводится из пароля и соли за заданное число итераций, и от него
 *          зависит наравне с ними. Число итераций хранится в стейте и входит в условие
 *          перевывода ключа, чтобы условие судило обо всех приметах вывода, а не о части
 *          из них. Тест закрепляет, что смена числа итераций шифротекст меняет
 *
 */
TEST_F(CryptoFixture, RoundsRederiveCryptoTest){
	// Буфер данных для работы
	const std::string data = "ANYKS";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем режим блочного шифрования без проверки подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::CFB);
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Выполняем шифрование данных на первом количестве итераций
	const std::string first = this->_crypto->encrypt <std::string> (data, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем наличие шифротекста
	ASSERT_FALSE(first.empty());
	// Проверяем обратимость шифрования на первом количестве итераций
	ASSERT_EQ(this->_crypto->decrypt <std::string> (first, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), data);
	// Устанавливаем другое количество итераций вывода ключа
	this->_crypto->roundAES(2000);
	// Выполняем шифрование данных на втором количестве итераций
	const std::string second = this->_crypto->encrypt <std::string> (data, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем наличие шифротекста
	ASSERT_FALSE(second.empty());
	// Проверяем обратимость шифрования на втором количестве итераций
	ASSERT_EQ(this->_crypto->decrypt <std::string> (second, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), data);
	/**
	 * Вектор инициализации у каждого сообщения свой, и сличать шифротексты напрямую
	 * нельзя: разойдутся они и при одном ключе. Судить о перевыводе ключа приходится
	 * расшифровкой чужим числом итераций - она обязана дать не тот открытый текст
	 */
	// Возвращаем первое количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	// Проверяем расхождение открытого текста при чужом количестве итераций
	EXPECT_NE(this->_crypto->decrypt <std::string> (second, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), data);
}

/**
 * @brief Тест снятия очереди ошибок библиотеки криптографии
 *
 * @details Библиотека криптографии складывает причины отказов в очередь, принадлежащую
 *          потоку, и сама её не опорожняет. Модуль очередь не читал вовсе, и оставленные
 *          им причины доставались соседнему коду - работа с защищённым соединением
 *          очередь как раз вычитывает и выдаёт в лог. Тест закрепляет, что после всякого
 *          отказа очередь остаётся пустой
 *
 */
TEST_F(CryptoFixture, ErrorQueueCryptoTest){
	// Буфер данных для работы
	const std::vector <uint8_t> data = {0x41, 0x4E, 0x59, 0x4B, 0x53};
	/**
	 * Отказ шифрования ключом RSA, под дополнение слишком коротким
	 */
	{
		// Открытый ключ RSA разрядностью в пятьсот двенадцать разрядов
		const std::string key =
			"-----BEGIN PUBLIC KEY-----\n"
			"MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAMk9wBK+qvVfzPAltyHTRHEA6wTTSdr0\n"
			"qFG+d9RE7cFSql5IEG0uAaDoROuEOasosU4lnCOztIG00lEXltpY48MCAwEAAQ==\n"
			"-----END PUBLIC KEY-----\n";
		// Буфер шифротекста
		std::vector <uint8_t> sealed;
		// Выполняем ввод ключа RSA негодной разрядности
		ASSERT_TRUE(this->_crypto->setPublicKeyRSA(key));
		// Проверяем отказ шифрования ключом RSA
		ASSERT_FALSE(this->_crypto->encryptWithPublicKey(data, sealed));
		// Проверяем пустоту очереди ошибок после отказа
		EXPECT_EQ(::ERR_get_error(), 0UL);
	}
	/**
	 * Отказ ввода ключа RSA, записью PEM не являющегося
	 */
	{
		// Проверяем отказ ввода ключа RSA
		ASSERT_FALSE(this->_crypto->setPublicKeyRSA("это не ключ вовсе"));
		// Проверяем пустоту очереди ошибок после отказа
		EXPECT_EQ(::ERR_get_error(), 0UL);
	}
	/**
	 * Отказ расшифровки поддельного шифротекста
	 */
	{
		// Выполняем генерацию приватного ключа RSA
		ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA(2048));
		// Буфер шифротекста
		std::vector <uint8_t> sealed;
		// Выполняем шифрование данных ключом RSA
		ASSERT_TRUE(this->_crypto->encryptWithPublicKey(data, sealed));
		// Выполняем подделку октета шифротекста
		sealed[sealed.size() / 2] = static_cast <uint8_t> (sealed[sealed.size() / 2] ^ 0x01);
		// Буфер открытого текста
		std::vector <uint8_t> opened;
		// Проверяем отказ расшифровки поддельного шифротекста
		ASSERT_FALSE(this->_crypto->decryptWithPrivateKey(sealed, opened));
		// Проверяем пустоту очереди ошибок после отказа
		EXPECT_EQ(::ERR_get_error(), 0UL);
	}
	/**
	 * Отказ расшифровки поддельного шифротекста режима с проверкой подлинности
	 */
	{
		// Устанавливаем пароль шифрования
		this->_crypto->password("password");
		// Устанавливаем соль шифрования
		this->_crypto->salt("salt");
		// Устанавливаем режим блочного шифрования с проверкой подлинности
		this->_crypto->mode(awh::crypto_t::mode_t::GCM);
		// Выполняем шифрование текста
		std::string sealed = this->_crypto->encrypt <std::string> (std::string("ANYKS"), awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
		// Проверяем наличие шифротекста
		ASSERT_FALSE(sealed.empty());
		// Выполняем подделку октета шифротекста
		sealed[sealed.size() / 2] = static_cast <char> (sealed[sealed.size() / 2] ^ 0x01);
		// Проверяем отказ расшифровки поддельного шифротекста
		ASSERT_TRUE(this->_crypto->decrypt <std::string> (sealed, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
		// Проверяем пустоту очереди ошибок после отказа
		EXPECT_EQ(::ERR_get_error(), 0UL);
	}
}

/**
 * @brief Тест удержания выведенного ключа при повторном заведении потока
 *
 * @details Заведение потока сбрасывало стейт целиком и выводило ключ заново, а вывод
 *          ключа стоит ста тысяч итераций - на замере 6.4 мс против сотых долей у
 *          самого шифрования. Теперь ключ, выведенный теми же приметами, удерживается,
 *          и сбрасываются приметы одного лишь потока. Тест закрепляет, что удержание
 *          работы не портит: каждый поток обратим, вектор инициализации у каждого свой,
 *          а смена приметы вывода ключ выводит заново
 *
 */
TEST_F(CryptoFixture, StreamKeyReuseCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!! Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	/**
	 * @brief Замыкание прогона потока шифрования и расшифровки
	 *
	 * @param hash тип хэш-суммы вывода ключа
	 * @return     шифротекст прогона
	 *
	 */
	auto stream = [&](const awh::crypto_t::hash_t hash) -> std::string {
		// Зашифрованный текст
		std::string encoded;
		// Выполняем инициализацию контекста шифрования
		EXPECT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, hash, awh::crypto_t::cipher_t::AES256));
		/**
		 * Выполняем передачу текста в потоковое шифрование порциями
		 */
		for(size_t offset = 0; offset < text.size(); offset += 16)
			// Выполняем шифрование очередной порции текста
			encoded.append(this->_crypto->encrypt <std::string> (text.data() + offset, ((text.size() - offset) < 16 ? (text.size() - offset) : 16), hash, awh::crypto_t::cipher_t::AES256));
		// Выполняем завершение потокового шифрования
		EXPECT_TRUE(this->_crypto->finalize(encoded));
		// Расшифрованный текст
		std::string decoded;
		// Выполняем инициализацию контекста расшифровки
		EXPECT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, hash, awh::crypto_t::cipher_t::AES256));
		/**
		 * Выполняем передачу зашифрованного текста в потоковую расшифровку порциями
		 */
		for(size_t offset = 0; offset < encoded.size(); offset += 16)
			// Выполняем расшифровку очередной порции текста
			decoded.append(this->_crypto->decrypt <std::string> (encoded.data() + offset, ((encoded.size() - offset) < 16 ? (encoded.size() - offset) : 16), hash, awh::crypto_t::cipher_t::AES256));
		// Выполняем завершение потоковой расшифровки
		EXPECT_TRUE(this->_crypto->finalize(decoded));
		// Проверяем обратимость потокового шифрования
		EXPECT_EQ(decoded, text);
		// Выводим шифротекст прогона
		return encoded;
	};
	// Набор шифротекстов прогонов на удержанном ключе
	std::unordered_set <std::string> results;
	/**
	 * Выполняем перебор прогонов потока на одних и тех же приметах вывода ключа
	 */
	for(uint32_t i = 0; i < 16; i++){
		// Выполняем прогон потока
		const std::string encoded = stream(awh::crypto_t::hash_t::SHA256);
		// Проверяем наличие шифротекста
		ASSERT_FALSE(encoded.empty());
		/**
		 * Вектор инициализации берётся случайным на каждый поток, и удержание ключа
		 * этого менять не должно: одинаковый шифротекст означал бы, что вектор достался
		 * потоку от предыдущего
		 */
		// Проверяем неповторимость шифротекста прогона
		ASSERT_TRUE(results.emplace(encoded).second) << i;
	}
	/**
	 * Смена приметы вывода ключ выводит заново, и работа от этого не страдает
	 */
	// Выполняем прогон потока на другой хэш-сумме вывода ключа
	ASSERT_FALSE(stream(awh::crypto_t::hash_t::SHA512).empty());
	// Выполняем прогон потока на прежней хэш-сумме вывода ключа
	ASSERT_FALSE(stream(awh::crypto_t::hash_t::SHA256).empty());
}

/**
 * @brief Тест сверки частично заданных доводов вызова с заведённым потоком
 *
 * @details Незаданные доводы вызова берутся из потока, а заданные с ним сверяются (4.4).
 *          Прежде сверка велась лишь тогда, когда задан был тип шифрования: вызов с иной
 *          хэш-суммой и незаданным шифром признавался работой поверх потока, и хэш-сумма
 *          его молча отбрасывалась - работа думала, что вывела ключ одной хэш-суммой, а
 *          вывела другой. Тест закрепляет отказ на всяком заданном доводе, с потоком
 *          расходящемся
 *
 */
TEST_F(CryptoFixture, StreamPartialMismatchCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	/**
	 * @brief Замыкание прогона вызова поверх заведённого потока
	 *
	 * @param hash   тип хэш-суммы довода вызова
	 * @param cipher тип шифрования довода вызова
	 * @return       шифротекст прогона
	 *
	 */
	auto attempt = [&](const awh::crypto_t::hash_t hash, const awh::crypto_t::cipher_t cipher) -> std::string {
		// Выполняем инициализацию контекста шифрования
		EXPECT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
		// Выводим шифротекст прогона
		return this->_crypto->encrypt <std::string> (text.data(), text.size(), hash, cipher);
	};
	/**
	 * Оба довода незаданы - работа идёт приметами потока
	 */
	// Проверяем работу поверх потока при незаданных доводах
	EXPECT_FALSE(attempt(awh::crypto_t::hash_t::NONE, awh::crypto_t::cipher_t::NONE).empty());
	/**
	 * Оба довода заданы и с потоком сходятся - работа идёт
	 */
	// Проверяем работу поверх потока при сходящихся доводах
	EXPECT_FALSE(attempt(awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256).empty());
	/**
	 * Задан один лишь тип шифрования, и он с потоком сходится
	 */
	// Проверяем работу поверх потока при сходящемся типе шифрования
	EXPECT_FALSE(attempt(awh::crypto_t::hash_t::NONE, awh::crypto_t::cipher_t::AES256).empty());
	/**
	 * Задана одна лишь хэш-сумма, и она с потоком сходится
	 */
	// Проверяем работу поверх потока при сходящейся хэш-сумме
	EXPECT_FALSE(attempt(awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::NONE).empty());
	/**
	 * Оба довода заданы, и хэш-сумма с потоком расходится
	 */
	// Проверяем отказ работы при расходящейся хэш-сумме и заданном шифре
	EXPECT_TRUE(attempt(awh::crypto_t::hash_t::SHA512, awh::crypto_t::cipher_t::AES256).empty());
	/**
	 * Задан один лишь тип шифрования, и он с потоком расходится
	 */
	// Проверяем отказ работы при расходящемся типе шифрования
	EXPECT_TRUE(attempt(awh::crypto_t::hash_t::NONE, awh::crypto_t::cipher_t::AES128).empty());
	/**
	 * Задана одна лишь хэш-сумма, и она с потоком расходится: прежде этот вызов
	 * молча работал прежней хэш-суммой
	 */
	// Проверяем отказ работы при расходящейся хэш-сумме и незаданном шифре
	EXPECT_TRUE(attempt(awh::crypto_t::hash_t::SHA512, awh::crypto_t::cipher_t::NONE).empty());
}

/**
 * @brief Тест порядка действий при смене пароля и соли вывода ключа
 *
 * @details Новое значение собирается прежде правки объекта: присвоение отводит память и
 *          потому способно отказать, а прежний порядок к поре отказа уже гасил прежнее
 *          значение и стейта не сбрасывал - объект оставался с нулями в поле и с прежним
 *          выведенным ключом в стейте. Тест закрепляет, что переставленный порядок работы
 *          не изменил: смена всякой приметы вывода ключ перевыводит, а шифротексты
 *          сходятся лишь при тех же приметах
 *
 */
TEST_F(CryptoFixture, PasswordOrderCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем режим блочного шифрования гаммированием
	this->_crypto->mode(awh::crypto_t::mode_t::CFB);
	// Устанавливаем количество итераций вывода ключа
	this->_crypto->roundAES(1000);
	/**
	 * @brief Замыкание прогона шифрования на заданных приметах вывода ключа
	 *
	 * @param password пароль шифрования
	 * @param salt     соль вывода ключа
	 * @return         шифротекст прогона
	 *
	 */
	auto seal = [&](const std::string & password, const std::string & salt) -> std::string {
		// Устанавливаем пароль шифрования
		this->_crypto->password(password);
		// Устанавливаем соль вывода ключа
		this->_crypto->salt(salt);
		// Выводим шифротекст прогона
		return this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	};
	/**
	 * @brief Замыкание расшифровки на заданных приметах вывода ключа
	 *
	 * @param sealed   шифротекст для расшифровки
	 * @param password пароль шифрования
	 * @param salt     соль вывода ключа
	 * @return         открытый текст расшифровки
	 *
	 */
	auto open = [&](const std::string & sealed, const std::string & password, const std::string & salt) -> std::string {
		// Устанавливаем пароль шифрования
		this->_crypto->password(password);
		// Устанавливаем соль вывода ключа
		this->_crypto->salt(salt);
		// Выводим открытый текст расшифровки
		return this->_crypto->decrypt <std::string> (sealed, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	};
	/**
	 * Короткое значение лежит внутри самого объекта строки, длинное - в отведённой
	 * памяти: порядок действий обязан держаться на обоих
	 */
	// Перечень примет вывода ключа обеих длин
	const struct {
		const char * password;
		const char * salt;
	} records[] = {
		{"short", "salt"},
		{"a password long enough to leave the string object itself", "a salt long enough to leave the string object itself"}
	};
	// Выполняем перебор всех примет вывода ключа
	for(auto & item : records){
		// Выполняем шифрование на заданных приметах вывода ключа
		const std::string sealed = seal(item.password, item.salt);
		// Проверяем наличие шифротекста
		ASSERT_FALSE(sealed.empty()) << item.password;
		/**
		 * Смена приметы и возврат её обратно ключ перевыводят дважды, и обратимость
		 * от этого страдать не должна
		 */
		// Выполняем смену пароля шифрования
		ASSERT_FALSE(seal("other password", item.salt).empty()) << item.password;
		// Выполняем смену соли вывода ключа
		ASSERT_FALSE(seal(item.password, "other salt").empty()) << item.password;
		// Проверяем обратимость шифрования после возврата прежних примет
		ASSERT_EQ(open(sealed, item.password, item.salt), text) << item.password;
		// Проверяем расхождение открытого текста при чужом пароле
		EXPECT_NE(open(sealed, "other password", item.salt), text) << item.password;
		/**
		 * Соль меняется в одиночку, без правки пароля: установка пароля стейт сбрасывает
		 * сама, и смена обеих примет разом не показала бы, сбрасывает ли его установка соли
		 */
		// Выполняем возврат прежних примет вывода ключа
		ASSERT_EQ(open(sealed, item.password, item.salt), text) << item.password;
		// Выполняем смену одной лишь соли вывода ключа
		this->_crypto->salt("other salt");
		// Проверяем расхождение открытого текста при чужой соли
		EXPECT_NE(this->_crypto->decrypt <std::string> (sealed, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), text) << item.password;
		/**
		 * Пароль меняется в одиночку по той же причине - установка соли стейт сбрасывает сама
		 */
		// Выполняем возврат прежних примет вывода ключа
		ASSERT_EQ(open(sealed, item.password, item.salt), text) << item.password;
		// Выполняем смену одного лишь пароля шифрования
		this->_crypto->password("other password");
		// Проверяем расхождение открытого текста при чужом пароле
		EXPECT_NE(this->_crypto->decrypt <std::string> (sealed, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), text) << item.password;
	}
}

/**
 * @brief Тест отказов, называющих свою причину
 *
 * @details Завершение потока, не заведённого вовсе, и заведение потока кодированием
 *          BASE64 отвергаются с записью причины в лог: буфер завершение не трогает, и
 *          признак остаётся единственной приметой отказа, а кодирование BASE64 прежде
 *          отвергалось в глубине заведения ключа как шифрование неизвестного вида. Тест
 *          закрепляет сами отказы и целость объекта после них
 *
 */
TEST_F(CryptoFixture, RefusalReasonCryptoTest){
	// Текст для шифрования
	const std::string text = "Anyks Framework, Hello World!!!";
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль вывода ключа
	this->_crypto->salt("salt");
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	/**
	 * Завершение потока, не заведённого вовсе, буфер не трогает
	 */
	// Буфер завершения потока
	std::string buffer = "prefix";
	// Проверяем отказ завершения потока, не заведённого вовсе
	EXPECT_FALSE(this->_crypto->finalize(buffer));
	// Проверяем неизменность буфера при отказе завершения
	EXPECT_EQ(buffer, "prefix");
	/**
	 * Кодирование BASE64 потоком не выполняется
	 */
	// Проверяем отказ заведения потока кодированием BASE64
	EXPECT_FALSE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::NONE, awh::crypto_t::cipher_t::BASE64));
	// Проверяем отказ завершения потока после отказа заведения
	EXPECT_FALSE(this->_crypto->finalize(buffer));
	/**
	 * Разовое кодирование BASE64 отказом заведения потока не задето
	 */
	// Выполняем разовое кодирование BASE64
	const std::string encoded = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::NONE, awh::crypto_t::cipher_t::BASE64);
	// Проверяем наличие записи BASE64
	ASSERT_FALSE(encoded.empty());
	// Проверяем обратимость разового кодирования BASE64
	EXPECT_EQ(this->_crypto->decrypt <std::string> (encoded, awh::crypto_t::hash_t::NONE, awh::crypto_t::cipher_t::BASE64), text);
	/**
	 * Заведение потока шифрованием отказами не задето
	 */
	// Проверяем заведение потока шифрования
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Зашифрованный текст
	std::string sealed = this->_crypto->encrypt <std::string> (text.data(), text.size());
	// Проверяем завершение заведённого потока
	ASSERT_TRUE(this->_crypto->finalize(sealed));
	// Проверяем наличие шифротекста
	EXPECT_FALSE(sealed.empty());
}

/**
 * @brief Тест независимости цены завершения потока от прежде принятых порций
 *
 * @details Удерживаемый хвост потока накапливает поданную порцию целиком, и ёмкость
 *          его берётся от самой крупной порции за всю жизнь объекта. Гашение хвоста
 *          идёт по всей ёмкости, а опустошение ёмкости не отпускало: объект, единожды
 *          принявший крупную порцию, платил за всякое следующее завершение потока
 *          гашением памяти той порции - на замере короткий оборот дорожал в тысячу
 *          раз. Цена завершения обязана отвечать поданному потоку, а не памяти
 *          прежних потоков
 *
 */
TEST_F(CryptoFixture, TailCapacityCryptoTest){
	// Размер крупной порции подачи в октетах
	static constexpr size_t CHUNK = (64 * 1024);
	// Количество коротких оборотов замера
	static constexpr size_t ROUNDS = 500;
	// Данные короткого оборота
	const std::string text = "Anyks Framework, Hello World!!!";
	/**
	 * @brief Функция заведения объекта шифрования
	 *
	 * @param crypto объект шифрования
	 *
	 */
	auto prepare = [](awh::crypto_t & crypto) noexcept -> void {
		// Устанавливаем пароль шифрования
		crypto.password("password");
		// Устанавливаем соль шифрования
		crypto.salt("salt");
		// Устанавливаем количество итераций вывода ключа
		crypto.roundAES(1000);
		// Устанавливаем режим блочного шифрования с проверкой подлинности
		crypto.mode(awh::crypto_t::mode_t::GCM);
	};
	/**
	 * @brief Функция замера коротких оборотов потока
	 *
	 * @param crypto объект шифрования
	 * @return       затраченное на обороты время в секундах
	 *
	 */
	auto cycling = [&](awh::crypto_t & crypto) noexcept -> double {
		// Запоминаем момент начала замера
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем требуемое количество коротких оборотов потока
		 */
		for(size_t i = 0; i < ROUNDS; i++){
			// Шифротекст короткого оборота
			std::string sealed;
			// Выполняем заведение потока шифрования
			crypto.initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
			// Выполняем шифрование данных короткого оборота
			sealed.append(crypto.encrypt <std::string> (text));
			// Выполняем завершение потока шифрования
			crypto.finalize(sealed);
		}
		// Выводим затраченное на обороты время
		return std::chrono::duration <double> (std::chrono::steady_clock::now() - start).count();
	};
	// Объект шифрования, крупных порций не принимавший
	awh::crypto_t clean(this->_fmk.get(), this->_log.get());
	// Выполняем заведение объекта шифрования, крупных порций не принимавшего
	prepare(clean);
	// Выполняем заведение объекта шифрования, крупную порцию принимающего
	prepare(* this->_crypto);
	// Данные крупного потока
	const std::string bulk(CHUNK * 4, 'A');
	// Шифротекст крупного потока
	std::string sealed;
	// Выполняем заведение потока шифрования крупного потока
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем шифрование данных крупного потока
	sealed.append(this->_crypto->encrypt <std::string> (bulk));
	// Выполняем завершение потока шифрования крупного потока
	ASSERT_TRUE(this->_crypto->finalize(sealed));
	// Открытый текст крупного потока
	std::string opened;
	// Выполняем заведение потока расшифровки крупного потока
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	/**
	 * Выполняем подачу шифротекста крупными порциями
	 */
	for(size_t offset = 0; offset < sealed.size(); offset += CHUNK)
		// Выполняем расшифровку очередной крупной порции потока
		opened.append(this->_crypto->decrypt <std::string> (sealed.data() + offset, ((sealed.size() - offset) < CHUNK ? (sealed.size() - offset) : CHUNK)));
	// Выполняем завершение потока расшифровки крупного потока
	ASSERT_TRUE(this->_crypto->finalize(opened));
	// Проверяем расшифровку крупного потока
	ASSERT_EQ(opened, bulk);
	// Выполняем прогрев обоих объектов шифрования
	cycling(clean);
	// Выполняем прогрев объекта шифрования, крупную порцию принявшего
	cycling(* this->_crypto);
	// Выполняем замер коротких оборотов объекта, крупных порций не принимавшего
	const double light = cycling(clean);
	// Выполняем замер коротких оборотов объекта, крупную порцию принявшего
	const double heavy = cycling(* this->_crypto);
	/**
	 * Запас взят с большим избытком: замер идёт под общей нагрузкой набора, и
	 * ловить он должен зависимость от прежних порций, а не колебания планировщика.
	 * Дефект давал разницу в три порядка
	 */
	// Проверяем независимость цены короткого оборота от прежде принятых порций
	EXPECT_LT(heavy, light * 20.0) << "коротких оборотов: " << ROUNDS << ", после крупной порции: " << heavy << " с, без неё: " << light << " с";
}
