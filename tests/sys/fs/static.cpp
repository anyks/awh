/**
 * @file static.cpp
 * @date 2026-01-25
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
 * @brief Статические тесты модуля работы с файловой системой — проверка создания и сброса объекта модуля,
 *        а также корректности чтения и записи файлов, обхода каталогов, получения атрибутов и построения путей
 *
 * @copyright Copyright © 2026
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
	this->_fs = std::make_unique <awh::fs_t> ();
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
	this->_fs = std::make_unique <awh::fs_t> ();
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
	this->_fs->readdir(testDir, "", true, [&]([[maybe_unused]] const awh::fs_t::type_t type, std::string_view name) noexcept -> void {
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
 * @brief Проверка работы с файлом, открытым кем-то ещё
 *
 * @details Проверка заведена по дефекту, найденному под MS Windows: обращения к
 *          файлам открывались с дозволением одного лишь чтения
 *          (`FILE_SHARE_READ`), и пока файл держал открытым кто-то ещё - движок
 *          наблюдения за файловой системой держит наблюдаемый файл именно так, -
 *          система отвечала отказом `ERROR_SHARING_VIOLATION`
 *
 * @note Молчание отказа и было главной бедой: `append` уходил мимо файла, `size`
 *       отвечал нулём, и ни один из них об отказе не сообщал. Проверка потому и
 *       сличает не отсутствие отказа, а САМ ИСХОД - выросший размер и прочитанное
 *       содержимое
 *
 * @note Проверка нужна на всех системах, а не под одной MS Windows: у прочих
 *       открытие файла никого не запирает вовсе, и одинаковое поведение - как раз
 *       то, что закрепляется
 *
 */
TEST_F(FSFixture, SharedOpenFileTest){
	// Путь к проверяемому файлу
	const std::string file = "./shared_open_probe.txt";
	// Первоначальное содержимое проверяемого файла
	const std::string content = "AAA";
	// Содержимое, дописываемое к проверяемому файлу
	const std::string extra = "BBB";
	// Удаляем файл, если он остался от прошлого прогона
	if(this->_fs->type(file) != awh::fs_t::type_t::NONE)
		// Выполняем удаление проверяемого файла
		ASSERT_TRUE(this->_fs->unlink(file));
	// Записываем проверяемый файл
	this->_fs->write(file, content.c_str());
	// Файл обязан опознаваться файлом
	ASSERT_EQ(this->_fs->type(file), awh::fs_t::type_t::FILE);
	/**
	 * Открываем файл вторым описателем и НЕ закрываем его
	 *
	 * @note Открытие идёт на чтение и запись: запертость наступает именно от чужого
	 *       права записи, и открытие на одно лишь чтение дефекта бы не показало
	 */
	FILE * holder = ::fopen(file.c_str(), "r+b");
	// Второй описатель обязан завестись
	ASSERT_NE(holder, nullptr);
	// Размер файла обязан читаться и при открытом чужом описателе
	ASSERT_EQ(this->_fs->size(file), content.size());
	// Содержимое файла обязано читаться и при открытом чужом описателе
	ASSERT_EQ(this->_fs->read <std::string> (file), content);
	// Выполняем дозапись данных в файл при открытом чужом описателе
	this->_fs->append(file, extra.c_str());
	// Размер файла обязан вырасти на размер дописанного
	ASSERT_EQ(this->_fs->size(file), (content.size() + extra.size()));
	// Прочитанное обязано совпасть с записанным и дописанным
	ASSERT_EQ(this->_fs->read <std::string> (file), (content + extra));
	// Закрываем второй описатель
	::fclose(holder);
	// Удаляем проверяемый файл
	ASSERT_TRUE(this->_fs->unlink(file));
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

/**
 * @brief Проверка пакетной дозаписи одним внешним объектом файла
 *
 * @details Внешний объект файла затем и заводится, чтобы ОДИН описатель обслуживал
 *          МНОГО обращений к одному адресу. Проверка сличает состав файла после
 *          нескольких дозаписей и убеждается, что дозапись ложится в конец даже
 *          после стороннего смещения позиции записи
 *
 * @note Признак `O_APPEND` (а под MS Windows `FILE_APPEND_DATA`) принадлежит самому
 *       описателю, а не вызову: ядро кладёт запись в конец, минуя текущую позицию.
 *       Свойство это и проверяется чужой записью в середину между дозаписями
 *
 */
TEST_F(FSFixture, BatchAppendByExternalFileHandleTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Проверяемый файл
	const std::string file = "test_batch_append_unit.bin";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(file) != awh::fs_t::type_t::NONE)
		// Удаляем проверяемый файл
		ASSERT_TRUE(this->_fs->unlink(file));
	{
		// Заводим объект файла средствами самого модуля
		const auto handle = this->_fs->handleFile();
		// Объект файла обязан завестись
		ASSERT_TRUE(handle != nullptr);
		/**
		 * Выполняем дозапись одним и тем же описателем
		 */
		for(uint8_t i = 0; i < 4; i++)
			// Выполняем дозапись очередной доли
			this->_fs->append(file, "ab", 2, handle);
	}
	// Состав файла обязан сложиться из всех долей по порядку
	ASSERT_EQ(this->_fs->read <std::string> (file), "abababab");
	// Размер файла обязан равняться сумме долей
	ASSERT_EQ(this->_fs->size(file), 8u);
	{
		// Заводим объект файла средствами самого модуля
		const auto handle = this->_fs->handleFile();
		// Выполняем дозапись первой доли
		this->_fs->append(file, "1", 1, handle);
		// Выполняем чужую запись в самое начало файла
		this->_fs->write(file, "Z", 1, awh::fs_t::seek_t::BEGIN, 0);
		// Выполняем дозапись второй доли
		this->_fs->append(file, "2", 1, handle);
	}
	// Обе доли обязаны лечь в конец, а чужая запись - в начало
	ASSERT_EQ(this->_fs->read <std::string> (file), "Zbababab12");
	// Удаляем проверяемый файл
	ASSERT_TRUE(this->_fs->unlink(file));
}

