/**
 * @file: static.cpp
 * @date: 2026-01-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты модуля работы с файловой системой — проверка создания и сброса объекта модуля,
 *        а также корректности чтения и записи файлов, обхода каталогов, получения атрибутов и построения путей
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fs.hpp"

/**
 * @brief Метод настройки тестовой фикстуры
 *
 */
TEST_F(FSFixture, CreateFSTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Выполняем сброс объекта
	this->_fs.reset();
	// Проверяем что объект сброшен
	ASSERT_TRUE(this->_fs == nullptr);
}

/**
 * @brief Метод очистки тестовой фикстуры
 *
 */
TEST_F(FSFixture, ResetAndCreateFSTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Выполняем сброс объекта
	this->_fs.reset();
	// Проверяем что объект сброшен
	ASSERT_TRUE(this->_fs == nullptr);
	// Создаём объект работы с ФС
	this->_fs = std::make_unique <awh::fs_t> (this->_fmk.get(), this->_log.get());
	// Проверяем что объект создан
	ASSERT_TRUE(this->_fs != nullptr);
}

/**
 * @brief Метод повторного создания объекта работы с ФС
 *
 */
TEST_F(FSFixture, ReCreateFSTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Создаём объект работы с ФС
	this->_fs = std::make_unique <awh::fs_t> (this->_fmk.get(), this->_log.get());
	// Проверяем что объект создан
	ASSERT_TRUE(this->_fs != nullptr);
}

/**
 * @brief Тестирование методов работы с ФС
 *
 */
TEST_F(FSFixture, FSTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	
	// Проверка типа текущей директории
	awh::fs_t::type_t type = this->_fs->type(".");
	ASSERT_EQ(type, awh::fs_t::type_t::DIR);

	// Проверка полного пути
	std::string path = this->_fs->fullpath(".");
	ASSERT_FALSE(path.empty());
	
	// -------------------------------------------------------------
	// Тест создания и удаления каталогов
	// -------------------------------------------------------------
	std::string testDir = "test_dir_fs_unit";
	
	// Удаляем если существует (cleanup)
	if(this->_fs->type(testDir) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(testDir));

	// Создаем каталог
	ASSERT_TRUE(this->_fs->mkdir(testDir));
	ASSERT_EQ(this->_fs->type(testDir), awh::fs_t::type_t::DIR);
	
	// -------------------------------------------------------------
	// Тест операций с файлами (write, read, append, size)
	// -------------------------------------------------------------
	std::string testFile = testDir + "/test.txt";
	std::string content1 = "Hello";
	std::string content2 = " World";
	
	// Записываем файл (write)
	this->_fs->write(testFile, content1.c_str());
	ASSERT_EQ(this->_fs->type(testFile), awh::fs_t::type_t::FILE);
	
	// Проверяем размер
	ASSERT_EQ(this->_fs->size(testFile), content1.size());
	
	// Добавляем данные (append)
	this->_fs->append(testFile, content2.c_str());
	ASSERT_EQ(this->_fs->size(testFile), content1.size() + content2.size());
	
	// Читаем весь файл (read string)
	std::string readContent = this->_fs->read <std::string> (testFile);
	ASSERT_EQ(readContent, content1 + content2);
	
	// Читаем со смещением (read partial)
	std::string partialContent = this->_fs->read <std::string> (testFile, awh::fs_t::seek_t::BEGIN, content1.size());
	ASSERT_EQ(partialContent, content2);
	
	// -------------------------------------------------------------
	// Тест прав доступа (chmod)
	// -------------------------------------------------------------
	#if !_WIN32 && !_WIN64
		// Получаем текущие права
		uint32_t perms = this->_fs->chmod(testFile);
		// Меняем права (например 0777)
		ASSERT_TRUE(this->_fs->chmod(testFile, 0777));
		ASSERT_EQ(this->_fs->chmod(testFile) & 0777, 0777);
		// Возвращаем старые (или стандартные 0644)
		this->_fs->chmod(testFile, perms);
	#endif

	// -------------------------------------------------------------
	// Тест ссылок (symlink, hardlink)
	// -------------------------------------------------------------
	std::string symLink = testDir + "/symlink.txt";
	std::string hardLink = testDir + "/hardlink.txt";

	// Создаем символьную ссылку
	this->_fs->symlink(testFile, symLink);
	/**
	 * Для операционной системы MS Windows
	 *
	 * @note Символьной ссылки в понимании POSIX у MS Windows нет: заведение её требует
	 *       особого права SeCreateSymbolicLinkPrivilege, какого у обычного пользователя
	 *       не бывает. Оттого Filesystem::symlink заводит там ярлык оболочки, а тот
	 *       обязан носить расширение «.lnk» - без него оболочка ярлыка не распознаёт.
	 *       Проверять надлежит то имя, какое библиотека и создала
	 *
	 */
	#if _WIN32 || _WIN64
		// Дополняем адрес ссылки расширением ярлыка оболочки
		symLink.append(".lnk");
	#endif
	ASSERT_EQ(this->_fs->type(symLink), awh::fs_t::type_t::LINK);
	
	// Создаем жесткую ссылку
	this->_fs->hardlink(testFile, hardLink);
	ASSERT_EQ(this->_fs->type(hardLink), awh::fs_t::type_t::FILE); // Hardlink looks like a file
	
	// Проверяем чтение через симлинк
	std::string linkContent = this->_fs->read <std::string> (symLink);
	ASSERT_EQ(linkContent, content1 + content2);
	
	// -------------------------------------------------------------
	// Тест readdir (список файлов)
	// -------------------------------------------------------------
	std::vector <std::string> foundFiles;
	this->_fs->readdir(testDir, "", true, [&](const awh::fs_t::type_t type, std::string_view name) noexcept -> void {
		foundFiles.emplace_back(name);
	});
	
	// Должны найти test.txt, symlink.txt, hardlink.txt
	ASSERT_GE(foundFiles.size(), 3);
	
	// -------------------------------------------------------------
	// Тест readfile (построчное чтение)
	// -------------------------------------------------------------
	// Создаем многострочный файл
	std::string multiLineFile = testDir + "/multiline.txt";
	std::string mlContent = "Line1\nLine2\nLine3";
	this->_fs->write(multiLineFile, mlContent.c_str());
	
	std::vector <std::string> lines;
	this->_fs->readfile(multiLineFile, [&](std::string_view line) noexcept -> void {
		lines.emplace_back(line);
	});
	
	ASSERT_GE(lines.size(), 3);
	ASSERT_EQ(lines[0], "Line1");
	ASSERT_EQ(lines[1], "Line2");
	ASSERT_EQ(lines[2], "Line3");
	
	// -------------------------------------------------------------
	// Очистка
	// -------------------------------------------------------------
	// Удаляем каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(testDir));
	ASSERT_EQ(this->_fs->type(testDir), awh::fs_t::type_t::NONE);
}

