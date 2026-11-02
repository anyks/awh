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
 *        прогон подготовленных наборов входных данных через функции модуля с проверкой чтения и записи файлов,
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
	// Входной путь
	std::string input = "";
	// Признак того, что путь обязан быть абсолютным
	bool shouldBeAbsolute = false;
};

/**
 * @brief Класс параметризованных тестов для путей
 *
 */
class FSPathTests : public FSFixture, public testing::WithParamInterface<PathTestParams> {};

/**
 * @brief Тест функции fullpath
 *
 */
TEST_P(FSPathTests, FullPathTest) {
	// Получаем параметры теста
	const auto& params = GetParam();
	// Вызываем fullpath
	std::string result = awh::fs::fullpath(params.input);
	// Проверяем что путь не пустой
	ASSERT_FALSE(result.empty());
	/**
	 * Для операционных систем семейства Unix
	 *
	 * @note В тестах предполагается Unix-окружение на основе списка файлов
	 *
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Если путь должен быть абсолютным
		if(params.shouldBeAbsolute)
			// Путь обязан начинаться с разделителя
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
	// Путь для разбора
	std::string path = "";
	// Ожидаемое имя
	std::string expectedName = "";
	// Ожидаемое расширение
	std::string expectedExt = "";
	// Признак файла: true - файл, false - каталог
	bool isFile = false;
};

/**
 * @brief Класс параметризованных тестов для компонентов
 *
 */
class FSComponentsTests : public FSFixture, public testing::WithParamInterface<ComponentsTestParams> {};

/**
 * @brief Тест функции components
 *
 */
TEST_P(FSComponentsTests, ComponentsTest) {
	// Получаем параметры теста
	const auto& params = GetParam();
	// Создаём временную структуру для теста
	std::string tempPath = params.path;
	/**
	 * Для корректной работы components файл или каталог обязан существовать:
	 * fs::components внутри вызывает fullpath и type
	 */
	// Создаём временный файл или каталог
	std::string realPath = "fs_comp_test_" + std::to_string(std::rand());
	// Если проверяется файл
	if(params.isFile)
		// Создаём файл
		awh::fs::write(realPath, "test");
	// Иначе создаём каталог
	else awh::fs::mkdir(realPath);
	// Получаем компоненты созданного пути
	auto [name, ext] = awh::fs::components(realPath);
	// Очищаем предыдущий путь
	awh::fs::unlink(realPath);
	// Создаём новый путь с ожидаемым именем
	std::string filename = params.expectedName;
	// Если расширение ожидается
	if(!params.expectedExt.empty())
		// Добавляем точку и расширение
		filename += "." + params.expectedExt;
	// Если проверяется файл
	if(params.isFile)
		// Создаём файл
		awh::fs::write(filename, "test");
	// Иначе создаём каталог
	else awh::fs::mkdir(filename);
	// Получаем компоненты пути с ожидаемым именем
	auto result = awh::fs::components(filename);
	// Имя обязано совпасть с ожидаемым
	ASSERT_EQ(result.first, params.expectedName);
	// Расширение обязано совпасть с ожидаемым
	ASSERT_EQ(result.second, params.expectedExt);
	// Удаляем временный файл или каталог
	awh::fs::unlink(filename);
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
