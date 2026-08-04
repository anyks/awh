/**
 * @file: unicode.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля Юникода — поиск значения свойства
 *        символа двоичным поиском по таблицам диапазонов, разбор имён свойств и простое
 *        приведение регистра с наборами символов, приводимых к одному значению
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <unicode/unicode.hpp>
#include <sys/ascii.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Пространство имён вспомогательных функций свойств Юникода
 *
 */
namespace {
	/**
	 * @brief Функция поиска диапазона, содержащего кодовое значение символа
	 *
	 * @param table  таблица диапазонов кодовых значений
	 * @param count  количество диапазонов таблицы
	 * @param code   кодовое значение искомого символа
	 * @param begin  индекс первого просматриваемого диапазона
	 * @param length количество просматриваемых диапазонов
	 * @return       индекс обнаруженного диапазона либо количество диапазонов
	 *
	 */
	inline size_t search(const awh::unicode::interval_t * table, const size_t count, const uint32_t code, const size_t begin, const size_t length) noexcept {
		// Получаем наименьший индекс просматриваемого участка таблицы
		size_t lower = begin;
		// Получаем наибольший индекс просматриваемого участка таблицы
		size_t upper = (begin + length);
		/**
		 * Выполняем двоичный поиск диапазона в таблице
		 */
		while(lower < upper) {
			// Получаем индекс середины просматриваемого участка
			const size_t middle = (lower + ((upper - lower) / 2));
			/**
			 * Если кодовое значение находится левее диапазона
			 */
			if(code < table[middle].begin)
				// Продолжаем поиск в левой половине участка
				upper = middle;
			/**
			 * Если кодовое значение находится правее диапазона
			 */
			else if(code > table[middle].end)
				// Продолжаем поиск в правой половине участка
				lower = (middle + 1);
			// Выводим индекс обнаруженного диапазона
			else return middle;
		}
		// Выводим отсутствие диапазона, содержащего кодовое значение
		return count;
	}
	/**
	 * @brief Функция проверки принадлежности категории её группе
	 *
	 * @param category идентификатор общей категории символа
	 * @param group    идентификатор проверяемой группы категорий
	 * @return         результат проверки принадлежности категории группе
	 *
	 */
	inline bool grouped(const uint16_t category, const uint16_t group) noexcept {
		/**
		 * Определяем идентификатор проверяемой группы категорий
		 */
		switch(group) {
			// Выполняем проверку принадлежности группе прочих символов
			case static_cast <uint16_t> (awh::unicode::property_id_t::C):
				return ((category >= static_cast <uint16_t> (awh::unicode::property_id_t::Cc)) &&
				 (category <= static_cast <uint16_t> (awh::unicode::property_id_t::Cs)));
			// Выполняем проверку принадлежности группе букв
			case static_cast <uint16_t> (awh::unicode::property_id_t::L):
				return ((category >= static_cast <uint16_t> (awh::unicode::property_id_t::Ll)) &&
				 (category <= static_cast <uint16_t> (awh::unicode::property_id_t::Lu)));
			/**
			 * Выполняем проверку принадлежности группе букв, изменяющих регистр
			 */
			case static_cast <uint16_t> (awh::unicode::property_id_t::L_AMP):
				return ((category == static_cast <uint16_t> (awh::unicode::property_id_t::Ll)) ||
				 (category == static_cast <uint16_t> (awh::unicode::property_id_t::Lt)) ||
				 (category == static_cast <uint16_t> (awh::unicode::property_id_t::Lu)));
			// Выполняем проверку принадлежности группе знаков
			case static_cast <uint16_t> (awh::unicode::property_id_t::M):
				return ((category >= static_cast <uint16_t> (awh::unicode::property_id_t::Mc)) &&
				 (category <= static_cast <uint16_t> (awh::unicode::property_id_t::Mn)));
			// Выполняем проверку принадлежности группе чисел
			case static_cast <uint16_t> (awh::unicode::property_id_t::N):
				return ((category >= static_cast <uint16_t> (awh::unicode::property_id_t::Nd)) &&
				 (category <= static_cast <uint16_t> (awh::unicode::property_id_t::No)));
			// Выполняем проверку принадлежности группе знаков пунктуации
			case static_cast <uint16_t> (awh::unicode::property_id_t::P):
				return ((category >= static_cast <uint16_t> (awh::unicode::property_id_t::Pc)) &&
				 (category <= static_cast <uint16_t> (awh::unicode::property_id_t::Ps)));
			// Выполняем проверку принадлежности группе символов
			case static_cast <uint16_t> (awh::unicode::property_id_t::S):
				return ((category >= static_cast <uint16_t> (awh::unicode::property_id_t::Sc)) &&
				 (category <= static_cast <uint16_t> (awh::unicode::property_id_t::So)));
			// Выполняем проверку принадлежности группе разделителей
			case static_cast <uint16_t> (awh::unicode::property_id_t::Z):
				return ((category >= static_cast <uint16_t> (awh::unicode::property_id_t::Zl)) &&
				 (category <= static_cast <uint16_t> (awh::unicode::property_id_t::Zs)));
		}
		// Выводим результат проверки совпадения категории с проверяемой
		return (category == group);
	}
};

