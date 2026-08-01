/**
 * @file: crypto.cpp
 * @date: 2026-01-20
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с модулем криптографии — демонстрация симметричного шифрования и расшифровки данных,
 *        вычисления хешей и кодирования в Base64
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <sys/crypto.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект для работы с криптографией
	crypto_t crypto(&fmk, &log);
	// Строка для компрессии данных
	const string data = "Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?";
	// Печатаем заголовок в отладочный вывод компрессии AES256
	cout << " ======== AES256 ======== " << endl;
	/**
	 * Количество итераций вывода ключа занижено ради скорости примера: в работе
	 * его занижать нельзя - на нём и стоит стойкость пароля к перебору. По
	 * умолчанию их сто тысяч, и трогать это без нужды не следует
	 */
	// Устанавливаем количество раундов шифрования
	crypto.roundAES(10);
	// Устанавливаем соль шифрования
	crypto.salt("anyks_salt");
	// Устанавливаем пароль шифрования
	crypto.password("anyks_password");
	/**
	 * Режим блочного шифрования по умолчанию проверяет подлинность данных:
	 * шифротекст растёт на вектор инициализации и имитовставку, зато подделка
	 * его обнаруживается. Режим гаммирования доступен отдельно, подделку он
	 * не обнаруживает
	 */
	// Устанавливаем режим блочного шифрования с проверкой подлинности
	crypto.mode(crypto_t::mode_t::GCM);
	// Выполняем кодирование текста
	string encoded = crypto.encrypt <string> (data, crypto_t::hash_t::SHA256, crypto_t::cipher_t::AES256);
	/**
	 * Шифротекст двоичен и печатной записи не имеет: вывод его как есть портит
	 * поток вывода управляющими октетами, поэтому он показывается в BASE64
	 */
	// Возвращаем результат хэширования
	cout << "Encoded1 data AES256: " << crypto.encrypt <string> (encoded.data(), encoded.size(), crypto_t::hash_t::SHA256, crypto_t::cipher_t::BASE64) << ", SIZE=" << encoded.size() << endl << flush;
	/**
	 * Вектор инициализации берётся случайным на каждое сообщение, поэтому
	 * шифрование одних и тех же данных даёт разный шифротекст
	 */
	// Выполняем повторное кодирование того же текста
	cout << "Encoded1 differs from the repeated one: " << (crypto.encrypt <string> (data, crypto_t::hash_t::SHA256, crypto_t::cipher_t::AES256) != encoded) << endl << flush;
	// Формируем поддельный шифротекст
	string tampered = encoded;
	// Выполняем изменение октета шифротекста
	tampered[tampered.size() / 2] = static_cast <char> (tampered[tampered.size() / 2] ^ 0x01);
	/**
	 * Об удаче работы судят по её признаку, а не по пустоте буфера: пустой
	 * открытый текст - законный итог расшифровки пустого сообщения, и от отказа
	 * его отличает только признак
	 */
	// Буфер открытого текста
	string decoded;
	// Возвращаем результат обнаружения подделки шифротекста
	cout << "Tampered ciphertext rejected: " << !crypto.decrypt <string> (tampered.data(), tampered.size(), decoded, crypto_t::hash_t::SHA256, crypto_t::cipher_t::AES256) << endl << flush;
	// Выполняем расшифровку подлинного шифротекста
	if(!crypto.decrypt <string> (encoded.data(), encoded.size(), decoded, crypto_t::hash_t::SHA256, crypto_t::cipher_t::AES256))
		// Выводим сообщение об отказе расшифровки
		cout << "Decryption could not be performed" << endl << flush;
	// Возвращаем результат хэширования
	cout << "Decoded1 data AES256: " << decoded << ", SIZE=" << decoded.size() << endl << flush;
	// Инициализируем объект криптографии для другого типа хэша
	crypto.initialize(crypto_t::event_t::ENCODE, crypto_t::hash_t::SHA224, crypto_t::cipher_t::AES192);
	// Выполняем кодирование текста
	encoded = crypto.encrypt <string> (data);
	// Добавляем ещё один слой шифрования
	encoded.append(crypto.encrypt <string> (data));
	/**
	 * Завершение потока обязательно и его признак обязателен к проверке: без
	 * завершения шифротекст остаётся без имитовставки, а её отказ означает
	 * подделку данных
	 */
	// Завершаем процесс шифрования
	if(!crypto.finalize(encoded))
		// Выводим сообщение об отказе завершения потока
		cout << "Stream encryption could not be finalized" << endl << flush;
	// Возвращаем результат хэширования
	cout << "Encoded2 data AES192: " << crypto.encrypt <string> (encoded.data(), encoded.size(), crypto_t::hash_t::SHA256, crypto_t::cipher_t::BASE64) << ", SIZE=" << encoded.size() << endl << flush;
	// Инициализируем объект криптографии для другого типа хэша
	crypto.initialize(crypto_t::event_t::DECODE, crypto_t::hash_t::SHA224, crypto_t::cipher_t::AES192);
	// Выполняем декомпрессию данных
	decoded = crypto.decrypt <string> (encoded);
	// Завершаем процесс дешифрования
	if(!crypto.finalize(decoded))
		// Выводим сообщение об отказе завершения потока
		cout << "Stream decryption could not be finalized, the ciphertext may be tampered with" << endl << flush;
	// Возвращаем результат хэширования
	cout << "Decoded2 data AES192: " << decoded << ", SIZE=" << decoded.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl;
	// Печатаем заголовок в отладочный вывод компрессии BASE64
	cout << " ======== BASE64 ======== " << endl;
	// Выполняем кодирование текста
	encoded = crypto.encrypt <string> (data, crypto_t::hash_t::NONE, crypto_t::cipher_t::BASE64);
	// Возвращаем результат хэширования
	cout << "Encoded data BASE64: " << encoded << ", SIZE=" << encoded.size() << endl;
	// Выполняем декомпрессию данных
	decoded = crypto.decrypt <string> (encoded, crypto_t::hash_t::NONE, crypto_t::cipher_t::BASE64);
	// Возвращаем результат хэширования
	cout << "Decoded data BASE64: " << decoded << ", SIZE=" << decoded.size() << endl;
	// Возвращаем пустую строку
	cout << endl;
	// Печатаем заголовок в отладочный вывод компрессии HASH SHA256
	cout << " ======== HASH SHA256 ======== " << endl;
	// Выполняем кодирование текста
	encoded = crypto.hash <string> (data, crypto_t::hash_t::SHA256);
	// Возвращаем результат хэширования
	cout << "Encoded data SHA256: " << encoded << ", SIZE=" << encoded.size() << endl;
	// Возвращаем пустую строку
	cout << endl;
	// Печатаем заголовок в отладочный вывод компрессии HASH MD5
	cout << " ======== HASH MD5 ======== " << endl;
	// Выполняем кодирование текста
	encoded = crypto.hash <string> (data, crypto_t::hash_t::MD5);
	// Возвращаем результат хэширования
	cout << "Encoded data MD5: " << encoded << ", SIZE=" << encoded.size() << endl;
	// Возвращаем пустую строку
	cout << endl;
	// Печатаем заголовок в отладочный вывод компрессии HASH HMAC SHA256
	cout << " ======== HASH HMAC SHA256 ======== " << endl;
	// Выполняем кодирование текста
	encoded = crypto.hmac <string> (string_view{"test"}, data, crypto_t::hash_t::SHA256);
	// Возвращаем результат хэширования
	cout << "Encoded data SHA256: " << encoded << ", SIZE=" << encoded.size() << endl;
	// Возвращаем пустую строку
	cout << endl;
	// Печатаем заголовок в отладочный вывод компрессии HASH HMAC MD5
	cout << " ======== HASH HMAC MD5 ======== " << endl;
	// Выполняем кодирование текста
	encoded = crypto.hmac <string> (string_view{"test"}, data, crypto_t::hash_t::MD5);
	// Возвращаем результат хэширования
	cout << "Encoded data MD5: " << encoded << ", SIZE=" << encoded.size() << endl;
	// Возвращаем пустую строку
	cout << endl;
	// Печатаем заголовок в отладочный вывод компрессии HASH 32-BIT
	cout << " ======== HASH 32-BIT ======== " << endl;
	// Выполняем кодирование текста
	uint32_t value1 = crypto.hash <uint32_t> (data);
	// Возвращаем результат хэширования
	cout << "Encoded data 32-BIT: " << value1 << ", SIZE=" << sizeof(value1) << endl;
	// Возвращаем пустую строку
	cout << endl;
	// Печатаем заголовок в отладочный вывод компрессии HASH 64-BIT
	cout << " ======== HASH 64-BIT ======== " << endl;
	// Выполняем кодирование текста
	uint64_t value2 = crypto.hash <uint64_t> (data);
	// Возвращаем результат хэширования
	cout << "Encoded data 64-BIT: " << value2 << ", SIZE=" << sizeof(value2) << endl;
	// Возвращаем пустую строку
	cout << endl;
	// Печатаем заголовок в отладочный вывод компрессии ENCRYPTION
	cout << " ======== ENCRYPTION ======== " << endl;
	// Генерируем пару ключей RSA
	if(crypto.generatePrivateKeyRSA()){
		// Сохраняем публичный ключ в файл
		if(crypto.savePublicKeyRSA("public_key.pem")){
			// Сохраняем приватный ключ в файл
			if(crypto.savePrivateKeyRSA("private_key.pem")){
				// Загружаем публичный ключ из файла
				if(crypto.loadPublicKeyRSA("public_key.pem")){
					// Буфер данных для подписи
					vector <uint8_t> signature;
					// Буфер данных для шифрования
					vector <uint8_t> buffer(data.begin(), data.end());
					// Шифруем данные публичным ключом RSA
					crypto.encryptWithPublicKey(buffer, signature);
					// Формируем строку из зашифрованных данных
					const_cast <string &> (data).assign(signature.begin(), signature.end());
					// Возвращаем результат
					cout << "Encrypted data RSA: " << data << ", SIZE=" << data.size() << endl;
					// Загружаем приватный ключ из файла
					if(crypto.loadPrivateKeyRSA("private_key.pem")){
						// Расшифровываем данные приватным ключом RSA
						crypto.decryptWithPrivateKey(signature, buffer);
						// Формируем строку из расшифрованных данных
						const_cast <string &> (data).assign(buffer.begin(), buffer.end());
						// Возвращаем результат
						cout << "Decrypted data RSA: " << data << ", SIZE=" << data.size() << endl;
						// Подписываем данные приватным ключом RSA
						crypto.signWithPrivateKey(buffer, crypto_t::hash_t::MD5, signature);
						// Выполняем верификацию данных публичным ключом RSA
						if(crypto.verifyWithPublicKey(buffer, signature, crypto_t::hash_t::MD5))
							// Возвращаем результат
							cout << "Signature verified successfully!" << endl;
						// Если верификация не удалась
						else cout << "Signature verification failed!" << endl;
						// Получаем приватный ключ RSA
						const string prikey = crypto.getPrivateKeyRSA();
						// Возвращаем приватный ключ RSA
						cout << "Private Key:" << endl << prikey << endl;
						// Устанавливаем приватный ключ RSA
						crypto.setPrivateKeyRSA(prikey);
						// Получаем публичный ключ RSA
						const string pubkey = crypto.getPublicKeyRSA();
						// Возвращаем публичный ключ RSA
						cout << "Public Key:" << endl << pubkey << endl;
						// Устанавливаем публичный ключ RSA
						crypto.setPublicKeyRSA(pubkey);
					}
				}
			}
		}
	}
	// Возвращаем пустую строку
	cout << endl;
	// Возвращаем результат
	return EXIT_SUCCESS;
}
