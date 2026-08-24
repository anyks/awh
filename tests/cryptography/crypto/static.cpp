/**
 * @file static.cpp
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
 * @brief Статические тесты модуля криптографии — проверка создания и сброса объекта модуля,
 *        а также корректности симметричного шифрования и расшифровки данных, вычисления хешей и кодирования в Base64
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include <random>
#include "crypto.hpp"

/**
 * Заголовочный файл очереди ошибок библиотеки криптографии
 */
#include <openssl/err.h>

/**
 * Стандартный заголовочный файл работы с файлами
 */
#include <chrono>
#include <thread>
#include <atomic>
#include <fstream>

/**
 * Стандартный заголовочный файл примет файла
 */
#include <sys/stat.h>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Заголовочный файл работы со списками управления доступом
	 */
	#include <aclapi.h>
#endif

/**
 * @brief Функция проверки, что файл доступен одному лишь его владельцу
 *
 * @param path путь к проверяемому файлу
 * @return     признак того, что доступ к файлу закрыт для всех прочих
 *
 * @details У систем POSIX проверка эта выражается разрядами прав: доступ владельца
 *          на чтение и запись, и ничего более. У MS Windows разрядов прав нет вовсе,
 *          а `stat` там отвечает `0666` либо `0444` по одному лишь признаку «только
 *          для чтения», ничего о настоящем доступе не сообщая
 *
 *          Поэтому у MS Windows проверяется то, чем доступ там и выражен: список
 *          управления доступом файла. Годным считается список из единственной записи,
 *          выданной тому же пользователю, от имени которого работает проверка
 *
 */