/**
 * @brief Функция извлечения общей категории символа
 *
 * @param code кодовое значение символа
 * @return     идентификатор общей категории символа
 *
 */
uint16_t awh::unicode::general(const uint32_t code) noexcept {
	// Выполняем поиск диапазона, содержащего кодовое значение символа
	const size_t index = search(CATEGORIES, CATEGORIES_COUNT, code, 0, CATEGORIES_COUNT);
	/**
	 * Если диапазон, содержащий кодовое значение, обнаружен
	 */
	if(index < CATEGORIES_COUNT)
		// Выводим идентификатор общей категории символа
		return CATEGORIES[index].value;
	// Выводим категорию неназначенного символа
	return static_cast <uint16_t> (property_id_t::Cn);
}
/**
 * @brief Функция извлечения идентификатора свойства по его имени
 *
 * @param name имя свойства Юникода
 * @return     идентификатор свойства либо признак нераспознанного имени
 *
 */
uint16_t awh::unicode::property(string_view name) noexcept {
	// Приведённое к нормальному виду имя свойства Юникода
	string key;
	/**
	 * Выполняем приведение имени свойства Юникода к нормальному виду
	 */
	for(auto & letter : name) {
		/**
		 * Если очередной символ является разделителем имени свойства
		 */
		if((letter == '_') || (letter == '-') || (letter == ' '))
			// Переходим к следующему символу имени свойства
			continue;
		// Выполняем добавление символа имени свойства в нижнем регистре
		key.append(1, ascii::toLower(letter));
	}
	// Получаем наименьший индекс просматриваемого участка таблицы имён
	size_t lower = 0;
	// Получаем наибольший индекс просматриваемого участка таблицы имён
	size_t upper = NAMES_COUNT;
	/**
	 * Выполняем двоичный поиск имени свойства в таблице соответствия
	 */
	while(lower < upper) {
		// Получаем индекс середины просматриваемого участка
		const size_t middle = (lower + ((upper - lower) / 2));
		// Выполняем сличение искомого имени с именем таблицы
		const int32_t result = key.compare(NAMES[middle].name);
		/**
		 * Если искомое имя предшествует имени таблицы
		 */
		if(result < 0)
			// Продолжаем поиск в левой половине участка
			upper = middle;
		/**
		 * Если искомое имя следует за именем таблицы
		 */
		else if(result > 0)
			// Продолжаем поиск в правой половине участка
			lower = (middle + 1);
		// Выводим идентификатор обнаруженного свойства
		else return NAMES[middle].id;
	}
	// Выводим признак нераспознанного имени свойства
	return static_cast <uint16_t> (property_id_t::UNKNOWN);
}
/**
 * @brief Функция проверки обладания символом свойством Юникода
 *
 * @param code кодовое значение проверяемого символа
 * @param id   идентификатор проверяемого свойства Юникода
 * @return     результат проверки обладания символом свойством
 *
 */
