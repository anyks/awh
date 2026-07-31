/**
 * @file: unicode.hpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл свойств Юникода модуля регулярных выражений — таблицы общих
 *        категорий, письменностей, их расширений, двоичных свойств, классов двунаправленности
 *        и приведения регистра, а также поиск свойств символа и разбор имён свойств
 *
 * @section unicode_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Таблицы порождаются из состава подмодуля эталонной реализации</b>
 *          порождателем sh/generate_unicode.py, а не из состава стандарта Юникода
 *          напрямую. Издание стандарта тем самым совпадает с изданием, которого
 *          держится эталон, и сверка с ним остаётся осмысленной. Смена издания
 *          подмодуля требует повторного порождения таблиц.
 *
 *          <b>Набор принимаемых имён свойств снимается опросом эталонной реализации</b>
 *          стендом tools/verify/regex/accepted.cpp и хранится файлом sh/unicode.accepted.
 *          Набор имён, признаваемых эталоном, ни таблицами стандарта, ни его текстом
 *          не задаётся: часть имён эталон отвергает, а часть разрешает сверх стандарта.
 *
 *          <b>Свойство Bidi_Mirrored берётся из таблицы BidiMirroring.txt</b>, а не
 *          из девятого поля таблицы UnicodeData.txt, откуда его берёт сам стандарт.
 *          Так поступает эталонная реализация, и наборы символов этих двух источников
 *          между собой не совпадают.
 *
 *          <b>Обозначения «bc=M», «bc=C» и «bc=Control» задают двоичные свойства</b>
 *          Bidi_Mirrored и Bidi_Control, тогда как обозначение вида «bc=» задаёт класс
 *          двунаправленности. Так их разрешает эталонная реализация, сличающая имена
 *          нестрого, и совместимость с ней здесь важнее строгости обозначения.
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_UNICODE__
#define __AWH_REGEX_UNICODE__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён модуля регулярных выражений
	 *
	 */
	namespace regex {
		/**
		 * @brief Основание идентификаторов письменностей
		 *
		 */
		constexpr uint16_t SCRIPT_BASE = 0x0100;

		/**
		 * @brief Основание идентификаторов письменностей с их расширениями
		 *
		 * @details Имя письменности, записанное без указания вида свойства, соответствует
		 *          объединению письменности символа и набора её расширений, что установлено
		 *          сличением с эталонной реализацией.
		 *
		 */
		constexpr uint16_t UNITED_BASE = 0x0400;

		/**
		 * @brief Основание идентификаторов расширений письменностей
		 *
		 */
		constexpr uint16_t EXTENDED_BASE = 0x0800;

		/**
		 * @brief Основание идентификаторов двоичных свойств
		 *
		 */
		constexpr uint16_t BINARY_BASE = 0x1000;

		/**
		 * @brief Основание идентификаторов классов двунаправленности
		 *
		 */
		constexpr uint16_t BIDI_BASE = 0x1800;

		/**
		 * @brief Признак завершения набора письменностей расширения
		 *
		 */
		constexpr uint16_t EXTENSION_END = 0xFFFF;

		/**
		 * @brief Номер письменности неназначенных символов
		 *
		 * @details Письменность неназначенных символов диапазонами не задана
		 *          и отведена номером, следующим за заданными письменностями.
		 *
		 */
		extern __AWH_SHARED_EXPORT__ const size_t SCRIPTS_UNKNOWN;

		/**
		 * @brief Класс разбиения текста на графемные кластеры
		 *
		 * @details Значения соответствуют свойству разбиения графемных кластеров
		 *          приложения по разбиению текста стандарта Юникода.
		 *
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
		 * @brief Положение символа в сочетании индийских письменностей
		 *
		 */
		enum class indic_t : uint8_t {
			NONE      = 0x00, // Положение не задано
			CONSONANT = 0x01, // Согласный символ
			EXTEND    = 0x02, // Продолжающий символ сочетания
			LINKER    = 0x03  // Соединяющий символ сочетания
		};

		/**
		 * @brief Диапазон кодовых значений символов, обладающих значением свойства
		 *
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
		 * @brief Диапазон кодовых значений с общим смещением приведения регистра
		 *
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
		 * @brief Соответствие имени свойства его идентификатору
		 *
		 */
		typedef struct __AWH_SHARED_EXPORT__ Naming {
			// Имя свойства, приведённое к нормальному виду
			const char * name;
			// Идентификатор свойства Юникода
			uint16_t id;
		} naming_t;

		/**
		 * Таблицы свойств Юникода, порождаемые средством «sh/generate_unicode.py»
		 */
		extern __AWH_SHARED_EXPORT__ const interval_t CATEGORIES[];
		extern __AWH_SHARED_EXPORT__ const size_t CATEGORIES_COUNT;
		extern __AWH_SHARED_EXPORT__ const interval_t SCRIPTS[];
		extern __AWH_SHARED_EXPORT__ const size_t SCRIPTS_COUNT;
		extern __AWH_SHARED_EXPORT__ const interval_t EXTENSIONS[];
		extern __AWH_SHARED_EXPORT__ const size_t EXTENSIONS_COUNT;
		extern __AWH_SHARED_EXPORT__ const interval_t BIDIRECTIONAL[];
		extern __AWH_SHARED_EXPORT__ const size_t BIDIRECTIONAL_COUNT;
		extern __AWH_SHARED_EXPORT__ const interval_t BINARIES[];
		extern __AWH_SHARED_EXPORT__ const size_t BINARIES_COUNT;
		extern __AWH_SHARED_EXPORT__ const uint16_t EXTENSION_SETS[];
		extern __AWH_SHARED_EXPORT__ const uint32_t EXTENSION_OFFSETS[];
		extern __AWH_SHARED_EXPORT__ const uint32_t BINARY_SPANS[];
		extern __AWH_SHARED_EXPORT__ const size_t BINARY_COUNT;
		extern __AWH_SHARED_EXPORT__ const naming_t NAMES[];
		extern __AWH_SHARED_EXPORT__ const size_t NAMES_COUNT;
		extern __AWH_SHARED_EXPORT__ const folding_t FOLDING[];
		extern __AWH_SHARED_EXPORT__ const size_t FOLDING_COUNT;
		extern __AWH_SHARED_EXPORT__ const interval_t ORBITS[];
		extern __AWH_SHARED_EXPORT__ const size_t ORBITS_COUNT;
		extern __AWH_SHARED_EXPORT__ const uint32_t ORBIT_SETS[];
		extern __AWH_SHARED_EXPORT__ const interval_t CLUSTERS[];
		extern __AWH_SHARED_EXPORT__ const size_t CLUSTERS_COUNT;
		extern __AWH_SHARED_EXPORT__ const interval_t INDIC[];
		extern __AWH_SHARED_EXPORT__ const size_t INDIC_COUNT;

		/**
		 * @brief Функция извлечения идентификатора свойства по его имени
		 *
		 * @details Имя свойства приводится к нормальному виду: буквы записываются
		 *          в нижнем регистре, разделители имени опускаются. Имена свойств,
		 *          не поддерживаемых модулем, не распознаются.
		 *
		 * @param name имя свойства Юникода
		 * @return     идентификатор свойства либо признак нераспознанного имени
		 *
		 */
		__AWH_SHARED_EXPORT__ uint16_t property(string_view name) noexcept;
		/**
		 * @brief Функция проверки обладания символом свойством Юникода
		 *
		 * @param code кодовое значение проверяемого символа
		 * @param id   идентификатор проверяемого свойства Юникода
		 * @return     результат проверки обладания символом свойством
		 *
		 */
		__AWH_SHARED_EXPORT__ bool holds(const uint32_t code, const uint16_t id) noexcept;
		/**
		 * @brief Функция извлечения общей категории символа
		 *
		 * @param code кодовое значение символа
		 * @return     идентификатор общей категории символа
		 *
		 */
		__AWH_SHARED_EXPORT__ uint16_t general(const uint32_t code) noexcept;
		/**
		 * @brief Функция простого приведения регистра символа
		 *
		 * @details Приведение выполняется по таблице простого приведения регистра
		 *          Юникода, приводящей символы, различающиеся лишь регистром,
		 *          к одному кодовому значению.
		 *
		 * @param code кодовое значение приводимого символа
		 * @return     приведённое кодовое значение символа
		 *
		 */
		__AWH_SHARED_EXPORT__ uint32_t casefold(const uint32_t code) noexcept;
		/**
		 * @brief Функция извлечения набора символов, приводимых к одному значению
		 *
		 * @details Набор содержит все символы, приводимые приведением регистра
		 *          к одному значению, включая сам символ. Символы, не имеющие иного
		 *          регистра, набора не образуют.
		 *
		 * @param code   кодовое значение символа
		 * @param result набор символов, приводимых к одному значению
		 * @return       результат наличия набора приведения регистра
		 *
		 */
		__AWH_SHARED_EXPORT__ bool variants(const uint32_t code, vector <uint32_t> & result) noexcept;
		/**
		 * @brief Функция извлечения класса разбиения текста на графемные кластеры
		 *
		 * @param code кодовое значение символа
		 * @return     класс разбиения текста на графемные кластеры
		 *
		 */
		__AWH_SHARED_EXPORT__ cluster_t cluster(const uint32_t code) noexcept;
		/**
		 * @brief Функция извлечения положения символа в сочетании индийских письменностей
		 *
		 * @param code кодовое значение символа
		 * @return     положение символа в сочетании индийских письменностей
		 *
		 */
		__AWH_SHARED_EXPORT__ indic_t indic(const uint32_t code) noexcept;
	};
};

#endif // __AWH_REGEX_UNICODE__
