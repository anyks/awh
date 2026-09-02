/**
 * @file types.hpp
 * @date 2026-08-03
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
 * @brief Заголовочный файл общих определений модуля приведения доменных имён —
 *        состояния символов таблицы преобразований, виды соединения символов,
 *        режимы приведения, коды ошибок и виды записей таблиц
 *
 * \~english
 * @brief Header file of the common definitions of the domain name mapping module —
 *        statuses of the characters of the transformations table, character joining types,
 *        mapping modes, error codes and the kinds of table entries
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_IDNA_TYPES__
#define __AWH_IDNA_TYPES__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/global.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён модуля приведения доменных имён
	 *
	 * \~english
	 * @brief Namespace of the domain name mapping module
	 *
	 * \~
	 */
	namespace idna {
		/**
		 * \~russian
		 * @brief Приставка, которой обозначается запись метки кодировкой Punycode
		 *
		 * \~english
		 * @brief Prefix designating a label record in the Punycode encoding
		 *
		 * \~
		 */
		constexpr const char * PREFIX = "xn--";

		/**
		 * \~russian
		 * @brief Наибольшая длина метки доменного имени в записи ASCII
		 *
		 * \~english
		 * @brief Largest length of a domain name label in the ASCII record
		 *
		 * \~
		 */
		constexpr size_t MAX_LABEL = 63;

		/**
		 * \~russian
		 * @brief Наибольшая длина доменного имени в записи ASCII
		 *
		 * \~english
		 * @brief Largest length of a domain name in the ASCII record
		 *
		 * \~
		 */
		constexpr size_t MAX_DOMAIN = 253;

		/**
		 * \~russian
		 * @brief Состояние символа в таблице преобразований
		 *
		 * @details Состояния заданы таблицей преобразований приложения по обработке
		 *          доменных имён стандарта Юникода.
		 *
		 * \~english
		 * @brief Status of a character in the transformations table
		 *
		 * @details The statuses are defined by the transformations table of the annex on domain
		 *          name processing of the Unicode standard.
		 *
		 * \~
		 */
		enum class status_t : uint8_t {
			VALID                 = 0x00, // Символ допустим и преобразованию не подлежит
			IGNORED               = 0x01, // Символ опускается
			MAPPED                = 0x02, // Символ преобразуется набором символов
			DEVIATION             = 0x03, // Символ преобразуется лишь переходным режимом
			DISALLOWED            = 0x04, // Символ недопустим
			DISALLOWED_STD3_VALID = 0x05, // Символ допустим вне правил записи имён узлов
			DISALLOWED_STD3_MAPPED = 0x06 // Символ преобразуется вне правил записи имён узлов
		};

		/**
		 * \~russian
		 * @brief Вид соединения символа
		 *
		 * @details Виды заданы свойством вида соединения стандарта Юникода
		 *          и применяются правилами сочетания соединителей.
		 *
		 * \~english
		 * @brief Joining type of a character
		 *
		 * @details The types are defined by the joining type property of the Unicode standard
		 *          and are applied by the joiner combining rules.
		 *
		 * \~
		 */
		enum class joining_t : uint8_t {
			NONE        = 0x00, // Символ не соединяется
			TRANSPARENT = 0x01, // Символ соединению не препятствует
			CAUSING     = 0x02, // Символ вызывает соединение
			LEFT        = 0x03, // Символ соединяется слева
			RIGHT       = 0x04, // Символ соединяется справа
			DUAL        = 0x05  // Символ соединяется с обеих сторон
		};

		/**
		 * \~russian
		 * @brief Настройка приведения доменного имени
		 *
		 * @details Значения складываются побитово. Набор режимов задан приложением
		 *          по обработке доменных имён стандарта Юникода.
		 *
		 * \~english
		 * @brief Setting of the domain name mapping
		 *
		 * @details The values are combined bitwise. The set of modes is defined by the annex
		 *          on domain name processing of the Unicode standard.
		 *
		 * \~
		 */
		enum class option_t : uint16_t {
			NONE          = 0x0000, // Режимы не установлены
			TRANSITIONAL  = 0x0001, // Переходный режим преобразования символов
			STD3          = 0x0002, // Применение правил записи имён узлов
			HYPHENS       = 0x0004, // Проверка размещения знаков переноса
			BIDI          = 0x0008, // Проверка правила двунаправленного письма
			JOINERS       = 0x0010, // Проверка правил сочетания соединителей
			LENGTH        = 0x0020  // Проверка длины доменного имени и его меток
		};

		/**
		 * \~russian
		 * @brief Набор настроек приведения, применяемый по умолчанию
		 *
		 * @details Переходный режим преобразования символов приложением по обработке
		 *          доменных имён объявлен устаревшим и по умолчанию не применяется.
		 *          Правила записи имён узлов по умолчанию не применяются: приложение
		 *          оставляет их применение на усмотрение потребителя.
		 *
		 * \~english
		 * @brief Set of mapping settings applied by default
		 *
		 * @details The transitional character transformation mode has been declared obsolete by the
		 *          annex on domain name processing and is not applied by default.
		 *          The host name record rules are not applied by default: the annex
		 *          leaves their application to the discretion of the consumer.
		 *
		 * \~
		 */
		constexpr uint16_t DEFAULT_MODE = (
			static_cast <uint16_t> (option_t::HYPHENS) |
			static_cast <uint16_t> (option_t::BIDI) |
			static_cast <uint16_t> (option_t::JOINERS) |
			static_cast <uint16_t> (option_t::LENGTH)
		);

		/**
		 * \~russian
		 * @brief Код ошибки приведения доменного имени
		 *
		 * \~english
		 * @brief Code of a domain name mapping error
		 *
		 * \~
		 */
		enum class error_t : uint8_t {
			NONE          = 0x00, // Ошибки не обнаружено
			ENCODING      = 0x01, // Доменное имя записано неправильно
			DISALLOWED    = 0x02, // Доменное имя содержит недопустимый символ
			PUNYCODE      = 0x03, // Запись метки кодировкой Punycode разобрать не вышло
			HYPHEN        = 0x04, // Знак переноса размещён недопустимо
			LEADING_MARK  = 0x05, // Метка начинается с сочетающегося знака
			NOT_NORMAL    = 0x06, // Метка не приведена к нормальному представлению
			BIDI          = 0x07, // Метка нарушает правило двунаправленного письма
			CONTEXT       = 0x08, // Метка нарушает правило сочетания соединителей
			LABEL_LENGTH  = 0x09, // Длина метки доменного имени недопустима
			DOMAIN_LENGTH = 0x0A  // Длина доменного имени недопустима
		};

		/**
		 * \~russian
		 * @brief Запись таблицы преобразований символов
		 *
		 * \~english
		 * @brief Entry of the character transformations table
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Mapping {
			// Наименьшее кодовое значение диапазона
			uint32_t begin;
			// Наибольшее кодовое значение диапазона
			uint32_t end;
			// Смещение преобразования в наборе кодовых значений
			uint32_t offset;
			// Количество кодовых значений преобразования
			uint16_t length;
			// Состояние символов диапазона
			status_t status;
		} mapping_t;

		/**
		 * \~russian
		 * @brief Запись таблицы видов соединения символов
		 *
		 * \~english
		 * @brief Entry of the character joining types table
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Joining {
			// Наименьшее кодовое значение диапазона
			uint32_t begin;
			// Наибольшее кодовое значение диапазона
			uint32_t end;
			// Вид соединения символов диапазона
			joining_t joining;
		} joining_range_t;
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"

#endif // __AWH_IDNA_TYPES__