/**
 * @brief Проверка повторного обзора каталога внешним объектом каталога
 *
 * @details Внешний объект каталога служит каталогу, названному в `path`, ровно как
 *          объект файла служит файлу. Повторный обзор тем же объектом идёт перемоткой,
 *          мимо повторного открытия, - и обязан дать тот же самый состав
 *
 * @note Проверка эта зряча: убери перемотку, и повторный обзор даст ПУСТО - описатель
 *       остаётся стоять на исчерпанном конце каталога
 *
 */
TEST_F(FSFixture, RepeatedSurveyByExternalDirHandleTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string root = "test_dir_handle_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(root) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(root));
	// Заводим дерево каталогов
	ASSERT_TRUE(this->_fs->mkdir(root + "/a/b"));
	// Заводим ещё один каталог
	ASSERT_TRUE(this->_fs->mkdir(root + "/c"));
	// Заводим файлы по всей глубине дерева
	this->_fs->write(root + "/one.txt", "1", 1);
	// Заводим файл первого уровня вложенности
	this->_fs->write(root + "/a/two.txt", "2", 1);
	// Заводим файл второго уровня вложенности
	this->_fs->write(root + "/a/b/three.txt", "3", 1);
	// Заводим файл соседнего каталога
	this->_fs->write(root + "/c/four.log", "4", 1);
	// Состав дерева, снятый без внешнего объекта каталога
	std::vector <std::string> plain;
	// Выполняем обход дерева без внешнего объекта каталога
	this->_fs->readdir(root, "", true, [&plain]([[maybe_unused]] const awh::fs_t::type_t type, std::string_view address) noexcept -> void {
		// Запоминаем очередную запись
		plain.emplace_back(address);
	});
	// Дерево обязано дать все семь записей
	ASSERT_EQ(plain.size(), 7u);
	// Заводим объект каталога средствами самого модуля
	const auto handle = this->_fs->handleDir();
	// Объект каталога обязан завестись
	ASSERT_TRUE(handle != nullptr);
	// Состав дерева, снятый внешним объектом каталога
	std::vector <std::string> first;
	// Выполняем обход дерева внешним объектом каталога
	this->_fs->readdir(root, "", true, [&first]([[maybe_unused]] const awh::fs_t::type_t type, std::string_view address) noexcept -> void {
		// Запоминаем очередную запись
		first.emplace_back(address);
	}, true, handle);
	// Состав обязан совпасть с составом без внешнего объекта
	ASSERT_EQ(first, plain);
	// Состав дерева, снятый тем же объектом повторно
	std::vector <std::string> second;
	// Выполняем повторный обход дерева тем же объектом каталога
	this->_fs->readdir(root, "", true, [&second]([[maybe_unused]] const awh::fs_t::type_t type, std::string_view address) noexcept -> void {
		// Запоминаем очередную запись
		second.emplace_back(address);
	}, true, handle);
	// Повторный обзор обязан дать тот же самый состав
	ASSERT_EQ(second, plain);
	// Удаляем корневой каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(root));
}

