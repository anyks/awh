/**
 * @file fs.cpp
 * @date 2026-01-24
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
 * @brief Пример работы с модулем файловой системы — демонстрация чтения и записи файлов, обхода каталогов,
 *        получения атрибутов и создания и удаления объектов файловой системы, а также пакетной
 *        работы внешними объектами файла и каталога, сброса записанного на носитель и обхода
 *        каталогов с остановкой и продолжением
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
#include <sys/fs.hpp>
#include <sys/fmk.hpp>

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
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 */
	awh::fmk::initialize();
	// Создаём объект для работы с логами
	// Создаём объект для работы с файловой системой
	fs_t fs;
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
	/**
	 * Пакетная работа одним объектом файла
	 *
	 * @note Объект файла заводит сам модуль и отдаёт умным указателем: описатель открывается
	 *       единожды и обслуживает все обращения к одному адресу. Без него всякое обращение
	 *       открывало бы файл заново, а открытие стоит перехода в ядро - под MS Windows около
	 *       0.37 мс на вызов
	 */
	{
		// Создаём объект файла средствами самого модуля
		const auto handle = fs.handleFile();
		// Признак того, что все доли легли в файл
		bool laid = true;
		// Выполняем пакетную дозапись одним описателем
		for(uint8_t i = 0; i < 4; i++)
			// Записи теперь отвечают вызывающему, а не одному лишь журналу
			laid = (fs.append("../Batch.txt", "Hello World!!!\n", handle) && laid);
		// Печатаем исход пакетной дозаписи
		cout << " !!! Batch append: " << laid << endl;
		/**
		 * Выполняем сброс записанного из ядра на носитель
		 *
		 * @note Сбрасывать надлежит ТЕМ ЖЕ объектом, каким шла запись: иной описатель
		 *       записанного этим не доведёт. Признак долговечности велит довести запись
		 *       до самого носителя, а не до вместилища накопителя
		 */
		cout << " !!! Flush: " << fs.flush("../Batch.txt", true, handle) << endl;
	}
	/**
	 * Чтение тем же объектом файла
	 *
	 * @note Объект несёт права той работы, какая его завела: заведённый чтением годен
	 *       одному чтению, заведённый дозаписью - одной дозаписи, а заведённый записью -
	 *       и записи, и чтению
	 */
	{
		// Создаём объект файла средствами самого модуля
		const auto handle = fs.handleFile();
		// Читаем файл построчно одним описателем
		fs.readfile("../Batch.txt", [](string_view str) noexcept -> void {
			// Возвращаем строку файла
			cout << " !!!! BATCH LINE: " << str << endl;
		}, fs_t::seek_t::BEGIN, 0, handle);
	}
	// Удаляем файл пакетной записи
	fs.unlink("../Batch.txt");
	/**
	 * Обход каталога с остановкой и продолжением
	 *
	 * @note Отклик у walkdir вправе обход остановить, вернув ложь, а объект каталога хранит
	 *       место остановки: следующий вызов продолжает с того же места, а не с начала.
	 *       Возврат говорит, довершён ли обход до конца
	 */
	{
		// Создаём объект каталога средствами самого модуля
		const auto handle = fs.handleDir();
		// Признак того, что обход довершён до конца
		bool done = false;
		// Число долей, за которое обход довершён
		uint16_t rounds = 0;
		/**
		 * Выполняем обход долями по три записи
		 */
		while(!done && (rounds < 1000)){
			// Число записей, выданных в этой доле
			uint8_t count = 0;
			// Выполняем очередную долю обхода
			done = fs.walkdir("..", "", false, [&count](const fs_t::type_t type, string_view filename) noexcept -> bool {
				// Печатаем имя файла
				cout << " !!!! WALK: " << filename << " || TYPE: " << static_cast <uint16_t> (type) << endl;
				// Останавливаем обход после третьей записи
				return (++count < 3);
			}, true, handle);
			// Считаем доли обхода
			rounds++;
			// Печатаем границу доли обхода
			cout << " !!!! ---- часть обхода " << rounds << (done ? " (обход довершён)" : "") << endl;
		}
	}
	/**
	 * Повторный обзор того же каталога тем же объектом
	 *
	 * @note Идёт он перемоткой, мимо повторного открытия. При рекурсивном обходе объект
	 *       служит КОРНЮ: подкаталоги модуль открывает своими описателями
	 */
	{
		// Создаём объект каталога средствами самого модуля
		const auto handle = fs.handleDir();
		// Число записей в каталоге
		size_t count = 0;
		/**
		 * Выполняем два обзора подряд одним объектом каталога
		 */
		for(uint8_t round = 0; round < 2; round++){
			// Сбрасываем счётчик записей
			count = 0;
			// Выполняем обзор каталога
			fs.readdir("..", "", false, [&count]([[maybe_unused]] const fs_t::type_t type, [[maybe_unused]] string_view filename) noexcept -> void {
				// Считаем записи каталога
				count++;
			}, true, handle);
			// Печатаем число найденных записей
			cout << " !!! Survey " << static_cast <uint16_t> (round) << ": " << count << endl;
		}
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
