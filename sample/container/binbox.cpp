/**
 * @file: binbox.cpp
 * @date: 2026-02-28
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с контейнером бинарных данных — демонстрация добавления записей произвольного размера,
 *        обхода их итератором и извлечения содержимого
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
#include <container/binbox.hpp>

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
	// Создаём объект для работы с бинарным контейнером
	binbox_t binbox(&fmk, &log);
	// Добавляем строковый тип данных в контейнер
	binbox.add("Text", string{"Hello World!!!"});
	// Добавляем бинарные данные в контейнер
	binbox.add("Buffer", "Hello World!!!", 14);
	// Добавляем простые типы данных в контейнер
	binbox.add("Int", 123456789);
	binbox.add("Float", 3.1415926f);
	binbox.add("Double", 3.14159265358979323846);
	binbox.add("Boolean", true);
	/**
	 * Проверяем все записи в контейнере
	 */
	for(auto & record : binbox){
		// Возвращаем размер данных
		cout << "Size: " << record.size << endl;
		// Если это текстовые данные
		if(record.size == 14)
			// Возвращаем данные в виде строки
			cout << "Data: " << string(reinterpret_cast <const char *> (record.buffer.get()), record.size) << endl;
	}
	// Печатаем разделитель
	cout << "-----------------------------" << endl;
	// Сохраняем контейнер в файл
	binbox.save("binbox.dat");
	// Загружаем контейнер из файла
	binbox.load("binbox.dat");
	/**
	 * Проверяем все записи в контейнере после загрузки из файла
	 */
	for(auto & record : binbox){
		// Возвращаем размер данных
		cout << "Size: " << record.size << endl;
		// Если это текстовые данные
		if(record.size == 14)
			// Возвращаем данные в виде строки
			cout << "Data: " << string(reinterpret_cast <const char *> (record.buffer.get()), record.size) << endl;
	}
	// Печатаем разделитель
	cout << "-----------------------------" << endl;
	// Размер буфера данных
	size_t size = 0;
	// Буфер для извлечения бинарных данных
	uint8_t * buffer = nullptr;
	// Извлекаем бинарные данные из контейнера
	binbox.get("Buffer", &buffer, &size);
	// Возвращаем буфер данных в виде строки
	cout << " Buffer: " << string(reinterpret_cast <const char *> (buffer), size) << endl;
	// Возвращаем текст из контейнера
	cout << " Text: " << binbox.get <string>("Text") << endl;
	// Возвращаем простые типы данных из контейнера
	cout << " Int: " << binbox.get <int> ("Int") << endl;
	cout << " Float: " << binbox.get <float> ("Float") << endl;
	cout << " Double: " << binbox.get <double> ("Double") << endl;
	cout << " Boolean: " << binbox.get <bool> ("Boolean") << endl;
	// Возвращаем результат
	return EXIT_SUCCESS;
}
