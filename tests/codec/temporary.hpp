/**
 * @file temporary.hpp
 * @date 2026-09-01
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
 * \~russian
 * @brief Выдача пути во временном каталоге системы для проверок кодеков
 *
 * \~english
 * @brief The giving of a path in the temporary directory of a system for the tests of the codecs
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_TESTS_CODEC_TEMPORARY__
#define __AWH_TESTS_CODEC_TEMPORARY__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdlib>

/**
 * @brief Функция выдачи пути во временном каталоге системы
 *
 * @details Зашитый путь `/tmp/...` переносимым НЕ является: родная программа MS Windows
 *          разрешает его от корня текущего диска - выходит `E:\tmp\`, какого на машине
 *          нет, и открытие файла отвечает отказом. Замер 01.09.2026 на стенде Windows
 *          ARM64 дал девять отказов проверок кодеков ровно по этой причине, а заведение
 *          каталога `E:\tmp` вручную их снимало, ничего в коде не трогая
 *
 * @note Каталог берётся ПЕРЕМЕННЫМИ ОКРУЖЕНИЯ, а не работой системы: `GetTempPath` тянет
 *       за собою весь слой заголовков MS Windows в файл проверки, тогда как переменную
 *       `TEMP` эта система ставит всегда, а `TMPDIR` - обычай POSIX. Отступление же на
 *       `/tmp` держит прежнее поведение там, где ни одна из них не задана
 *
 * @warning Заголовок этот подключается НЕСКОЛЬКИМИ файлами проверок, собираемыми в ОДНУ
 *          программу: работа объявлена `inline` намеренно, и безымянного пространства
 *          имён здесь быть не должно - оно завело бы у всякого файла свою копию, а
 *          вместе с нею и повод к расхождению
 *
 * @param name название временного файла
 * @return     полный путь к временному файлу
 *
 */
inline std::string temporary(const std::string & name) noexcept {
	// Выполняем получение временного каталога, объявленного окружением
	const char * directory = ::getenv("TMPDIR");
	// Если временный каталог окружением не объявлен, берём обычай MS Windows
	if((directory == nullptr) || (directory[0] == '\0'))
		// Выполняем получение временного каталога MS Windows
		directory = ::getenv("TEMP");
	// Если временный каталог так и не объявлен, берём обычай POSIX
	if((directory == nullptr) || (directory[0] == '\0'))
		// Выполняем выдачу пути во временном каталоге по обычаю POSIX
		return (std::string{"/tmp/"} + name);
	// Собираемый путь к временному файлу
	std::string result(directory);
	/**
	 * Если разделитель на конце каталога отсутствует, дописываем его
	 *
	 * @note Разделителем берётся косая черта: MS Windows принимает её наравне с
	 *       обратной во всякой работе с файлами, и различать их незачем
	 */
	if(!result.empty() && (result.back() != '/') && (result.back() != '\\'))
		// Выполняем дописывание разделителя каталога
		result.append(1, '/');
	// Выводим собранный путь к временному файлу
	return result.append(name);
}

#endif // __AWH_TESTS_CODEC_TEMPORARY__
