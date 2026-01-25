/**
 * @file: static.cpp
 * @date: 2026-01-25
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