bool awh::unicode::holds(const uint32_t code, const uint16_t id) noexcept {
	/**
	 * Если проверяется класс двунаправленности символа
	 */
	if(id >= BIDI_BASE) {
		// Выполняем поиск диапазона, содержащего кодовое значение символа
		const size_t index = search(BIDIRECTIONAL, BIDIRECTIONAL_COUNT, code, 0, BIDIRECTIONAL_COUNT);
		/**
		 * Если диапазон, содержащий кодовое значение, не обнаружен
		 */
		if(index >= BIDIRECTIONAL_COUNT)
			// Выводим результат проверки обладания символом свойством
			return false;
		// Выводим результат сличения класса двунаправленности символа
		return (BIDIRECTIONAL[index].value == (id - BIDI_BASE));
	}
	/**
	 * Если проверяется двоичное свойство символа
	 */
	if(id >= BINARY_BASE) {
		// Получаем номер проверяемого двоичного свойства
		const size_t number = (id - BINARY_BASE);
		/**
		 * Если номер двоичного свойства находится за пределами таблицы
		 */
		if(number >= BINARY_COUNT)
			// Выводим результат проверки обладания символом свойством
			return false;
		// Получаем смещение диапазонов проверяемого двоичного свойства
		const size_t offset = BINARY_SPANS[number * 2];
		// Получаем количество диапазонов проверяемого двоичного свойства
		const size_t length = BINARY_SPANS[(number * 2) + 1];
		// Выводим результат поиска кодового значения в диапазонах свойства
		return (search(BINARIES, BINARIES_COUNT, code, offset, length) < BINARIES_COUNT);
	}
	/**
	 * Если проверяется расширение письменности символа
	 */
	if(id >= EXTENDED_BASE) {
		// Получаем номер проверяемой письменности
		const uint16_t number = (id - EXTENDED_BASE);
		// Выполняем поиск диапазона расширений, содержащего кодовое значение
		const size_t index = search(EXTENSIONS, EXTENSIONS_COUNT, code, 0, EXTENSIONS_COUNT);
		/**
		 * Если диапазон расширений, содержащий кодовое значение, обнаружен
		 */
		if(index < EXTENSIONS_COUNT) {
			// Получаем смещение набора письменностей расширения
			const uint32_t * offset = (EXTENSION_OFFSETS + EXTENSIONS[index].value);
			/**
			 * Выполняем обход набора письменностей расширения
			 */
			for(size_t i = (* offset); EXTENSION_SETS[i] != EXTENSION_END; i++) {
				/**
				 * Если письменность расширения совпадает с проверяемой
				 */
				if(EXTENSION_SETS[i] == number)
					// Выводим результат проверки обладания символом свойством
					return true;
			}
			// Выводим отсутствие проверяемой письменности в наборе расширения
			return false;
		}
		/**
		 * Выполняем проверку письменности символа
		 *
		 * @details Расширение письменности, не заданное для символа явно, совпадает
		 *          с его письменностью, поэтому проверка выполняется по ней.
		 *
		 */
		return holds(code, static_cast <uint16_t> (SCRIPT_BASE + number));
	}
	/**
	 * Если проверяется письменность символа вместе с её расширениями
	 */
	if(id >= UNITED_BASE) {
		// Получаем номер проверяемой письменности
		const uint16_t number = (id - UNITED_BASE);
		/**
		 * Если символ принадлежит проверяемой письменности
		 */
		if(holds(code, static_cast <uint16_t> (SCRIPT_BASE + number)))
			// Выводим результат проверки обладания символом свойством
			return true;
		// Выводим результат проверки принадлежности расширениям письменности
		return holds(code, static_cast <uint16_t> (EXTENDED_BASE + number));
	}
	/**
	 * Если проверяется письменность символа
	 */
	if(id >= SCRIPT_BASE) {
		// Получаем номер проверяемой письменности
		const uint16_t number = (id - SCRIPT_BASE);
		// Выполняем поиск диапазона, содержащего кодовое значение символа
		const size_t index = search(SCRIPTS, SCRIPTS_COUNT, code, 0, SCRIPTS_COUNT);
		/**
		 * Если диапазон, содержащий кодовое значение, не обнаружен
		 *
		 * @details Символы, письменность которых не задана, принадлежат письменности
		 *          неназначенных символов, отведённой последним номером.
		 *
		 */
		if(index >= SCRIPTS_COUNT)
			// Выводим результат сличения с письменностью неназначенных символов
			return (static_cast <size_t> (number) == SCRIPTS_UNKNOWN);
		// Выводим результат сличения письменности символа
		return (SCRIPTS[index].value == number);
	}
	/**
	 * Определяем идентификатор проверяемого свойства Юникода
	 */
	switch(id) {
		// Выводим результат проверки свойства любого символа
		case static_cast <uint16_t> (property_id_t::ANY): return true;
		/**
		 * Выводим результат проверки расширенного класса букв и цифр
		 */
		case static_cast <uint16_t> (property_id_t::XAN): {
			// Получаем общую категорию проверяемого символа
			const uint16_t category = general(code);
			// Выводим результат проверки принадлежности буквам либо числам
			return (grouped(category, static_cast <uint16_t> (property_id_t::L)) ||
			 grouped(category, static_cast <uint16_t> (property_id_t::N)));
		}
		/**
		 * Выводим результат проверки символов слова режима соответствия Юникоду
		 *
		 * @details Состав класса установлен сличением с эталонной реализацией и не
		 *          совпадает с расширенным классом «Xwd»: помимо букв и чисел в него
		 *          входят неотступающие знаки и соединительная пунктуация целиком.
		 *
		 */
		case static_cast <uint16_t> (property_id_t::XWD):
		/**
		 * Выводим результат проверки символов слова режима соответствия Юникоду
		 */
		case static_cast <uint16_t> (property_id_t::UCP_WORD): {
			// Получаем общую категорию проверяемого символа
			const uint16_t category = general(code);
			/**
			 * Если символ является неотступающим знаком либо соединительной пунктуацией
			 */
			if((category == static_cast <uint16_t> (property_id_t::Mn)) ||
			 (category == static_cast <uint16_t> (property_id_t::Pc)))
				// Выводим результат проверки обладания символом свойством
				return true;
			// Выводим результат проверки принадлежности буквам либо числам
			return (grouped(category, static_cast <uint16_t> (property_id_t::L)) ||
			 grouped(category, static_cast <uint16_t> (property_id_t::N)));
		}
		/**
		 * Выводим результат проверки пробельных символов режима соответствия Юникоду
		 *
		 * @details Состав класса установлен сличением с эталонной реализацией и не
		 *          совпадает с расширенным классом «Xps»: помимо разделителей в него
		 *          входят перечисленные управляющие символы и разделитель гласной.
		 *
		 */
		case static_cast <uint16_t> (property_id_t::XPS):
		/**
		 * Выводим результат проверки расширенного класса пробельных разделителей
		 */
		case static_cast <uint16_t> (property_id_t::XSP):
		/**
		 * Выводим результат проверки пробельных символов режима соответствия Юникоду
		 */
		case static_cast <uint16_t> (property_id_t::UCP_SPACE): {
			/**
			 * Если символ является управляющим пробельным символом
			 */
			if(((code >= 0x09) && (code <= 0x0D)) || (code == 0x85) || (code == 0x180E))
				// Выводим результат проверки обладания символом свойством
				return true;
			// Выводим результат проверки принадлежности разделителям
			return grouped(general(code), static_cast <uint16_t> (property_id_t::Z));
		}
		/**
		 * Выводим результат проверки знаков пунктуации класса POSIX режима «UCP»
		 *
		 * @details Состав класса установлен сличением с эталонной реализацией:
		 *          знаки пунктуации целиком, а из символов - лишь те, коды каких
		 *          принадлежат набору ASCII. Знак доллара, плюс, знак «меньше»
		 *          и им подобные относятся к символам, а не к пунктуации, но
		 *          классу POSIX «punct» принадлежат, поскольку принадлежали ему
		 *          и до появления Юникода.
		 *
		 */
		case static_cast <uint16_t> (property_id_t::PX_PUNCT): {
			// Получаем общую категорию проверяемого символа
			const uint16_t category = general(code);
			/**
			 * Если символ принадлежит знакам пунктуации
			 */
			if(grouped(category, static_cast <uint16_t> (property_id_t::P)))
				// Выводим результат проверки обладания символом свойством
				return true;
			/**
			 * Если символ набору ASCII не принадлежит
			 */
			if(code > 0x7F)
				// Выводим результат проверки обладания символом свойством
				return false;
			// Выводим результат проверки принадлежности символам
			return grouped(category, static_cast <uint16_t> (property_id_t::S));
		}
		/**
		 * Выводим результат проверки видимых символов класса POSIX режима «UCP»
		 *
		 * @details Состав класса установлен сличением с эталонной реализацией:
		 *          всё, помимо разделителей и прочих символов, - однако символы
		 *          форматирования, вопреки принадлежности прочим, в класс входят,
		 *          за вычетом шести перечисленных, вида не имеющих вовсе.
		 *
		 */
		case static_cast <uint16_t> (property_id_t::PX_GRAPH): {
			// Получаем общую категорию проверяемого символа
			const uint16_t category = general(code);
			/**
			 * Если символ принадлежит разделителям
			 */
			if(grouped(category, static_cast <uint16_t> (property_id_t::Z)))
				// Выводим результат проверки обладания символом свойством
				return false;
			/**
			 * Если символ прочим символам не принадлежит
			 */
			if(!grouped(category, static_cast <uint16_t> (property_id_t::C)))
				// Выводим результат проверки обладания символом свойством
				return true;
			/**
			 * Если символ символом форматирования не является
			 */
			if(category != static_cast <uint16_t> (property_id_t::Cf))
				// Выводим результат проверки обладания символом свойством
				return false;
			// Выводим результат проверки отсутствия символа среди невидимых
			return ((code != 0x061C) && (code != 0x180E) && ((code < 0x2066) || (code > 0x2069)));
		}
		/**
		 * Выводим результат проверки печатаемых символов класса POSIX режима «UCP»
		 *
		 * @details Состав класса установлен сличением с эталонной реализацией:
		 *          видимые символы вместе с пробельными разделителями. Разделитель
		 *          гласной монгольского письма классу «print» принадлежит, а классу
		 *          «graph» - нет, поэтому списки исключаемых символов разнятся.
		 *
		 */
		case static_cast <uint16_t> (property_id_t::PX_PRINT): {
			// Получаем общую категорию проверяемого символа
			const uint16_t category = general(code);
			/**
			 * Если символ принадлежит разделителям строк либо абзацев
			 */
			if((category == static_cast <uint16_t> (property_id_t::Zl)) ||
			 (category == static_cast <uint16_t> (property_id_t::Zp)))
				// Выводим результат проверки обладания символом свойством
				return false;
			/**
			 * Если символ прочим символам не принадлежит
			 */
			if(!grouped(category, static_cast <uint16_t> (property_id_t::C)))
				// Выводим результат проверки обладания символом свойством
				return true;
			/**
			 * Если символ символом форматирования не является
			 */
			if(category != static_cast <uint16_t> (property_id_t::Cf))
				// Выводим результат проверки обладания символом свойством
				return false;
			// Выводим результат проверки отсутствия символа среди невидимых
			return ((code != 0x061C) && ((code < 0x2066) || (code > 0x2069)));
		}
		/**
		 * Выводим результат проверки шестнадцатеричных цифр класса POSIX режима «UCP»
		 *
		 * @details Состав класса установлен сличением с эталонной реализацией:
		 *          помимо цифр набора ASCII в класс входят их полноширинные
		 *          начертания.
		 *
		 */
		case static_cast <uint16_t> (property_id_t::PX_XDIGIT):
			// Выводим результат проверки принадлежности шестнадцатеричным цифрам
			return (((code >= 0x30) && (code <= 0x39)) ||
			 ((code >= 0x41) && (code <= 0x46)) ||
			 ((code >= 0x61) && (code <= 0x66)) ||
			 ((code >= 0xFF10) && (code <= 0xFF19)) ||
			 ((code >= 0xFF21) && (code <= 0xFF26)) ||
			 ((code >= 0xFF41) && (code <= 0xFF46)));
		/**
		 * Выводим результат проверки расширенного класса пробельных символов
		 */
		/**
		 * Выводим результат проверки расширенного класса символов имён
		 */
		/**
		 * Выполняем проверку принадлежности символа набору ASCII
		 */
		case static_cast <uint16_t> (property_id_t::ASCII): {
			// Выводим результат проверки принадлежности символа набору ASCII
			return (code <= 0x7F);
		}
		case static_cast <uint16_t> (property_id_t::XUC): {
			// Выводим результат проверки принадлежности символам имён
			return ((code == 0x24) || (code == 0x40) || (code == 0x60));
		}
	}
	// Выводим результат проверки принадлежности символа категории либо её группе
	return grouped(general(code), id);
}
/**
 * @brief Функция извлечения класса двунаправленности символа
 *
 * @param code кодовое значение символа
 * @return     идентификатор класса двунаправленности символа
 *
 */
