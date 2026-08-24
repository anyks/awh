/**
 * @file parameterized.cpp
 * @date 2026-01-21
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
 * @brief Параметризованные тесты модуля криптографии — прогон подготовленных наборов входных данных через методы
 *        модуля с проверкой симметричного шифрования и расшифровки данных, вычисления хешей и кодирования в Base64
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "crypto.hpp"

/**
 * @brief Параметры теста выполнения хэширования строки в число
 *
 */
struct HashNumberTestParameter {
	// Текст для хэширования
	std::string text = "";
	// Результат 32-битного хэша
	uint32_t result1 = 0;
	// Результат 64-битного хэша
	uint64_t result2 = 0;
	// Результат 128-битного хэша
	awh::crypto_t::uint128_t result3 = {0};
};

/**
 * @brief Класс параметризованной тестовой фикстуры для работы с хэшированием в число
 *
 */
class HashNumberTestParameterizedFixture : public CryptoFixture, public ::testing::WithParamInterface <HashNumberTestParameter> {
	public:
		// Параметры теста
		HashNumberTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного создания и проверки работы хэширования в число
 *
 */
TEST_P(HashNumberTestParameterizedFixture, HashNumberTest){
	// Проверяем результат 32-битного хэширования
	ASSERT_EQ(this->_parameter.result1, this->_crypto->hash <uint32_t> (this->_parameter.text));
	// Проверяем результат 64-битного хэширования
	ASSERT_EQ(this->_parameter.result2, this->_crypto->hash <uint64_t> (this->_parameter.text));
	// Проверяем результат 128-битного хэширования
	ASSERT_EQ(this->_parameter.result3, this->_crypto->hash <awh::crypto_t::uint128_t> (this->_parameter.text));
}

/**
 * @brief Инициализация параметров теста работы хэширования в число
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, HashNumberTestParameterizedFixture,
	::testing::Values(
		HashNumberTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			540735289,
			17155673404674340665ULL,
			{0x39, 0xF7, 0x3A, 0x20, 0x11, 0x32, 0x15, 0xEE, 0xE4, 0x5D, 0xF2, 0x55, 0x98, 0xF9, 0x57, 0x78}
		}),
		HashNumberTestParameter({
			"Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework!!!!!!!!!!!!!!!!?",
			2298495141,
			7532577026855682213ULL,
			{0xA5, 0x40, 0x00, 0x89, 0xE5, 0x16, 0x89, 0x68, 0x9A, 0x76, 0x6B, 0xA7, 0x90, 0xC3, 0xFF, 0xC8}
		})
	)
);

/**
 * @brief Параметры теста выполнения хэширования строки в строку
 *
 */
struct HashStringTestParameter {
	// Текст для проверки
	std::string text = "";
	// Результат хэширования
	std::string result1 = "";
	// Результат хэширования HMAC
	std::string result2 = "";
	// Тип алгоритма хэширования
	awh::crypto_t::hash_t type = awh::crypto_t::hash_t::NONE;
};

/**
 * @brief Класс параметризованной тестовой ф1икстуры для работы с хэшированием в строку
 *
 */
class HashStringTestParameterizedFixture : public CryptoFixture, public ::testing::WithParamInterface <HashStringTestParameter> {
	public:
		// Параметры теста
		HashStringTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения хэширования строки в строку
 *
 */
TEST_P(HashStringTestParameterizedFixture, HashStringTest){
	// Проверяем результат выполнения хэширования
	ASSERT_EQ(this->_parameter.result1, this->_crypto->hash <std::string> (this->_parameter.text, this->_parameter.type));
	// Проверяем результат выполнения хэширования HMAC
	ASSERT_EQ(this->_parameter.result2, this->_crypto->hmac <std::string> (this->_parameter.result1, this->_parameter.text, this->_parameter.type));
}

/**
 * @brief Инициализация параметров теста работы хэширования в строку
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, HashStringTestParameterizedFixture,
	::testing::Values(
		HashStringTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"7039bdd0df3e5cec78934c32ab931c91",
			"9f10e6df14d6e2c3776a03b465663a0c",
			awh::crypto_t::hash_t::MD5
		}),
		HashStringTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"adaccf563788bc09516f3b13c5fb969dee97ed41",
			"d5db41118255ea7a4db2b941b321d75bf3acbd88",
			awh::crypto_t::hash_t::SHA1
		}),
		HashStringTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"e919819b0c7386e38e040c7bbd876bcbe35de8c4364179b28f4ebb95",
			"c6cfa6a7ca15f589fcae4a213a8998b581fe3a2759cfa51cf36ba102",
			awh::crypto_t::hash_t::SHA224
		}),
		HashStringTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"3a4f9cfad7df5274dc724737d9176cc57787aaa930e0b4e7d82398fd4a82facf",
			"17d253ace96030f0f6d7325cc69a80a0cbf47a6a6421c69f1f78d1c5dc5cbff5",
			awh::crypto_t::hash_t::SHA256
		}),
		HashStringTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"b996ff91f7ef04fe8ea7ee01dd1015d31882daee84054a54ad1e517175ff0c9adab2ce958deac16828268b2f599a96c0",
			"a9700ad0692bca6760d206a1ef8c5c6be6626de37817655b22e02893587ba367558c8d8a69ea20926ac31facbb9a4c72",
			awh::crypto_t::hash_t::SHA384
		}),
		HashStringTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"4eb68fe9cda573be3ef1183b34a776fbc62bbb6e0571632afbbcb292e95e655160d984ded4b8fe9b1a221d9d5250c5c6587fc18556194b95c7159ba8e19c1779",
			"a4eaf14ba267f1af48e3ceda03b9ae2addfcd8fd57b1ed35dd0bca4ecb15836d4beab59902fefc56f5c2054529196fe2e287891e4e18f902812a002090413ede",
			awh::crypto_t::hash_t::SHA512
		})
	)
);

/**
 * @brief Параметры теста выполнения шифрования BASE64
 *
 */
struct Base64TestParameter {
	// Текст для проверки
	std::string text = "";
	// Результат шифрования
	std::string result = "";
};

/**
 * @brief Класс параметризованной тестовой фикстуры для работы с шифрованием BASE64
 *
 */
class Base64TestParameterizedFixture : public CryptoFixture, public ::testing::WithParamInterface <Base64TestParameter> {
	public:
		// Параметры теста
		Base64TestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения шифрованием BASE64
 *
 */
TEST_P(Base64TestParameterizedFixture, Base64Test){
	// Выполняем шифрование текста
	const std::string & encoded = this->_crypto->encrypt <std::string> (this->_parameter.text, awh::crypto_t::hash_t::NONE, awh::crypto_t::cipher_t::BASE64);
	// Проверяем результат выполнения шифрования
	ASSERT_EQ(this->_parameter.result, encoded);
	// Выполняем дешифрование текста
	const std::string & decoded = this->_crypto->decrypt <std::string> (encoded, awh::crypto_t::hash_t::NONE, awh::crypto_t::cipher_t::BASE64);
	// Проверяем результат выполнения дешифрования
	ASSERT_EQ(this->_parameter.text, decoded);
}

/**
 * @brief Инициализация параметров теста работы шифрованием BASE64
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, Base64TestParameterizedFixture,
	::testing::Values(
		Base64TestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"SGVsbG8gV29ybGQsIEhlbGxvIFdvcmxkLCBIZWxsbyBXb3JsZCwgSGVsbG8gV29ybGQsIEhlbGxvIFdvcmxkLCBIZWxsbyBXb3JsZCEhISEhISEhISEhISEhISE/",
		}),
		Base64TestParameter({
			"Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework, Anyks Framework!!!!!!!!!!!!!!!!?",
			"QW55a3MgRnJhbWV3b3JrLCBBbnlrcyBGcmFtZXdvcmssIEFueWtzIEZyYW1ld29yaywgQW55a3MgRnJhbWV3b3JrLCBBbnlrcyBGcmFtZXdvcmssIEFueWtzIEZyYW1ld29yayEhISEhISEhISEhISEhISE/",
		})
	)
);

/**
 * @brief Параметры теста выполнения шифрования с паролем
 *
 */
struct EncodeWithPasswordTestParameter {
	// Количество раундов шифрования
	int32_t round = 0;
	// Текст для проверки
	std::string text = "";
	// Соль шифрования
	std::string salt = "";
	// Пароль шифрования
	std::string password = "";
	// Тип хэш-суммы
	awh::crypto_t::hash_t hash = awh::crypto_t::hash_t::NONE;
	// Тип шифрования
	awh::crypto_t::cipher_t cipher = awh::crypto_t::cipher_t::NONE;
};

/**
 * @brief Класс параметризованной тестовой фикстуры для работы с шифрованием с паролем
 *
 */
class EncodeWithPasswordParameterizedFixture : public CryptoFixture, public ::testing::WithParamInterface <EncodeWithPasswordTestParameter> {
	public:
		// Параметры теста
		EncodeWithPasswordTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения шифрованием с паролем версия 1
 *
 */
TEST_P(EncodeWithPasswordParameterizedFixture, EncodeWithPassword1Test){
	// Устанавливаем количество раундов шифрования
	this->_crypto->roundAES(this->_parameter.round);
	// Устанавливаем соль шифрования
	this->_crypto->salt(this->_parameter.salt);
	// Устанавливаем пароль шифрования
	this->_crypto->password(this->_parameter.password);
	// Выполняем шифрование текста
	const std::vector <uint8_t> & result = this->_crypto->encrypt <std::vector <uint8_t>> (this->_parameter.text, this->_parameter.hash, this->_parameter.cipher);
	// Проверяем результат выполнения шифрования
	ASSERT_FALSE(result.empty());
	// Проверяем результат выполнения дешифрования
	ASSERT_EQ(this->_parameter.text, this->_crypto->decrypt <std::string> (result, this->_parameter.hash, this->_parameter.cipher));
}

/**
 * @brief Тест параметризованного выполнения шифрованием с паролем версия 2
 *
 */
TEST_P(EncodeWithPasswordParameterizedFixture, EncodeWithPassword2Test){
	// Устанавливаем количество раундов шифрования
	this->_crypto->roundAES(this->_parameter.round);
	// Устанавливаем соль шифрования
	this->_crypto->salt(this->_parameter.salt);
	// Устанавливаем пароль шифрования
	this->_crypto->password(this->_parameter.password);
	// Инициализируем объект криптографии для другого типа хэша
	this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, this->_parameter.hash, this->_parameter.cipher);
	// Выполняем шифрование текста
	std::vector <uint8_t> result = this->_crypto->encrypt <std::vector <uint8_t>> (this->_parameter.text);
	// Завершаем процесс шифрования
	ASSERT_TRUE(this->_crypto->finalize(result));
	// Проверяем результат выполнения шифрования
	ASSERT_FALSE(result.empty());
	// Инициализируем объект криптографии для другого типа хэша
	this->_crypto->initialize(awh::crypto_t::event_t::DECODE, this->_parameter.hash, this->_parameter.cipher);
	// Выполняем дешифрование текста
	std::string text = this->_crypto->decrypt <std::string> (result);
	// Завершаем процесс дешифрования
	ASSERT_TRUE(this->_crypto->finalize(text));
	// Проверяем результат выполнения дешифрования
	ASSERT_EQ(text, this->_parameter.text);
}

/**
 * @brief Инициализация параметров теста работы шифрованием с паролем
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, EncodeWithPasswordParameterizedFixture,
	::testing::Values(
		EncodeWithPasswordTestParameter({
			10,
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_salt",
			"anyks_password",
			awh::crypto_t::hash_t::MD5,
			awh::crypto_t::cipher_t::AES128
		}),
		EncodeWithPasswordTestParameter({
			5,
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_salt",
			"anyks_password",
			awh::crypto_t::hash_t::SHA1,
			awh::crypto_t::cipher_t::AES192
		}),
		EncodeWithPasswordTestParameter({
			8,
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_salt",
			"anyks_password",
			awh::crypto_t::hash_t::SHA224,
			awh::crypto_t::cipher_t::AES256
		}),
		EncodeWithPasswordTestParameter({
			18,
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_salt",
			"anyks_password",
			awh::crypto_t::hash_t::SHA256,
			awh::crypto_t::cipher_t::AES128
		}),
		EncodeWithPasswordTestParameter({
			13,
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_salt",
			"anyks_password",
			awh::crypto_t::hash_t::SHA384,
			awh::crypto_t::cipher_t::AES192
		}),
		EncodeWithPasswordTestParameter({
			22,
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_salt",
			"anyks_password",
			awh::crypto_t::hash_t::SHA512,
			awh::crypto_t::cipher_t::AES256
		})
	)
);

/**
 * @brief Параметры теста выполнения шифрования с ключом
 *
 */
struct EncodeWithKeyTestParameter {
	// Текст для проверки
	std::string text = "";
	// Пароль шифрования
	std::string password = "";
	// Тип хэш-суммы
	awh::crypto_t::hash_t hash = awh::crypto_t::hash_t::NONE;
};

/**
 * @brief Класс параметризованной тестовой фикстуры для работы с шифрованием с ключом
 *
 */
class EncodeWithKeyParameterizedFixture : public CryptoFixture, public ::testing::WithParamInterface <EncodeWithKeyTestParameter> {
	public:
		// Параметры теста
		EncodeWithKeyTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения шифрованием с ключом
 *
 */
TEST_P(EncodeWithKeyParameterizedFixture, EncodeWithKeyTest){
	// Устанавливаем пароль шифрования
	this->_crypto->password(this->_parameter.password);
	// Генерируем пару ключей RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA());
	// Сохраняем публичный ключ RSA в файл
	ASSERT_TRUE(this->_crypto->savePublicKeyRSA("public_key.pem"));
	// Сохраняем приватный ключ RSA в файл
	ASSERT_TRUE(this->_crypto->savePrivateKeyRSA("private_key.pem"));
	// Загружаем публичный ключ RSA из файла
	ASSERT_TRUE(this->_crypto->loadPublicKeyRSA("public_key.pem"));
	// Буфер данных для результата шифрования
	std::vector <uint8_t> result;
	// Буфер данных для шифрования
	std::vector <uint8_t> buffer(this->_parameter.text.begin(), this->_parameter.text.end());
	// Шифруем данные публичным ключом RSA
	this->_crypto->encryptWithPublicKey(buffer, result);
	// Проверяем результат выполнения шифрования
	ASSERT_FALSE(result.empty());
	// Загружаем приватный ключ из файла
	ASSERT_TRUE(this->_crypto->loadPrivateKeyRSA("private_key.pem"));
	// Расшифровываем данные приватным ключом RSA
	this->_crypto->decryptWithPrivateKey(result, buffer);
	// Проверяем результат выполнения дешифрования
	ASSERT_FALSE(buffer.empty());
	// Извлекаем результат в строку
	std::string text(buffer.begin(), buffer.end());
	// Проверяем результат выполнения дешифрования
	ASSERT_EQ(text, this->_parameter.text);
}

/**
 * @brief Инициализация параметров теста работы шифрованием с ключом
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, EncodeWithKeyParameterizedFixture,
	::testing::Values(
		EncodeWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::MD5
		}),
		EncodeWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::SHA1
		}),
		EncodeWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::SHA224
		}),
		EncodeWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::SHA256
		}),
		EncodeWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::SHA384
		}),
		EncodeWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::SHA512
		})
	)
);

/**
 * @brief Параметры теста выполнения подписывания с ключом
 *
 */
struct SignWithKeyTestParameter {
	// Текст для проверки
	std::string text = "";
	// Пароль шифрования
	std::string password = "";
	// Тип хэш-суммы
	awh::crypto_t::hash_t hash = awh::crypto_t::hash_t::NONE;
};

/**
 * @brief Класс параметризованной тестовой фикстуры для работы с подписыванием с ключом
 *
 */
class SignWithKeyParameterizedFixture : public CryptoFixture, public ::testing::WithParamInterface <SignWithKeyTestParameter> {
	public:
		// Параметры теста
		SignWithKeyTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения подписывания с ключом
 *
 */
TEST_P(SignWithKeyParameterizedFixture, SignWithKeyTest){
	// Устанавливаем пароль шифрования
	this->_crypto->password(this->_parameter.password);
	// Генерируем пару ключей RSA
	ASSERT_TRUE(this->_crypto->generatePrivateKeyRSA());
	// Получаем приватный ключ RSA
	const std::string privkey = this->_crypto->getPrivateKeyRSA();
	// Проверяем результат получения приватного ключа RSA
	ASSERT_FALSE(privkey.empty());
	// Получаем публичный ключ RSA
	const std::string pubkey = this->_crypto->getPublicKeyRSA();
	// Проверяем результат получения публичного ключа RSA
	ASSERT_FALSE(pubkey.empty());
	// Устанавливаем приватный ключ RSA
	ASSERT_TRUE(this->_crypto->setPrivateKeyRSA(privkey));
	// Буфер данных для подписи
	std::vector <uint8_t> signature;
	// Буфер данных для шифрования
	std::vector <uint8_t> buffer(this->_parameter.text.begin(), this->_parameter.text.end());
	// Подписываем данные приватным ключом RSA
	this->_crypto->signWithPrivateKey(buffer, this->_parameter.hash, signature);
	// Устанавливаем публичный ключ RSA
	ASSERT_TRUE(this->_crypto->setPublicKeyRSA(pubkey));
	// Проверяем подпись публичным ключом RSA
	ASSERT_TRUE(this->_crypto->verifyWithPublicKey(buffer, signature, this->_parameter.hash));
}

/**
 * @brief Инициализация параметров теста работы подписыванием с ключом
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, SignWithKeyParameterizedFixture,
	::testing::Values(
		SignWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::MD5
		}),
		SignWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::SHA1
		}),
		SignWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::SHA224
		}),
		SignWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::SHA256
		}),
		SignWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::SHA384
		}),
		SignWithKeyTestParameter({
			"Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?",
			"anyks_password",
			awh::crypto_t::hash_t::SHA512
		})
	)
);
