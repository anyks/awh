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
	// Выводим результат
	return EXIT_SUCCESS;
}