/**
 * @brief Проверка обхода каталога долями с остановкой и продолжением
 *
 * @details Отклик у `walkdir` вправе обход остановить, вернув ложь. Если при этом
 *          передан внешний объект каталога, объект хранит место остановки, и
 *          следующий вызов продолжает обход с того же места, а не с начала
 *
 * @note Проверка сличает состав, собранный долями, с составом, собранным одним
 *       обходом: продолжение обязано не терять записи и не выдавать их дважды.
 *       Без внешнего объекта каталога продолжать нечем - обход всякий раз
 *       начинается заново, и это проверяется отдельно
 *
 */
TEST_F(FSFixture, WalkdirResumeByExternalDirHandleTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string root = "test_dir_walk_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(root) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(root));
	// Заводим дерево каталогов
	ASSERT_TRUE(this->_fs->mkdir(root + "/a/b"));
	// Заводим ещё один каталог
	ASSERT_TRUE(this->_fs->mkdir(root + "/c"));
	// Заводим файлы по всей глубине дерева
	this->_fs->write(root + "/one.txt", "1", 1);
	// Заводим файл первого уровня вложенности
	this->_fs->write(root + "/a/two.txt", "2", 1);
	// Заводим файл второго уровня вложенности
	this->_fs->write(root + "/a/b/three.txt", "3", 1);
	// Заводим файл соседнего каталога
	this->_fs->write(root + "/c/four.log", "4", 1);
	// Состав дерева, снятый одним обходом
	std::vector <std::string> whole;
	// Выполняем обход дерева целиком
	this->_fs->readdir(root, "", true, [&whole]([[maybe_unused]] const awh::fs_t::type_t type, std::string_view address) noexcept -> void {
		// Запоминаем очередную запись
		whole.emplace_back(address);
	});
	// Дерево обязано дать все семь записей
	ASSERT_EQ(whole.size(), 7u);
	// Заводим объект каталога средствами самого модуля
	const auto handle = this->_fs->handleDir();
	// Состав дерева, собранный долями
	std::vector <std::string> parts;
	// Число долей, за которое обход довершён
	uint16_t rounds = 0;
	// Признак того, что обход довершён до конца
	bool done = false;
	/**
	 * Выполняем обход долями по две записи
	 */
	while(!done && (rounds < 100)){
		// Число записей, выданных в этой доле
		uint8_t count = 0;
		// Выполняем очередную долю обхода
		done = this->_fs->walkdir(root, "", true, [&parts, &count]([[maybe_unused]] const awh::fs_t::type_t type, std::string_view address) noexcept -> bool {
			// Запоминаем очередную запись
			parts.emplace_back(address);
			// Останавливаем обход после второй записи
			return (++count < 2);
		}, true, handle);
		// Считаем доли обхода
		rounds++;
	}
	// Обход обязан быть довершён до конца
	ASSERT_TRUE(done);
	// Долей обязано выйти больше одной, иначе обход не дробился вовсе
	ASSERT_GT(rounds, 1u);
	// Состав, собранный долями, обязан совпасть с составом одного обхода
	ASSERT_EQ(parts, whole);
	// Состав, снятый без внешнего объекта каталога
	std::vector <std::string> single;
	// Число записей, выданных без внешнего объекта каталога
	uint8_t count = 0;
	// Выполняем обход без внешнего объекта каталога
	const bool result = this->_fs->walkdir(root, "", true, [&single, &count]([[maybe_unused]] const awh::fs_t::type_t type, std::string_view address) noexcept -> bool {
		// Запоминаем очередную запись
		single.emplace_back(address);
		// Останавливаем обход после второй записи
		return (++count < 2);
	});
	// Остановленный обход довершённым до конца считаться не вправе
	ASSERT_FALSE(result);
	// Без внешнего объекта каталога обход отдаёт ровно то, что успел
	ASSERT_EQ(single.size(), 2u);
	// Удаляем корневой каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(root));
}

