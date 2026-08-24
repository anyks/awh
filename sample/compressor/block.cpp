/**
 * @file block.cpp
 * @date 2026-07-13
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
 * @brief Пример блочной компрессии — демонстрация сжатия и распаковки данных целиком за один вызов разными
 *        алгоритмами с сопоставлением исходного и восстановленного содержимого
 *
 * @copyright Copyright © 2026
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
#include <compressor/block.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Главная функция приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main(){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект для блочной компрессии данных
	awh::compressor::block_t compressor(&log);
	// Строка для компрессии данных
	const string data = "Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?";
	// Печатаем заголовок в отладочный вывод компрессии LZ4
	cout << " ======== LZ4 ======== " << endl << flush;
	// Выполняем компрессию данных
	string compressed = compressor.compress <string> (data, awh::compressor::method_t::LZ4);
	// Возвращаем результат работы компрессора
	cout << "Compressed data LZ4: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	string decompressed = compressor.decompress <string> (compressed, awh::compressor::method_t::LZ4);
	// Возвращаем результат работы компрессора
	cout << "Decompressed data LZ4: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl << flush;
	// Печатаем заголовок в отладочный вывод компрессии LZMA
	cout << " ======== LZMA ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, awh::compressor::method_t::LZMA);
	// Возвращаем результат работы компрессора
	cout << "Compressed data LZMA: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, awh::compressor::method_t::LZMA);
	// Возвращаем результат работы компрессора
	cout << "Decompressed data LZMA: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl << flush;
	// Печатаем заголовок в отладочный вывод компрессии ZSTD
	cout << " ======== ZSTD ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, awh::compressor::method_t::ZSTD);
	// Возвращаем результат работы компрессора
	cout << "Compressed data ZSTD: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, awh::compressor::method_t::ZSTD);
	// Возвращаем результат работы компрессора
	cout << "Decompressed data ZSTD: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl << flush;
	// Печатаем заголовок в отладочный вывод компрессии GZIP
	cout << " ======== GZIP ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, awh::compressor::method_t::GZIP);
	// Возвращаем результат работы компрессора
	cout << "Compressed data GZIP: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, awh::compressor::method_t::GZIP);
	// Возвращаем результат работы компрессора
	cout << "Decompressed data GZIP: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl << flush;
	// Печатаем заголовок в отладочный вывод компрессии BZIP2
	cout << " ======== BZIP2 ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, awh::compressor::method_t::BZIP2);
	// Возвращаем результат работы компрессора
	cout << "Compressed data BZIP2: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, awh::compressor::method_t::BZIP2);
	// Возвращаем результат работы компрессора
	cout << "Decompressed data BZIP2: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl << flush;
	// Печатаем заголовок в отладочный вывод компрессии BROTLI
	cout << " ======== BROTLI ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, awh::compressor::method_t::BROTLI);
	// Возвращаем результат работы компрессора
	cout << "Compressed data BROTLI: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, awh::compressor::method_t::BROTLI);
	// Возвращаем результат работы компрессора
	cout << "Decompressed data BROTLI: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl << flush;
	// Печатаем заголовок в отладочный вывод компрессии LIZARD
	cout << " ======== LIZARD ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, awh::compressor::method_t::LIZARD);
	// Возвращаем результат работы компрессора
	cout << "Compressed data LIZARD: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, awh::compressor::method_t::LIZARD);
	// Возвращаем результат работы компрессора
	cout << "Decompressed data LIZARD: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl << flush;
	// Печатаем заголовок в отладочный вывод компрессии SNAPPY
	cout << " ======== SNAPPY ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, awh::compressor::method_t::SNAPPY);
	// Возвращаем результат работы компрессора
	cout << "Compressed data SNAPPY: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, awh::compressor::method_t::SNAPPY);
	// Возвращаем результат работы компрессора
	cout << "Decompressed data SNAPPY: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl << flush;
	// Печатаем заголовок в отладочный вывод компрессии DEFLATE
	cout << " ======== DEFLATE ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, awh::compressor::method_t::DEFLATE);
	// Возвращаем результат работы компрессора
	cout << "Compressed data DEFLATE: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, awh::compressor::method_t::DEFLATE);
	// Возвращаем результат работы компрессора
	cout << "Decompressed data DEFLATE: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl << flush;
	// Печатаем заголовок в отладочный вывод компрессии DENSITY
	cout << " ======== DENSITY ======== " << endl << flush;
	// Выполняем компрессию данных
	compressed = compressor.compress <string> (data, awh::compressor::method_t::DENSITY);
	// Возвращаем результат работы компрессора
	cout << "Compressed data DENSITY: " << compressed << ", SIZE=" << compressed.size() << endl << flush;
	// Выполняем декомпрессию данных
	decompressed = compressor.decompress <string> (compressed, awh::compressor::method_t::DENSITY);
	// Возвращаем результат работы компрессора
	cout << "Decompressed data DENSITY: " << decompressed << ", SIZE=" << decompressed.size() << endl << flush;
	// Возвращаем пустую строку
	cout << endl << flush;
	// Возвращаем результат
	return EXIT_SUCCESS;
}