static bool restricted(const std::string & path) noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Список управления доступом файла
		PACL dacl = nullptr;
		// Описатель защиты файла
		PSECURITY_DESCRIPTOR descriptor = nullptr;
		// Если список управления доступом файла снять не удалось
		if(::GetNamedSecurityInfoA(path.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &dacl, nullptr, &descriptor) != ERROR_SUCCESS)
			// Выводим признак отказа
			return false;
		// Признак того, что доступ к файлу закрыт для всех прочих
		bool result = false;
		/**
		 * Выполняем проверку списка управления доступом
		 */
		do {
			// Если список управления доступом у файла отсутствует - доступ открыт всем
			if(dacl == nullptr)
				// Прекращаем проверку
				break;
			// Если список содержит не единственную запись
			if(dacl->AceCount != 1)
				// Прекращаем проверку
				break;
			// Запись списка управления доступом
			LPVOID entry = nullptr;
			// Если запись списка получить не удалось
			if(!::GetAce(dacl, 0, &entry))
				// Прекращаем проверку
				break;
			// Заголовок записи списка управления доступом
			ACE_HEADER * header = reinterpret_cast <ACE_HEADER *> (entry);
			// Если запись дозволением не является
			if(header->AceType != ACCESS_ALLOWED_ACE_TYPE)
				// Прекращаем проверку
				break;
			// Защитное обозначение, которому выдано дозволение
			PSID granted = reinterpret_cast <PSID> (&(reinterpret_cast <ACCESS_ALLOWED_ACE *> (entry)->SidStart));
			// Дескриптор маркера доступа процесса
			HANDLE token = nullptr;
			// Если маркер доступа процесса получить не удалось
			if(!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
				// Прекращаем проверку
				break;
			// Размер сведений о пользователе процесса
			DWORD size = 0;
			// Запрашиваем размер сведений о пользователе процесса
			::GetTokenInformation(token, TokenUser, nullptr, 0, &size);
			// Буфер сведений о пользователе процесса
			std::vector <uint8_t> user(static_cast <size_t> (size), 0);
			// Если сведения о пользователе процесса получены
			if((size > 0) && ::GetTokenInformation(token, TokenUser, user.data(), size, &size))
				// Проверяем что дозволение выдано пользователю процесса
				result = (::EqualSid(granted, reinterpret_cast <TOKEN_USER *> (user.data())->User.Sid) != FALSE);
			// Закрываем дескриптор маркера доступа процесса
			::CloseHandle(token);
		} while(false);
		// Снимаем описатель защиты файла
		::LocalFree(descriptor);
		// Выводим признак того, что доступ к файлу закрыт для всех прочих
		return result;
	/**
	 * Для операционных систем Linux, FreeBSD, NetBSD, OpenBSD, macOS и Solaris
	 */
	#else
		// Приметы файла
		struct stat attributes;
		// Если приметы файла снять не удалось
		if(::stat(path.c_str(), &attributes) != 0)
			// Выводим признак отказа
			return false;
		// Проверяем что права файла даны одному лишь владельцу
		return (static_cast <uint32_t> (attributes.st_mode & 0777) == static_cast <uint32_t> (0600));
	#endif
}

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
	 *
	 * Оборотов берётся заведомо больше, чем вмещает одно наполнение запаса случайных
	 * данных: неповторимость требуется не только внутри наполнения, но и через его
	 * границу - выдача из невыданного остатка вперемешку с набранным заново повторила
	 * бы векторы, а внутри одного наполнения проверка того не увидела бы вовсе
	 */
	for(uint16_t i = 0; i < 512; i++){
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
	EXPECT_EQ(vectors.size(), static_cast <size_t> (512));
	// Очищаем набор собранных векторов инициализации
	vectors.clear();
	/**
	 * Выполняем перебор разовых работ на удержанном ключе
	 */
	for(uint16_t i = 0; i < 512; i++){
		// Выполняем разовое шифрование сообщения
		const std::string encoded = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
		// Проверяем что шифротекст вектор инициализации несёт
		ASSERT_GE(encoded.size(), static_cast <size_t> (12));
		// Собираем вектор инициализации из начала шифротекста
		vectors.emplace(encoded.substr(0, 12));
	}
	// Проверяем неповторимость векторов инициализации разовых работ
	EXPECT_EQ(vectors.size(), static_cast <size_t> (512));
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
	// Проверяем что доступ к файлу закрыт для всех, кроме владельца
	EXPECT_TRUE(restricted(path));
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
	// Проверяем что доступ к перезаписанному файлу закрыт для всех, кроме владельца
	EXPECT_TRUE(restricted(path));
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
 * @brief Тест незавершённого потока шифрования
 *
 * @details Заведение потока вызовом initialize обращает всякий последующий encrypt в
 *          очередную порцию потока, а не в разовую работу: вектор инициализации
 *          выписывается один раз, имитовставка дописывается лишь вызовом finalize, а
 *          расшифровка удерживает последние октеты как возможную имитовставку. Оттого
 *          поток, не завершённый вызовом finalize, шифротекста не даёт - выдача его
 *          короче полного шифротекста ровно на имитовставку, и расшифровка такой выдачи
 *          теряет последние октеты.
 *
 *          Тест закрепляет именно это: не порчу, а недоведённую работу. Разбор дефекта,
 *          принесённого владельцем кодека ABC, показал, что уклад «initialize плюс один
 *          encrypt» читается потребителем как разовая работа, тогда как это открытый
 *          поток, и молчание тут - не молчание об ошибке, а отсутствие ошибки: сколько
 *          порций будет подано, работа знать не может
 *
 */
TEST_F(CryptoFixture, StreamUnfinishedCryptoTest){
	// Размер вектора инициализации режима с проверкой подлинности
	constexpr size_t IV_SIZE = 12;
	// Размер имитовставки режима с проверкой подлинности
	constexpr size_t TAG_SIZE = 16;
	// Текст для шифрования
	const std::string text(400, 'z');
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Выполняем разовое шифрование текста без заведения потока
	const std::string oneshot = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем надбавку разовой работы - вектор инициализации с имитовставкой
	ASSERT_EQ(oneshot.size(), text.size() + IV_SIZE + TAG_SIZE);
	// Выполняем заведение потока шифрования
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем подачу всего текста одной порцией потока
	std::string streamed = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	/**
	 * Порция потока несёт вектор инициализации, но не несёт имитовставки: та
	 * дописывается завершением, и до него шифротекст не готов
	 */
	// Проверяем недостачу имитовставки у незавершённого потока
	ASSERT_EQ(streamed.size(), oneshot.size() - TAG_SIZE);
	// Запоминаем шифротекст незавершённого потока
	const std::string partial = streamed;
	// Выполняем завершение потокового шифрования
	ASSERT_TRUE(this->_crypto->finalize(streamed));
	// Проверяем полноту шифротекста завершённого потока
	ASSERT_EQ(streamed.size(), oneshot.size());
	// Расшифрованный текст
	std::string decoded;
	// Выполняем заведение потока расшифровки
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем расшифровку шифротекста завершённого потока
	decoded.append(this->_crypto->decrypt <std::string> (streamed, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем завершение потоковой расшифровки
	ASSERT_TRUE(this->_crypto->finalize(decoded));
	// Проверяем обратимость завершённого потока
	ASSERT_EQ(decoded, text);
	/**
	 * Завершение освобождает контекст потока, и работа возвращается к разовой:
	 * тот же шифротекст поверх завершённого потока читается целиком и верно
	 */
	// Выполняем разовую расшифровку того же шифротекста поверх завершённого потока
	ASSERT_EQ(this->_crypto->decrypt <std::string> (streamed, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256), text);
	/**
	 * Поток - это одно сообщение, а не работа многократного пользования: второе
	 * сообщение поверх НЕзавершённого потока идёт продолжением прежнего счёта, и
	 * здоровый шифротекст читается в мусор. Отказа тут нет и быть не может -
	 * порция потока от сообщения неотличима
	 */
	// Выполняем заведение потока расшифровки
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем расшифровку здорового шифротекста первой порцией потока
	const std::string first = this->_crypto->decrypt <std::string> (streamed, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	/**
	 * Полный шифротекст читается порцией целиком: удерживаются ровно те октеты,
	 * что и оказываются имитовставкой
	 */
	// Проверяем полноту вычитанного открытого текста
	ASSERT_EQ(first, text);
	// Выполняем расшифровку того же шифротекста второй порцией того же потока
	const std::string second = this->_crypto->decrypt <std::string> (streamed, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем расхождение выдачи с открытым текстом
	ASSERT_NE(second, text);
	/**
	 * Шифротекст незавершённого потока имитовставки не несёт, и расшифровка
	 * удерживает как её последние октеты самого шифротекста: теряется ровно
	 * имитовставка, а вычитанное остаётся верным началом открытого текста
	 */
	// Выполняем заведение потока расшифровки
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем расшифровку шифротекста незавершённого потока
	const std::string truncated = this->_crypto->decrypt <std::string> (partial, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем недостачу последних октетов открытого текста
	ASSERT_EQ(truncated.size(), text.size() - TAG_SIZE);
	// Проверяем верность вычитанного начала
	ASSERT_EQ(truncated, text.substr(0, truncated.size()));
	/**
	 * В режиме без проверки подлинности незавершённый поток безвреден: имитовставки
	 * нет вовсе, удерживать расшифровке нечего, и надбавка сходится к одному вектору
	 * инициализации. Замер закрепляется здесь же, чтобы расхождение режимов не
	 * пришлось выводить рассуждением: опасность приходится ровно на GCM
	 */
	// Устанавливаем режим блочного шифрования без проверки подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::CFB);
	// Выполняем разовое шифрование текста без заведения потока
	const std::string plainshot = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Выполняем заведение потока шифрования
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем подачу всего текста одной порцией потока
	std::string plainstream = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Проверяем полноту шифротекста незавершённого потока
	ASSERT_EQ(plainstream.size(), plainshot.size());
	// Выполняем завершение потокового шифрования
	ASSERT_TRUE(this->_crypto->finalize(plainstream));
	// Проверяем неизменность шифротекста завершением
	ASSERT_EQ(plainstream.size(), plainshot.size());
	// Расшифрованный текст режима без проверки подлинности
	std::string plainback;
	// Выполняем заведение потока расшифровки
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::DECODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем расшифровку шифротекста потока
	plainback.append(this->_crypto->decrypt <std::string> (plainstream, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Выполняем завершение потоковой расшифровки
	ASSERT_TRUE(this->_crypto->finalize(plainback));
	// Проверяем обратимость потока режима без проверки подлинности
	ASSERT_EQ(plainback, text);
}

/**
 * @brief Тест расхождения сторон по разрядности шифра и по режиму блочного шифрования
 *
 * @details Ни разрядность шифра, ни режим блочного шифрования в шифротексте не хранятся -
 *          обе стороны ставят их сами, и расхождение сторон возможно. Тест закрепляет, чем
 *          оно кончается, и разница тут не в модуле, а в самом режиме: в режиме с проверкой
 *          подлинности расхождение ловится имитовставкой и кончается отказом, а в режиме без
 *          неё ловить нечем - гаммирование выдаёт открытый текст любой длины на любом ключе,
 *          и выдача мусора успехом здесь неизбежна.
 *
 *          Закрепляется это ради того, чтобы вывод «отсутствие вида шифра в записи ничем не
 *          грозит», снятый на GCM, не переносили на CFB: на GCM шесть расхождений из шести
 *          отвергаются, на CFB шесть из шести проходят молча
 *
 */
TEST_F(CryptoFixture, ModeMismatchCryptoTest){
	// Текст для шифрования
	const std::string text(400, 'z');
	// Устанавливаем пароль шифрования
	this->_crypto->password("password");
	// Устанавливаем соль шифрования
	this->_crypto->salt("salt");
	// Набор разрядностей шифра
	const awh::crypto_t::cipher_t ciphers[3] = {awh::crypto_t::cipher_t::AES128, awh::crypto_t::cipher_t::AES192, awh::crypto_t::cipher_t::AES256};
	/**
	 * Выполняем перебор режимов блочного шифрования
	 */
	for(const awh::crypto_t::mode_t mode : {awh::crypto_t::mode_t::GCM, awh::crypto_t::mode_t::CFB}){
		// Признак режима с проверкой подлинности
		const bool secured = (mode == awh::crypto_t::mode_t::GCM);
		// Устанавливаем режим блочного шифрования
		this->_crypto->mode(mode);
		/**
		 * Выполняем перебор разрядностей шифра укладывающей стороны
		 */
		for(const awh::crypto_t::cipher_t put : ciphers){
			// Выполняем шифрование текста укладывающей стороной
			const std::string encoded = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, put);
			// Проверяем наличие шифротекста
			ASSERT_FALSE(encoded.empty());
			/**
			 * Выполняем перебор разрядностей шифра читающей стороны
			 */
			for(const awh::crypto_t::cipher_t get : ciphers){
				// Открытый текст читающей стороны
				std::string decoded;
				// Выполняем расшифровку шифротекста читающей стороной
				const bool outcome = this->_crypto->decrypt(encoded.data(), encoded.size(), decoded, awh::crypto_t::hash_t::SHA256, get);
				// Если разрядность шифра у сторон сходится
				if(put == get){
					// Проверяем успех работы
					ASSERT_TRUE(outcome) << static_cast <uint16_t> (mode);
					// Проверяем обратимость шифрования
					ASSERT_EQ(decoded, text) << static_cast <uint16_t> (mode);
				/**
				 * Расхождение разрядности ловится имитовставкой, и лишь ею: в режиме
				 * без проверки подлинности ловить его нечем
				 */
				// Если разрядность шифра у сторон расходится
				} else if(secured) {
					// Проверяем отказ работы
					ASSERT_FALSE(outcome) << static_cast <uint16_t> (put);
					// Проверяем отсутствие выдачи при отказе
					ASSERT_TRUE(decoded.empty()) << static_cast <uint16_t> (put);
				// Если работа идёт режимом без проверки подлинности
				} else {
					// Проверяем выдачу работы
					ASSERT_TRUE(outcome) << static_cast <uint16_t> (put);
					// Проверяем расхождение выдачи с открытым текстом
					ASSERT_NE(decoded, text) << static_cast <uint16_t> (put);
				}
			}
		}
	}
	/**
	 * Расхождение сторон по самому режиму кончается тем же порядком: шифротекст
	 * режима без проверки подлинности отвергается имитовставкой, а шифротекст режима
	 * с нею читается гаммированием молча - вектор инициализации там короче, и первые
	 * октеты шифротекста уходят в гамму как его часть
	 */
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Выполняем шифрование текста режимом с проверкой подлинности
	const std::string authentic = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Устанавливаем режим блочного шифрования без проверки подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::CFB);
	// Выполняем шифрование текста режимом без проверки подлинности
	const std::string plain = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
	// Открытый текст расхождения по режиму
	std::string decoded;
	// Выполняем расшифровку шифротекста режима с проверкой подлинности гаммированием
	ASSERT_TRUE(this->_crypto->decrypt(authentic.data(), authentic.size(), decoded, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем расхождение выдачи с открытым текстом
	ASSERT_NE(decoded, text);
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	this->_crypto->mode(awh::crypto_t::mode_t::GCM);
	// Выполняем очистку открытого текста расхождения по режиму
	decoded.clear();
	// Выполняем расшифровку шифротекста гаммирования режимом с проверкой подлинности
	ASSERT_FALSE(this->_crypto->decrypt(plain.data(), plain.size(), decoded, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем отсутствие выдачи при отказе
	ASSERT_TRUE(decoded.empty());
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

/**
 * Если операционной системой не является MS Windows
 */
#if !_WIN32 && !_WIN64
	/**
	 * Заголовочные файлы работы с процессами
	 */
	#include <unistd.h>
	#include <sys/wait.h>

	/**
	 * @brief Тест неповторимости векторов инициализации при разветвлении процесса
	 *
	 * @details Случайность для векторов берётся пачкой: одно обращение к библиотеке
	 *          криптографии набирает запас на несколько десятков векторов. Память
	 *          потомку достаётся списком с родительской, и невыданный остаток запаса
	 *          оказался бы у обоих один и тот же - родитель и потомок выдали бы
	 *          одинаковые векторы на одном ключе, а повтор вектора на одном ключе
	 *          рушит стойкость режима GCM полностью
	 *
	 */
	TEST_F(CryptoFixture, ForkVectorCryptoTest){
		// Размер вектора инициализации режима GCM
		static constexpr size_t IVEC = 12;
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
		/**
		 * @brief Функция получения вектора инициализации очередного шифрования
		 *
		 * @return вектор инициализации из начала шифротекста
		 *
		 */
		auto vector = [&]() noexcept -> std::string {
			// Шифротекст сообщения
			std::string encoded;
			// Выполняем шифрование сообщения
			encoded = this->_crypto->encrypt <std::string> (text, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256);
			// Выводим вектор инициализации из начала шифротекста
			return ((encoded.size() >= IVEC) ? encoded.substr(0, IVEC) : std::string());
		};
		/**
		 * Запас случайных данных набирается прежде разветвления: невыданный остаток
		 * его и есть то, что потомок унаследовал бы от родителя
		 */
		// Выполняем набор запаса случайных данных
		ASSERT_EQ(vector().size(), IVEC);
		// Канал передачи вектора инициализации от потомка
		int32_t fds[2] = {-1, -1};
		// Выполняем заведение канала передачи вектора инициализации
		ASSERT_EQ(::pipe(fds), 0);
		// Выполняем разветвление процесса
		const pid_t pid = ::fork();
		// Проверяем выполненное разветвление процесса
		ASSERT_GE(pid, 0);
		// Если работа идёт в потомке
		if(pid == 0){
			// Закрываем сторону чтения канала передачи
			::close(fds[0]);
			// Получаем вектор инициализации потомка
			const std::string & result = vector();
			// Выполняем передачу вектора инициализации родителю
			const ssize_t bytes = ::write(fds[1], result.data(), result.size());
			// Закрываем сторону записи канала передачи
			::close(fds[1]);
			// Завершаем работу потомка минуя разрушение объектов набора
			::_exit((bytes == static_cast <ssize_t> (IVEC)) ? 0 : 1);
		}
		// Закрываем сторону записи канала передачи
		::close(fds[1]);
		// Буфер вектора инициализации потомка
		std::string child(IVEC, '\0');
		// Выполняем чтение вектора инициализации потомка
		const ssize_t bytes = ::read(fds[0], child.data(), child.size());
		// Закрываем сторону чтения канала передачи
		::close(fds[0]);
		// Состояние завершения работы потомка
		int32_t status = 0;
		// Выполняем ожидание завершения работы потомка
		ASSERT_EQ(::waitpid(pid, &status, 0), pid);
		// Проверяем получение вектора инициализации потомка
		ASSERT_EQ(bytes, static_cast <ssize_t> (IVEC));
		// Получаем вектор инициализации родителя
		const std::string & parent = vector();
		// Проверяем получение вектора инициализации родителя
		ASSERT_EQ(parent.size(), IVEC);
		// Проверяем неповторимость векторов инициализации родителя и потомка
		EXPECT_NE(parent, child);
	}
#endif

/**
 * @brief Тест выработки ключей подписи всех заведённых видов
 *
 */
TEST_F(CryptoFixture, SignatureKindsCryptoTest){
	/**
	 * Выполняем перебор всех заведённых видов подписи
	 */
	for(auto & kind : {awh::crypto_t::signature_t::RSA, awh::crypto_t::signature_t::ECDSA, awh::crypto_t::signature_t::ED25519, awh::crypto_t::signature_t::GOST, awh::crypto_t::signature_t::GOST512}){
		// Выполняем выработку ключа подписи
		ASSERT_TRUE(this->_crypto->generateKey("owner", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем вид подписи выработанного ключа
		EXPECT_EQ(this->_crypto->signature("owner"), kind) << "kind = " << static_cast <uint16_t> (kind);
	}
	// Проверяем отсутствие вида подписи у имени, ключа не имеющего
	EXPECT_EQ(this->_crypto->signature("stranger"), awh::crypto_t::signature_t::NONE);
	// Проверяем отказ выработки ключа вида, разбору не знакомого
	EXPECT_FALSE(this->_crypto->generateKey("none", awh::crypto_t::signature_t::NONE));
}

/**
 * @brief Тест подписи и её проверки всеми заведёнными видами
 *
 * @details Закрепляет три отказа, ради которых подпись и заводится: подпись отвергается
 *          чужим ключом, испорченная на разряд подпись отвергается, испорченное на
 *          разряд сообщение отвергается
 *
 */
TEST_F(CryptoFixture, SignatureVerifyCryptoTest){
	// Данные для подписи
	const std::string text = "Anyks Framework, Hello World!!!";
	/**
	 * Выполняем перебор всех заведённых видов подписи
	 */
	for(auto & kind : {awh::crypto_t::signature_t::RSA, awh::crypto_t::signature_t::ECDSA, awh::crypto_t::signature_t::ED25519}){
		// Тип хэш-суммы, схеме подписи отвечающий
		const awh::crypto_t::hash_t hash = ((kind == awh::crypto_t::signature_t::ED25519) ? awh::crypto_t::hash_t::NONE : awh::crypto_t::hash_t::SHA256);
		// Выполняем выработку ключа владельца
		ASSERT_TRUE(this->_crypto->generateKey("owner", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Выполняем выработку ключа постороннего
		ASSERT_TRUE(this->_crypto->generateKey("stranger", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Подпись данных
		std::vector <uint8_t> signature;
		// Выполняем подписание данных ключом владельца
		ASSERT_TRUE(this->_crypto->sign("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), hash, signature)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем наличие выработанной подписи
		ASSERT_FALSE(signature.empty()) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем подпись ключом владельца
		EXPECT_TRUE(this->_crypto->verify("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), signature, hash)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем отказ подписи чужим ключом
		EXPECT_FALSE(this->_crypto->verify("stranger", reinterpret_cast <const uint8_t *> (text.data()), text.size(), signature, hash)) << "kind = " << static_cast <uint16_t> (kind);
		// Подпись, испорченная на один разряд
		std::vector <uint8_t> tampered = signature;
		// Выполняем порчу одного разряда подписи
		tampered[tampered.size() / 2] ^= 0x01;
		// Проверяем отказ подписи, испорченной на разряд
		EXPECT_FALSE(this->_crypto->verify("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), tampered, hash)) << "kind = " << static_cast <uint16_t> (kind);
		// Сообщение, испорченное на один разряд
		std::string message = text;
		// Выполняем порчу одного разряда сообщения
		message[message.size() / 2] = static_cast <char> (message[message.size() / 2] ^ 0x01);
		// Проверяем отказ подписи испорченного на разряд сообщения
		EXPECT_FALSE(this->_crypto->verify("owner", reinterpret_cast <const uint8_t *> (message.data()), message.size(), signature, hash)) << "kind = " << static_cast <uint16_t> (kind);
	}
}

/**
 * @brief Тест уместности типа хэш-суммы по видам подписи
 *
 * @details Схемам RSA и ECDSA тип хэш-суммы обязателен, а схеме Ed25519 неуместен: она
 *          подписывает сообщение сама. Договор различие это выражает прямо, а не
 *          сглаживает: поданная схеме Ed25519 хэш-сумма подписью хэш-суммы не станет
 *
 */
TEST_F(CryptoFixture, SignatureHashContractCryptoTest){
	// Данные для подписи
	const std::string text = "Anyks Framework, Hello World!!!";
	// Подпись данных
	std::vector <uint8_t> signature;
	// Выполняем выработку ключа Ed25519
	ASSERT_TRUE(this->_crypto->generateKey("pure", awh::crypto_t::signature_t::ED25519));
	// Проверяем отказ подписи Ed25519 при поданной хэш-сумме
	EXPECT_FALSE(this->_crypto->sign("pure", reinterpret_cast <const uint8_t *> (text.data()), text.size(), awh::crypto_t::hash_t::SHA256, signature));
	// Проверяем пустоту буфера подписи при отказе
	EXPECT_TRUE(signature.empty());
	/**
	 * Выполняем перебор видов подписи, подписывающих хэш-сумму
	 */
	for(auto & kind : {awh::crypto_t::signature_t::RSA, awh::crypto_t::signature_t::ECDSA}){
		// Выполняем выработку ключа подписи
		ASSERT_TRUE(this->_crypto->generateKey("digest", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем отказ подписи при отсутствии типа хэш-суммы
		EXPECT_FALSE(this->_crypto->sign("digest", reinterpret_cast <const uint8_t *> (text.data()), text.size(), awh::crypto_t::hash_t::NONE, signature)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем пустоту буфера подписи при отказе
		EXPECT_TRUE(signature.empty()) << "kind = " << static_cast <uint16_t> (kind);
	}
}

/**
 * @brief Тест поточности видов подписи
 *
 * @details Поточной подписи Ed25519 не имеет вовсе, и отказ его называет причину видом
 *          подписи. Спросить о поточности можно наперёд: потребитель, написавший работу
 *          под поточный договор, иначе наткнулся бы на отказ посреди работы при одной
 *          лишь смене вида ключа
 *
 */
TEST_F(CryptoFixture, SignatureStreamableCryptoTest){
	// Проверяем поточность вида подписи RSA
	EXPECT_TRUE(this->_crypto->streamable(awh::crypto_t::signature_t::RSA));
	// Проверяем поточность вида подписи ECDSA
	EXPECT_TRUE(this->_crypto->streamable(awh::crypto_t::signature_t::ECDSA));
	// Проверяем отсутствие поточности у вида подписи Ed25519
	EXPECT_FALSE(this->_crypto->streamable(awh::crypto_t::signature_t::ED25519));
	// Выполняем выработку ключа Ed25519
	ASSERT_TRUE(this->_crypto->generateKey("pure", awh::crypto_t::signature_t::ED25519));
	// Проверяем отказ заведения потока подписи видом, поточным не бывающим
	EXPECT_FALSE(this->_crypto->signInitialize("pure", awh::crypto_t::hash_t::SHA256));
	// Подпись потока
	std::vector <uint8_t> signature;
	// Проверяем отказ подачи в поток, заведения не прошедший
	EXPECT_FALSE(this->_crypto->signUpdate(reinterpret_cast <const uint8_t *> ("test"), 4));
	// Проверяем отказ завершения потока, заведения не прошедшего
	EXPECT_FALSE(this->_crypto->signFinalize(signature));
}

/**
 * @brief Тест совпадения поточной подписи с подписью буфером целиком
 *
 * @details Подпись схемы RSA с дополнением PKCS#1 v1.5 от случайности не зависит, и
 *          поточная её выработка обязана совпасть с разовой число в число. Схемы же
 *          ECDSA и PSS подпись вырабатывают на случайном значении, и сличать их подписи
 *          дословно нельзя вовсе - у них сличается принятие проверкой
 *
 */
TEST_F(CryptoFixture, SignatureStreamMatchCryptoTest){
	// Данные для подписи
	const std::string text = "Anyks Framework, Hello World!!! Anyks Framework, Hello World!!!";
	// Устанавливаем схему дополнения подписи, от случайности не зависящую
	this->_crypto->padding(awh::crypto_t::padding_t::PKCS1);
	// Выполняем выработку ключа подписи
	ASSERT_TRUE(this->_crypto->generateKey("owner", awh::crypto_t::signature_t::RSA));
	// Подпись данных буфером целиком
	std::vector <uint8_t> whole;
	// Выполняем подписание данных буфером целиком
	ASSERT_TRUE(this->_crypto->sign("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), awh::crypto_t::hash_t::SHA256, whole));
	// Подпись данных потоком
	std::vector <uint8_t> stream;
	// Выполняем заведение потока подписи
	ASSERT_TRUE(this->_crypto->signInitialize("owner", awh::crypto_t::hash_t::SHA256));
	/**
	 * Выполняем подачу данных потоку по одному октету
	 */
	for(size_t i = 0; i < text.size(); i++)
		// Выполняем подачу очередного октета потоку подписи
		ASSERT_TRUE(this->_crypto->signUpdate(reinterpret_cast <const uint8_t *> (text.data() + i), 1));
	// Выполняем завершение потока подписи
	ASSERT_TRUE(this->_crypto->signFinalize(stream));
	// Проверяем совпадение поточной подписи с подписью буфером целиком
	EXPECT_EQ(stream, whole);
	// Проверяем принятие поточной подписи проверкой
	EXPECT_TRUE(this->_crypto->verify("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), stream, awh::crypto_t::hash_t::SHA256));
	/**
	 * Схема ECDSA подпись вырабатывает на случайном значении: сличать её подписи
	 * дословно нельзя, и совпадение потока с разовой работой у неё судится проверкой
	 */
	// Выполняем выработку ключа ECDSA
	ASSERT_TRUE(this->_crypto->generateKey("ecdsa", awh::crypto_t::signature_t::ECDSA));
	// Подпись данных потоком ключом ECDSA
	std::vector <uint8_t> signature;
	// Выполняем заведение потока подписи ключом ECDSA
	ASSERT_TRUE(this->_crypto->signInitialize("ecdsa", awh::crypto_t::hash_t::SHA256));
	// Выполняем подачу данных потоку подписи двумя порциями
	ASSERT_TRUE(this->_crypto->signUpdate(reinterpret_cast <const uint8_t *> (text.data()), text.size() / 2));
	// Выполняем подачу остатка данных потоку подписи
	ASSERT_TRUE(this->_crypto->signUpdate(reinterpret_cast <const uint8_t *> (text.data() + (text.size() / 2)), text.size() - (text.size() / 2)));
	// Выполняем завершение потока подписи
	ASSERT_TRUE(this->_crypto->signFinalize(signature));
	// Проверяем принятие поточной подписи ключом ECDSA
	EXPECT_TRUE(this->_crypto->verify("ecdsa", reinterpret_cast <const uint8_t *> (text.data()), text.size(), signature, awh::crypto_t::hash_t::SHA256));
}

/**
 * @brief Тест длины подписи, спрашиваемой наперёд
 *
 * @details Работам, правящим запись на месте, место под подпись приходится резервировать
 *          заранее. Постоянной длины подпись имеет не всегда: у ECDSA запись DER несёт
 *          два числа переменной длины, и одна пара ключей даёт подписи разной длины на
 *          разных сообщениях
 *
 */
TEST_F(CryptoFixture, SignatureLengthCryptoTest){
	// Данные для подписи
	const std::string text = "Anyks Framework, Hello World!!!";
	// Выполняем выработку ключа Ed25519
	ASSERT_TRUE(this->_crypto->generateKey("pure", awh::crypto_t::signature_t::ED25519));
	// Проверяем постоянную длину подписи Ed25519
	EXPECT_EQ(this->_crypto->length("pure"), static_cast <size_t> (64));
	// Проверяем верхний предел длины подписи Ed25519
	EXPECT_EQ(this->_crypto->limit("pure"), static_cast <size_t> (64));
	// Выполняем выработку ключа RSA
	ASSERT_TRUE(this->_crypto->generateKey("rsa", awh::crypto_t::signature_t::RSA));
	// Проверяем постоянную длину подписи RSA, равную разрядности ключа
	EXPECT_EQ(this->_crypto->length("rsa"), static_cast <size_t> (2048 / 8));
	// Проверяем совпадение предела с точной длиной подписи RSA
	EXPECT_EQ(this->_crypto->limit("rsa"), this->_crypto->length("rsa"));
	// Выполняем выработку ключа ECDSA
	ASSERT_TRUE(this->_crypto->generateKey("ecdsa", awh::crypto_t::signature_t::ECDSA));
	// Проверяем отсутствие постоянной длины подписи ECDSA
	EXPECT_EQ(this->_crypto->length("ecdsa"), static_cast <size_t> (0));
	// Проверяем наличие верхнего предела длины подписи ECDSA
	ASSERT_GT(this->_crypto->limit("ecdsa"), static_cast <size_t> (0));
	// Проверяем отсутствие длины у имени, ключа не имеющего
	EXPECT_EQ(this->_crypto->limit("stranger"), static_cast <size_t> (0));
	/**
	 * Выполняем перебор ключей, длину подписи объявляющих
	 */
	for(auto & name : {"pure", "rsa", "ecdsa"}){
		// Подпись данных
		std::vector <uint8_t> signature;
		// Тип хэш-суммы, схеме подписи отвечающий
		const awh::crypto_t::hash_t hash = ((this->_crypto->signature(name) == awh::crypto_t::signature_t::ED25519) ? awh::crypto_t::hash_t::NONE : awh::crypto_t::hash_t::SHA256);
		// Выполняем подписание данных
		ASSERT_TRUE(this->_crypto->sign(name, reinterpret_cast <const uint8_t *> (text.data()), text.size(), hash, signature)) << "name = " << name;
		// Проверяем, что выработанная подпись предела не превышает
		EXPECT_LE(signature.size(), this->_crypto->limit(name)) << "name = " << name;
		// Получаем объявленную точную длину подписи
		const size_t length = this->_crypto->length(name);
		// Если точная длина подписи объявлена
		if(length > 0)
			// Проверяем совпадение выработанной подписи с объявленной длиной
			EXPECT_EQ(signature.size(), length) << "name = " << name;
	}
}

/**
 * @brief Тест отпечатка открытого ключа
 *
 * @details Отпечаток считается от канонической записи открытого ключа и выдаётся полными
 *          тридцатью двумя октетами. Требуется он проверяющей стороне, а та закрытого
 *          ключа не имеет вовсе
 *
 */
TEST_F(CryptoFixture, SignatureFingerprintCryptoTest){
	/**
	 * Выполняем перебор всех заведённых видов подписи
	 */
	for(auto & kind : {awh::crypto_t::signature_t::RSA, awh::crypto_t::signature_t::ECDSA, awh::crypto_t::signature_t::ED25519}){
		// Выполняем выработку ключа владельца
		ASSERT_TRUE(this->_crypto->generateKey("owner", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Выполняем выработку ключа постороннего
		ASSERT_TRUE(this->_crypto->generateKey("stranger", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Получаем отпечаток открытого ключа владельца
		const std::vector <uint8_t> & owner = this->_crypto->fingerprint <std::vector <uint8_t>> ("owner");
		// Проверяем длину отпечатка открытого ключа
		ASSERT_EQ(owner.size(), static_cast <size_t> (32)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем постоянство отпечатка одного и того же ключа
		EXPECT_EQ(this->_crypto->fingerprint <std::vector <uint8_t>> ("owner"), owner) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем несовпадение отпечатков разных ключей
		EXPECT_NE(this->_crypto->fingerprint <std::vector <uint8_t>> ("stranger"), owner) << "kind = " << static_cast <uint16_t> (kind);
		// Получаем шестнадцатеричную запись отпечатка открытого ключа
		const std::string & hex = this->_crypto->fingerprint <std::string> ("owner", awh::crypto_t::format_t::HEX);
		// Проверяем длину шестнадцатеричной записи отпечатка
		EXPECT_EQ(hex.size(), static_cast <size_t> (64)) << "kind = " << static_cast <uint16_t> (kind);
		/**
		 * Отпечаток считается от открытой части ключа: проверяющая сторона закрытого
		 * ключа не имеет, и требовать его для опознания было бы нечем
		 */
		// Получаем запись открытого ключа владельца
		const std::string & pem = this->_crypto->getKey("owner", awh::crypto_t::key_type_t::PUBLIC);
		// Проверяем получение записи открытого ключа
		ASSERT_FALSE(pem.empty()) << "kind = " << static_cast <uint16_t> (kind);
		// Объект работы, закрытого ключа не имеющий
		awh::crypto_t verifier(this->_fmk.get(), this->_log.get());
		// Выполняем ввод одного лишь открытого ключа
		ASSERT_TRUE(verifier.setKey("owner", pem, awh::crypto_t::key_type_t::PUBLIC)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем совпадение отпечатка, снятого без закрытого ключа
		EXPECT_EQ(verifier.fingerprint <std::vector <uint8_t>> ("owner"), owner) << "kind = " << static_cast <uint16_t> (kind);
	}
	// Проверяем пустоту отпечатка у имени, ключа не имеющего
	EXPECT_TRUE(this->_crypto->fingerprint <std::vector <uint8_t>> ("nobody").empty());
}

/**
 * @brief Тест проверки подписи одним лишь открытым ключом
 *
 * @details Проверяющая сторона закрытого ключа не имеет, и проверка обязана идти без
 *          него: иначе всякий, кто способен проверить подпись, способен и подписать
 *
 */
TEST_F(CryptoFixture, SignaturePublicOnlyCryptoTest){
	// Данные для подписи
	const std::string text = "Anyks Framework, Hello World!!!";
	/**
	 * Выполняем перебор всех заведённых видов подписи
	 */
	for(auto & kind : {awh::crypto_t::signature_t::RSA, awh::crypto_t::signature_t::ECDSA, awh::crypto_t::signature_t::ED25519}){
		// Тип хэш-суммы, схеме подписи отвечающий
		const awh::crypto_t::hash_t hash = ((kind == awh::crypto_t::signature_t::ED25519) ? awh::crypto_t::hash_t::NONE : awh::crypto_t::hash_t::SHA256);
		// Выполняем выработку ключа владельца
		ASSERT_TRUE(this->_crypto->generateKey("owner", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Подпись данных
		std::vector <uint8_t> signature;
		// Выполняем подписание данных ключом владельца
		ASSERT_TRUE(this->_crypto->sign("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), hash, signature)) << "kind = " << static_cast <uint16_t> (kind);
		// Получаем запись открытого ключа владельца
		const std::string & pem = this->_crypto->getKey("owner", awh::crypto_t::key_type_t::PUBLIC);
		// Проверяем получение записи открытого ключа
		ASSERT_FALSE(pem.empty()) << "kind = " << static_cast <uint16_t> (kind);
		// Объект работы, закрытого ключа не имеющий
		awh::crypto_t verifier(this->_fmk.get(), this->_log.get());
		// Выполняем ввод одного лишь открытого ключа
		ASSERT_TRUE(verifier.setKey("owner", pem, awh::crypto_t::key_type_t::PUBLIC)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем вид подписи введённого открытого ключа
		EXPECT_EQ(verifier.signature("owner"), kind) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем подпись одним лишь открытым ключом
		EXPECT_TRUE(verifier.verify("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), signature, hash)) << "kind = " << static_cast <uint16_t> (kind);
		// Подпись, выработанная одним лишь открытым ключом
		std::vector <uint8_t> denied;
		// Проверяем отказ подписания одним лишь открытым ключом
		EXPECT_FALSE(verifier.sign("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), hash, denied)) << "kind = " << static_cast <uint16_t> (kind);
	}
}

/**
 * @brief Тест нескольких ключей на одном объекте работы
 *
 * @details Один контейнер подписывают владелец и заверитель, а проверяющая сторона
 *          сличает с несколькими открытыми ключами подряд. Заводить объект работы на
 *          всякий ключ негодно: у него внутри стейт шифрования, к подписи отношения не
 *          имеющий вовсе
 *
 */
TEST_F(CryptoFixture, SignatureKeyringCryptoTest){
	// Данные для подписи
	const std::string text = "Anyks Framework, Hello World!!!";
	// Выполняем выработку ключа владельца
	ASSERT_TRUE(this->_crypto->generateKey("owner", awh::crypto_t::signature_t::ED25519));
	// Выполняем выработку ключа заверителя
	ASSERT_TRUE(this->_crypto->generateKey("notary", awh::crypto_t::signature_t::ECDSA));
	// Подпись владельца
	std::vector <uint8_t> owner;
	// Подпись заверителя
	std::vector <uint8_t> notary;
	// Выполняем подписание данных ключом владельца
	ASSERT_TRUE(this->_crypto->sign("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), awh::crypto_t::hash_t::NONE, owner));
	// Выполняем подписание данных ключом заверителя
	ASSERT_TRUE(this->_crypto->sign("notary", reinterpret_cast <const uint8_t *> (text.data()), text.size(), awh::crypto_t::hash_t::SHA256, notary));
	// Проверяем подпись владельца его же ключом
	EXPECT_TRUE(this->_crypto->verify("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), owner, awh::crypto_t::hash_t::NONE));
	// Проверяем подпись заверителя его же ключом
	EXPECT_TRUE(this->_crypto->verify("notary", reinterpret_cast <const uint8_t *> (text.data()), text.size(), notary, awh::crypto_t::hash_t::SHA256));
	// Проверяем отказ подписи владельца ключом заверителя
	EXPECT_FALSE(this->_crypto->verify("notary", reinterpret_cast <const uint8_t *> (text.data()), text.size(), owner, awh::crypto_t::hash_t::SHA256));
	// Проверяем несовпадение отпечатков ключей владельца и заверителя
	EXPECT_NE(this->_crypto->fingerprint <std::vector <uint8_t>> ("owner"), this->_crypto->fingerprint <std::vector <uint8_t>> ("notary"));
	// Выполняем снятие ключа заверителя
	EXPECT_TRUE(this->_crypto->removeKey("notary"));
	// Проверяем отсутствие снятого ключа в связке
	EXPECT_EQ(this->_crypto->signature("notary"), awh::crypto_t::signature_t::NONE);
	// Проверяем отказ снятия ключа, в связке не лежащего
	EXPECT_FALSE(this->_crypto->removeKey("notary"));
	// Проверяем, что ключ владельца снятие соседа пережил
	EXPECT_TRUE(this->_crypto->verify("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), owner, awh::crypto_t::hash_t::NONE));
}

/**
 * @brief Тест обращения ключей подписи через записи и файлы
 *
 */
TEST_F(CryptoFixture, SignatureKeyStorageCryptoTest){
	// Данные для подписи
	const std::string text = "Anyks Framework, Hello World!!!";
	/**
	 * Выполняем перебор всех заведённых видов подписи
	 */
	for(auto & kind : {awh::crypto_t::signature_t::RSA, awh::crypto_t::signature_t::ECDSA, awh::crypto_t::signature_t::ED25519,
	 awh::crypto_t::signature_t::GOST, awh::crypto_t::signature_t::GOST512}){
		/**
		 * Тип хэш-суммы, схеме подписи отвечающий
		 *
		 * @note Схемы Ed25519 и ГОСТ хэш-суммы не принимают: первая подписывает
		 *       сообщение сама, вторая предписывает хэш-функцию собою
		 */
		const awh::crypto_t::hash_t hash = (((kind == awh::crypto_t::signature_t::ED25519) ||
		 (kind == awh::crypto_t::signature_t::GOST) || (kind == awh::crypto_t::signature_t::GOST512)) ?
		 awh::crypto_t::hash_t::NONE : awh::crypto_t::hash_t::SHA256);
		// Выполняем выработку ключа владельца
		ASSERT_TRUE(this->_crypto->generateKey("owner", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Получаем отпечаток выработанного ключа
		const std::vector <uint8_t> & origin = this->_crypto->fingerprint <std::vector <uint8_t>> ("owner");
		// Выполняем запись закрытого ключа в файл
		ASSERT_TRUE(this->_crypto->saveKey("owner", "sign_private.pem", awh::crypto_t::key_type_t::PRIVATE)) << "kind = " << static_cast <uint16_t> (kind);
		// Выполняем запись открытого ключа в файл
		ASSERT_TRUE(this->_crypto->saveKey("owner", "sign_public.pem", awh::crypto_t::key_type_t::PUBLIC)) << "kind = " << static_cast <uint16_t> (kind);
		/**
		 * Права файла закрытого ключа сличаются с правами одного лишь владельца тем же
		 * порядком, каким сличаются права файла ключа RSA (5.29)
		 */
		#if !_WIN32 && !_WIN64
			// Приметы файла закрытого ключа
			struct stat info;
			// Выполняем снятие примет файла закрытого ключа
			ASSERT_EQ(::stat("sign_private.pem", &info), 0) << "kind = " << static_cast <uint16_t> (kind);
			// Проверяем права файла закрытого ключа
			EXPECT_EQ(static_cast <uint32_t> (info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)), static_cast <uint32_t> (S_IRUSR | S_IWUSR)) << "kind = " << static_cast <uint16_t> (kind);
		#endif
		// Объект работы, читающий ключи из файлов
		awh::crypto_t reader(this->_fmk.get(), this->_log.get());
		// Выполняем чтение закрытого ключа из файла
		ASSERT_TRUE(reader.loadKey("owner", "sign_private.pem", awh::crypto_t::key_type_t::PRIVATE)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем совпадение отпечатка прочитанного ключа с выработанным
		EXPECT_EQ(reader.fingerprint <std::vector <uint8_t>> ("owner"), origin) << "kind = " << static_cast <uint16_t> (kind);
		// Подпись данных прочитанным ключом
		std::vector <uint8_t> signature;
		// Выполняем подписание данных прочитанным ключом
		ASSERT_TRUE(reader.sign("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), hash, signature)) << "kind = " << static_cast <uint16_t> (kind);
		// Выполняем чтение открытого ключа из файла
		ASSERT_TRUE(reader.loadKey("public", "sign_public.pem", awh::crypto_t::key_type_t::PUBLIC)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем подпись прочитанным открытым ключом
		EXPECT_TRUE(reader.verify("public", reinterpret_cast <const uint8_t *> (text.data()), text.size(), signature, hash)) << "kind = " << static_cast <uint16_t> (kind);
		// Выполняем снятие файла закрытого ключа
		::remove("sign_private.pem");
		// Выполняем снятие файла открытого ключа
		::remove("sign_public.pem");
	}
	// Проверяем отказ чтения ключа из отсутствующего файла
	EXPECT_FALSE(this->_crypto->loadKey("owner", "sign_missing.pem", awh::crypto_t::key_type_t::PUBLIC));
	// Проверяем отказ ввода записи ключа, ключом не являющейся
	EXPECT_FALSE(this->_crypto->setKey("owner", "not a key at all", awh::crypto_t::key_type_t::PUBLIC));
}

/**
 * @brief Тест одновременной проверки подписи из нескольких потоков выполнения
 *
 * @details Проверка подписи объявлена работой, изменяемого состояния объекта не
 *          трогающей: связка ключей при ней лишь читается, а контекст заводится свой на
 *          всякий вызов. Свойство это требуется потребителю - он зовёт проверку из потока
 *          цикла событий, пока другой поток работает с тем же объектом, - и незакреплённое
 *          оно держится на одном лишь честном слове до первой правки
 *
 * @note Гонку сама по себе проверка не ловит: гонка проявляется не всегда, а
 *       порознь взятые вызовы могут разойтись во времени. Ловит её прогон под
 *       сторожем потоков (см. DECISIONS 5а.16); эта же проверка стоит на страже
 *       грубой поломки - отказа проверки либо срыва работы при одновременном вызове
 *
 */
TEST_F(CryptoFixture, SignatureConcurrentVerifyCryptoTest){
	// Количество потоков выполнения
	static constexpr size_t THREADS = 8;
	// Количество проверок подписи в каждом потоке
	static constexpr size_t ROUNDS = 200;
	// Данные для подписи
	const std::string text = "Anyks Framework, Hello World!!!";
	// Имена ключей связки
	const std::vector <std::string> names = {"ed", "ec", "rsa"};
	// Виды подписи ключей связки
	const std::vector <awh::crypto_t::signature_t> kinds = {
		awh::crypto_t::signature_t::ED25519,
		awh::crypto_t::signature_t::ECDSA,
		awh::crypto_t::signature_t::RSA
	};
	// Подписи данных ключами связки
	std::vector <std::vector <uint8_t>> signatures(names.size());
	/**
	 * Выполняем заведение ключей связки и выработку подписей
	 */
	for(size_t i = 0; i < names.size(); i++){
		// Выполняем выработку ключа подписи
		ASSERT_TRUE(this->_crypto->generateKey(names.at(i), kinds.at(i)));
		// Тип хэш-суммы, схеме подписи отвечающий
		const awh::crypto_t::hash_t hash = ((kinds.at(i) == awh::crypto_t::signature_t::ED25519) ? awh::crypto_t::hash_t::NONE : awh::crypto_t::hash_t::SHA256);
		// Выполняем выработку подписи данных
		ASSERT_TRUE(this->_crypto->sign(names.at(i), reinterpret_cast <const uint8_t *> (text.data()), text.size(), hash, signatures.at(i)));
	}
	// Количество принятых проверкой подписей
	std::atomic <size_t> accepted{0};
	// Количество отвергнутых проверкой подписей
	std::atomic <size_t> rejected{0};
	// Потоки выполнения, проверку зовущие
	std::vector <std::thread> threads;
	/**
	 * Выполняем заведение потоков выполнения
	 */
	for(size_t i = 0; i < THREADS; i++){
		// Выполняем заведение очередного потока выполнения
		threads.emplace_back([&, i]() noexcept {
			/**
			 * Выполняем перебор проверок подписи
			 */
			for(size_t k = 0; k < ROUNDS; k++){
				// Порядковый номер ключа, которым идёт проверка
				const size_t index = ((i + k) % names.size());
				// Тип хэш-суммы, схеме подписи отвечающий
				const awh::crypto_t::hash_t hash = ((kinds.at(index) == awh::crypto_t::signature_t::ED25519) ? awh::crypto_t::hash_t::NONE : awh::crypto_t::hash_t::SHA256);
				/**
				 * Проверка зовётся у одного и того же объекта из всех потоков разом,
				 * и ключи при этом берутся разные: связка читается вперемешку
				 */
				// Если подпись принята проверкой
				if(this->_crypto->verify(names.at(index), reinterpret_cast <const uint8_t *> (text.data()), text.size(), signatures.at(index), hash))
					// Считаем принятую проверкой подпись
					accepted.fetch_add(1);
				// Если подпись проверкой отвергнута
				else rejected.fetch_add(1);
			}
		});
	}
	/**
	 * Выполняем ожидание завершения потоков выполнения
	 */
	for(auto & thread : threads)
		// Выполняем ожидание завершения очередного потока выполнения
		thread.join();
	// Проверяем, что все подписи приняты проверкой
	EXPECT_EQ(accepted.load(), (THREADS * ROUNDS));
	// Проверяем, что ни одна подпись не отвергнута
	EXPECT_EQ(rejected.load(), static_cast <size_t> (0));
	/**
	 * Связка после одновременного чтения обязана остаться прежней: работа проверки её
	 * не трогает вовсе
	 */
	for(size_t i = 0; i < names.size(); i++)
		// Проверяем сохранность вида подписи ключа связки
		EXPECT_EQ(this->_crypto->signature(names.at(i)), kinds.at(i)) << "name = " << names.at(i);
}

/**
 * @brief Тест поточной проверки подписи
 *
 * @details Поточная проверка нужна чужой записи, подписанной целиком: своя работа
 *          подписывает короткую свёртку, а чужая в память может и не подняться
 *
 */
TEST_F(CryptoFixture, SignatureStreamVerifyCryptoTest){
	// Данные для подписи
	const std::string text = "Anyks Framework, Hello World!!! Anyks Framework, Hello World!!!";
	// Устанавливаем схему дополнения подписи
	this->_crypto->padding(awh::crypto_t::padding_t::PSS);
	/**
	 * Выполняем перебор видов подписи, поточную работу имеющих
	 */
	for(auto & kind : {awh::crypto_t::signature_t::RSA, awh::crypto_t::signature_t::ECDSA}){
		// Выполняем выработку ключа владельца
		ASSERT_TRUE(this->_crypto->generateKey("owner", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Выполняем выработку ключа постороннего
		ASSERT_TRUE(this->_crypto->generateKey("stranger", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Подпись данных буфером целиком
		std::vector <uint8_t> signature;
		// Выполняем подписание данных буфером целиком
		ASSERT_TRUE(this->_crypto->sign("owner", reinterpret_cast <const uint8_t *> (text.data()), text.size(), awh::crypto_t::hash_t::SHA256, signature)) << "kind = " << static_cast <uint16_t> (kind);
		/**
		 * Поточная проверка обязана принять подпись, выработанную буфером целиком: иначе
		 * чужую запись, подписанную разовой работой, проверить потоком было бы нельзя
		 */
		// Выполняем заведение потока проверки подписи
		ASSERT_TRUE(this->_crypto->verifyInitialize("owner", awh::crypto_t::hash_t::SHA256)) << "kind = " << static_cast <uint16_t> (kind);
		/**
		 * Выполняем подачу данных потоку по одному октету
		 */
		for(size_t i = 0; i < text.size(); i++)
			// Выполняем подачу очередного октета потоку проверки
			ASSERT_TRUE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (text.data() + i), 1)) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем принятие подписи поточной проверкой
		EXPECT_TRUE(this->_crypto->verifyFinalize(signature)) << "kind = " << static_cast <uint16_t> (kind);
		// Подпись, испорченная на один разряд
		std::vector <uint8_t> tampered = signature;
		// Выполняем порчу одного разряда подписи
		tampered[tampered.size() / 2] ^= 0x01;
		// Выполняем заведение потока проверки подписи
		ASSERT_TRUE(this->_crypto->verifyInitialize("owner", awh::crypto_t::hash_t::SHA256)) << "kind = " << static_cast <uint16_t> (kind);
		// Выполняем подачу данных потоку проверки
		ASSERT_TRUE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (text.data()), text.size())) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем отказ подписи, испорченной на разряд
		EXPECT_FALSE(this->_crypto->verifyFinalize(tampered)) << "kind = " << static_cast <uint16_t> (kind);
		// Выполняем заведение потока проверки подписи чужим ключом
		ASSERT_TRUE(this->_crypto->verifyInitialize("stranger", awh::crypto_t::hash_t::SHA256)) << "kind = " << static_cast <uint16_t> (kind);
		// Выполняем подачу данных потоку проверки
		ASSERT_TRUE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (text.data()), text.size())) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем отказ подписи, проверяемой чужим ключом
		EXPECT_FALSE(this->_crypto->verifyFinalize(signature)) << "kind = " << static_cast <uint16_t> (kind);
		/**
		 * Испорченный поток обязан быть отвергнут наравне с испорченной подписью: подпись
		 * покрывает поданное целиком, и потеря одного разряда подачи её рушит
		 */
		// Поток, испорченный на один разряд
		std::string message = text;
		// Выполняем порчу одного разряда потока
		message[message.size() / 2] = static_cast <char> (message[message.size() / 2] ^ 0x01);
		// Выполняем заведение потока проверки подписи
		ASSERT_TRUE(this->_crypto->verifyInitialize("owner", awh::crypto_t::hash_t::SHA256)) << "kind = " << static_cast <uint16_t> (kind);
		// Выполняем подачу испорченного потока проверке
		ASSERT_TRUE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (message.data()), message.size())) << "kind = " << static_cast <uint16_t> (kind);
		// Проверяем отказ подписи испорченного на разряд потока
		EXPECT_FALSE(this->_crypto->verifyFinalize(signature)) << "kind = " << static_cast <uint16_t> (kind);
	}
	// Выполняем выработку ключа Ed25519
	ASSERT_TRUE(this->_crypto->generateKey("pure", awh::crypto_t::signature_t::ED25519));
	// Проверяем отказ заведения потока проверки видом, поточным не бывающим
	EXPECT_FALSE(this->_crypto->verifyInitialize("pure", awh::crypto_t::hash_t::SHA256));
	// Проверяем отказ подачи в поток проверки, заведения не прошедший
	EXPECT_FALSE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (text.data()), text.size()));
	// Проверяем отказ завершения потока проверки, заведения не прошедшего
	EXPECT_FALSE(this->_crypto->verifyFinalize(std::vector <uint8_t> (64, 0)));
}

/**
 * @brief Тест раздельности потоков подписи и её проверки
 *
 * @details Поток на объекте один, а работы у него две, и контексты их между собой не
 *          взаимозаменяемы: подача проверяемого в поток выработки прошла бы молча, а
 *          подпись вышла бы не той, которой ждут
 *
 */
TEST_F(CryptoFixture, SignatureStreamDirectionCryptoTest){
	// Данные для подписи
	const std::string text = "Anyks Framework, Hello World!!!";
	// Буфер подписи
	std::vector <uint8_t> signature;
	// Выполняем выработку ключа подписи
	ASSERT_TRUE(this->_crypto->generateKey("owner", awh::crypto_t::signature_t::ECDSA));
	// Выполняем заведение потока выработки подписи
	ASSERT_TRUE(this->_crypto->signInitialize("owner", awh::crypto_t::hash_t::SHA256));
	// Проверяем отказ подачи в поток проверки, заведённый выработкой
	EXPECT_FALSE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (text.data()), text.size()));
	// Проверяем отказ завершения проверки у потока, заведённого выработкой
	EXPECT_FALSE(this->_crypto->verifyFinalize(std::vector <uint8_t> (72, 0)));
	// Выполняем подачу данных потоку выработки подписи
	ASSERT_TRUE(this->_crypto->signUpdate(reinterpret_cast <const uint8_t *> (text.data()), text.size()));
	// Выполняем завершение потока выработки подписи
	ASSERT_TRUE(this->_crypto->signFinalize(signature));
	// Выполняем заведение потока проверки подписи
	ASSERT_TRUE(this->_crypto->verifyInitialize("owner", awh::crypto_t::hash_t::SHA256));
	// Проверяем отказ подачи в поток выработки, заведённый проверкой
	EXPECT_FALSE(this->_crypto->signUpdate(reinterpret_cast <const uint8_t *> (text.data()), text.size()));
	// Буфер подписи потока, заведённого проверкой
	std::vector <uint8_t> denied;
	// Проверяем отказ завершения выработки у потока, заведённого проверкой
	EXPECT_FALSE(this->_crypto->signFinalize(denied));
	// Выполняем подачу данных потоку проверки подписи
	ASSERT_TRUE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (text.data()), text.size()));
	// Проверяем принятие подписи поточной проверкой
	EXPECT_TRUE(this->_crypto->verifyFinalize(signature));
	/**
	 * Заведение потока поверх прежнего прежний сбрасывает: продолжать его нечем, и
	 * завершение его обязано отказать
	 */
	// Выполняем заведение потока выработки подписи
	ASSERT_TRUE(this->_crypto->signInitialize("owner", awh::crypto_t::hash_t::SHA256));
	// Выполняем заведение потока проверки поверх потока выработки
	ASSERT_TRUE(this->_crypto->verifyInitialize("owner", awh::crypto_t::hash_t::SHA256));
	// Проверяем отказ завершения выработки у сброшенного потока
	EXPECT_FALSE(this->_crypto->signFinalize(denied));
}

/**
 * @brief Тест оглашения сброса незавершённого потока подписи
 *
 * @details Заведение потока поверх незавершённого прежний сбрасывает, а не отвергает
 *          заведение: тем же порядком ведёт себя поток шифрования, и почерк у модуля
 *          один - оглашают сброс оба, см. StreamDiscardCryptoTest. Но сброс этот оглашается: незавершённый поток несёт поданное в него, и
 *          работа эта пропадает - молчаливая же пропажа выглядит работающим обиходом,
 *          покуда кто-нибудь не хватится подписи, которой нет
 *
 */
TEST_F(CryptoFixture, SignatureStreamDiscardCryptoTest){
	// Количество полученных предупреждений
	size_t records = 0;
	// Выполняем выработку ключа подписи
	ASSERT_TRUE(this->_crypto->generateKey("owner", awh::crypto_t::signature_t::ECDSA));
	// Выполняем подписку на записи лога
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
	// Выполняем заведение потока подписи
	ASSERT_TRUE(this->_crypto->signInitialize("owner", awh::crypto_t::hash_t::SHA256));
	// Проверяем, что заведение первого потока предупреждения не даёт
	EXPECT_EQ(records, static_cast <size_t> (0));
	// Выполняем подачу данных потоку подписи
	ASSERT_TRUE(this->_crypto->signUpdate(reinterpret_cast <const uint8_t *> ("Anyks"), 5));
	// Выполняем заведение потока подписи поверх незавершённого
	ASSERT_TRUE(this->_crypto->signInitialize("owner", awh::crypto_t::hash_t::SHA256));
	// Проверяем, что сброс незавершённого потока оглашён
	EXPECT_EQ(records, static_cast <size_t> (1));
	// Выполняем заведение потока проверки поверх незавершённого потока подписи
	ASSERT_TRUE(this->_crypto->verifyInitialize("owner", awh::crypto_t::hash_t::SHA256));
	// Проверяем, что сброс незавершённого потока оглашён и здесь
	EXPECT_EQ(records, static_cast <size_t> (2));
	// Выполняем завершение потока проверки подписи
	EXPECT_FALSE(this->_crypto->verifyFinalize(std::vector <uint8_t> (72, 0)));
	// Выполняем заведение потока подписи на завершённом потоке
	ASSERT_TRUE(this->_crypto->signInitialize("owner", awh::crypto_t::hash_t::SHA256));
	// Проверяем, что заведение на завершённом потоке предупреждения не даёт
	EXPECT_EQ(records, static_cast <size_t> (2));
}

/**
 * @brief Тест оглашения разрядности по видам подписи
 *
 * @details Порог разрядности взят у RSA и прочим схемам не отвечает: ключ Ed25519 имеет
 *          253 разряда, а ключ на кривой P-256 - 256, и оба заведомо ниже порога RSA,
 *          хотя стойкость их с ним сравнима. Разрядность у этих схем задана самой схемой
 *          и вызывающей стороной не выбирается: оглашение было бы предупреждением о том,
 *          что исправить нельзя и не нужно
 *
 */
TEST_F(CryptoFixture, SignatureStrengthNoiseCryptoTest){
	// Количество полученных предупреждений
	size_t records = 0;
	// Записи ключей, подлежащих вводу
	std::vector <std::string> keys;
	/**
	 * Выполняем выработку ключей схем, разрядность которых задана самой схемой
	 */
	for(auto & kind : {awh::crypto_t::signature_t::ED25519, awh::crypto_t::signature_t::ECDSA}){
		// Выполняем выработку ключа подписи
		ASSERT_TRUE(this->_crypto->generateKey("origin", kind)) << "kind = " << static_cast <uint16_t> (kind);
		// Получаем запись закрытого ключа
		keys.push_back(this->_crypto->getKey("origin", awh::crypto_t::key_type_t::PRIVATE));
		// Проверяем получение записи закрытого ключа
		ASSERT_FALSE(keys.back().empty()) << "kind = " << static_cast <uint16_t> (kind);
	}
	// Выполняем подписку на записи лога
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
	/**
	 * Выполняем ввод выработанных ключей
	 */
	for(size_t i = 0; i < keys.size(); i++)
		// Проверяем ввод записи закрытого ключа
		ASSERT_TRUE(this->_crypto->setKey("imported", keys.at(i), awh::crypto_t::key_type_t::PRIVATE)) << "index = " << i;
	// Проверяем, что разрядность схем с заданной разрядностью не оглашается
	EXPECT_EQ(records, static_cast <size_t> (0));
}

/**
 * @brief Тест взаимного признания подписи ГОСТ с чужой работой
 *
 * @details Закрепляет числа, выработанные сторонней реализацией (gost-engine на
 *          стенде Debian): открытый ключ, хэш-сумма и подпись взяты у неё, а
 *          признаются здесь. Проверка эта дороже всех прочих: самосогласованность
 *          своей реализации ничего не стоит, если подпись не принимает никто другой
 *
 */
TEST_F(CryptoFixture, GostForeignVectorCryptoTest){
	// Закрытый ключ, выработанный чужой работой
	static const char * PRIVATE =
		"-----BEGIN PRIVATE KEY-----\n"
		"MEYCAQAwHwYIKoUDBwEBAQEwEwYHKoUDAgIjAQYIKoUDBwEBAgIEII7TH1p3P098\n"
		"4H99bbFdagXUqBDzk3uXh4sQ6zjA45+C\n"
		"-----END PRIVATE KEY-----\n";
	// Выполняем ввод закрытого ключа чужой работы
	ASSERT_TRUE(this->_crypto->setKey("foreign", PRIVATE, awh::crypto_t::key_type_t::PRIVATE));
	// Проверяем опознание вида подписи введённого ключа
	EXPECT_EQ(this->_crypto->signature("foreign"), awh::crypto_t::signature_t::GOST);
	// Подписываемое сообщение
	static const char * MESSAGE = "проба взаимного признания подписи";
	// Подпись, выработанная чужой работой
	static const uint8_t SIGNATURE[64] = {
		0xd5, 0x28, 0x4f, 0x11, 0x8c, 0x9f, 0xc7, 0x0d, 0xfa, 0xb2, 0xa6, 0x23, 0x6f, 0x79, 0x0c, 0xbd,
		0x7f, 0xd6, 0x62, 0xdf, 0x3f, 0x17, 0xdb, 0x15, 0x71, 0x19, 0x93, 0x2e, 0x86, 0xae, 0xf4, 0x31,
		0x73, 0x9d, 0xbc, 0x4f, 0x24, 0xaf, 0x81, 0xc3, 0x96, 0x40, 0x65, 0x0c, 0x90, 0xd7, 0x11, 0xf4,
		0x49, 0x51, 0xd9, 0x60, 0x3c, 0x1a, 0x8f, 0x4e, 0xca, 0x2c, 0x08, 0xc3, 0xe8, 0x2d, 0xb6, 0xb0
	};
	// Набор подписи чужой работы
	const std::vector <uint8_t> signature(SIGNATURE, SIGNATURE + sizeof(SIGNATURE));
	// Проверяем принятие подписи чужой работы
	EXPECT_TRUE(this->_crypto->verify("foreign", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), signature, awh::crypto_t::hash_t::NONE));
	/**
	 * Выполняем перебор всех октетов подписи
	 */
	for(size_t i = 0; i < signature.size(); i++){
		// Испорченная на разряд подпись
		std::vector <uint8_t> broken = signature;
		// Выполняем порчу очередного октета подписи
		broken[i] ^= 0x01;
		// Проверяем отказ испорченной подписи
		EXPECT_FALSE(this->_crypto->verify("foreign", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), broken, awh::crypto_t::hash_t::NONE)) << "octet = " << i;
	}
	// Испорченное на разряд сообщение
	std::string spoiled(MESSAGE);
	// Выполняем порчу сообщения
	spoiled[3] ^= 0x01;
	// Проверяем отказ испорченного сообщения
	EXPECT_FALSE(this->_crypto->verify("foreign", reinterpret_cast <const uint8_t *> (spoiled.data()), spoiled.size(), signature, awh::crypto_t::hash_t::NONE));
}

/**
 * @brief Тест переносимости отпечатка ключа ГОСТ
 *
 * @details Отпечаток обязан считаться от той же канонической записи, что и у прочих
 *          видов подписи, иначе опознание владельца у ГОСТ вышло бы своим и
 *          непереносимым. Закрепляется значение, сходящееся с записью, которую
 *          читает сторонняя реализация
 *
 */
TEST_F(CryptoFixture, GostFingerprintCryptoTest){
	// Открытый ключ, выработанный чужой работой
	static const char * PUBLIC =
		"-----BEGIN PUBLIC KEY-----\n"
		"MGYwHwYIKoUDBwEBAQEwEwYHKoUDAgIjAQYIKoUDBwEBAgIDQwAEQAXLvVqRmvXA\n"
		"T3ScADd4NoiatJ94U4wk9+WxLfQ2UDjTW1I7rKPZvfRVmPDwpGemk813CKob/njp\n"
		"zGLsfMCXZ68=\n"
		"-----END PUBLIC KEY-----\n";
	// Выполняем ввод открытого ключа чужой работы
	ASSERT_TRUE(this->_crypto->setKey("foreign", PUBLIC, awh::crypto_t::key_type_t::PUBLIC));
	// Получаем отпечаток открытого ключа
	const std::string fingerprint = this->_crypto->fingerprint <std::string> ("foreign", awh::crypto_t::format_t::HEX);
	// Проверяем длину отпечатка
	EXPECT_EQ(fingerprint.size(), static_cast <size_t> (64));
	/**
	 * Отпечаток закрепляется числом, взятым у сторонней реализации: она выписывает
	 * тот же ключ канонической записью и берёт от неё ту же хэш-сумму. Без этого
	 * закрепления неверная, но самосогласованная запись прошла бы проверку - сличение
	 * своих отпечатков между собой такой ошибки не ловит вовсе
	 */
	// Проверяем совпадение отпечатка со снятым у сторонней реализации
	EXPECT_EQ(fingerprint, "27fad764d3694ad584c9b23cbfd57bcee84fda9dbefd33b0797f41277046268b");
	// Проверяем неизменность отпечатка от повторного получения
	EXPECT_EQ(this->_crypto->fingerprint <std::string> ("foreign", awh::crypto_t::format_t::HEX), fingerprint);
	// Выполняем повторный ввод того же ключа под иным именем
	ASSERT_TRUE(this->_crypto->setKey("second", PUBLIC, awh::crypto_t::key_type_t::PUBLIC));
	// Проверяем совпадение отпечатков одного и того же ключа
	EXPECT_EQ(this->_crypto->fingerprint <std::string> ("second", awh::crypto_t::format_t::HEX), fingerprint);
	// Выполняем выработку своего ключа ГОСТ
	ASSERT_TRUE(this->_crypto->generateKey("own", awh::crypto_t::signature_t::GOST));
	// Проверяем несовпадение отпечатков разных ключей
	EXPECT_NE(this->_crypto->fingerprint <std::string> ("own", awh::crypto_t::format_t::HEX), fingerprint);
}

/**
 * @brief Тест обмена ключами ГОСТ через записи PEM
 *
 * @details Закрепляет, что выписанный ключ читается обратно и даёт тот же отпечаток,
 *          а закрытая часть даёт ту же открытую. Записи эти читает и сторонняя
 *          реализация - проверено на стенде, см. DECISIONS 5б.6
 *
 */
TEST_F(CryptoFixture, GostKeyStorageCryptoTest){
	// Выполняем выработку ключа подписи
	ASSERT_TRUE(this->_crypto->generateKey("own", awh::crypto_t::signature_t::GOST));
	// Получаем отпечаток выработанного ключа
	const std::string fingerprint = this->_crypto->fingerprint <std::string> ("own", awh::crypto_t::format_t::HEX);
	// Выполняем выписку закрытого ключа
	const std::string secret = this->_crypto->getKey("own", awh::crypto_t::key_type_t::PRIVATE);
	// Выполняем выписку открытого ключа
	const std::string open = this->_crypto->getKey("own", awh::crypto_t::key_type_t::PUBLIC);
	// Проверяем наличие заголовка записи закрытого ключа
	EXPECT_NE(secret.find("-----BEGIN PRIVATE KEY-----"), std::string::npos);
	// Проверяем наличие заголовка записи открытого ключа
	EXPECT_NE(open.find("-----BEGIN PUBLIC KEY-----"), std::string::npos);
	// Выполняем ввод выписанного закрытого ключа
	ASSERT_TRUE(this->_crypto->setKey("restored", secret, awh::crypto_t::key_type_t::PRIVATE));
	// Проверяем совпадение отпечатка восстановленного ключа
	EXPECT_EQ(this->_crypto->fingerprint <std::string> ("restored", awh::crypto_t::format_t::HEX), fingerprint);
	// Выполняем ввод выписанного открытого ключа
	ASSERT_TRUE(this->_crypto->setKey("opened", open, awh::crypto_t::key_type_t::PUBLIC));
	// Проверяем совпадение отпечатка открытого ключа
	EXPECT_EQ(this->_crypto->fingerprint <std::string> ("opened", awh::crypto_t::format_t::HEX), fingerprint);
	// Подписываемое сообщение
	static const char * MESSAGE = "содержимое, подписанное восстановленным ключом";
	// Набор выработанной подписи
	std::vector <uint8_t> signature;
	// Выполняем выработку подписи восстановленным ключом
	ASSERT_TRUE(this->_crypto->sign("restored", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), awh::crypto_t::hash_t::NONE, signature));
	// Проверяем проверку подписи открытым ключом
	EXPECT_TRUE(this->_crypto->verify("opened", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), signature, awh::crypto_t::hash_t::NONE));
	/**
	 * Ключ без закрытой части подписывать не может, и отказ этот обязан быть явным:
	 * молчаливая выдача пустой подписи прошла бы у потребителя за подписанное
	 */
	// Набор подписи открытым ключом
	std::vector <uint8_t> refused;
	// Проверяем отказ выработки подписи ключом без закрытой части
	EXPECT_FALSE(this->_crypto->sign("opened", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), awh::crypto_t::hash_t::NONE, refused));
	// Проверяем пустоту буфера подписи после отказа
	EXPECT_TRUE(refused.empty());
}

/**
 * @brief Тест договора хэш-функции у подписи ГОСТ
 *
 * @details Схема ГОСТ предписывает себе хэш-функцию сама, оттого поданный тип
 *          хэш-суммы отвергается: приняв его молча, работа подписала бы предписанным,
 *          а не поданным, и потребитель считал бы подпись иной, чем она есть
 *
 */
TEST_F(CryptoFixture, GostHashContractCryptoTest){
	// Выполняем выработку ключа подписи
	ASSERT_TRUE(this->_crypto->generateKey("own", awh::crypto_t::signature_t::GOST));
	// Подписываемое сообщение
	static const char * MESSAGE = "договор хэш-функции";
	// Набор выработанной подписи
	std::vector <uint8_t> signature;
	/**
	 * Выполняем перебор всех типов хэш-суммы библиотеки криптографии
	 */
	for(auto & hash : {awh::crypto_t::hash_t::MD5, awh::crypto_t::hash_t::SHA1, awh::crypto_t::hash_t::SHA256, awh::crypto_t::hash_t::SHA512}){
		// Проверяем отказ выработки подписи с поданным типом хэш-суммы
		EXPECT_FALSE(this->_crypto->sign("own", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), hash, signature)) << "hash = " << static_cast <uint16_t> (hash);
		// Проверяем отказ заведения потока подписи с поданным типом хэш-суммы
		EXPECT_FALSE(this->_crypto->signInitialize("own", hash)) << "hash = " << static_cast <uint16_t> (hash);
	}
	// Проверяем выработку подписи при пустом типе хэш-суммы
	EXPECT_TRUE(this->_crypto->sign("own", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), awh::crypto_t::hash_t::NONE, signature));
	// Проверяем постоянство длины подписи
	EXPECT_EQ(signature.size(), static_cast <size_t> (64));
	// Проверяем совпадение длины подписи с заявленной
	EXPECT_EQ(this->_crypto->length("own"), signature.size());
	// Проверяем совпадение предела длины подписи с заявленной
	EXPECT_EQ(this->_crypto->limit("own"), signature.size());
}

/**
 * @brief Тест независимости поточной подписи ГОСТ от нарезки на куски
 *
 * @details Поточная подпись обязана совпадать с разовой при всякой нарезке потока:
 *          состояние счёта хэш-суммы накапливает неполный блок, и ошибка накопления
 *          дала бы подпись, зависящую от размера порции, - а порции задаёт вызывающая
 *          сторона, и воспроизвести такую подпись было бы нечем
 *
 */
TEST_F(CryptoFixture, GostStreamChunkCryptoTest){
	// Выполняем выработку ключа подписи
	ASSERT_TRUE(this->_crypto->generateKey("own", awh::crypto_t::signature_t::GOST));
	// Подписываемое содержимое
	std::string content;
	/**
	 * Выполняем наполнение подписываемого содержимого
	 */
	for(size_t i = 0; i < 5000; i++)
		// Выполняем добавление очередного знака содержимого
		content.append(1, static_cast <char> ('a' + (i % 26)));
	/**
	 * Выполняем перебор всех размеров куска подачи
	 */
	for(auto & chunk : {static_cast <size_t> (1), static_cast <size_t> (7), static_cast <size_t> (63), static_cast <size_t> (64), static_cast <size_t> (65), static_cast <size_t> (333), static_cast <size_t> (8192)}){
		// Выполняем заведение потока выработки подписи
		ASSERT_TRUE(this->_crypto->signInitialize("own", awh::crypto_t::hash_t::NONE)) << "chunk = " << chunk;
		/**
		 * Выполняем подачу содержимого кусками
		 */
		for(size_t offset = 0; offset < content.size(); offset += chunk){
			// Определяем размер очередного куска
			const size_t size = (((content.size() - offset) < chunk) ? (content.size() - offset) : chunk);
			// Выполняем подачу очередного куска в поток
			ASSERT_TRUE(this->_crypto->signUpdate(reinterpret_cast <const uint8_t *> (content.data() + offset), size)) << "chunk = " << chunk;
		}
		// Набор выработанной подписи
		std::vector <uint8_t> signature;
		// Выполняем завершение потока выработки подписи
		ASSERT_TRUE(this->_crypto->signFinalize(signature)) << "chunk = " << chunk;
		/**
		 * Поточная подпись сличается разовой проверкой, а не с числами другой подписи:
		 * схема ГОСТ случайна, и две подписи одного содержимого числами не совпадают
		 */
		// Проверяем разовую проверку поточной подписи
		EXPECT_TRUE(this->_crypto->verify("own", reinterpret_cast <const uint8_t *> (content.data()), content.size(), signature, awh::crypto_t::hash_t::NONE)) << "chunk = " << chunk;
		// Выполняем заведение потока проверки подписи
		ASSERT_TRUE(this->_crypto->verifyInitialize("own", awh::crypto_t::hash_t::NONE)) << "chunk = " << chunk;
		/**
		 * Выполняем подачу содержимого кусками в поток проверки
		 */
		for(size_t offset = 0; offset < content.size(); offset += chunk){
			// Определяем размер очередного куска
			const size_t size = (((content.size() - offset) < chunk) ? (content.size() - offset) : chunk);
			// Выполняем подачу очередного куска в поток проверки
			ASSERT_TRUE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (content.data() + offset), size)) << "chunk = " << chunk;
		}
		// Проверяем поточную проверку поточной подписи
		EXPECT_TRUE(this->_crypto->verifyFinalize(signature)) << "chunk = " << chunk;
	}
}

/**
 * @brief Тест поточной работы схемы ГОСТ
 *
 * @details Закрепляет справку о поточности и невзаимозаменяемость потоков выработки
 *          и проверки: подача проверяемого в поток выработки принята была бы молча
 *
 */
TEST_F(CryptoFixture, GostStreamDirectionCryptoTest){
	// Проверяем справку о поточной работе схемы ГОСТ
	EXPECT_TRUE(this->_crypto->streamable(awh::crypto_t::signature_t::GOST));
	// Выполняем выработку ключа подписи
	ASSERT_TRUE(this->_crypto->generateKey("own", awh::crypto_t::signature_t::GOST));
	// Подписываемое сообщение
	static const char * MESSAGE = "направление потока";
	// Набор выработанной подписи
	std::vector <uint8_t> signature;
	// Выполняем заведение потока выработки подписи
	ASSERT_TRUE(this->_crypto->signInitialize("own", awh::crypto_t::hash_t::NONE));
	// Выполняем подачу сообщения в поток выработки
	ASSERT_TRUE(this->_crypto->signUpdate(reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE)));
	// Проверяем отказ подачи в поток проверки, заведённый как поток выработки
	EXPECT_FALSE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE)));
	// Выполняем завершение потока выработки подписи
	ASSERT_TRUE(this->_crypto->signFinalize(signature));
	// Выполняем заведение потока проверки подписи
	ASSERT_TRUE(this->_crypto->verifyInitialize("own", awh::crypto_t::hash_t::NONE));
	// Выполняем подачу сообщения в поток проверки
	ASSERT_TRUE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE)));
	// Проверяем отказ завершения потока выработки, заведённого как поток проверки
	EXPECT_FALSE(this->_crypto->signFinalize(signature));
}

/**
 * @brief Тест оглашения сброса незавершённого потока шифрования
 *
 * @details Поток шифрования сбрасывался при повторном заведении молча, тогда как поток
 *          подписи сброс оглашал (5а.18). Расхождение это правится в пользу оглашения:
 *          незавершённый поток несёт поданное в него, и работа эта пропадает, а
 *          молчаливая пропажа выглядит работающим обиходом до тех пор, пока кто-нибудь
 *          не хватится записи, которой нет
 *
 */
TEST_F(CryptoFixture, StreamDiscardCryptoTest){
	// Количество полученных предупреждений
	size_t records = 0;
	/**
	 * Пароль берётся стойким намеренно: слабый даёт своё предупреждение, и счётчик
	 * записей считал бы его вместе с оглашением сброса потока
	 */
	// Выполняем установку пароля шифрования
	this->_crypto->password("Qw8#zR2!vN5@hL7$pM3&kT6^");
	/**
	 * Соль задаётся намеренно: вывод ключа без неё даёт своё законное предупреждение,
	 * и счётчик записей считал бы его вместе с оглашением сброса потока
	 */
	// Выполняем установку соли шифрования
	this->_crypto->salt("j4Hs9Wk2Lp7Qz5Xr");
	// Выполняем подписку на записи лога
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
	// Выполняем заведение потока шифрования
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем, что заведение первого потока предупреждения не даёт
	EXPECT_EQ(records, static_cast <size_t> (0));
	// Набор зашифрованного содержимого
	std::vector <uint8_t> buffer;
	// Выполняем подачу данных потоку шифрования
	ASSERT_TRUE(this->_crypto->encrypt(reinterpret_cast <const uint8_t *> ("Anyks"), 5, buffer));
	// Выполняем заведение потока шифрования поверх незавершённого
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем, что сброс незавершённого потока оглашён
	EXPECT_EQ(records, static_cast <size_t> (1));
	// Выполняем завершение потока шифрования
	ASSERT_TRUE(this->_crypto->finalize(buffer));
	// Выполняем заведение потока шифрования на завершённом потоке
	ASSERT_TRUE(this->_crypto->initialize(awh::crypto_t::event_t::ENCODE, awh::crypto_t::hash_t::SHA256, awh::crypto_t::cipher_t::AES256));
	// Проверяем, что заведение на завершённом потоке предупреждения не даёт
	EXPECT_EQ(records, static_cast <size_t> (1));
}

/**
 * @brief Тест хэш-функции ГОСТ Р 34.11-2012
 *
 * @details Закрепляет числа сторонней реализации: значения выработаны gost-engine и
 *          обязаны совпадать октет в октет, иначе свёртка, посчитанная нами, чужой
 *          работой опознана не будет
 *
 */
TEST_F(CryptoFixture, StreebogHashCryptoTest){
	// Проверяем хэш-сумму на 256 разрядов
	EXPECT_EQ(this->_crypto->hash <std::string> (std::string_view("test message"), awh::crypto_t::hash_t::STREEBOG256),
	 "0639b3cbb205941a9811f329d893057881382c5ab11d3e7e8423ec0a7a196bbb");
	// Проверяем хэш-сумму на 512 разрядов
	EXPECT_EQ(this->_crypto->hash <std::string> (std::string_view("test message"), awh::crypto_t::hash_t::STREEBOG512),
	 "600c1335c84f405ea2a8c10bf3d18dfe1aed19ad847da206d70310a83ab1a8ae444e15692cd3c1dfef9452850b36e8aab3ee46b288d900d0956bff39ec3d1ef5");
	/**
	 * Сообщение, октетов не имеющее, отвергается договором всего фреймворка, и вид
	 * ГОСТ тут не исключение: хэш-сумма его - пустота, а не значение стандарта.
	 * Закрепляется именно это, иначе правка договора прошла бы здесь незамеченной
	 */
	// Проверяем отказ пустого сообщения
	EXPECT_TRUE(this->_crypto->hash <std::string> (std::string_view(""), awh::crypto_t::hash_t::STREEBOG256).empty());
	// Проверяем, что отказ этот общий, а не свойственный виду ГОСТ
	EXPECT_TRUE(this->_crypto->hash <std::string> (std::string_view(""), awh::crypto_t::hash_t::SHA256).empty());
	// Проверяем длину записи хэш-суммы на 256 разрядов
	EXPECT_EQ(this->_crypto->hash <std::string> (std::string_view("anyks"), awh::crypto_t::hash_t::STREEBOG256).size(), static_cast <size_t> (64));
	// Проверяем длину записи хэш-суммы на 512 разрядов
	EXPECT_EQ(this->_crypto->hash <std::string> (std::string_view("anyks"), awh::crypto_t::hash_t::STREEBOG512).size(), static_cast <size_t> (128));
	// Проверяем несовпадение хэш-сумм разных сообщений
	EXPECT_NE(this->_crypto->hash <std::string> (std::string_view("anyks"), awh::crypto_t::hash_t::STREEBOG256),
	 this->_crypto->hash <std::string> (std::string_view("anykt"), awh::crypto_t::hash_t::STREEBOG256));
}

/**
 * @brief Тест имитовставки на хэш-функции ГОСТ Р 34.11-2012
 *
 * @details Построение обычное по RFC 2104, но своими силами: библиотека криптографии
 *          хэш-функции этой не знает. Закрепляются числа сторонней реализации
 *
 */
TEST_F(CryptoFixture, StreebogHmacCryptoTest){
	// Ключ имитовставки из двадцати октетов
	const std::string key(20, static_cast <char> (0x0b));
	// Проверяем имитовставку на 256 разрядов
	EXPECT_EQ(this->_crypto->hmac <std::string> (key, std::string_view("test message"), awh::crypto_t::hash_t::STREEBOG256),
	 "814d737ae33a4776ca8aa5ae4b5d84a1f645478b6cfa8ef675fc6bee19e48407");
	// Проверяем имитовставку на 512 разрядов
	EXPECT_EQ(this->_crypto->hmac <std::string> (key, std::string_view("test message"), awh::crypto_t::hash_t::STREEBOG512),
	 "3105310b85662bb65eae8aa6aaa9e4758a3df4aa69f5dea1d00aa5b0049988d418bd47a7162af19cebdc83691aa6ad68cc772f7fb1b9677f28d0f3270e275118");
	/**
	 * Ключ длиннее блока заменяется своей хэш-суммой, и путь этот проверяется особо:
	 * ошибка в нём видна лишь на длинном ключе
	 */
	// Ключ длиннее блока хэш-функции
	const std::string big(100, static_cast <char> (0x42));
	// Проверяем длину имитовставки на длинном ключе
	EXPECT_EQ(this->_crypto->hmac <std::string> (big, std::string_view("test message"), awh::crypto_t::hash_t::STREEBOG256).size(), static_cast <size_t> (64));
	// Проверяем несовпадение имитовставок с разными ключами
	EXPECT_NE(this->_crypto->hmac <std::string> (key, std::string_view("test message"), awh::crypto_t::hash_t::STREEBOG256),
	 this->_crypto->hmac <std::string> (big, std::string_view("test message"), awh::crypto_t::hash_t::STREEBOG256));
}

/**
 * @brief Тест вывода ключа шифрования на хэш-функции ГОСТ Р 34.11-2012
 *
 * @details Вывод ключа библиотеки криптографии берёт её же хэш-функцию, а этой она не
 *          знает вовсе, оттого вывод строится своими силами по RFC 8018. Закрепляется
 *          не значение ключа, а его пригодность: зашифрованное обязано расшифровываться
 *
 */
TEST_F(CryptoFixture, StreebogCipherCryptoTest){
	// Выполняем установку пароля шифрования
	this->_crypto->password("Qw8#zR2!vN5@hL7$pM3&kT6^");
	// Выполняем установку соли шифрования
	this->_crypto->salt("j4Hs9Wk2Lp7Qz5Xr");
	// Шифруемое содержимое
	const std::string content = "содержимое, зашифрованное на ключе ГОСТ";
	/**
	 * Выполняем перебор обеих разрядностей хэш-функции
	 */
	for(auto & hash : {awh::crypto_t::hash_t::STREEBOG256, awh::crypto_t::hash_t::STREEBOG512}){
		// Выполняем шифрование содержимого
		const std::string sealed = this->_crypto->encrypt <std::string> (content, hash, awh::crypto_t::cipher_t::AES256);
		// Проверяем, что содержимое зашифровано
		ASSERT_FALSE(sealed.empty()) << "hash = " << static_cast <uint16_t> (hash);
		// Проверяем, что зашифрованное от исходного отличается
		EXPECT_NE(sealed, content) << "hash = " << static_cast <uint16_t> (hash);
		// Выполняем расшифровку содержимого
		EXPECT_EQ(this->_crypto->decrypt <std::string> (sealed, hash, awh::crypto_t::cipher_t::AES256), content) << "hash = " << static_cast <uint16_t> (hash);
	}
	/**
	 * Ключ, выведенный разными хэш-функциями, обязан выйти разным: иначе разрядность
	 * хэш-функции на вывод не влияла бы вовсе
	 */
	// Выполняем шифрование хэш-функцией на 256 разрядов
	const std::string first = this->_crypto->encrypt <std::string> (content, awh::crypto_t::hash_t::STREEBOG256, awh::crypto_t::cipher_t::AES256);
	// Проверяем отказ расшифровки ключом иной хэш-функции
	EXPECT_NE(this->_crypto->decrypt <std::string> (first, awh::crypto_t::hash_t::STREEBOG512, awh::crypto_t::cipher_t::AES256), content);
	/**
	 * Обратимость сама по себе вывод ключа не закрепляет: вывод, ошибочный, но
	 * постоянный, зашифрованное собою же и расшифрует. Оттого закрепляется
	 * зашифрованное прежде, на том же пароле и той же соли - расшифровка его
	 * обязана дать исходное содержимое, а всякая правка вывода ключа её сорвёт
	 */
	// Набор зашифрованного прежде, по разрядностям хэш-функции
	static const std::pair <awh::crypto_t::hash_t, const char *> PINNED[2] = {
		{awh::crypto_t::hash_t::STREEBOG256, "321b17e32daebc11beb735ceb7a4159b00c221a5da987f26851384be47a3c2d8fe7b66509d7b0260cb3bf0fd967c827d429095957913def2201f3b6e288c9926edf056fbad5464bcacaaf26616174d9cbd96e432d6c8fdb8ba79173068a91f9588573ca459"},
		{awh::crypto_t::hash_t::STREEBOG512, "f68aaa8cb2098cee8c4479c50ce5bcc8bda00008db326a14281fdf520673783e5cfec96bf97c46510582edfa3e6e917ed846378eb30b407e94aacaa7a59732503a6e2bb75d3e795401fbeb8ff4e7e83f48d1d216c84b0ace032211e9ecda44f5931b23929f"}
	};
	/**
	 * Выполняем перебор зашифрованного прежде
	 */
	for(auto & pinned : PINNED){
		// Набор зашифрованного содержимого
		std::string sealed;
		// Выполняем разбор записи шестнадцатеричным видом
		for(size_t i = 0; pinned.second[i] != '\0'; i += 2){
			// Октет записи
			uint32_t letter = 0;
			// Выполняем считывание октета записи
			::sscanf(pinned.second + i, "%02x", &letter);
			// Выполняем добавление октета в набор
			sealed.append(1, static_cast <char> (letter));
		}
		// Проверяем расшифровку зашифрованного прежде
		EXPECT_EQ(this->_crypto->decrypt <std::string> (sealed, pinned.first, awh::crypto_t::cipher_t::AES256), content) << "hash = " << static_cast <uint16_t> (pinned.first);
	}
}

/**
 * @brief Тест отказа подписи на хэш-функции ГОСТ Р 34.11-2012
 *
 * @details Подписи RSA и ECDSA с этой хэш-функцией не вырабатываются: сочетание не
 *          описано ни одним сводом, а библиотека криптографии её не знает. Отказ обязан
 *          быть явным: подпись, выработанная иной хэш-функцией вместо запрошенной,
 *          прошла бы за запрошенную
 *
 */
TEST_F(CryptoFixture, StreebogSignatureRefusalCryptoTest){
	// Подписываемое сообщение
	static const char * MESSAGE = "сообщение";
	// Набор выработанной подписи
	std::vector <uint8_t> signature;
	/**
	 * Выполняем перебор видов подписи, хэш-функцию принимающих
	 */
	for(auto & kind : {awh::crypto_t::signature_t::RSA, awh::crypto_t::signature_t::ECDSA}){
		// Выполняем выработку ключа подписи
		ASSERT_TRUE(this->_crypto->generateKey("owner", kind)) << "kind = " << static_cast <uint16_t> (kind);
		/**
		 * Выполняем перебор обеих разрядностей хэш-функции
		 */
		for(auto & hash : {awh::crypto_t::hash_t::STREEBOG256, awh::crypto_t::hash_t::STREEBOG512}){
			// Проверяем отказ выработки подписи
			EXPECT_FALSE(this->_crypto->sign("owner", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), hash, signature)) << "kind = " << static_cast <uint16_t> (kind);
			// Проверяем пустоту буфера подписи после отказа
			EXPECT_TRUE(signature.empty()) << "kind = " << static_cast <uint16_t> (kind);
			// Проверяем отказ заведения потока подписи
			EXPECT_FALSE(this->_crypto->signInitialize("owner", hash)) << "kind = " << static_cast <uint16_t> (kind);
		}
	}
}


/**
 * @brief Тест взаимного признания подписи ГОСТ Р 34.10-2012 на 512 разрядов
 *
 * @details Числа выработаны чужой работой - gost-engine v3.0.3, - и закрепляются здесь
 *          целиком: закрытый ключ, открытый ключ и подпись по каждому из трёх наборов
 *          свойств кривых ТК26. Самосогласованность своей работы взаимного признания не
 *          доказывает: подпись, выработанная и проверенная одной лишь этой работой,
 *          прошла бы и при общем для обеих сторон искажении схемы
 *
 */
TEST_F(CryptoFixture, Gost512ForeignVectorCryptoTest){
	// Подписанное чужой работой сообщение
	static const char * MESSAGE = "vzaimnoe priznanie podpisi 512";
	/**
	 * @brief Набор чужой работы по одному набору свойств кривой
	 *
	 */
	struct sample_t {
		// Название набора свойств кривой
		const char * name;
		// Закрытый ключ, выработанный чужой работой
		const char * secret;
		// Открытый ключ, выработанный чужой работой
		const char * opened;
		// Подпись, выработанная чужой работой
		const char * signature;
	};
	// Набор чужой работы по всем наборам свойств кривых
	static const sample_t SAMPLES[3] = {
		{
			"A",
			"-----BEGIN PRIVATE KEY-----\n"
			"MGgCAQAwIQYIKoUDBwEBAQIwFQYJKoUDBwECAQIBBggqhQMHAQECAwRAJQEmmx4F\n"
			"tuN6gPmNlXIYb/tsnLwENKKwaLZrYiNSPUzsdDYaSjOD4WIR4z+ZK16E2Xg8O00Q\n"
			"ShTqIPZ2J6teHA==\n"
			"-----END PRIVATE KEY-----\n",
			"-----BEGIN PUBLIC KEY-----\n"
			"MIGqMCEGCCqFAwcBAQECMBUGCSqFAwcBAgECAQYIKoUDBwEBAgMDgYQABIGAWNvj\n"
			"PRzBH3QC7TO5+h5iy8Emr3BS6Ia0GRA9QK/j1r3+d6BrBOgDHeYuSv/lr77r4m3k\n"
			"ADjrbqqP2xEgiDMixcvriJBl0D5ILvAXhtfztN/LwQFRm+O+pzyybGHx5BeGv3eR\n"
			"pgdR1DjpCL97M4hcKvjYJrcyUEaVdsEGnenuGMk=\n"
			"-----END PUBLIC KEY-----\n",
			"fd1206d0976617f15712014b75e7623bff66cf047bb30464aa67ebd5fc8b682286dc806ddd795c9f8552b970ef9dc5d77e2ac6194eaa7d3a57bc8146a8fa08052f6cf2f0e6e7319e6cec9ef098b20d9ba05f7b93aec963c49e64abad17b9069305f7b2d3796a0f4ff8261797997f41df99a344453042db6c633e0b652071c7c6"
		},
		{
			"B",
			"-----BEGIN PRIVATE KEY-----\n"
			"MGgCAQAwIQYIKoUDBwEBAQIwFQYJKoUDBwECAQICBggqhQMHAQECAwRAIBcStpez\n"
			"hHMLY+qsMWo8+BxtRgDzrQSMu0r/PhNkGvEBDCMZuW7HiGf3gbPALFR1riNfwydU\n"
			"Yr7B8VvMdap+aw==\n"
			"-----END PRIVATE KEY-----\n",
			"-----BEGIN PUBLIC KEY-----\n"
			"MIGqMCEGCCqFAwcBAQECMBUGCSqFAwcBAgECAgYIKoUDBwEBAgMDgYQABIGAs1lM\n"
			"EW4eQsxYZs7Y7+so2P9ytqUep1qQoWdUu+WdXjswGhml8VJPA0kYODxQlWgYHuCr\n"
			"m1W3KU83CuaLuspxbykl8i8tz3fv3Tf4JPP4/SV5oAxZIhIRzXk++4FcW8OJZWR2\n"
			"OzvSxUPpe1tn2gzWMIr1e/H/0CZ3IbJhQMInv1Y=\n"
			"-----END PUBLIC KEY-----\n",
			"4bcd30da1a683dbfd50c076e2d355e50d0d5f71e60bbe71ce8425a82623cd050273d5c579045b86445c909b2c01de081b507266110a5bd78bb351f4b5d7e5e155bd6d960e122ac658e9c54e9a689f8461dc895494b6d49d6175f0d4d3a48406f0d379280abb56319ac8d09cf259920d6daf10c0503ffa6795d0b591501748407"
		},
		{
			"C",
			"-----BEGIN PRIVATE KEY-----\n"
			"MF4CAQAwFwYIKoUDBwEBAQIwCwYJKoUDBwECAQIDBEAN45wOMVFqt/cuzjmx+0si\n"
			"teRxwfyM0W+OLHOAtaSf+rB+bBKgqpi6h+IVRTWvfOcqrHEchptjouew6+Qv1wsF\n"
			"-----END PRIVATE KEY-----\n",
			"-----BEGIN PUBLIC KEY-----\n"
			"MIGgMBcGCCqFAwcBAQECMAsGCSqFAwcBAgECAwOBhAAEgYA2ZuVKAujMbtt6jrUa\n"
			"4PamFtw1mr931eruHaXsS4kBTbSHi+2TzMBHwlTMzWuvfsUbtOvtrW0degBhyZeU\n"
			"P9nDBA1iJbksQD7k8kGRrrL7UndGu+XuSM7cdLGoKIrnc40EzAUj2Fs+K9BtBLrl\n"
			"4JLThfmXIHAa9gk+eqUPUulBNA==\n"
			"-----END PUBLIC KEY-----\n",
			"3c4a753f0cdec3e978e819f16db2ff27c1715af81eaa1a78643ba70ffc6b44bd66682768093a8f726d354da72ad1f7df6785ded133c28af3d94f329d43d4e00f01dbcaf9f495ac8fc2f55687d22756997b3d3a8954eb67ad2921bb03ebf14cf5556cbee987df008efd1731bd43a88a14b5ab30740d5677ebebde37ffc6169d18"
		},
	};
	/**
	 * Выполняем перебор всех наборов чужой работы
	 */
	for(auto & sample : SAMPLES){
		// Набор подписи чужой работы
		std::vector <uint8_t> signature;
		/**
		 * Выполняем разбор записи подписи шестнадцатеричным видом
		 */
		for(size_t i = 0; sample.signature[i] != '\0'; i += 2){
			// Октет записи подписи
			uint32_t octet = 0;
			// Выполняем считывание октета записи подписи
			::sscanf(sample.signature + i, "%02x", &octet);
			// Выполняем добавление октета в набор подписи
			signature.push_back(static_cast <uint8_t> (octet));
		}
		// Проверяем ввод открытого ключа чужой работы
		ASSERT_TRUE(this->_crypto->setKey("opened", sample.opened, awh::crypto_t::key_type_t::PUBLIC)) << sample.name;
		// Проверяем опознание вида подписи по разрядности набора свойств кривой
		EXPECT_EQ(this->_crypto->signature("opened"), awh::crypto_t::signature_t::GOST512) << sample.name;
		// Проверяем постоянную длину подписи схемы
		EXPECT_EQ(this->_crypto->length("opened"), static_cast <size_t> (128)) << sample.name;
		// Проверяем признание подписи чужой работы
		EXPECT_TRUE(this->_crypto->verify("opened", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), signature, awh::crypto_t::hash_t::NONE)) << sample.name;
		/**
		 * Портимые октеты подписи
		 *
		 * @note Порча берётся не по всякому октету, а по краям обоих чисел подписи и
		 *       по их стыку: проверка одной подписи схемы на 512 разрядов стоит около
		 *       75 миллисекунд, и сплошная порча удваивала бы время всего набора
		 *       проверок ради того же самого утверждения
		 */
		static const size_t BROKEN[6] = {0, 1, 63, 64, 65, 127};
		/**
		 * Выполняем перебор портимых октетов подписи
		 */
		for(auto & offset : BROKEN){
			// Набор испорченной подписи
			std::vector <uint8_t> broken = signature;
			// Выполняем порчу очередного октета подписи
			broken[offset] ^= 0x01;
			// Проверяем отказ признания испорченной подписи
			EXPECT_FALSE(this->_crypto->verify("opened", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), broken, awh::crypto_t::hash_t::NONE)) << sample.name << " октет " << offset;
		}
		// Проверяем ввод закрытого ключа чужой работы
		ASSERT_TRUE(this->_crypto->setKey("secret", sample.secret, awh::crypto_t::key_type_t::PRIVATE)) << sample.name;
		// Проверяем опознание вида подписи закрытого ключа
		EXPECT_EQ(this->_crypto->signature("secret"), awh::crypto_t::signature_t::GOST512) << sample.name;
		/**
		 * Отпечаток снимается с обеих частей ключа: открытая часть выводится из закрытой
		 * при вводе, и расхождение отпечатков означало бы неверный вывод
		 */
		EXPECT_EQ(this->_crypto->fingerprint <std::string> ("secret", awh::crypto_t::format_t::HEX),
		 this->_crypto->fingerprint <std::string> ("opened", awh::crypto_t::format_t::HEX)) << sample.name;
		// Набор своей подписи на чужом ключе
		std::vector <uint8_t> own;
		// Проверяем выработку подписи на закрытом ключе чужой работы
		ASSERT_TRUE(this->_crypto->sign("secret", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), awh::crypto_t::hash_t::NONE, own)) << sample.name;
		// Проверяем признание своей подписи открытым ключом чужой работы
		EXPECT_TRUE(this->_crypto->verify("opened", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), own, awh::crypto_t::hash_t::NONE)) << sample.name;
	}
}

/**
 * @brief Тест полного оборота схемы ГОСТ Р 34.10-2012 на 512 разрядов
 *
 * @details Оборот идёт целиком своими силами: выработка ключа, разовая подпись,
 *          поточная подпись разными нарезками, выписывание ключа записями PEM и ввод
 *          его обратно. Взаимное признание с чужой работой закрепляется отдельно
 *
 */
TEST_F(CryptoFixture, Gost512CircleCryptoTest){
	// Подписываемое содержимое
	std::string content;
	// Выполняем набор подписываемого содержимого
	for(size_t i = 0; i < 5000; i++)
		content.append(1, static_cast <char> ('a' + (i % 26)));
	// Проверяем выработку ключа подписи
	ASSERT_TRUE(this->_crypto->generateKey("own", awh::crypto_t::signature_t::GOST512));
	// Проверяем опознание вида подписи
	EXPECT_EQ(this->_crypto->signature("own"), awh::crypto_t::signature_t::GOST512);
	// Проверяем справку о поточности схемы
	EXPECT_TRUE(this->_crypto->streamable(awh::crypto_t::signature_t::GOST512));
	// Проверяем постоянство длины подписи
	EXPECT_EQ(this->_crypto->length("own"), static_cast <size_t> (128));
	// Проверяем совпадение предела с длиной подписи
	EXPECT_EQ(this->_crypto->limit("own"), static_cast <size_t> (128));
	// Набор выработанной подписи
	std::vector <uint8_t> signature;
	// Проверяем выработку подписи
	ASSERT_TRUE(this->_crypto->sign("own", reinterpret_cast <const uint8_t *> (content.data()), content.size(), awh::crypto_t::hash_t::NONE, signature));
	// Проверяем длину выработанной подписи
	EXPECT_EQ(signature.size(), static_cast <size_t> (128));
	// Проверяем признание своей подписи
	EXPECT_TRUE(this->_crypto->verify("own", reinterpret_cast <const uint8_t *> (content.data()), content.size(), signature, awh::crypto_t::hash_t::NONE));
	/**
	 * Тип хэш-суммы схемой предписан, и подача его отвергается: иная хэш-функция
	 * вместо предписанной прошла бы за предписанную
	 */
	EXPECT_FALSE(this->_crypto->sign("own", reinterpret_cast <const uint8_t *> (content.data()), content.size(), awh::crypto_t::hash_t::STREEBOG512, signature));
	// Выписка закрытой части ключа
	const std::string secret = this->_crypto->getKey("own", awh::crypto_t::key_type_t::PRIVATE);
	// Выписка открытой части ключа
	const std::string opened = this->_crypto->getKey("own", awh::crypto_t::key_type_t::PUBLIC);
	// Отпечаток выработанного ключа
	const std::string print = this->_crypto->fingerprint <std::string> ("own", awh::crypto_t::format_t::HEX);
	// Проверяем ввод выписанной закрытой части ключа
	ASSERT_TRUE(this->_crypto->setKey("restored", secret, awh::crypto_t::key_type_t::PRIVATE));
	// Проверяем ввод выписанной открытой части ключа
	ASSERT_TRUE(this->_crypto->setKey("public", opened, awh::crypto_t::key_type_t::PUBLIC));
	// Проверяем совпадение отпечатка восстановленного ключа
	EXPECT_EQ(this->_crypto->fingerprint <std::string> ("restored", awh::crypto_t::format_t::HEX), print);
	// Проверяем совпадение отпечатка открытой части ключа
	EXPECT_EQ(this->_crypto->fingerprint <std::string> ("public", awh::crypto_t::format_t::HEX), print);
	// Проверяем опознание вида подписи восстановленного ключа
	EXPECT_EQ(this->_crypto->signature("restored"), awh::crypto_t::signature_t::GOST512);
	// Набор подписи восстановленным ключом
	std::vector <uint8_t> restored;
	// Проверяем выработку подписи восстановленным ключом
	ASSERT_TRUE(this->_crypto->sign("restored", reinterpret_cast <const uint8_t *> (content.data()), content.size(), awh::crypto_t::hash_t::NONE, restored));
	// Проверяем признание её открытой частью ключа
	EXPECT_TRUE(this->_crypto->verify("public", reinterpret_cast <const uint8_t *> (content.data()), content.size(), restored, awh::crypto_t::hash_t::NONE));
	// Набор подписи, выработать которую нельзя
	std::vector <uint8_t> empty;
	// Проверяем отказ выработки подписи ключом без закрытой части
	EXPECT_FALSE(this->_crypto->sign("public", reinterpret_cast <const uint8_t *> (content.data()), content.size(), awh::crypto_t::hash_t::NONE, empty));
	// Проверяем пустоту буфера подписи после отказа
	EXPECT_TRUE(empty.empty());
	// Размеры порций поточной подачи
	static const size_t CHUNKS[5] = {1, 63, 64, 65, 8192};
	/**
	 * Выполняем перебор всех размеров порций поточной подачи
	 */
	for(auto & chunk : CHUNKS){
		// Проверяем заведение потока выработки подписи
		ASSERT_TRUE(this->_crypto->signInitialize("own", awh::crypto_t::hash_t::NONE)) << "порция " << chunk;
		/**
		 * Выполняем подачу подписываемого содержимого порциями
		 */
		for(size_t offset = 0; offset < content.size(); offset += chunk){
			// Определяем размер очередной порции подачи
			const size_t size = (((content.size() - offset) < chunk) ? (content.size() - offset) : chunk);
			// Проверяем подачу очередной порции содержимого
			ASSERT_TRUE(this->_crypto->signUpdate(reinterpret_cast <const uint8_t *> (content.data() + offset), size)) << "порция " << chunk;
		}
		// Набор подписи, выработанной потоком
		std::vector <uint8_t> stream;
		// Проверяем завершение потока выработки подписи
		ASSERT_TRUE(this->_crypto->signFinalize(stream)) << "порция " << chunk;
		// Проверяем длину выработанной потоком подписи
		EXPECT_EQ(stream.size(), static_cast <size_t> (128)) << "порция " << chunk;
		// Проверяем признание выработанной потоком подписи разовым путём
		EXPECT_TRUE(this->_crypto->verify("own", reinterpret_cast <const uint8_t *> (content.data()), content.size(), stream, awh::crypto_t::hash_t::NONE)) << "порция " << chunk;
		// Проверяем заведение потока проверки подписи
		ASSERT_TRUE(this->_crypto->verifyInitialize("own", awh::crypto_t::hash_t::NONE)) << "порция " << chunk;
		/**
		 * Выполняем подачу проверяемого содержимого порциями
		 */
		for(size_t offset = 0; offset < content.size(); offset += chunk){
			// Определяем размер очередной порции подачи
			const size_t size = (((content.size() - offset) < chunk) ? (content.size() - offset) : chunk);
			// Проверяем подачу очередной порции содержимого
			ASSERT_TRUE(this->_crypto->verifyUpdate(reinterpret_cast <const uint8_t *> (content.data() + offset), size)) << "порция " << chunk;
		}
		// Проверяем признание подписи потоком
		EXPECT_TRUE(this->_crypto->verifyFinalize(stream)) << "порция " << chunk;
	}
}

/**
 * @brief Тест разделения схем ГОСТ Р 34.10-2012 по разрядности
 *
 * @details Схемы на 256 и на 512 разрядов - разные схемы, и работа обязана держать их
 *          порознь: подпись одной из них, признанная ключом другой, означала бы, что
 *          разрядность на деле ни на что не влияет
 *
 */
TEST_F(CryptoFixture, Gost512SeparationCryptoTest){
	// Подписываемое сообщение
	static const char * MESSAGE = "разделение схем по разрядности";
	// Проверяем выработку ключа схемы на 256 разрядов
	ASSERT_TRUE(this->_crypto->generateKey("short", awh::crypto_t::signature_t::GOST));
	// Проверяем выработку ключа схемы на 512 разрядов
	ASSERT_TRUE(this->_crypto->generateKey("wide", awh::crypto_t::signature_t::GOST512));
	// Проверяем расхождение длин подписи схем
	EXPECT_NE(this->_crypto->length("short"), this->_crypto->length("wide"));
	// Набор подписи схемы на 256 разрядов
	std::vector <uint8_t> shorter;
	// Набор подписи схемы на 512 разрядов
	std::vector <uint8_t> wider;
	// Проверяем выработку подписи схемой на 256 разрядов
	ASSERT_TRUE(this->_crypto->sign("short", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), awh::crypto_t::hash_t::NONE, shorter));
	// Проверяем выработку подписи схемой на 512 разрядов
	ASSERT_TRUE(this->_crypto->sign("wide", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), awh::crypto_t::hash_t::NONE, wider));
	// Проверяем отказ признания подписи одной схемы ключом другой
	EXPECT_FALSE(this->_crypto->verify("short", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), wider, awh::crypto_t::hash_t::NONE));
	// Проверяем отказ признания подписи другой схемы ключом первой
	EXPECT_FALSE(this->_crypto->verify("wide", reinterpret_cast <const uint8_t *> (MESSAGE), ::strlen(MESSAGE), shorter, awh::crypto_t::hash_t::NONE));
	/**
	 * Отпечатки ключей обеих схем обязаны разойтись: отпечаток снимается с записи
	 * открытой части ключа, а та несёт и опознаватель набора свойств кривой
	 */
	EXPECT_NE(this->_crypto->fingerprint <std::string> ("short", awh::crypto_t::format_t::HEX),
	 this->_crypto->fingerprint <std::string> ("wide", awh::crypto_t::format_t::HEX));
}

/**
 * @brief Тест опознавателей в записях ключа схемы ГОСТ Р 34.10-2012
 *
 * @details Признание чужих записей ещё не значит, что свои записи выйдут годными:
 *          разбор принимает опознаватели обеих разрядностей схемы, и ключ на 512
 *          разрядов, записанный с опознавателями схемы на 256, своим же разбором
 *          принимается обратно, а чужой работой - нет. Оттого закрепляется начало
 *          записи: опознаватели схемы, набора свойств кривой и хэш-функции взяты у
 *          записей, выработанных gost-engine v3.0.3 по тем же наборам свойств
 *
 */
TEST_F(CryptoFixture, GostRecordPrefixCryptoTest){
	/**
	 * @brief Закрепляемое начало записей одной схемы
	 *
	 */
	struct sample_t {
		// Вид подписи схемы
		awh::crypto_t::signature_t kind;
		// Начало записи открытой части ключа
		const char * opened;
		/**
		 * Начало записи закрытой части ключа
		 *
		 * @note У записи закрытой части закреплено своё начало, а не чужое: закрытая
		 *       часть записывается вложенной строкой октетов, а gost-engine пишет её
		 *       прямо, и записи расходятся длиною на два октета. Обе стороны принимают
		 *       обе записи - проверено вводом своей записи в gost-engine, - и потому
		 *       расхождение это оставлено как есть. Опознаватели же схемы закрепляются
		 *       записью открытой части, где они совпадают с чужой работой дословно
		 */
		const char * secret;
	};
	// Закрепляемые начала записей обеих разрядностей схемы
	static const sample_t SAMPLES[2] = {
		{
			awh::crypto_t::signature_t::GOST,
			"MGYwHwYIKoUDBwEBAQEwEwYHKoUDAgIjAQYIKoUDBwEBAgID",
			"MEgCAQAwHwYIKoUDBwEBAQEwEwYHKoUDAgIjAQYIKoUDBwEB"
		},
		{
			awh::crypto_t::signature_t::GOST512,
			"MIGqMCEGCCqFAwcBAQECMBUGCSqFAwcBAgECAQYIKoUDBwEB",
			"MGoCAQAwIQYIKoUDBwEBAQIwFQYJKoUDBwECAQIBBggqhQMH"
		}
	};
	/**
	 * Выполняем перебор обеих разрядностей схемы
	 */
	for(auto & sample : SAMPLES){
		// Проверяем выработку ключа подписи
		ASSERT_TRUE(this->_crypto->generateKey("own", sample.kind)) << "вид " << static_cast <uint16_t> (sample.kind);
		// Выписка открытой части ключа
		const std::string opened = this->_crypto->getKey("own", awh::crypto_t::key_type_t::PUBLIC);
		// Выписка закрытой части ключа
		const std::string secret = this->_crypto->getKey("own", awh::crypto_t::key_type_t::PRIVATE);
		// Начало содержимого записи открытой части ключа
		const size_t first = opened.find('\n');
		// Начало содержимого записи закрытой части ключа
		const size_t second = secret.find('\n');
		// Проверяем наличие содержимого записи открытой части ключа
		ASSERT_NE(first, std::string::npos) << "вид " << static_cast <uint16_t> (sample.kind);
		// Проверяем наличие содержимого записи закрытой части ключа
		ASSERT_NE(second, std::string::npos) << "вид " << static_cast <uint16_t> (sample.kind);
		// Проверяем начало записи открытой части ключа
		EXPECT_EQ(opened.substr(first + 1, ::strlen(sample.opened)), std::string(sample.opened)) << "вид " << static_cast <uint16_t> (sample.kind);
		// Проверяем начало записи закрытой части ключа
		EXPECT_EQ(secret.substr(second + 1, ::strlen(sample.secret)), std::string(sample.secret)) << "вид " << static_cast <uint16_t> (sample.kind);
	}
}

/**
 * @brief Тест переноса при сложении контрольной суммы хэш-функции
 *
 * @details Контрольная сумма блоков складывается словами по 64 разряда, и перенос в
 *          старшее слово ловится сличением суммы со слагаемым. Случай, когда слагаемое
 *          равно наибольшему слову, а перенос уже пришёл, даёт сумму, равную первому
 *          слагаемому: без особой проверки перенос там теряется молча. Ни одна проверка
 *          набора этого случая не задевала - он вскрылся пробой порчей, - оттого
 *          подаётся сообщение, попадающее в него ровно. Числа сняты чужой работой
 *          (gost12sum из gost-engine v3.0.3), а не своей
 *
 */
TEST_F(CryptoFixture, StreebogCarryCryptoTest){
	// Подаваемое сообщение в два блока
	std::string content(128, '\0');
	/**
	 * Первый блок несёт единицу в младшем слове и пятёрку в следующем, второй -
	 * наибольшие значения в обоих: сложение их даёт перенос в слово, сумма которого
	 * равна его же первому слагаемому
	 */
	// Выполняем установку единицы младшего слова первого блока
	content[0] = static_cast <char> (0x01);
	// Выполняем установку пятёрки следующего слова первого блока
	content[8] = static_cast <char> (0x05);
	/**
	 * Выполняем заполнение двух младших слов второго блока
	 */
	for(size_t i = 64; i < 80; i++)
		// Выполняем установку наибольшего значения октета
		content[i] = static_cast <char> (0xFF);
	// Проверяем хэш-сумму на 256 разрядов
	EXPECT_EQ(this->_crypto->hash <std::string> (content, awh::crypto_t::hash_t::STREEBOG256),
	 std::string("c440ef267d7972891ed59b0319ee3a38904b0d1ec016ce64ff8a722c718c8179"));
	// Проверяем хэш-сумму на 512 разрядов
	EXPECT_EQ(this->_crypto->hash <std::string> (content, awh::crypto_t::hash_t::STREEBOG512),
	 std::string("c1f318b38d38f8d98434820903c97dec59f7e0a2c65af2e97c7df07b0b9c1b2390779b3ec5a255bfec7c1ec89749dea61c8e9f366f568feef820cc5d0443c83d"));
}

/**
 * @brief Тест разбора порченых записей ключа ГОСТ Р 34.10-2012
 *
 * @details Записи ключа приходят извне, и разбор их - единственное место модуля, где
 *          работа читает недоверенное содержимое. Проверка ворошит подлинную запись
 *          случайными порчами и требует не отсутствия отказов, а того, чтобы всякая
 *          принятая запись оставалась годной: разрядность схемы отвечала бы длине
 *          подписи, а работы подписи и отпечатка не роняли бы работу. Прогон под
 *          санитайзерами двадцатью тысячами порч расхождений не дал
 *
 */
TEST_F(CryptoFixture, GostFuzzKeyCryptoTest){
	// Количество ворошимых записей
	static constexpr size_t ROUNDS = 2000;
	// Выполняем выработку подлинного ключа
	ASSERT_TRUE(this->_crypto->generateKey("origin", awh::crypto_t::signature_t::GOST512));
	// Выписка закрытой части подлинного ключа
	const std::string secret = this->_crypto->getKey("origin", awh::crypto_t::key_type_t::PRIVATE);
	// Выписка открытой части подлинного ключа
	const std::string opened = this->_crypto->getKey("origin", awh::crypto_t::key_type_t::PUBLIC);
	/**
	 * Источник случайности заведён с постоянного зерна: ворошитель обязан давать один
	 * и тот же набор порч на всякой машине, иначе отказ его невоспроизводим
	 */
	std::mt19937 generator(20260819);
	/**
	 * Выполняем перебор всех ворошимых записей
	 */
	for(size_t round = 0; round < ROUNDS; round++){
		// Признак порчи закрытой части ключа
		const bool secured = ((round % 2) == 0);
		// Ворошимая запись ключа
		std::string sample = (secured ? secret : opened);
		// Количество порч записи
		const size_t count = (1 + (generator() % 4));
		/**
		 * Выполняем внесение порч в запись
		 */
		for(size_t i = 0; i < count; i++)
			// Выполняем порчу очередного знака записи
			sample[generator() % sample.size()] = static_cast <char> (generator() % 256);
		/**
		 * Если порченая запись всё же принята
		 */
		if(this->_crypto->setKey("fuzz", sample, (secured ? awh::crypto_t::key_type_t::PRIVATE : awh::crypto_t::key_type_t::PUBLIC))){
			// Вид подписи принятой записи
			const awh::crypto_t::signature_t kind = this->_crypto->signature("fuzz");
			// Проверяем, что вид подписи принятой записи схеме ГОСТ отвечает
			ASSERT_TRUE((kind == awh::crypto_t::signature_t::GOST) || (kind == awh::crypto_t::signature_t::GOST512)) << "оборот " << round;
			// Проверяем согласие длины подписи с разрядностью схемы
			EXPECT_EQ(this->_crypto->length("fuzz"), ((kind == awh::crypto_t::signature_t::GOST512) ? 128u : 64u)) << "оборот " << round;
			// Набор выработанной подписи
			std::vector <uint8_t> signature;
			/**
			 * Работы подписи и проверки на порченом ключе законны: отказ их - обычный
			 * исход, а вот падение работы либо чтение за пределами буфера - нет
			 */
			// Выполняем выработку подписи порченым ключом
			this->_crypto->sign("fuzz", reinterpret_cast <const uint8_t *> ("x"), 1, awh::crypto_t::hash_t::NONE, signature);
			// Выполняем проверку выработанной подписи
			this->_crypto->verify("fuzz", reinterpret_cast <const uint8_t *> ("x"), 1, signature, awh::crypto_t::hash_t::NONE);
			// Проверяем снятие отпечатка порченого ключа
			EXPECT_FALSE(this->_crypto->fingerprint <std::string> ("fuzz", awh::crypto_t::format_t::HEX).empty()) << "оборот " << round;
		}
	}
}