uint16_t awh::unicode::bidirectional(const uint32_t code) noexcept {
	// Выполняем поиск диапазона, содержащего кодовое значение символа
	const size_t index = search(BIDIRECTIONAL, BIDIRECTIONAL_COUNT, code, 0, BIDIRECTIONAL_COUNT);
	/**
	 * Если диапазон, содержащий кодовое значение, обнаружен
	 */
	if(index < BIDIRECTIONAL_COUNT)
		// Выводим идентификатор класса двунаправленности символа
		return static_cast <uint16_t> (BIDI_BASE + BIDIRECTIONAL[index].value);
	// Выводим отсутствие класса двунаправленности символа
	return static_cast <uint16_t> (property_id_t::UNKNOWN);
}
/**
 * @brief Функция простого приведения регистра символа
 *
 * @param code кодовое значение приводимого символа
 * @return     приведённое кодовое значение символа
 *
 */
uint32_t awh::unicode::casefold(const uint32_t code) noexcept {
	// Получаем наименьший индекс просматриваемого участка таблицы
	size_t lower = 0;
	// Получаем наибольший индекс просматриваемого участка таблицы
	size_t upper = FOLDING_COUNT;
	/**
	 * Выполняем двоичный поиск диапазона приведения регистра
	 */
	while(lower < upper) {
		// Получаем индекс середины просматриваемого участка
		const size_t middle = (lower + ((upper - lower) / 2));
		/**
		 * Если кодовое значение находится левее диапазона
		 */
		if(code < FOLDING[middle].begin)
			// Продолжаем поиск в левой половине участка
			upper = middle;
		/**
		 * Если кодовое значение находится правее диапазона
		 */
		else if(code > FOLDING[middle].end)
			// Продолжаем поиск в правой половине участка
			lower = (middle + 1);
		/**
		 * Выводим приведённое кодовое значение символа
		 */
		else return static_cast <uint32_t> (static_cast <int64_t> (code) + FOLDING[middle].delta);
	}
	// Выводим кодовое значение символа без изменений
	return code;
}
/**
 * @brief Функция извлечения набора символов, приводимых к одному значению
 *
 * @param code   кодовое значение символа
 * @param result набор символов, приводимых к одному значению
 * @return       результат наличия набора приведения регистра
 *
 */
