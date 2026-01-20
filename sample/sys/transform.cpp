/**
 * @file: transform.cpp
 * @date: 2026-01-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные модули
 */
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <sys/transform.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект для трансформации данных
	transform_t transform(&log);
	// Строка для компрессии данных
	const string data = "Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?";
	// Выводим заголовок компрессии LZ4
	cout << " ======== LZ4 ======== " << endl;
	// Выполняем хэширование текста
	string compressed = transform.compress <string> (data, transform_t::compressor_t::LZ4);
	// Выводим результат хэширования
	cout << "Compressed data LZ4: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	string decompressed = transform.decompress <string> (compressed, transform_t::compressor_t::LZ4);
	// Выводим результат хэширования
	cout << "Decompressed data LZ4: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии LZMA
	cout << " ======== LZMA ======== " << endl;
	// Выполняем хэширование текста
	compressed = transform.compress <string> (data, transform_t::compressor_t::LZMA);
	// Выводим результат хэширования
	cout << "Compressed data LZMA: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decompress <string> (compressed, transform_t::compressor_t::LZMA);
	// Выводим результат хэширования
	cout << "Decompressed data LZMA: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии ZSTD
	cout << " ======== ZSTD ======== " << endl;
	// Выполняем хэширование текста
	compressed = transform.compress <string> (data, transform_t::compressor_t::ZSTD);
	// Выводим результат хэширования
	cout << "Compressed data ZSTD: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decompress <string> (compressed, transform_t::compressor_t::ZSTD);
	// Выводим результат хэширования
	cout << "Decompressed data ZSTD: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии GZIP
	cout << " ======== GZIP ======== " << endl;
	// Выполняем хэширование текста
	compressed = transform.compress <string> (data, transform_t::compressor_t::GZIP);
	// Выводим результат хэширования
	cout << "Compressed data GZIP: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decompress <string> (compressed, transform_t::compressor_t::GZIP);
	// Выводим результат хэширования
	cout << "Decompressed data GZIP: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии BZIP2
	cout << " ======== BZIP2 ======== " << endl;
	// Выполняем хэширование текста
	compressed = transform.compress <string> (data, transform_t::compressor_t::BZIP2);
	// Выводим результат хэширования
	cout << "Compressed data BZIP2: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decompress <string> (compressed, transform_t::compressor_t::BZIP2);
	// Выводим результат хэширования
	cout << "Decompressed data BZIP2: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии BROTLI
	cout << " ======== BROTLI ======== " << endl;
	// Выполняем хэширование текста
	compressed = transform.compress <string> (data, transform_t::compressor_t::BROTLI);
	// Выводим результат хэширования
	cout << "Compressed data BROTLI: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decompress <string> (compressed, transform_t::compressor_t::BROTLI);
	// Выводим результат хэширования
	cout << "Decompressed data BROTLI: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии LIZARD
	cout << " ======== LIZARD ======== " << endl;
	// Выполняем хэширование текста
	compressed = transform.compress <string> (data, transform_t::compressor_t::LIZARD);
	// Выводим результат хэширования
	cout << "Compressed data LIZARD: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decompress <string> (compressed, transform_t::compressor_t::LIZARD);
	// Выводим результат хэширования
	cout << "Decompressed data LIZARD: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии SNAPPY
	cout << " ======== SNAPPY ======== " << endl;
	// Выполняем хэширование текста
	compressed = transform.compress <string> (data, transform_t::compressor_t::SNAPPY);
	// Выводим результат хэширования
	cout << "Compressed data SNAPPY: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decompress <string> (compressed, transform_t::compressor_t::SNAPPY);
	// Выводим результат хэширования
	cout << "Decompressed data SNAPPY: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии DEFLATE
	cout << " ======== DEFLATE ======== " << endl;
	// Выполняем хэширование текста
	compressed = transform.compress <string> (data, transform_t::compressor_t::DEFLATE);
	// Выводим результат хэширования
	cout << "Compressed data DEFLATE: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decompress <string> (compressed, transform_t::compressor_t::DEFLATE);
	// Выводим результат хэширования
	cout << "Decompressed data DEFLATE: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии DENSITY
	cout << " ======== DENSITY ======== " << endl;
	// Выполняем хэширование текста
	compressed = transform.compress <string> (data, transform_t::compressor_t::DENSITY);
	// Выводим результат хэширования
	cout << "Compressed data DENSITY: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decompress <string> (compressed, transform_t::compressor_t::DENSITY);
	// Выводим результат хэширования
	cout << "Decompressed data DENSITY: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии AES256
	cout << " ======== AES256 ======== " << endl;
	// Устанавливаем количество раундов шифрования
	transform.roundAES(10);
	// Устанавливаем соль шифрования
	transform.salt("anyks_salt");
	// Устанавливаем пароль шифрования
	transform.password("anyks_password");
	// Выполняем кодирование текста
	compressed = transform.encode <string> (data, transform_t::cipher_t::AES256);
	// Выводим результат хэширования
	cout << "Encoded data AES256: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decode <string> (compressed, transform_t::cipher_t::AES256);
	// Выводим результат хэширования
	cout << "Decoded data AES256: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии BASE64
	cout << " ======== BASE64 ======== " << endl;
	// Выполняем кодирование текста
	compressed = transform.encode <string> (data, transform_t::cipher_t::BASE64);
	// Выводим результат хэширования
	cout << "Encoded data BASE64: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выполняем декомпрессию данных
	decompressed = transform.decode <string> (compressed, transform_t::cipher_t::BASE64);
	// Выводим результат хэширования
	cout << "Decoded data BASE64: " << decompressed << ", SIZE=" << decompressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии HASH SHA256
	cout << " ======== HASH SHA256 ======== " << endl;
	// Выполняем кодирование текста
	compressed = transform.hashing <string> (data, transform_t::hash_t::SHA256);
	// Выводим результат хэширования
	cout << "Encoded data SHA256: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии HASH MD5
	cout << " ======== HASH MD5 ======== " << endl;
	// Выполняем кодирование текста
	compressed = transform.hashing <string> (data, transform_t::hash_t::MD5);
	// Выводим результат хэширования
	cout << "Encoded data MD5: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии HASH HMAC SHA256
	cout << " ======== HASH HMAC SHA256 ======== " << endl;
	// Выполняем кодирование текста
	compressed = transform.hmac <string> ("test", data, transform_t::hash_t::SHA256);
	// Выводим результат хэширования
	cout << "Encoded data SHA256: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии HASH HMAC MD5
	cout << " ======== HASH HMAC MD5 ======== " << endl;
	// Выполняем кодирование текста
	compressed = transform.hmac <string> ("test", data, transform_t::hash_t::MD5);
	// Выводим результат хэширования
	cout << "Encoded data MD5: " << compressed << ", SIZE=" << compressed.size() << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии HASH 32-BIT
	cout << " ======== HASH 32-BIT ======== " << endl;
	// Выполняем кодирование текста
	uint32_t value1 = transform.hashing <uint32_t> (data);
	// Выводим результат хэширования
	cout << "Encoded data 32-BIT: " << value1 << ", SIZE=" << sizeof(value1) << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии HASH 64-BIT
	cout << " ======== HASH 64-BIT ======== " << endl;
	// Выполняем кодирование текста
	uint64_t value2 = transform.hashing <uint64_t> (data);
	// Выводим результат хэширования
	cout << "Encoded data 64-BIT: " << value2 << ", SIZE=" << sizeof(value2) << endl;
	// Выводим пустую строку
	cout << endl;
	// Выводим заголовок компрессии ENCRYPTION
	cout << " ======== ENCRYPTION ======== " << endl;
	// Генерируем пару ключей RSA
	if(transform.generatePrivateKeyRSA()){
		// Если генерация ключа прошла успешно
		if(transform.generatePublicKeyRSA()){
			// Сохраняем публичный ключ в файл
			if(transform.savePublicKeyRSA("public_key.pem")){
				// Сохраняем приватный ключ в файл
				if(transform.savePrivateKeyRSA("private_key.pem")){
					// Загружаем публичный ключ из файла
					if(transform.loadPublicKeyRSA("public_key.pem")){
						// Загружаем приватный ключ из файла
						if(transform.loadPrivateKeyRSA("private_key.pem")){
							// Буфер данных для подписи
							vector <uint8_t> signature;
							// Буфер данных для шифрования
							vector <uint8_t> buffer(data.begin(), data.end());
							// Подписываем данные приватным ключом RSA
							transform.signWithPrivateKey(buffer, signature);
							// Выполняем верификацию данных публичным ключом RSA
							if(transform.verifyWithPublicKey(buffer, signature)){
								// Шифруем данные публичным ключом RSA
								transform.encryptWithPublicKey(buffer, signature);
								// Формируем строку из зашифрованных данных
								const_cast <string &> (data).assign(signature.begin(), signature.end());
								// Выводим результат
								cout << "Encrypted data RSA: " << data << ", SIZE=" << data.size() << endl;
								// Расшифровываем данные приватным ключом RSA
								transform.decryptWithPrivateKey(signature, buffer);
								// Формируем строку из расшифрованных данных
								const_cast <string &> (data).assign(buffer.begin(), buffer.end());
								// Выводим результат
								cout << "Decrypted data RSA: " << data << ", SIZE=" << data.size() << endl;
							}
						}
					}
				}
			}
		}
	}
	// Выводим пустую строку
	cout << endl;
	// Выводим результат
	return EXIT_SUCCESS;
}
