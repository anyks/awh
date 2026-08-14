/**
 * @file stream.cpp
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
 * @brief Пример потоковой компрессии — демонстрация инкрементального сжатия и распаковки данных порциями с
 *        управлением режимом сброса буфера и финализацией потока
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <vector>
#include <utility>
#include <iostream>
#include <algorithm>

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
	// Создаём объект компрессии данных (используется как фабрика потоковых сессий)
	awh::compressor::block_t compressor(&log);
	// Строка для компрессии данных
	const string data = "Hello World, Hello World, Hello World, Hello World, Hello World, Hello World!!!!!!!!!!!!!!!!?";
	// Методы компрессии, поддерживающие потоковый режим
	const std::vector <std::pair <std::string, awh::compressor::method_t>> methods = {
		{"GZIP", awh::compressor::method_t::GZIP},
		{"DEFLATE", awh::compressor::method_t::DEFLATE},
		{"ZLIB", awh::compressor::method_t::ZLIB},
		{"ZSTD", awh::compressor::method_t::ZSTD},
		{"BROTLI", awh::compressor::method_t::BROTLI},
		{"LZMA", awh::compressor::method_t::LZMA},
		{"BZIP2", awh::compressor::method_t::BZIP2},
		{"LZ4", awh::compressor::method_t::LZ4},
		{"LIZARD", awh::compressor::method_t::LIZARD}
	};
	// Размер порции данных для потоковой обработки
	const size_t chunk = 16;
	/**
	 * Перебираем методы компрессии
	 */
	for(auto & item : methods){
		// Печатаем заголовок метода
		cout << " ======== STREAM " << item.first << " ======== " << endl << flush;
		// Буфер готового выхода порции
		string part = "";
		// Создаём потоковую сессию компрессии
		awh::compressor::stream_t encoder = compressor.stream(item.second, awh::compressor::event_t::ENCODE);
		// Результат компрессии
		string compressed = "";
		/**
		 * Выполняем компрессию порциями
		 */
		for(size_t i = 0; i < data.size(); i += chunk){
			// Вычисляем размер порции
			const size_t n = std::min(chunk, data.size() - i);
			// Подаём порцию в поток
			encoder.push(data.data() + i, n, part);
			// Добавляем полученный выход в результат
			compressed.append(part);
		}
		// Финализируем поток компрессии
		encoder.finish(part);
		// Добавляем хвост в результат
		compressed.append(part);
		// Создаём потоковую сессию декомпрессии
		awh::compressor::stream_t decoder = compressor.stream(item.second, awh::compressor::event_t::DECODE);
		// Результат декомпрессии
		string restored = "";
		/**
		 * Выполняем декомпрессию порциями
		 */
		for(size_t i = 0; i < compressed.size(); i += chunk){
			// Вычисляем размер порции
			const size_t n = std::min(chunk, compressed.size() - i);
			// Подаём порцию в поток
			decoder.push(compressed.data() + i, n, part);
			// Добавляем полученный выход в результат
			restored.append(part);
		}
		// Финализируем поток декомпрессии
		decoder.finish(part);
		// Добавляем остаток в результат
		restored.append(part);
		// Печатаем результат работы потокового режима
		cout << "Compressed size: " << compressed.size() << ", Restored size: " << restored.size() << ", Match: " << ((restored == data) ? "OK" : "FAIL") << endl << flush;
		// Возвращаем пустую строку
		cout << endl << flush;
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