/**
 * @brief Регрессия: рекурсивное создание глубоко вложенного пути (mkdir на std::string без snprintf/буфера PATH_MAX)
 *
 */
TEST_F(FSFixture, MkdirDeepNestedTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string root = "test_mkdir_deep_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(root) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(root));
	// Глубоко вложенный путь
	const std::string deep = root + "/a/b/c/d/e/f/g/h";
	// Создаём всю цепочку каталогов за один вызов
	ASSERT_TRUE(this->_fs->mkdir(deep));
	// Финальный каталог должен существовать
	ASSERT_EQ(this->_fs->type(deep), awh::fs_t::type_t::DIR);
	// Промежуточные каталоги тоже должны существовать
	ASSERT_EQ(this->_fs->type(root + "/a/b/c"), awh::fs_t::type_t::DIR);
	// Проверяем путь с завершающим разделителем
	const std::string withSlash = root + "/x/y/";
	// Создаём каталог с завершающим разделителем
	ASSERT_TRUE(this->_fs->mkdir(withSlash));
	// Каталог без завершающего разделителя должен существовать
	ASSERT_EQ(this->_fs->type(root + "/x/y"), awh::fs_t::type_t::DIR);
	// Удаляем корневой каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(root));
	// Каталог должен быть удалён
	ASSERT_EQ(this->_fs->type(root), awh::fs_t::type_t::NONE);
}

/**
 * @brief Регрессия: чтение со смещением за пределами размера файла не должно падать и должно возвращать пусто
 *
 */
