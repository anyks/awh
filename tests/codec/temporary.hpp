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
#include <sys/stat.h>

/**
 * Если операционная система является MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <direct.h>
#endif

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

/**
 * \~russian
 * @brief Функция заведения каталога, переносимая между системами
 *
 * @details Вызов `::mkdir()` разнится сигнатурой: POSIX берёт путь и права доступа, а
 *          MinGW у MS Windows - один лишь путь, ибо прав доступа в его виде там нет
 *          вовсе. Вызов с двумя доводами валит сборку с «too many arguments to function»,
 *          а проверки кодеков собираются ОДНОЙ программой - и не собирается вместе с
 *          нею ни одна проверка ни одного кодека, включая чужие
 *
 * @warning Беда эта была настоящей: наборы XML, JSON и CSV перестали собираться под
 *          MinGW с 04.09.2026, о чём сообщил владелец кодека CEF, у которого от этого
 *          не шли собственные проверки. Довод общий с ограждением `sys/resource.h` в
 *          самих наборах
 *
 * @note Работа объявлена `inline` и безымянного пространства имён не имеет по тому же
 *       доводу, что и `temporary()` выше: копия у всякого файла завела бы повод к
 *       расхождению
 *
 * @param path путь к заводимому каталогу
 * @return     признак успешного заведения каталога
 *
 * \~english
 * @brief Function of the creation of a directory, portable between the systems
 * @details The `::mkdir()` call differs in its signature: POSIX takes a path and the access
 *          rights, while MinGW at MS Windows takes the path alone
 * @param path path to the directory being created
 * @return     sign of the successful creation of the directory
 *
 * \~
 */
inline bool makeDirectory(const std::string & path) noexcept {
	/**
	 * Для операционной системы, MS Windows не являющейся
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем заведение каталога с правами доступа
		return (::mkdir(path.c_str(), 0755) == 0);
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		/**
		 * Выполняем заведение каталога без прав доступа
		 *
		 * @note Зовётся `_mkdir`, а не `mkdir`: последний объявлен в `direct.h` с пометкой
		 *       `__MINGW_ATTRIB_DEPRECATED_MSVC2005`, а непомеченный близнец его - это и
		 *       есть `_mkdir`, объявленный строкою выше в том же заголовке
		 *
		 * @warning Предупреждения об устаревании на стенде MinGW x86-64 (g++ 16.1)
		 *          НЕ наблюдалось - ни при `-Wall -Wextra`, ни при
		 *          `-Wdeprecated-declarations`, ни при неопределённом
		 *          `_CRT_NONSTDC_NO_DEPRECATE`, - и замер этот записан, чтобы следующий не
		 *          искал его заново. Замена сделана по самому ФАКТУ пометки в заголовке, а
		 *          не по наблюдаемому предупреждению: заголовок этот включают одиннадцать
		 *          наборов проверок, и предупреждение, всплыв у иного собирателя, придёт
		 *          сразу всем. Указал на пометку владелец кодека CEF
		 */
		return (::_mkdir(path.c_str()) == 0);
	#endif
}

#endif // __AWH_TESTS_CODEC_TEMPORARY__