/**
 * @brief Проверка ответа записи и дозаписи вызывающему
 *
 * @details Прежде `write` и `append` отвечали пустотой: отказ уходил в журнал, а
 *          вызывающий о нём не узнавал вовсе. Контейнеру же, обязанному ответить своим
 *          признаком ошибки, знать это необходимо
 *
 */
TEST_F(FSFixture, WriteAndAppendReportTheOutcomeTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Проверяемый файл
	const std::string file = "test_write_outcome_unit.bin";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(file) != awh::fs_t::type_t::NONE)
		// Удаляем проверяемый файл
		ASSERT_TRUE(this->_fs->unlink(file));
	// Запись обязана ответить успехом
	ASSERT_TRUE(this->_fs->write(file, "abcd", 4));
	// Дозапись обязана ответить успехом
	ASSERT_TRUE(this->_fs->append(file, "ef", 2));
	// Прочитанное обязано совпасть с записанным
	ASSERT_EQ(this->_fs->read <std::string> (file), "abcdef");
	// Запись по пустому адресу обязана ответить отказом
	ASSERT_FALSE(this->_fs->write("", "x", 1));
	// Дозапись по пустому адресу обязана ответить отказом
	ASSERT_FALSE(this->_fs->append("", "x", 1));
	// Запись пустого буфера обязана ответить отказом
	ASSERT_FALSE(this->_fs->write(file, nullptr, 0));
	// Удаляем проверяемый файл
	ASSERT_TRUE(this->_fs->unlink(file));
}

/**
 * @brief Проверка сброса записанного из ядра на носитель
 *
 * @details Запись, отвеченная успехом, лежит ещё во вместилище ядра, и обрыв питания её
 *          теряет. Работа `flush` доводит записанное до носителя. Проверить сам обрыв
 *          питания проверкой нельзя, оттого пиннится договор: сброс отвечает успехом на
 *          существующем файле - и своим объектом файла, и без него, - и отказом на
 *          отсутствующем
 *
 * @note Признак `durable` под MS Windows ничего не меняет: `FlushFileBuffers` сбрасывает
 *       и данные, и сведения о файле. Оба его значения проверяются на всех системах
 *
 */
TEST_F(FSFixture, FlushBringsTheWrittenOntoTheMediumTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Проверяемый файл
	const std::string file = "test_flush_unit.bin";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(file) != awh::fs_t::type_t::NONE)
		// Удаляем проверяемый файл
		ASSERT_TRUE(this->_fs->unlink(file));
	// Заводим объект файла средствами самого модуля
	const auto handle = this->_fs->handleFile();
	// Выполняем пакетную дозапись одним описателем
	ASSERT_TRUE(this->_fs->append(file, "abcd", 4, handle));
	// Сброс тем же объектом файла обязан быть выполнен
	ASSERT_TRUE(this->_fs->flush(file, true, handle));
	// Сброс без доведения до носителя обязан быть выполнен
	ASSERT_TRUE(this->_fs->flush(file, false, handle));
	// Сброс без внешнего объекта файла обязан быть выполнен
	ASSERT_TRUE(this->_fs->flush(file));
	// Записанное обязано быть на месте
	ASSERT_EQ(this->_fs->read <std::string> (file), "abcd");
	// Сброс отсутствующего файла обязан ответить отказом
	ASSERT_FALSE(this->_fs->flush("test_flush_unit_absent.bin"));
	// Сброс по пустому адресу обязан ответить отказом
	ASSERT_FALSE(this->_fs->flush(""));
	// Удаляем проверяемый файл
	ASSERT_TRUE(this->_fs->unlink(file));
}

/**
 * @brief Проверка того, что при заведённом объекте файла адрес не разбирается
 *
 * @details Разбор пути стоит перехода в ядро (`realpath` под POSIX), а при уже открытом
 *          описателе адрес не нужен вовсе - он идёт только в открытие. Проверка подаёт
 *          заведомо негодный адрес: разбор его бы не прошёл, а работа обязана лечь в тот
 *          файл, каким объект был заведён
 *
 * @warning Это же и договор об опасности: один объект файла обслуживает ОДИН файл, и
 *          поданное иное имя при заведённом объекте будет попросту пропущено
 *
 */