bool awh::unicode::variants(const uint32_t code, vector <uint32_t> & result) noexcept {
	// Выполняем очистку набора символов, приводимых к одному значению
	result.clear();
	// Выполняем поиск размещения набора приведения регистра символа
	const size_t index = search(ORBITS, ORBITS_COUNT, code, 0, ORBITS_COUNT);
	/**
	 * Если размещение набора приведения регистра не обнаружено
	 */
	if(index >= ORBITS_COUNT)
		// Выводим результат наличия набора приведения регистра
		return false;
	/**
	 * Выполняем извлечение набора символов, приводимых к одному значению
	 */
	for(size_t i = ORBITS[index].value; ORBIT_SETS[i] != 0; i++)
		// Выполняем добавление очередного символа набора
		result.push_back(ORBIT_SETS[i]);
	// Выводим результат наличия набора приведения регистра
	return !result.empty();
}
/**
 * @brief Функция извлечения класса разбиения текста на графемные кластеры
 *
 * @param code кодовое значение символа
 * @return     класс разбиения текста на графемные кластеры
 *
 */
awh::unicode::cluster_t awh::unicode::cluster(const uint32_t code) noexcept {
	// Выполняем поиск диапазона, содержащего кодовое значение символа
	const size_t index = search(CLUSTERS, CLUSTERS_COUNT, code, 0, CLUSTERS_COUNT);
	/**
	 * Если диапазон, содержащий кодовое значение, обнаружен
	 */
	if(index < CLUSTERS_COUNT)
		// Выводим класс разбиения текста на графемные кластеры
		return static_cast <cluster_t> (CLUSTERS[index].value);
	// Выводим класс прочих символов
	return cluster_t::OTHER;
}
/**
 * @brief Функция извлечения положения символа в сочетании индийских письменностей
 *
 * @param code кодовое значение символа
 * @return     положение символа в сочетании индийских письменностей
 *
 */
awh::unicode::indic_t awh::unicode::indic(const uint32_t code) noexcept {
	// Выполняем поиск диапазона, содержащего кодовое значение символа
	const size_t index = search(INDIC, INDIC_COUNT, code, 0, INDIC_COUNT);
	/**
	 * Если диапазон, содержащий кодовое значение, обнаружен
	 */
	if(index < INDIC_COUNT)
		// Выводим положение символа в сочетании индийских письменностей
		return static_cast <indic_t> (INDIC[index].value);
	// Выводим отсутствие положения в сочетании индийских письменностей
	return indic_t::NONE;
}