TEST_F(FSFixture, ReadOffsetBeyondSizeTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string dir = "test_read_offset_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(dir) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(dir));
	// Создаём каталог
	ASSERT_TRUE(this->_fs->mkdir(dir));
	// Путь к файлу
	const std::string file = dir + "/data.bin";
	// Содержимое файла (5 байт)
	const std::string content = "12345";
	// Записываем файл
	this->_fs->write(file, content.c_str());
	// Проверяем размер
	ASSERT_EQ(this->_fs->size(file), static_cast <uintmax_t> (content.size()));
	// Смещение больше размера файла — результат пустой, без падения и без огромной аллокации
	ASSERT_TRUE((this->_fs->read <std::string> (file, awh::fs_t::seek_t::BEGIN, 100)).empty());
	// Смещение равно размеру файла — результат пустой
	ASSERT_TRUE((this->_fs->read <std::string> (file, awh::fs_t::seek_t::BEGIN, content.size())).empty());
	// Смещение внутри файла — читаем остаток
	ASSERT_EQ((this->_fs->read <std::string> (file, awh::fs_t::seek_t::BEGIN, 3)), "45");
	// Удаляем каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(dir));
}

/**
 * @brief Регрессия: чтение файла блоками с нулевым размером блока (без const_cast на const-параметре)
 *
 */
TEST_F(FSFixture, ReadfileZeroChunkSizeTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string dir = "test_readfile_zero_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(dir) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(dir));
	// Создаём каталог
	ASSERT_TRUE(this->_fs->mkdir(dir));
	// Путь к файлу
	const std::string file = dir + "/blob.bin";
	// Формируем содержимое заведомо больше одной страницы памяти
	std::string content;
	/**
	 * Наполняем содержимое данными
	 */
	for(size_t i = 0; i < 10000; ++i)
		// Добавляем строку с номером
		content.append("0123456789ABCDEF");
	// Записываем файл
	this->_fs->write(file, content.c_str());
	// Накопитель прочитанных данных
	std::string collected;
	// Читаем файл блоками с нулевым размером блока (должен использоваться размер страницы)
	this->_fs->readfile(file, 0, [&](const void * buffer, const size_t size) noexcept -> void {
		// Если буфер получен — добавляем к накопителю
		if((buffer != nullptr) && (size > 0))
			// Добавляем данные блока
			collected.append(reinterpret_cast <const char *> (buffer), size);
	});
	// Прочитанные данные должны полностью совпасть с записанными
	ASSERT_EQ(collected, content);
	// Удаляем каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(dir));
}

/**
 * @brief Регрессия: рекурсивное удаление вложенного дерева каталогов (дочерние пути из разрешённого адреса)
 *
 */
TEST_F(FSFixture, UnlinkRecursiveNestedTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string root = "test_unlink_tree_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(root) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(root));
	// Создаём вложенную структуру каталогов
	ASSERT_TRUE(this->_fs->mkdir(root + "/sub1/sub2"));
	// Создаём файлы на разных уровнях вложенности
	this->_fs->write(root + "/f1.txt", "level0");
	// Файл первого уровня
	this->_fs->write(root + "/sub1/f2.txt", "level1");
	// Файл второго уровня
	this->_fs->write(root + "/sub1/sub2/f3.txt", "level2");
	// Проверяем, что файлы созданы
	ASSERT_EQ(this->_fs->type(root + "/sub1/sub2/f3.txt"), awh::fs_t::type_t::FILE);
	// Удаляем всё дерево рекурсивно
	ASSERT_TRUE(this->_fs->unlink(root));
	// Дерево должно быть полностью удалено
	ASSERT_EQ(this->_fs->type(root), awh::fs_t::type_t::NONE);
}

/**
 * @brief Регрессия: построчное чтение с разными переводами строк и без финального перевода (без O(n^2))
 *
 */
