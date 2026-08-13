/**
 * @file: types.hpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл общих определений модуля Юникода — идентификаторы свойств,
 *        основания их диапазонов, классы разбиения текста на графемные кластеры,
 *        положения символов в сочетаниях индийских письменностей и виды записей таблиц
 *
 * \~english
 * @brief Header file of the common definitions of the Unicode module — property identifiers,
 *        the bases of their ranges, the classes of splitting text into grapheme clusters,
 *        the positions of characters within Indic conjuncts and the kinds of table entries
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNICODE_TYPES__
#define __AWH_UNICODE_TYPES__

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
	 * @brief Пространство имён модуля Юникода
	 *
	 * \~english
	 * @brief Namespace of the Unicode module
	 *
	 * \~
	 */
	namespace unicode {
		/**
		 * \~russian
		 * @brief Основание идентификаторов письменностей
		 *
		 * \~english
		 * @brief Base of the script identifiers
		 *
		 * \~
		 */
		constexpr uint16_t SCRIPT_BASE = 0x0100;

		/**
		 * \~russian
		 * @brief Основание идентификаторов письменностей с их расширениями
		 *
		 * @details Имя письменности, записанное без указания вида свойства, соответствует
		 *          объединению письменности символа и набора её расширений, что установлено
		 *          сличением с эталонной реализацией.
		 *
		 * \~english
		 * @brief Base of the identifiers of scripts together with their extensions
		 *
		 * @details A script name recorded without stating the kind of property corresponds to the
		 *          union of the script of the character and the set of its extensions, which has been
		 *          established by comparison with the reference implementation.
		 *
		 * \~
		 */
		constexpr uint16_t UNITED_BASE = 0x0400;

		/**
		 * \~russian
		 * @brief Основание идентификаторов расширений письменностей
		 *
		 * \~english
		 * @brief Base of the identifiers of script extensions
		 *
		 * \~
		 */
		constexpr uint16_t EXTENDED_BASE = 0x0800;

		/**
		 * \~russian
		 * @brief Основание идентификаторов двоичных свойств
		 *
		 * \~english
		 * @brief Base of the identifiers of binary properties
		 *
		 * \~
		 */
		constexpr uint16_t BINARY_BASE = 0x1000;

		/**
		 * \~russian
		 * @brief Основание идентификаторов классов двунаправленности
		 *
		 * \~english
		 * @brief Base of the identifiers of bidirectional classes
		 *
		 * \~
		 */
		constexpr uint16_t BIDI_BASE = 0x1800;

		/**
		 * \~russian
		 * @brief Признак завершения набора письменностей расширения
		 *
		 * \~english
		 * @brief Sign of the end of the set of scripts of an extension
		 *
		 * \~
		 */
		constexpr uint16_t EXTENSION_END = 0xFFFF;

		/**
		 * \~russian
		 * @brief Номер письменности неназначенных символов
		 *
		 * @details Письменность неназначенных символов диапазонами не задана
		 *          и отведена номером, следующим за заданными письменностями.
		 *
		 * \~english
		 * @brief Number of the script of unassigned characters
		 *
		 * @details The script of unassigned characters is not defined by ranges
		 *          and is allotted the number following the defined scripts.
		 *
		 * \~
		 */
		extern __AWH_SHARED_EXPORT__ const size_t SCRIPTS_UNKNOWN;

		/**
		 * \~russian
		 * @brief Идентификатор свойства Юникода
		 *
		 * @details Значения соответствуют общим категориям Юникода и расширенным
		 *          классам символов PCRE. Идентификаторы письменностей размещаются
		 *          в диапазоне, начинающемся со значения «SCRIPT».
		 *
		 * \~english
		 * @brief Identifier of a Unicode property
		 *
		 * @details The values correspond to the Unicode general categories and to the extended
		 *          character classes of PCRE. The script identifiers are placed within the range
		 *          beginning at the "SCRIPT" value.
		 *
		 * \~
		 */
		enum class property_id_t : uint16_t {
			UNKNOWN = 0x0000, // Свойство не распознано
			ANY     = 0x0001, // Любой символ, свойство «Any»
			C       = 0x0002, // Прочие символы, категория «C»
			Cc      = 0x0003, // Управляющие символы, категория «Cc»
			Cf      = 0x0004, // Символы форматирования, категория «Cf»
			Cn      = 0x0005, // Неназначенные символы, категория «Cn»
			Co      = 0x0006, // Символы частного использования, категория «Co»
			Cs      = 0x0007, // Суррогатные символы, категория «Cs»
			L       = 0x0008, // Буквы, категория «L»
			Ll      = 0x0009, // Строчные буквы, категория «Ll»
			Lm      = 0x000A, // Модифицирующие буквы, категория «Lm»
			Lo      = 0x000B, // Прочие буквы, категория «Lo»
			Lt      = 0x000C, // Буквы с заглавной первой частью, категория «Lt»
			Lu      = 0x000D, // Прописные буквы, категория «Lu»
			L_AMP   = 0x000E, // Буквы, изменяющие регистр, категория «L&»
			M       = 0x000F, // Знаки, категория «M»
			Mc      = 0x0010, // Отступающие знаки, категория «Mc»
			Me      = 0x0011, // Охватывающие знаки, категория «Me»
			Mn      = 0x0012, // Неотступающие знаки, категория «Mn»
			N       = 0x0013, // Числа, категория «N»
			Nd      = 0x0014, // Десятичные цифры, категория «Nd»
			Nl      = 0x0015, // Буквенные числа, категория «Nl»
			No      = 0x0016, // Прочие числа, категория «No»
			P       = 0x0017, // Знаки пунктуации, категория «P»
			Pc      = 0x0018, // Соединительная пунктуация, категория «Pc»
			Pd      = 0x0019, // Тире, категория «Pd»
			Pe      = 0x001A, // Закрывающая пунктуация, категория «Pe»
			Pf      = 0x001B, // Завершающая пунктуация, категория «Pf»
			Pi      = 0x001C, // Начальная пунктуация, категория «Pi»
			Po      = 0x001D, // Прочая пунктуация, категория «Po»
			Ps      = 0x001E, // Открывающая пунктуация, категория «Ps»
			S       = 0x001F, // Символы, категория «S»
			Sc      = 0x0020, // Денежные символы, категория «Sc»
			Sk      = 0x0021, // Модифицирующие символы, категория «Sk»
			Sm      = 0x0022, // Математические символы, категория «Sm»
			So      = 0x0023, // Прочие символы, категория «So»
			Z       = 0x0024, // Разделители, категория «Z»
			Zl      = 0x0025, // Разделители строк, категория «Zl»
			Zp      = 0x0026, // Разделители абзацев, категория «Zp»
			Zs      = 0x0027, // Пробельные разделители, категория «Zs»
			XAN     = 0x0028, // Буквы и цифры, расширенный класс «Xan»
			XPS     = 0x0029, // Пробельные символы, расширенный класс «Xps»
			XSP     = 0x002A, // Пробельные символы, расширенный класс «Xsp»
			XWD     = 0x002B, // Символы слова, расширенный класс «Xwd»
			XUC     = 0x002C, // Символы имён универсальных символов, расширенный класс «Xuc»
			UCP_WORD  = 0x002D, // Символы слова режима «UCP», сокращённый класс «\w»
			UCP_SPACE = 0x002E, // Пробельные символы режима «UCP», сокращённый класс «\s»
			ASCII     = 0x002F, // Символы набора ASCII, свойство «ASCII»
			PX_PUNCT  = 0x0030, // Знаки пунктуации режима «UCP», класс POSIX «punct»
			PX_GRAPH  = 0x0031, // Видимые символы режима «UCP», класс POSIX «graph»
			PX_PRINT  = 0x0032, // Печатаемые символы режима «UCP», класс POSIX «print»
			PX_XDIGIT = 0x0033, // Шестнадцатеричные цифры режима «UCP», класс POSIX «xdigit»
			SCRIPT  = 0x0100  // Начало диапазона идентификаторов письменностей
		};

		/**
		 * \~russian
		 * @brief Класс разбиения текста на графемные кластеры
		 *
		 * @details Значения соответствуют свойству разбиения графемных кластеров
		 *          приложения по разбиению текста стандарта Юникода.
		 *
		 * \~english
		 * @brief Class of splitting text into grapheme clusters
		 *
		 * @details The values correspond to the grapheme cluster break property of the annex
		 *          on text segmentation of the Unicode standard.
		 *
		 * \~
		 */
		enum class cluster_t : uint8_t {
			CONTROL    = 0x00, // Управляющий символ
			CARRIAGE   = 0x01, // Возврат каретки
			LINEFEED   = 0x02, // Перевод строки
			EXTEND     = 0x03, // Продолжающий символ
			JOINER     = 0x04, // Соединитель нулевой ширины
			REGIONAL   = 0x05, // Указатель области
			PREPEND    = 0x06, // Предшествующий символ
			SPACING    = 0x07, // Отступающий знак
			HANGUL_L   = 0x08, // Начальная часть слога хангыля
			HANGUL_V   = 0x09, // Гласная часть слога хангыля
			HANGUL_T   = 0x0A, // Конечная часть слога хангыля
			HANGUL_LV  = 0x0B, // Слог хангыля без конечной части
			HANGUL_LVT = 0x0C, // Слог хангыля с конечной частью
			PICTORIAL  = 0x0D, // Расширенный изобразительный символ
			OTHER      = 0x0E  // Прочие символы
		};

		/**
		 * \~russian
		 * @brief Положение символа в сочетании индийских письменностей
		 *
		 * \~english
		 * @brief Position of a character within an Indic conjunct
		 *
		 * \~
		 */
		enum class indic_t : uint8_t {
			NONE      = 0x00, // Положение не задано
			CONSONANT = 0x01, // Согласный символ
			EXTEND    = 0x02, // Продолжающий символ сочетания
			LINKER    = 0x03  // Соединяющий символ сочетания
		};

		/**
		 * \~russian
		 * @brief Диапазон кодовых значений символов, обладающих значением свойства
		 *
		 * \~english
		 * @brief Range of code values of the characters holding a property value
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Interval {
			// Наименьшее кодовое значение диапазона
			uint32_t begin;
			// Наибольшее кодовое значение диапазона
			uint32_t end;
			// Значение свойства, которым обладают символы диапазона
			uint16_t value;
		} interval_t;

		/**
		 * \~russian
		 * @brief Диапазон кодовых значений с общим смещением приведения регистра
		 *
		 * \~english
		 * @brief Range of code values with a common case folding offset
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Folding {
			// Наименьшее кодовое значение диапазона
			uint32_t begin;
			// Наибольшее кодовое значение диапазона
			uint32_t end;
			// Смещение приведённого значения относительно исходного
			int32_t delta;
		} folding_t;

		/**
		 * \~russian
		 * @brief Разложение символа набором кодовых значений
		 *
		 * \~english
		 * @brief Decomposition of a character into a set of code values
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Decomposition {
			// Кодовое значение разлагаемого символа
			uint32_t code;
			// Смещение разложения в наборе кодовых значений
			uint32_t offset;
			// Количество кодовых значений разложения
			uint16_t length;
			// Признак разложения совместимости
			bool compat;
		} decomposition_t;

		/**
		 * \~russian
		 * @brief Каноническое сочетание пары символов
		 *
		 * \~english
		 * @brief Canonical composition of a pair of characters
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Composition {
			// Кодовое значение начального символа пары
			uint32_t first;
			// Кодовое значение сочетающегося символа пары
			uint32_t second;
			// Кодовое значение получающегося символа
			uint32_t code;
		} composition_t;

		/**
		 * \~russian
		 * @brief Вид нормального представления текста
		 *
		 * @details Виды заданы приложением по нормализации стандарта Юникода.
		 *          Представления совместимости приводят к общему виду символы,
		 *          различающиеся начертанием, и для приведения доменных имён
		 *          применяются лишь на этапе преобразования символов.
		 *
		 * \~english
		 * @brief Kind of the normal form of a text
		 *
		 * @details The kinds are defined by the annex on normalization of the Unicode standard.
		 *          The compatibility forms bring to a common form characters differing in their
		 *          glyph shape, and for domain name mapping they are applied only at the stage
		 *          of character transformation.
		 *
		 * \~
		 */
		enum class form_t : uint8_t {
			NFD  = 0x00, // Каноническое разложение
			NFC  = 0x01, // Каноническое разложение с последующим сочетанием
			NFKD = 0x02, // Разложение совместимости
			NFKC = 0x03  // Разложение совместимости с последующим сочетанием
		};

		/**
		 * \~russian
		 * @brief Соответствие имени свойства его идентификатору
		 *
		 * \~english
		 * @brief Correspondence of a property name to its identifier
		 *
		 * \~
		 */
		typedef struct __AWH_SHARED_EXPORT__ Naming {
			// Имя свойства, приведённое к нормальному виду
			const char * name;
			// Идентификатор свойства Юникода
			uint16_t id;
		} naming_t;
	};
};

#endif // __AWH_UNICODE_TYPES__