TEST_F(FSFixture, TheAddressIsNotParsedAtTheCreatedFileHandleTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Проверяемый файл
	const std::string file = "test_lazy_address_unit.bin";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(file) != awh::fs_t::type_t::NONE)
		// Удаляем проверяемый файл
		ASSERT_TRUE(this->_fs->unlink(file));
	// Заводим объект файла средствами самого модуля
	const auto handle = this->_fs->handleFile();
	// Выполняем первую дозапись, ею объект файла и заводится
	ASSERT_TRUE(this->_fs->append(file, "a", 1, handle));
	// Заведомо негодный адрес, разбор пути его бы не прошёл
	const std::string absent = "test_lazy_address_unit_no/such/path/at/all.bin";
	// Такого адреса быть не должно
	ASSERT_EQ(this->_fs->type(absent), awh::fs_t::type_t::NONE);
	// Дозапись негодным адресом при открытом описателе обязана быть выполнена
	ASSERT_TRUE(this->_fs->append(absent, "b", 1, handle));
	// Обе доли обязаны лечь в тот файл, каким объект был заведён
	ASSERT_EQ(this->_fs->read <std::string> (file), "ab");
	// Негодный адрес заведён быть не должен
	ASSERT_EQ(this->_fs->type(absent), awh::fs_t::type_t::NONE);
	// Удаляем проверяемый файл
	ASSERT_TRUE(this->_fs->unlink(file));
}

/**
 * @brief Проверка прав доступа, какие несёт объект файла
 *
 * @details Объект несёт права той работы, какая его завела, а не той, какой он передан
 *          следом. Проверка сличает все три случая: заведённый записью годен и чтению,
 *          заведённый дозаписью чтению не годен, заведённый чтением не годен записи
 *
 * @warning Права эти обязаны быть ОДИНАКОВЫ на всех системах. Под MS Windows запись
 *          испрашивает `GENERIC_READ` вместе с `GENERIC_WRITE` ровно ради этого:
 *          испроси она одно лишь право записи, чтение тем же объектом проходило бы
 *          под POSIX и отвечало отказом под MS Windows
 *
 * @note Отказ обязан быть слышен: `write` отвечает ложью, а `read` - ПУСТЫМ результатом.
 *       Прежде отказавшее чтение оставляло буфер размеченным нулями, и отличить
 *       прочитанное от неудавшегося было нельзя вовсе
 *
 */
TEST_F(FSFixture, TheFileHandleCarriesTheRightsOfItsCreatorTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Проверяемый файл
	const std::string file = "test_handle_rights_unit.bin";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(file) != awh::fs_t::type_t::NONE)
		// Удаляем проверяемый файл
		ASSERT_TRUE(this->_fs->unlink(file));
	// Заводим проверяемый файл
	ASSERT_TRUE(this->_fs->write(file, "abcdefgh", 8));
	{
		// Заводим объект файла средствами самого модуля
		const auto handle = this->_fs->handleFile();
		// Запись обязана ответить успехом, ею объект файла и заводится
		ASSERT_TRUE(this->_fs->write(file, "XY", 2, awh::fs_t::seek_t::BEGIN, 0, handle));
		// Прочитанное значение
		std::string result = "";
		// Чтение тем же объектом обязано пройти
		this->_fs->read(file, result, awh::fs_t::seek_t::BEGIN, 0, handle);
		// Прочитанное обязано совпасть с записанным
		ASSERT_EQ(result, "XYcdefgh");
	}
	{
		// Заводим объект файла средствами самого модуля
		const auto handle = this->_fs->handleFile();
		// Дозапись обязана ответить успехом, ею объект файла и заводится
		ASSERT_TRUE(this->_fs->append(file, "Z", 1, handle));
		// Прочитанное значение
		std::string result = "";
		// Чтение тем же объектом права не имеет
		this->_fs->read(file, result, awh::fs_t::seek_t::BEGIN, 0, handle);
		// Отказавшее чтение обязано оставить ПУСТО, а не размеченный нулями буфер
		ASSERT_TRUE(result.empty());
	}
	{
		// Заводим объект файла средствами самого модуля
		const auto handle = this->_fs->handleFile();
		// Прочитанное значение
		std::string result = "";
		// Чтение обязано пройти, им объект файла и заводится
		this->_fs->read(file, result, awh::fs_t::seek_t::BEGIN, 0, handle);
		// Прочитанное обязано совпасть с содержимым файла
		ASSERT_EQ(result, "XYcdefghZ");
		// Запись тем же объектом права не имеет и обязана ответить отказом
		ASSERT_FALSE(this->_fs->write(file, "Q", 1, awh::fs_t::seek_t::BEGIN, 0, handle));
	}
	// Отказавшая запись содержимого файла тронуть не смела
	ASSERT_EQ(this->_fs->read <std::string> (file), "XYcdefghZ");
	// Удаляем проверяемый файл
	ASSERT_TRUE(this->_fs->unlink(file));
}