TEST_F(FSFixture, ReadfileLineEndingsTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string dir = "test_readfile_lines_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(dir) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(dir));
	// Создаём каталог
	ASSERT_TRUE(this->_fs->mkdir(dir));
	/**
	 * @brief Вспомогательная функция чтения строк файла
	 *
	 * @param name    имя файла
	 * @param content содержимое для записи
	 * @return        список прочитанных строк
	 *
	 */
	auto readLines = [&](const std::string & name, const std::string & content) -> std::vector <std::string> {
		// Путь к файлу
		const std::string file = dir + "/" + name;
		// Записываем содержимое
		this->_fs->write(file, content.c_str());
		// Список прочитанных строк
		std::vector <std::string> lines;
		// Читаем файл построчно
		this->_fs->readfile(file, [&](std::string_view line) noexcept -> void {
			// Добавляем строку в список
			lines.emplace_back(line);
		});
		// Возвращаем результат
		return lines;
	};
	// Unix-переводы строк с финальным переводом
	{
		// Читаем строки
		const auto lines = readLines("unix.txt", "Line1\nLine2\nLine3\n");
		// Проверяем количество и содержимое
		ASSERT_EQ(lines.size(), 3u);
		// Содержимое строк
		ASSERT_EQ(lines[0], "Line1");
		// Содержимое строк
		ASSERT_EQ(lines[1], "Line2");
		// Содержимое строк
		ASSERT_EQ(lines[2], "Line3");
	}
	// Windows-переводы строк без финального перевода
	{
		// Читаем строки
		const auto lines = readLines("win.txt", "A\r\nB\r\nC");
		// Проверяем количество и содержимое
		ASSERT_EQ(lines.size(), 3u);
		// Содержимое строк (без \r)
		ASSERT_EQ(lines[0], "A");
		// Содержимое строк (без \r)
		ASSERT_EQ(lines[1], "B");
		// Содержимое строк (без \r)
		ASSERT_EQ(lines[2], "C");
	}
	// Пустая строка в середине должна сохраняться
	{
		// Читаем строки
		const auto lines = readLines("empty.txt", "x\n\ny");
		// Проверяем количество и содержимое
		ASSERT_EQ(lines.size(), 3u);
		// Первая строка
		ASSERT_EQ(lines[0], "x");
		// Пустая строка
		ASSERT_EQ(lines[1], "");
		// Последняя строка
		ASSERT_EQ(lines[2], "y");
	}
	// Удаляем каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(dir));
}

/**
 * @brief Регрессия: подсчёт размера и количества файлов с фильтром по расширению и рекурсией (прямой stat)
 *
 */
TEST_F(FSFixture, SizeCountExtensionTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string root = "test_size_ext_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(root) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(root));
	// Создаём вложенную структуру
	ASSERT_TRUE(this->_fs->mkdir(root + "/sub"));
	// Файлы верхнего уровня
	this->_fs->write(root + "/a.txt", "aaa");   // 3 байта, txt
	// Ещё один txt-файл
	this->_fs->write(root + "/b.txt", "bb");    // 2 байта, txt
	// Файл с другим расширением
	this->_fs->write(root + "/c.log", "c");     // 1 байт, log
	// Файл во вложенном каталоге
	this->_fs->write(root + "/sub/d.txt", "dddd"); // 4 байта, txt
	// Размер только txt-файлов с рекурсией: 3 + 2 + 4 = 9
	ASSERT_EQ(this->_fs->size(root, "txt", true), static_cast <uintmax_t> (9));
	// Размер только txt-файлов без рекурсии: 3 + 2 = 5
	ASSERT_EQ(this->_fs->size(root, "txt", false), static_cast <uintmax_t> (5));
	// Размер всех файлов с рекурсией: 3 + 2 + 1 + 4 = 10
	ASSERT_EQ(this->_fs->size(root, "", true), static_cast <uintmax_t> (10));
	// Количество txt-файлов с рекурсией: a, b, d = 3
	ASSERT_EQ(this->_fs->count(root, "txt", true), static_cast <uintmax_t> (3));
	// Количество txt-файлов без рекурсии: a, b = 2
	ASSERT_EQ(this->_fs->count(root, "txt", false), static_cast <uintmax_t> (2));
	// Количество всех файлов с рекурсией: a, b, c, d = 4
	ASSERT_EQ(this->_fs->count(root, "", true), static_cast <uintmax_t> (4));
	// Удаляем каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(root));
}

/**
 * @brief Регрессия: перегрузка type(addr, detectLinks) сохраняет определение базовых типов
 *
 */
TEST_F(FSFixture, TypeDetectLinksOverloadTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string dir = "test_type_detect_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(dir) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(dir));
	// Создаём каталог
	ASSERT_TRUE(this->_fs->mkdir(dir));
	// Путь к файлу
	const std::string file = dir + "/file.txt";
	// Создаём файл
	this->_fs->write(file, "data");
	// Каталог определяется как каталог независимо от детекта ссылок
	ASSERT_EQ(this->_fs->type(dir, true), awh::fs_t::type_t::DIR);
	// Каталог определяется как каталог и с отключённым детектом ссылок
	ASSERT_EQ(this->_fs->type(dir, false), awh::fs_t::type_t::DIR);
	// Файл определяется как файл независимо от детекта ссылок
	ASSERT_EQ(this->_fs->type(file, true), awh::fs_t::type_t::FILE);
	// Файл определяется как файл и с отключённым детектом ссылок
	ASSERT_EQ(this->_fs->type(file, false), awh::fs_t::type_t::FILE);
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Путь к символьной ссылке
		const std::string link = dir + "/link.txt";
		// Создаём символьную ссылку на файл
		this->_fs->symlink(file, link);
		// При включённом детекте ссылка определяется как ссылка
		ASSERT_EQ(this->_fs->type(link, true), awh::fs_t::type_t::LINK);
		// При отключённом детекте тип берётся по цели ссылки (обычный файл)
		ASSERT_EQ(this->_fs->type(link, false), awh::fs_t::type_t::FILE);
	#endif
	// Удаляем каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(dir));
}

