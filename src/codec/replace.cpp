/**
 * @file replace.cpp
 * @date 2026-09-07
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
 * @brief Переносимая подмена целевого файла временным, общая всем кодекам
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>

/**
 * Если операционная система является MS Windows
 *
 * @warning Заголовок этот приносит с собою макросы, чьи имена совпадают с именами
 *          перечислений кодеков - `ERROR`, `DELETE`, `TEXT` и прочие, - а препроцессор
 *          областей видимости не разбирает. Замер 07.09.2026: одно лишь включение
 *          `windows.h` в ЗАГОЛОВКЕ развалило сборку кодека JSON под MinGW, обратив
 *          `duplicate_t::ERROR` в число, тогда как разметка при этом собралась и прошла.
 *          Здесь включение заперто одной единицей трансляции и до кодеков не доходит, а
 *          ограда `suppress`/`restore` стоит сторожем на случай, если в этот файл
 *          добавят своё перечисление
 *
 * @note Работает ограда возвратом состояния, бывшего ДО включения, - оттого снимаются и
 *       те определения, что внёс сам `windows.h`
 */
#if defined(_WIN32) || defined(_WIN64)
	#include "../../include/sys/macro/suppress.hpp"
	#include <windows.h>
	#include "../../include/sys/macro/restore.hpp"
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/replace.hpp>

/**
 * @brief Метод подмены целевого файла временным
 *
 * @param temporary адрес временного файла записи
 * @param filename  адрес целевого файла записи
 * @return          признак успешной подмены
 *
 */
bool awh::codec::replace(const std::string & temporary, const std::string & filename) noexcept {
	/**
	 * Для операционной системы, MS Windows не являющейся
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем подмену целевого файла временным
		return (::rename(temporary.c_str(), filename.c_str()) == 0);
	/**
	 * Для операционной системы MS Windows
	 */
	#else
		/**
		 * Выполняем подмену целевого файла временным с заменою на месте
		 *
		 * @note Зовётся узкий вид, а не широкий: пути ходят здесь `std::string`
		 */
		return (::MoveFileExA(temporary.c_str(), filename.c_str(),
		                      (MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) != 0);
	#endif
}
