/**
 * @file: fs.cpp
 * @date: 2026-01-24
 * @license: LicenseRef-AWH-1.0
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
#include <sys/fs.hpp>

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
	// Создаём объект для работы с файловой системой
	fs_t fs(&fmk, &log);
	// Проверяем соответствие директории в файловой системе по адресу
	cout << " !!!! IS DIR " << (fs_t::type_t::DIR == fs.type("../tests")) << endl;
	// Проверяем соответствие файлу в файловой системе по адресу
	cout << " !!!! IS FILE " << (fs_t::type_t::FILE == fs.type("../README.md")) << endl;
	// Проверяем соответствие символьной ссылки в файловой системе по адресу
	cout << " !!!! IS LINK " << (fs_t::type_t::LINK == fs.type("../README2.md")) << endl;
	// Проверяем соответствие ярлыка в файловой системе по адресу
	cout << " !!!! IS LINK " << (fs_t::type_t::LINK == fs.type("../README3.md")) << endl;
	// Создаём символьную ссылку
	fs.symlink("../tests", "../tests2");
	// Возвращаем адрес символьной ссылки
	cout << " !!!! Symlink " << fs.fullpath("../tests2") << " -> " << fs.fullpath("../tests2", true) << endl;
	// Возвращаем адрес ярлыка
	cout << " !!!! Shortcut " << fs.fullpath("../README3.md") << " -> " << fs.fullpath("../README3.md", true) << endl;
	// Создаём жёсткую ссылку
	fs.hardlink("../README.md", "../README4.md");
	// Удаляем файл по адресу
	fs.unlink("../README4.md");
	// Удаляем символьную ссылку по адресу
	fs.unlink("../tests2");
	// Удаляем каталог по адресу
	fs.unlink("../tests3");
	// Устанавливаем права доступа к файлу
	fs.chmod("../README.md", fs.chmod("../README.md"));
	// Устанавливаем владельца на файл
	// fs.chown("../README.md", "forman", "staff");
	// Создаём каталог
	cout << " !!! Create Dir: " << fs.mkdir("../data/test/goga") << endl;
	// cout << " !!! Create Dir: " << fs.mkdir("../data/test/goga", "forman", "staff") << endl;
	// Удаляем каталог по адресу
	cout << " !!! Remove Dir: " << fs.unlink("../data") << endl;
	// Извлекаем название и расширение файла
	const fs_t::components_t & components = fs.components("../README.md", false, true);
	// Возвращаем название и расширение файла
	cout << " !!! File Name: " << components.first << endl;
	cout << " !!! File Ext: " << components.second << endl;
	// Подсчитываем количество файлов в каталоге
	cout << " !!! File Count: " << fs.count("..", "md", true) << endl;
	// Подсчитываем размер файла/каталога
	cout << " !!! File Size: " << fs.size("..", "", true) << endl;
	// Добавляем в файл бинарные данные
	fs.append("../Data.txt", L"Hello World!!!\n");
	// Записываем в файл бинарные данные
	fs.write("../Data.txt", L"Hello World and ANYKS!!!\n");
	// Читаем из файла построчно
	fs.readfile("../README3.md", [](string_view str) noexcept -> void {
		// Возвращаем строку файла
		cout << " !!!! LINE: " << str << endl;
	});
	// Читаем из файла бинарными блоками
	fs.readfile("../README3.md", 4096, [](const void * data, const size_t size) noexcept -> void {
		// Возвращаем размер прочитанного буфера
		cout << " !!!! BUFFER SIZE: " << size << " || " << string(static_cast <const char *> (data), size) << endl;
	});
	// Рекурсивно получаем все файлы в каталоге
	fs.readdir("..", "", true, [](const fs_t::type_t type, string_view filename) noexcept -> void {
		// Печатаем имя файла
		cout << " !!!! FILE: " << filename << " || TYPE: " << static_cast <uint16_t> (type) << endl;
	});
	// Рекурсивно получаем все файлы с фильтром по расширению в каталоге
	fs.readdir("..", "md", false, [](const fs_t::type_t type, string_view filename, string_view text) noexcept -> void {
		// Печатаем имя файла и его содержимое
		cout << " !!!! FILTERED FILE: " << filename << " || TYPE: " << static_cast <uint16_t> (type) << " || CONTENT: " << text << endl;
	});
	// Возвращаем результат
	return EXIT_SUCCESS;
}
