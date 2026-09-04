/**
 * @file common.hpp
 * @date 2026-09-02
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
 * @brief Заголовочный файл общих определений модуля параметров запуска — коды ошибок разбора,
 *        источники значений, виды лексем, настройки разбора и положение лексемы в поданном наборе
 *
 * \~english
 * @brief Header file of the common definitions of the module of the parameters of the launch — the error codes of the parsing,
 *        the sources of the values, the kinds of the lexemes, the settings of the parsing and the position of a lexeme in the submitted set
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_ARGS_COMMON__
#define __AWH_ARGS_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/log.hpp"
#include "../sys/global.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, TEXT у MS Windows. Имена снимаются лишь
 * на время объявлений - возврат в конце файла
 */
#include "../sys/macro/suppress.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён параметров запуска приложения
	 *
	 * @details Разбор параметров запуска, переменных окружения и текстовых потоков с
	 * укладкою разобранного в единое дерево значений. Осью хранения служит контейнер
	 * ABC: система видов его вмещает виды всех текстовых кодеков разом, оттого выдача
	 * настроек любым из них не требует ни второго дерева, ни вторых правил приведения
	 *
	 * @par Намеренные решения
	 *
	 * Перечисленное ниже не является пробелом реализации: это очерченные границы
	 * задачи, и каждое из решений закреплено проверочным испытанием
	 *
	 * @li **Записи параметров равноправны.** Формы «--name=VALUE», «--name VALUE»,
	 * «-name=VALUE» и «-name VALUE» разбираются одинаково, и положение параметра в
	 * наборе значения не имеет: набор обходится целиком, а порядок сохраняется лишь
	 * у позиционных доводов
	 *
	 * @li **Склейка коротких имён по умолчанию выключена.** Запись «-abc» при
	 * поддержанной форме «-name VALUE» неотличима от длинного имени под одним тире,
	 * и угадывание здесь молча меняло бы смысл набора. Разбор её тремя признаками
	 * включается настройкой и требует описания ожидаемых параметров
	 *
	 * @li **Разбор текста и разбор набора ведёт один и тот же код.** Поданный текст
	 * режется на слова с учётом кавычек и обратной косой, после чего подаётся тому же
	 * разборщику, что и набор запуска. Совпадение поведения обоих входов держится
	 * устройством, а не сличением двух реализаций
	 *
	 * \~english
	 * @brief Namespace of the parameters of the launch of an application
	 * @details The parsing of the parameters of the launch, of the variables of the environment and of the text streams with
	 * the laying of the parsed into a single tree of the values. The container ABC serves as the axis of the storage:
	 * its system of the kinds encompasses the kinds of all the text codecs at once, whereby the issuance
	 * of the settings by any of them requires neither a second tree nor second rules of the conversion
	 *
	 * \~
	 */
	namespace args {
		/**
		 * \~russian
		 * @brief Коды ошибок разбора параметров
		 *
		 * @details Разбор не выбрасывает исключений: признаком отказа служит код ошибки
		 * вместе с положением в поданном наборе, где отказ произошёл
		 *
		 * \~english
		 * @brief Error codes of the parsing of the parameters
		 * @details The parsing does not throw exceptions: the error code together with the position
		 * in the submitted set where the refusal has occurred serves as the sign of a refusal
		 *
		 * \~
		 */
		enum class error_t : uint8_t {
			NONE        = 0x00, // Отказа не произошло
			EMPTY_KEY   = 0x01, // Имя параметра пусто: запись «--=VALUE» либо «--»
			EMPTY_PATH  = 0x02, // Путь укладки пуст либо содержит пустое звено
			DEEP_PATH   = 0x03, // Глубина пути укладки превысила предел разбора
			LONG_KEY    = 0x04, // Длина имени параметра превысила предел разбора
			LONG_VALUE  = 0x05, // Длина значения параметра превысила предел разбора
			MANY_TOKENS = 0x06, // Число лексем превысило предел разбора
			UNPAIRED    = 0x07, // Кавычка в поданном тексте не закрыта
			DANGLING    = 0x08, // Обратная косая стоит последним знаком текста
			NO_VALUE    = 0x09, // Параметру потребно значение, а его нет вовсе
			ODD_VALUE   = 0x0A, // Параметру значение не потребно, а оно подано
			UNKNOWN     = 0x0B, // Имя параметра описанию ожидаемых неизвестно
			DUPLICATE   = 0x0C, // Параметр подан повторно, а кратность его одиночна
			CODEC       = 0x0D, // Разбор настроек кодеком окончился отказом
			UNSUPPORTED = 0x0E, // Кодек выдачи вместить поданное дерево не может
			FILESYSTEM  = 0x0F, // Файла настроек нет вовсе либо чтение его отказало
			REQUIRED    = 0x10, // Обязательный параметр описания не подан вовсе
			CLUSTER     = 0x11  // Склейка коротких имён содержит имя, описанию неизвестное
		};

		/**
		 * \~russian
		 * @brief Источники значений дерева настроек
		 *
		 * @details Источник хранится у каждого уложенного значения и служит двум делам:
		 * разрешению старшинства при слиянии и отладке настроек. Без него ответ на
		 * вопрос «откуда взялось это значение» неотличим от догадки
		 *
		 * @note Порядок членов задаёт старшинство: младший источник уступает старшему
		 *
		 * \~english
		 * @brief Sources of the values of the tree of the settings
		 * @details The source is stored at every laid value and serves two matters:
		 * the resolution of the seniority at the merging and the debugging of the settings. Without it the answer to
		 * the question «whence has this value come» is indistinguishable from a guess
		 * @note The order of the members sets the seniority: a junior source yields to a senior one
		 *
		 * \~
		 */
		enum class source_t : uint8_t {
			NONE    = 0x00, // Источник не определён
			DEFAULT = 0x01, // Значение по умолчанию из описания ожидаемых параметров
			FILE    = 0x02, // Файл настроек, разобранный одним из кодеков
			ENV     = 0x03, // Переменная окружения с оговорённым началом имени
			TEXT    = 0x04, // Текстовый поток, поданный набором либо с клавиатуры
			CLI     = 0x05  // Набор параметров запуска приложения
		};

		/**
		 * \~russian
		 * @brief Виды лексем разбора
		 *
		 * @details Лексема - единица выдачи разборщика. Разбор значения от разбора
		 * имени отделён намеренно: имя вместе со значением приходит одной лексемой
		 * вида PARAM, а довод без имени - лексемой вида OPERAND
		 *
		 * \~english
		 * @brief Kinds of the lexemes of the parsing
		 * @details A lexeme is a unit of the issuance of the parser. The parsing of a value is separated from the parsing
		 * of a name deliberately: a name together with a value comes by one lexeme
		 * of the kind PARAM, and an argument without a name by a lexeme of the kind OPERAND
		 *
		 * \~
		 */
		enum class token_t : uint8_t {
			NONE     = 0x00, // Лексема не определена
			PARAM    = 0x01, // Именованный параметр, со значением либо без него
			OPERAND  = 0x02, // Позиционный довод, имени не имеющий
			TERMINUS = 0x03  // Признак конца именованных параметров: запись «--»
		};

		/**
		 * \~russian
		 * @brief Положение лексемы в поданном наборе
		 *
		 * @details Служит указанию места отказа. У набора запуска строк и столбцов нет,
		 * оттого положение задаётся порядковым номером довода вместе со смещением
		 * внутри него; у текстового потока номер довода считается по словам разреза
		 *
		 * \~english
		 * @brief Position of a lexeme in the submitted set
		 * @details Serves the indication of the place of a refusal. The set of the launch has no lines and columns,
		 * whereby the position is set by the ordinal number of an argument together with the offset
		 * inside it; at a text stream the number of an argument is counted by the words of the cut
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Location {
			// Порядковый номер довода в поданном наборе, считая с нуля
			size_t index;
			// Смещение от начала довода в октетах
			size_t offset;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			Location() noexcept : index(0), offset(0) {}
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param index  порядковый номер довода в поданном наборе
			 * @param offset смещение от начала довода в октетах
			 *
			 * \~english
			 * @brief Constructor
			 * @param index ordinal number of an argument in the submitted set
			 * @param offset offset from the beginning of an argument in octets
			 *
			 * \~
			 */
			Location(const size_t index, const size_t offset) noexcept : index(index), offset(offset) {}
		} location_t;

		/**
		 * \~russian
		 * @brief Лексема разбора параметров
		 *
		 * @details Содержимое лексемы взято ссылками на поданный набор и живёт не дольше
		 * его: копий разбор не делает вовсе. Укладка в дерево копирует содержимое сама
		 *
		 * @note Признак наличия значения от пустоты значения отличен: запись «--name=»
		 * подаёт значение пустым, а запись «--name» не подаёт его вовсе
		 *
		 * \~english
		 * @brief Lexeme of the parsing of the parameters
		 * @details The content of a lexeme is taken by references to the submitted set and lives no longer than
		 * it: the parsing makes no copies at all. The laying into the tree copies the content itself
		 * @note The sign of the presence of a value differs from the emptiness of a value: the record «--name=»
		 * submits an empty value, and the record «--name» does not submit it at all
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Lexeme {
			// Вид лексемы разбора
			token_t type;
			// Признак наличия значения у именованного параметра
			bool assigned;
			// Имя параметра без ведущих тире, пусто у позиционного довода
			string_view key;
			// Значение параметра либо содержимое позиционного довода
			string_view value;
			// Положение лексемы в поданном наборе
			location_t location;
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * \~
			 */
			Lexeme() noexcept : type(token_t::NONE), assigned(false) {}
		} lexeme_t;

		/**
		 * \~russian
		 * @brief Метод извлечения описания кода ошибки разбора
		 *
		 * @details Описание отведено каждому коду по отдельности; коду неизвестному
		 * выдаётся общее описание, а не пустота
		 *
		 * @param error код ошибки разбора
		 * @return      описание кода ошибки разбора
		 *
		 * \~english
		 * @brief Method of the extraction of the description of an error code of the parsing
		 * @param error error code of the parsing
		 * @return description of the error code of the parsing
		 *
		 * \~
		 */
		__AWH_SHARED_EXPORT__ const char * message(const error_t error) noexcept;
	}
}

/**
 * Возвращаем подавленные системные макросы
 */
#include "../sys/macro/restore.hpp"

#endif // __AWH_ARGS_COMMON__
