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
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Функция подмены целевого файла временным
		 *
		 * @details Запись в файл ведётся через временный файл с последующей подменою: так
		 *          отказ посреди записи оставляет прежнее содержимое целым, а не наполовину выписанным.
		 *          Подмена эта у POSIX выполняется `rename()`, но у MS Windows тот же
		 *          вызов существующий файл НЕ ЗАМЕНЯЕТ - он отвечает отказом, и запись во второй раз
		 *          по одному и тому же пути не удаётся вовсе.
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
		 * @brief Function of the replacement of a target file by a temporary one
		 *
		 * @details The writing into a file is performed through a temporary file with a subsequent
		 *          replacement: thus a refusal in the middle of the writing leaves the previous content whole.
		 *          At POSIX this replacement is performed by `rename()`, but at MS Windows the same call
		 *          does NOT replace an existing file - it answers with a refusal.
		 *
		 * @warning This problem was real and was not found by reasoning: a set of codec checks,
		 *          first assembled under MinGW on 09/07/2026, gave four failures in three different codecs,
		 *          and for all four the SECOND work with a file along the same path failed, while the first one passed.
		 *          Proven by a probe at the MinGW stand: `rename()` responds to an existing file with `-1`, to a missing file - zero
		 *
		 * @note The replacement function for MS Windows is the same as that used by the cryptography module:
		 *       `MoveFileEx` with the attributes `MOVEFILE_REPLACE_EXISTING` and `MOVEFILE_WRITE_THROUGH`.
		 *       The first allows replacement on the spot, the second tells you to wait until the recording
		 *       is transferred to the device, otherwise the replacement is considered completed ahead of time.
		 *       The name is the narrow view (`MoveFileExA`), not the wide one: the paths go here `std::string`,
		 *       and the wide one will not accept them
		 *
		 * @note The body lives in `src/codec/replace.cpp`, and not here - according to the general tree rule
		 *       about pure header files. This order is important not only in appearance:
		 *       `windows.h` brings the macros `ERROR`, `DELETE`, `TEXT`, and if included in the header,
		 *       they would diverge for any codec that included it. In the source code,
		 *       they are locked in one translation unit. The owner of the INI, YAML and TOML codecs pointed this out
		 *
		 * @note Demolishing the target file before `rename()` would be a simpler function, but it is WRONG:
		 *       between demolition and renaming, the target file does not exist at all,
		 *       and failure at that moment leaves the user without the previous contents - exactly what the temporary file is created for
		 *
		 * @param temporary address of the temporary file of the writing
		 * @param filename  address of the target file of the writing
		 * @return          sign of the successful replacement
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool replace(const string & temporary, const string & filename) noexcept;
		/**
		 * \~russian
		 * @brief Функция подмены целевого файла временным
		 *
		 * @details Запись в файл ведётся через временный файл с последующей подменою: так
		 *          отказ посреди записи оставляет прежнее содержимое целым, а не наполовину выписанным.
		 *          Подмена эта у POSIX выполняется `rename()`, но у MS Windows тот же
		 *          вызов существующий файл НЕ ЗАМЕНЯЕТ - он отвечает отказом, и запись во второй раз
		 *          по одному и тому же пути не удаётся вовсе.
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
		 *       свершённой прежде времени. Зовётся узкий вид (`MoveFileExW`), а не широкий:
		 *       пути ходят здесь `std::wstring`, и широкий их не примет
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
		 * @brief Function of the replacement of a target file by a temporary one
		 *
		 * @details The writing into a file is performed through a temporary file with a subsequent
		 *          replacement: thus a refusal in the middle of the writing leaves the previous content whole.
		 *          At POSIX this replacement is performed by `rename()`, but at MS Windows the same call
		 *          does NOT replace an existing file - it answers with a refusal.
		 *
		 * @warning This problem was real and was not found by reasoning: a set of codec checks,
		 *          first assembled under MinGW on 09/07/2026, gave four failures in three different codecs,
		 *          and for all four the SECOND work with a file along the same path failed, while the first one passed.
		 *          Proven by a probe at the MinGW stand: `rename()` responds to an existing file with `-1`, to a missing file - zero
		 *
		 * @note The replacement function for MS Windows is the same as that used by the cryptography module:
		 *       `MoveFileEx` with the attributes `MOVEFILE_REPLACE_EXISTING` and `MOVEFILE_WRITE_THROUGH`.
		 *       The first allows replacement on the spot, the second tells you to wait until the recording
		 *       is transferred to the device, otherwise the replacement is considered completed ahead of time.
		 *       The name is the narrow view (`MoveFileExW`), not the wide one: the paths go here `std::wstring`,
		 *       and the wide one will not accept them
		 *
		 * @note The body lives in `src/codec/replace.cpp`, and not here - according to the general tree rule
		 *       about pure header files. This order is important not only in appearance:
		 *       `windows.h` brings the macros `ERROR`, `DELETE`, `TEXT`, and if included in the header,
		 *       they would diverge for any codec that included it. In the source code,
		 *       they are locked in one translation unit. The owner of the INI, YAML and TOML codecs pointed this out
		 *
		 * @note Demolishing the target file before `rename()` would be a simpler function, but it is WRONG:
		 *       between demolition and renaming, the target file does not exist at all,
		 *       and failure at that moment leaves the user without the previous contents - exactly what the temporary file is created for
		 *
		 * @param temporary address of the temporary file of the writing
		 * @param filename  address of the target file of the writing
		 * @return          sign of the successful replacement
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ bool replace(const wstring & temporary, const wstring & filename) noexcept;
	}
}

#endif // __AWH_CODEC_REPLACE__