/**
 * @brief Проверка пакетного чтения одним внешним объектом файла
 *
 * @details Внешний объект файла годен не одной записи: чтение долями по смещению и
 *          построчное чтение обслуживаются тем же одним описателем. Проверка сличает
 *          прочитанное объектом с прочитанным без него
 *
 */
TEST_F(FSFixture, BatchReadingByExternalFileHandleTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Проверяемый файл
	const std::string file = "test_batch_read_unit.txt";
	// Содержимое проверяемого файла
	const std::string content = "Line1\nLine2\nLine3";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(file) != awh::fs_t::type_t::NONE)
		// Удаляем проверяемый файл
		ASSERT_TRUE(this->_fs->unlink(file));
	// Заводим проверяемый файл
	ASSERT_TRUE(this->_fs->write(file, content.c_str(), content.size()));
	{
		// Заводим объект файла средствами самого модуля
		const auto handle = this->_fs->handleFile();
		// Собранные доли содержимого
		std::string collected = "";
		/**
		 * Выполняем чтение долями по два октета одним описателем
		 */
		for(size_t offset = 0; offset < content.size(); offset += 2){
			// Выполняем чтение очередной доли
			this->_fs->read(file, 2, [&collected](const void * buffer, const size_t size, [[maybe_unused]] const size_t offset, [[maybe_unused]] const size_t left) noexcept -> bool {
				// Собираем прочитанную долю
				collected.append(static_cast <const char *> (buffer), size);
				// Велим остановиться на первой же доле
				return false;
			}, offset, handle);
		}
		// Собранное обязано совпасть с содержимым файла
		ASSERT_EQ(collected, content);
	}
	{
		// Заводим объект файла средствами самого модуля
		const auto handle = this->_fs->handleFile();
		// Прочитанные строки
		std::vector <std::string> lines;
		// Выполняем построчное чтение внешним объектом файла
		this->_fs->readfile(file, [&lines](std::string_view line) noexcept -> void {
			// Запоминаем очередную строку
			lines.emplace_back(line);
		}, awh::fs_t::seek_t::BEGIN, 0, handle);
		// Строк обязано выйти ровно три
		ASSERT_EQ(lines.size(), 3u);
		// Первая строка обязана совпасть
		ASSERT_EQ(lines[0], "Line1");
		// Последняя строка обязана совпасть
		ASSERT_EQ(lines[2], "Line3");
	}
	// Удаляем проверяемый файл
	ASSERT_TRUE(this->_fs->unlink(file));
}

/**
 * @brief Проверка обхода построчного и поблочного с остановкой и продолжением
 *
 * @details Два прочих вида `walkdir` отдают не одни имена, а ещё и содержимое файлов -
 *          строками либо блоками. Остановка у них выполняется на границе строки либо
 *          блока, и продолжение начинает прерванный файл С НАЧАЛА: места остановки
 *          внутри файла объект каталога не хранит
 *
 * @note Оттого проверка сличает не порядок долей, а СОСТАВ собранного: доли вправе
 *       повториться, но ни одна строка потеряться не смеет
 *
 */
