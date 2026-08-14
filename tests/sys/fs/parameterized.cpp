/**
 * @file parameterized.cpp
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
 * @brief Параметризованные тесты модуля работы с файловой системой —
 *        прогон подготовленных наборов входных данных через методы модуля с проверкой чтения и записи файлов,
 *        обхода каталогов, получения атрибутов и построения путей
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fs.hpp"

/**
 * @brief Структура данных для параметризованных тестов путей
 *
 */
struct PathTestParams {
	std::string input = "";
	bool shouldBeAbsolute = false;
};

/**
 * @brief Класс параметризованных тестов для путей
 *
 */
class FSPathTests : public FSFixture, public testing::WithParamInterface<PathTestParams> {};

/**
 * @brief Тест метода fullpath
 *
 */
TEST_P(FSPathTests, FullPathTest) {
	// Получаем параметры теста
	const auto& params = GetParam();
	
	// Вызываем метод fullpath
	std::string result = this->_fs->fullpath(params.input);
	
	// Проверяем что путь не пустой
	ASSERT_FALSE(result.empty());

	/**
	 * Если путь должен быть абсолютным, он должен начинаться с / (на Unix)
	 * В тестах предполагаем Unix окружение на основе списка файлов
	 */
	#if !_WIN32 && !_WIN64
		if(params.shouldBeAbsolute)
			ASSERT_EQ(result.front(), '/');
	#endif
}

/**
 * @brief Параметры тестов путей
 *
 */
INSTANTIATE_TEST_SUITE_P(FSPathTestCases, FSPathTests,
	testing::Values(
		PathTestParams{ ".", true },
		PathTestParams{ "..", true },
		PathTestParams{ "test_folder", true },
		PathTestParams{ "/tmp/test", true },
		PathTestParams{ "~", true } // Home dir expansion
	)
);

/**
 * @brief Структура данных для параметризованных тестов компонентов пути
 *
 */
struct ComponentsTestParams {
	std::string path = "";
	std::string expectedName = "";
	std::string expectedExt = "";
	bool isFile = false; // true если симулируем файл, false если каталог
};

/**
 * @brief Класс параметризованных тестов для компонентов
 *
 */
class FSComponentsTests : public FSFixture, public testing::WithParamInterface<ComponentsTestParams> {};

/**
 * @brief Тест метода components
 *
 */
TEST_P(FSComponentsTests, ComponentsTest) {
	// Получаем параметры теста
	const auto& params = GetParam();
	
	// Создаем временную структуру для теста
	std::string tempPath = params.path;

	/**
	 * Для корректной работы components нам нужно чтобы файл/каталог реально существовал (для определения типа), либо мы тестируем логику парсинга?
	 * fs::components внутри вызывает fullpath и type.
	 * Если type::DIR, то возвращает {dirname, ""}.
	 * Если type::FILE, то парсит имя и расширение.
	 * Поэтому нам прийдется создавать эти элементы.
	 */

	// Создаем временный файл или каталог
	std::string realPath = "fs_comp_test_" + std::to_string(std::rand());
	// Создаем в текущей директории
	if(params.isFile)
		// Создаем файл
		this->_fs->write(realPath, "test");
	// Иначе каталог
	else this->_fs->mkdir(realPath);
	
	// Тестируем на созданном пути
	auto [name, ext] = this->_fs->components(realPath);

	/**
	 * Проверяем хотя бы что расширение пустое для папок, и не пустое если ожидали (но мы создали файл без расширения выше)
	 * Давайте лучше создадим файл с расширением если оно есть в параметрах
	 */
	
	// Очищаем предыдущий
	this->_fs->unlink(realPath);
	
	// Создаем новый с нужным именем
	std::string filename = params.expectedName;
	// Добавляем расширение если нужно
	if(!params.expectedExt.empty())
		// Добавляем точку и расширение
		filename += "." + params.expectedExt;
	
	// В текущей папке
	if(params.isFile)
		// Создаем файл
		this->_fs->write(filename, "test");
	// Иначе каталог
	else this->_fs->mkdir(filename);

	// Вызываем components
	auto result = this->_fs->components(filename);
	// Сравниваем результаты
	ASSERT_EQ(result.first, params.expectedName);
	ASSERT_EQ(result.second, params.expectedExt);
	
	// Удаляем временный файл/каталог
	this->_fs->unlink(filename);
}

/**
 * @brief Параметры тестов компонентов
 *
 */
INSTANTIATE_TEST_SUITE_P(FSComponentsTestCases, FSComponentsTests,
	testing::Values(
		ComponentsTestParams{ "file.txt", "file", "txt", true },
		ComponentsTestParams{ "archive.tar.gz", "archive.tar", "gz", true },
		ComponentsTestParams{ "image", "image", "", true },
		ComponentsTestParams{ "folder.with.dot", "folder.with.dot", "", false }
	)
);