/**
 * @brief Проверка работы с путями, записанными кириллицей
 *
 * @details Проверка заведена по дефекту, найденному под MS Windows: узкие обращения
 *          системы принимают путь не в UTF-8, а в кодовой странице системы (CP1251
 *          на русской машине), и путь `кириллица.txt` создавал файл с именем
 *          `?????????.txt`. Отказа при этом не было вовсе - обращение отвечало
 *          годным дескриптором, - оттого дефект и жил незамеченным
 *
 * @note Проверка нужна на всех системах, а не под одной MS Windows: у прочих узкие
 *       обращения принимают UTF-8 как есть, и одинаковое поведение движков по
 *       кириллическим путям - как раз то, что закрепляется
 *
 */
TEST_F(FSFixture, CyrillicPathTest){
	// Путь к каталогу с кириллическим названием
	const std::string dir = "тестовый каталог";
	// Путь к файлу с кириллическим названием
	const std::string file = (dir + "/мой файл.txt");
	// Содержимое проверяемого файла
	const std::string content = "Проверка кириллицы";
	// Удаляем каталог, если он остался от прошлого прогона
	if(this->_fs->type(dir) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(dir));
	// Создаём каталог с кириллическим названием
	ASSERT_TRUE(this->_fs->mkdir(dir));
	// Каталог обязан опознаваться каталогом
	ASSERT_EQ(this->_fs->type(dir), awh::fs_t::type_t::DIR);
	// Записываем файл с кириллическим названием
	this->_fs->write(file, content.c_str());
	// Файл обязан опознаваться файлом
	ASSERT_EQ(this->_fs->type(file), awh::fs_t::type_t::FILE);
	// Размер файла обязан совпасть с размером записанного
	ASSERT_EQ(this->_fs->size(file), content.size());
	// Прочитанное обязано совпасть с записанным
	ASSERT_EQ(this->_fs->read <std::string> (file), content);
	// Выполняем дозапись данных в файл
	this->_fs->append(file, content.c_str());
	// Размер файла обязан удвоиться после дозаписи
	ASSERT_EQ(this->_fs->size(file), (content.size() * 2));
	/**
	 * Название файла обязано вернуться обходом каталога тем же, каким его задавали
	 *
	 * @note Именно здесь и всплывала подмена кодовой страницы: файл создавался, но
	 *       обход каталога отдавал иное название, и найти созданное было нельзя
	 */
	bool found = false;
	// Выполняем обход каталога
	this->_fs->readdir(dir, "", false, [&found](const awh::fs_t::type_t type, std::string_view name) noexcept -> void {
		// Если найдено название заданного файла
		if((type == awh::fs_t::type_t::FILE) && (name.find("мой файл.txt") != std::string_view::npos))
			// Отмечаем название найденным
			found = true;
	});
	// Название файла обязано найтись обходом каталога
	ASSERT_TRUE(found);
	// Полный путь обязан оканчиваться заданным названием
	ASSERT_NE(this->_fs->fullpath(file).find("мой файл.txt"), std::string::npos);
	// Удаляем каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(dir));
}

/**
 * @brief Проверка раскрытия переменных окружения в пути
 *
 * @details Под MS Windows путь вправе нести в себе переменные окружения
 *          (`%TEMP%\text.txt`), и раскрывать их обязан разбор пути. Прочие системы
 *          такой записи не знают, оттого проверка ограничена MS Windows
 *
 */
TEST_F(FSFixture, EnvironmentPathTest){
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Собираем путь с переменной окружения
		const std::string path = "%TEMP%\\awh_env_probe.txt";
		// Получаем полный путь
		const std::string result = this->_fs->fullpath(path);
		// Переменная окружения обязана быть раскрыта
		ASSERT_EQ(result.find('%'), std::string::npos);
		// Раскрытый путь обязан оканчиваться заданным названием
		ASSERT_NE(result.find("awh_env_probe.txt"), std::string::npos);
		// Раскрытый путь обязан отличаться от заданного
		ASSERT_NE(result, path);
	#endif
}