TEST_F(FSFixture, WalkdirOverContentStopsAndResumesTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string root = "test_walk_content_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(root) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(root));
	// Заводим дерево каталогов
	ASSERT_TRUE(this->_fs->mkdir(root + "/nested"));
	// Заводим файлы по всей глубине дерева
	ASSERT_TRUE(this->_fs->write(root + "/one.txt", "a1\na2", 5));
	// Заводим файл вложенного каталога
	ASSERT_TRUE(this->_fs->write(root + "/nested/two.txt", "b1\nb2", 5));
	// Строки, снятые одним обходом
	std::set <std::string> whole;
	// Выполняем обход дерева целиком
	ASSERT_TRUE(this->_fs->walkdir(root, "txt", true, [&whole]([[maybe_unused]] const awh::fs_t::type_t type, [[maybe_unused]] std::string_view filename, std::string_view text) noexcept -> bool {
		// Запоминаем очередную строку
		whole.emplace(text);
		// Велим продолжать обход
		return true;
	}));
	// Строк обязано выйти четыре
	ASSERT_EQ(whole.size(), 4u);
	// Заводим объект каталога средствами самого модуля
	const auto handle = this->_fs->handleDir();
	// Строки, собранные долями
	std::set <std::string> parts;
	// Число долей, за которое обход довершён
	uint16_t rounds = 0;
	// Признак того, что обход довершён до конца
	bool done = false;
	/**
	 * Выполняем обход долями по одной строке
	 */
	while(!done && (rounds < 100)){
		// Выполняем очередную долю обхода
		done = this->_fs->walkdir(root, "txt", true, [&parts]([[maybe_unused]] const awh::fs_t::type_t type, [[maybe_unused]] std::string_view filename, std::string_view text) noexcept -> bool {
			// Запоминаем очередную строку
			parts.emplace(text);
			// Останавливаем обход на первой же строке
			return false;
		}, true, handle);
		// Считаем доли обхода
		rounds++;
	}
	// Обход обязан быть довершён до конца
	ASSERT_TRUE(done);
	// Долей обязано выйти больше одной, иначе обход не дробился вовсе
	ASSERT_GT(rounds, 1u);
	// Состав, собранный долями, обязан совпасть с составом одного обхода
	ASSERT_EQ(parts, whole);
	// Блоки, снятые одним обходом
	std::set <std::string> blocks;
	// Выполняем обход дерева блоками
	ASSERT_TRUE(this->_fs->walkdir(root, "txt", 4096, true, [&blocks]([[maybe_unused]] const awh::fs_t::type_t type, [[maybe_unused]] std::string_view filename, const void * buffer, const size_t size) noexcept -> bool {
		// Запоминаем очередной блок
		blocks.emplace(static_cast <const char *> (buffer), size);
		// Велим продолжать обход
		return true;
	}));
	// Блоков обязано выйти два, по одному на файл
	ASSERT_EQ(blocks.size(), 2u);
	// Удаляем корневой каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(root));
}

/**
 * @brief Проверка смены каталога, которому служит внешний объект
 *
 * @details Объект служит каталогу, названному в `path`. Поданный с иным адресом, он
 *          обслуживать прежний перестаёт: прежний описатель закрывается, а состояние
 *          прерванного обхода теряется
 *
 * @note Проверка эта пиннит опасность, а не удобство: незавершённый обход, у какого
 *       сменили адрес, продолжить уже нельзя, и молчаливой его утраты быть не должно
 *
 */
TEST_F(FSFixture, TheDirHandleServesOneDirectoryAtATimeTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Корневой каталог теста
	const std::string root = "test_dir_switch_unit";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(root) != awh::fs_t::type_t::NONE)
		// Удаляем каталог рекурсивно
		ASSERT_TRUE(this->_fs->unlink(root));
	// Заводим два соседних каталога
	ASSERT_TRUE(this->_fs->mkdir(root + "/first"));
	// Заводим второй каталог
	ASSERT_TRUE(this->_fs->mkdir(root + "/second"));
	// Заводим файлы первого каталога
	ASSERT_TRUE(this->_fs->write(root + "/first/a.txt", "a", 1));
	// Заводим ещё один файл первого каталога
	ASSERT_TRUE(this->_fs->write(root + "/first/b.txt", "b", 1));
	// Заводим файл второго каталога
	ASSERT_TRUE(this->_fs->write(root + "/second/c.txt", "c", 1));
	// Заводим объект каталога средствами самого модуля
	const auto handle = this->_fs->handleDir();
	// Число записей, выданных первым обходом
	uint8_t count = 0;
	// Начинаем обход первого каталога и обрываем его на первой записи
	ASSERT_FALSE(this->_fs->walkdir(root + "/first", "", false, [&count]([[maybe_unused]] const awh::fs_t::type_t type, [[maybe_unused]] std::string_view address) noexcept -> bool {
		// Останавливаем обход на первой же записи
		return (++count < 1);
	}, true, handle));
	// Записей обязано выйти ровно одна
	ASSERT_EQ(count, 1u);
	// Состав второго каталога
	std::vector <std::string> second;
	// Обходим ТЕМ ЖЕ объектом иной каталог
	ASSERT_TRUE(this->_fs->walkdir(root + "/second", "", false, [&second]([[maybe_unused]] const awh::fs_t::type_t type, std::string_view address) noexcept -> bool {
		// Запоминаем очередную запись
		second.emplace_back(address);
		// Велим продолжать обход
		return true;
	}, true, handle));
	// Второй каталог обязан быть отдан целиком и с начала
	ASSERT_EQ(second.size(), 1u);
	// Отданная запись обязана принадлежать второму каталогу
	ASSERT_NE(second[0].find("c.txt"), std::string::npos);
	// Состав первого каталога, снятый тем же объектом следом
	std::vector <std::string> first;
	// Обход первого каталога обязан пойти С НАЧАЛА, прерванное состояние утеряно
	ASSERT_TRUE(this->_fs->walkdir(root + "/first", "", false, [&first]([[maybe_unused]] const awh::fs_t::type_t type, std::string_view address) noexcept -> bool {
		// Запоминаем очередную запись
		first.emplace_back(address);
		// Велим продолжать обход
		return true;
	}, true, handle));
	// Первый каталог обязан быть отдан целиком, а не остатком прерванного обхода
	ASSERT_EQ(first.size(), 2u);
	// Удаляем корневой каталог рекурсивно
	ASSERT_TRUE(this->_fs->unlink(root));
}

