/**
 * @file replace.hpp
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_REPLACE__
#define __AWH_CODEC_REPLACE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/global.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён кодеков
	 *
	 * \~english
	 * @brief Namespace of the codecs
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Метод подмены целевого файла временным
		 *
		 * @details Запись в файл ведётся через временный файл с последующей подменою: так
		 * отказ посреди записи оставляет прежнее содержимое целым, а не наполовину
		 * выписанным. Подмена эта у POSIX выполняется `rename()`, но у MS Windows тот же
		 * вызов существующий файл НЕ ЗАМЕНЯЕТ - он отвечает отказом, и запись во второй раз
		 * по одному и тому же пути не удаётся вовсе
		 *
		 * @warning Беда эта была настоящей и найдена не рассуждением: набор проверок
		 *          кодеков, впервые собравшись под MinGW 07.09.2026, дал четыре отказа в
		 *          трёх разных кодеках, и у всех четырёх падала ВТОРАЯ работа с файлом по
		 *          тому же пути, а первая проходила. Доказано щупом на стенде MinGW:
		 *          `rename()` на существующий файл отвечает `-1`, на отсутствующий - нулём
		 *
		 * @note Способ замены у MS Windows взят тот же, каким пользуется модуль
		 *       криптографии: `MoveFileEx` с признаками `MOVEFILE_REPLACE_EXISTING` и
		 *       `MOVEFILE_WRITE_THROUGH`. Первый дозволяет замену на месте, второй велит
		 *       дождаться, пока запись ляжет на устройство, - иначе подмена считается
		 *       свершённой прежде времени. Зовётся узкий вид (`MoveFileExA`), а не широкий:
		 *       пути ходят здесь `std::string`, и широкий их не примет
		 *
		 * @note Тело живёт в `src/codec/replace.cpp`, а не здесь, - по общему правилу
		 *       дерева о чистых заголовочных файлах. Порядок этот важен не только видом:
		 *       `windows.h` приносит макросы `ERROR`, `DELETE`, `TEXT`, и, стой включение
		 *       в заголовке, они расходились бы по всякому кодеку, его включившему. В
		 *       исходнике же они заперты одной единицей трансляции. Указал на это владелец
		 *       кодеков INI, YAML и TOML
		 *
		 * @note Снос целевого файла перед `rename()` был бы способом более простым, но
		 *       НЕВЕРНЫМ: между сносом и переименованием целевого файла не существует
		 *       вовсе, и отказ в этот миг оставляет потребителя без прежнего содержимого -
		 *       ровно то, ради чего временный файл и заводится
		 *
		 * @param temporary адрес временного файла записи
		 * @param filename  адрес целевого файла записи
		 * @return          признак успешной подмены
		 *
		 * \~english
		 * @brief Method of the replacement of a target file by a temporary one
		 * @details The writing into a file is performed through a temporary file with a subsequent
		 * replacement: thus a refusal in the middle of the writing leaves the previous content whole.
		 * At POSIX this replacement is performed by `rename()`, but at MS Windows the same call
		 * does NOT replace an existing file - it answers with a refusal
		 * @param temporary address of the temporary file of the writing
		 * @param filename  address of the target file of the writing
		 * @return          sign of the successful replacement
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool replace(const std::string & temporary, const std::string & filename) noexcept;
	}
}

#endif // __AWH_CODEC_REPLACE__
