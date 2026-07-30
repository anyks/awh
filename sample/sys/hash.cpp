/**
 * @file: hash.cpp
 * @date: 2026-07-30
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с быстрым некриптографическим хэшированием — демонстрация одноразового
 *        и потокового хэширования, вывода результата в числа произвольной разрядности и
 *        применения длинных чисел в качестве ключей ассоциативных контейнеров
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <vector>
#include <iostream>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/hash.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект хэширования данных
	hash_t hash;
	// Текст для хэширования
	const string text = "Hello World!!!";

	// Печатаем заголовок раздела
	cout << "----- Одноразовое хэширование -----" << endl;
	// Выводим 32-битный хэш текста
	cout << "UInt32:  " << hash.hash <uint32_t> (text) << endl;
	// Выводим 64-битный хэш текста
	cout << "UInt64:  " << hash.hash <uint64_t> (text) << endl;
	// Выводим 128-битный хэш текста
	cout << "UInt128: " << hash.hash <uint128_t> (text).print(bignum::format_t::HEX) << endl;
	// Выводим 256-битный хэш текста
	cout << "UInt256: " << hash.hash <uint256_t> (text).print(bignum::format_t::HEX) << endl;
	// Выводим 512-битный хэш текста
	cout << "UInt512: " << hash.hash <uint512_t> (text).print(bignum::format_t::HEX) << endl;
	/**
	 * Хэш меньшей разрядности является началом хэша большей разрядности, поэтому
	 * 64-битный хэш совпадает с младшими разрядами 256-битного хэша того же текста
	 */
	// Выводим младшие разряды 256-битного хэша текста
	cout << "Prefix:  " << static_cast <uint64_t> (hash.hash <uint256_t> (text)) << endl;

	// Печатаем заголовок раздела
	cout << endl << "----- Хэширование с ключом -----" << endl;
	// Устанавливаем начальное значение хэширования
	hash.seed(0x414E594B53ULL);
	// Выводим 64-битный хэш текста с ключом
	cout << "UInt64:  " << hash.hash <uint64_t> (text) << endl;
	// Сбрасываем начальное значение хэширования
	hash.seed(0);

	// Печатаем заголовок раздела
	cout << endl << "----- Потоковое хэширование -----" << endl;
	{
		// Создаём объект потокового хэширования
		hash_t stream;
		// Выполняем передачу первой части текста
		stream.update("Hello ");
		// Выполняем передачу второй части текста
		stream.update("World");
		// Выполняем передачу третьей части текста
		stream.update("!!!");
		// Выводим размер обработанных данных
		cout << "Length:  " << stream.length() << endl;
		// Выводим результат потокового хэширования
		cout << "UInt64:  " << stream.digest <uint64_t> () << endl;
		// Выводим совпадение потокового результата с одноразовым
		cout << "Equal:   " << (stream.digest <uint64_t> () == hash.hash <uint64_t> (text) ? "Yes" : "No") << endl;
		// Выполняем передачу продолжения текста, состояние хэширования получением результата не нарушено
		stream.update(" Anyks Framework");
		// Выводим результат хэширования продолженного текста
		cout << "UInt64:  " << stream.digest <uint64_t> () << endl;
	}

	// Печатаем заголовок раздела
	cout << endl << "----- Хэширование буферов данных -----" << endl;
	{
		// Формируем буфер данных для хэширования
		const vector <uint8_t> buffer(text.begin(), text.end());
		// Выводим хэш буфера данных
		cout << "Buffer:  " << hash.hash <uint64_t> (buffer) << endl;
		// Выводим хэш сырых данных
		cout << "Raw:     " << hash.hash <uint64_t> (text.data(), text.size()) << endl;
		// Создаём длинное число для хэширования
		const uint256_t num = uint256_t(1) << 200;
		// Выводим хэш длинного числа
		cout << "BigNum:  " << hash.hash <uint64_t> (num) << endl;
	}

	// Печатаем заголовок раздела
	cout << endl << "----- Длинное число как ключ контейнера -----" << endl;
	{
		// Создаём ассоциативный контейнер с ключом в виде длинного числа
		unordered_map <uint256_t, string> storage;
		// Добавляем записи в контейнер
		storage.emplace(hash.hash <uint256_t> ("Hello World!!!"), "Hello World!!!");
		// Добавляем записи в контейнер
		storage.emplace(hash.hash <uint256_t> ("Anyks Framework"), "Anyks Framework");
		// Выполняем поиск записи в контейнере по хэшу текста
		auto i = storage.find(hash.hash <uint256_t> ("Anyks Framework"));
		/**
		 * Если запись в контейнере найдена
		 */
		if(i != storage.end())
			// Выводим найденную запись контейнера
			cout << "Found:   " << i->second << endl;
		// Выводим количество записей в контейнере
		cout << "Records: " << storage.size() << endl;
	}
	// Выводим результат работы приложения
	return EXIT_SUCCESS;
}