/**
 * @brief Проверка усечения файла до заданной длины
 *
 * @details Работа эта закрывает то, чего записью не сделать: `write` нулевой длины
 *          отвечает отказом, а усечение существующего файла записью не выражается
 *          вовсе. Усечение до нуля служит и способом завести ПУСТОЙ файл
 *
 * @note Файл короче заданной длины наращивается нулями - так велит работа самих
 *       систем, и своего здесь не придумано
 *
 */
TEST_F(FSFixture, TruncateCutsAndCreatesEmptyTest){
	// Если объект работы с ФС создан
	ASSERT_TRUE(this->_fs != nullptr);
	// Проверяемый файл
	const std::string file = "test_truncate_unit.bin";
	// Удаляем остатки предыдущего запуска
	if(this->_fs->type(file) != awh::fs_t::type_t::NONE)
		// Удаляем проверяемый файл
		ASSERT_TRUE(this->_fs->unlink(file));
	// Усечение отсутствующего файла обязано завести его пустым
	ASSERT_TRUE(this->_fs->truncate(file));
	// Файл обязан появиться
	ASSERT_EQ(this->_fs->type(file), awh::fs_t::type_t::FILE);
	// Длина заведённого файла обязана быть нулевой
	ASSERT_EQ(this->_fs->size(file), 0u);
	// Заполняем проверяемый файл
	ASSERT_TRUE(this->_fs->write(file, "abcdefgh", 8));
	// Усечение до длины меньшей обязано пройти
	ASSERT_TRUE(this->_fs->truncate(file, 3));
	// От содержимого обязано остаться ровно начало
	ASSERT_EQ(this->_fs->read <std::string> (file), "abc");
	// Наращивание до длины большей обязано пройти
	ASSERT_TRUE(this->_fs->truncate(file, 6));
	// Длина файла обязана вырасти
	ASSERT_EQ(this->_fs->size(file), 6u);
	// Прежнее начало обязано уцелеть, а хвост обязан быть нулевым
	ASSERT_EQ(this->_fs->read <std::string> (file), std::string("abc\0\0\0", 6));
	// Очистка существующего файла обязана пройти
	ASSERT_TRUE(this->_fs->truncate(file, 0));
	// Файл обязан остаться на месте пустым
	ASSERT_EQ(this->_fs->type(file), awh::fs_t::type_t::FILE);
	// Длина очищенного файла обязана быть нулевой
	ASSERT_EQ(this->_fs->size(file), 0u);
	// Усечение по негодному пути обязано ответить отказом
	ASSERT_FALSE(this->_fs->truncate("test_truncate_unit_no/such/path/at/all.bin"));
	// Усечение по пустому адресу обязано ответить отказом
	ASSERT_FALSE(this->_fs->truncate(""));
	// Удаляем проверяемый файл
	ASSERT_TRUE(this->_fs->unlink(file));
}
