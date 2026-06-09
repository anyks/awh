/**
 * @file: compressor.cpp
 * @date: 2026-01-21
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
#include <sys/compressor.hpp>

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
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект для компрессии данных
	compressor_t compressor(&log);
	// Строка для компрессии данных
	const string data = "Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?";
	// Выводим заголовок компрессии LZ4
	cout << " ======== LZ4 ======== " << endl << flush;
	// Выполняем компрессию данных
	string compressed = compressor.compress <string> (data, compressor_t::method_t::LZ4);
	// Выводим результат работы компрессора
	cout << "Compressed data LZ4: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	string decompressed = compressor.decompress <string> (compressed, compressor_t::method_t::LZ4);
	// Выводим результат работы компрессора
	cout << "Decompressed data LZ4: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Выводим пустую строку
	cout << endl << flush;
	// Выводим заголовок компрессии LZMA
	cout << " ======== LZMA ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, compressor_t::method_t::LZMA);
	// Выводим результат работы компрессора
	cout << "Compressed data LZMA: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, compressor_t::method_t::LZMA);
	// Выводим результат работы компрессора
	cout << "Decompressed data LZMA: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Выводим пустую строку
	cout << endl << flush;
	// Выводим заголовок компрессии ZSTD
	cout << " ======== ZSTD ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, compressor_t::method_t::ZSTD);
	// Выводим результат работы компрессора
	cout << "Compressed data ZSTD: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, compressor_t::method_t::ZSTD);
	// Выводим результат работы компрессора
	cout << "Decompressed data ZSTD: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Выводим пустую строку
	cout << endl << flush;
	// Выводим заголовок компрессии GZIP
	cout << " ======== GZIP ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, compressor_t::method_t::GZIP);
	// Выводим результат работы компрессора
	cout << "Compressed data GZIP: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, compressor_t::method_t::GZIP);
	// Выводим результат работы компрессора
	cout << "Decompressed data GZIP: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Выводим пустую строку
	cout << endl << flush;
	// Выводим заголовок компрессии BZIP2
	cout << " ======== BZIP2 ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, compressor_t::method_t::BZIP2);
	// Выводим результат работы компрессора
	cout << "Compressed data BZIP2: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, compressor_t::method_t::BZIP2);
	// Выводим результат работы компрессора
	cout << "Decompressed data BZIP2: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Выводим пустую строку
	cout << endl << flush;
	// Выводим заголовок компрессии BROTLI
	cout << " ======== BROTLI ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, compressor_t::method_t::BROTLI);
	// Выводим результат работы компрессора
	cout << "Compressed data BROTLI: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, compressor_t::method_t::BROTLI);
	// Выводим результат работы компрессора
	cout << "Decompressed data BROTLI: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Выводим пустую строку
	cout << endl << flush;
	// Выводим заголовок компрессии LIZARD
	cout << " ======== LIZARD ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, compressor_t::method_t::LIZARD);
	// Выводим результат работы компрессора
	cout << "Compressed data LIZARD: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, compressor_t::method_t::LIZARD);
	// Выводим результат работы компрессора
	cout << "Decompressed data LIZARD: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Выводим пустую строку
	cout << endl << flush;
	// Выводим заголовок компрессии SNAPPY
	cout << " ======== SNAPPY ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, compressor_t::method_t::SNAPPY);
	// Выводим результат работы компрессора
	cout << "Compressed data SNAPPY: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, compressor_t::method_t::SNAPPY);
	// Выводим результат работы компрессора
	cout << "Decompressed data SNAPPY: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Выводим пустую строку
	cout << endl << flush;
	// Выводим заголовок компрессии DEFLATE
	cout << " ======== DEFLATE ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, compressor_t::method_t::DEFLATE);
	// Выводим результат работы компрессора
	cout << "Compressed data DEFLATE: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, compressor_t::method_t::DEFLATE);
	// Выводим результат работы компрессора
	cout << "Decompressed data DEFLATE: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Выводим пустую строку
	cout << endl << flush;
	// Выводим заголовок компрессии DENSITY
	cout << " ======== DENSITY ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, compressor_t::method_t::DENSITY);
	// Выводим результат работы компрессора
	cout << "Compressed data DENSITY: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, compressor_t::method_t::DENSITY);
	// Выводим результат работы компрессора
	cout << "Decompressed data DENSITY: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Выводим пустую строку
	cout << endl << flush;
	// Выводим результат
	return EXIT_SUCCESS;
}
