/**
 * @file: normalize.cpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация нормализации текста — полное разложение символов, каноническое
 *        упорядочение сочетающихся знаков и последующее каноническое сочетание,
 *        а также вычисляемое разложение и сочетание слогов хангыля
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <encoding/unicode/normalize.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Пространство имён вспомогательных значений нормализации текста
 *
 */
namespace {
	/**
	 * @brief Наименьшее кодовое значение слога хангыля
	 *
	 */
	constexpr uint32_t HANGUL_BASE = 0xAC00;

	/**
	 * @brief Наименьшее кодовое значение начальной части слога хангыля
	 *
	 */
	constexpr uint32_t LEADING_BASE = 0x1100;

	/**
	 * @brief Наименьшее кодовое значение гласной части слога хангыля
	 *
	 */
	constexpr uint32_t VOWEL_BASE = 0x1161;

	/**
	 * @brief Кодовое значение, предшествующее конечным частям слога хангыля
	 *
	 */
	constexpr uint32_t TRAILING_BASE = 0x11A7;

	/**
	 * @brief Количество начальных частей слога хангыля
	 *
	 */
	constexpr uint32_t LEADING_COUNT = 19;

	/**
	 * @brief Количество гласных частей слога хангыля
	 *
	 */
	constexpr uint32_t VOWEL_COUNT = 21;

	/**
	 * @brief Количество конечных частей слога хангыля вместе с их отсутствием
	 *
	 */
	constexpr uint32_t TRAILING_COUNT = 28;

	/**
	 * @brief Количество слогов хангыля, образуемых одной начальной частью
	 *
	 */
	constexpr uint32_t VOWEL_SPAN = (VOWEL_COUNT * TRAILING_COUNT);

	/**
	 * @brief Количество слогов хангыля
	 *
	 */
	constexpr uint32_t HANGUL_COUNT = (LEADING_COUNT * VOWEL_SPAN);

	/**
	 * @brief Функция поиска разложения символа в таблице разложений
	 *
	 * @param code кодовое значение разлагаемого символа
	 * @return     разложение символа либо пустой указатель
	 *
	 */
	inline const awh::unicode::decomposition_t * search(const uint32_t code) noexcept {
		// Получаем наименьший индекс просматриваемого участка таблицы
		size_t lower = 0;
		// Получаем наибольший индекс просматриваемого участка таблицы
		size_t upper = awh::unicode::DECOMPOSITIONS_COUNT;
		/**
		 * Выполняем двоичный поиск разложения символа в таблице
		 */
		while(lower < upper) {
			// Получаем индекс середины просматриваемого участка
			const size_t middle = (lower + ((upper - lower) / 2));
			/**
			 * Если искомое кодовое значение предшествует значению таблицы
			 */
			if(code < awh::unicode::DECOMPOSITIONS[middle].code)
				// Продолжаем поиск в левой половине участка
				upper = middle;
			/**
			 * Если искомое кодовое значение следует за значением таблицы
			 */
			else if(code > awh::unicode::DECOMPOSITIONS[middle].code)
				// Продолжаем поиск в правой половине участка
				lower = (middle + 1);
			// Выводим обнаруженное разложение символа
			else return &awh::unicode::DECOMPOSITIONS[middle];
		}
		// Выводим отсутствие разложения символа
		return nullptr;
	}
};

/**
 * @brief Функция извлечения канонического класса сочетания символа
 *
 * @param code кодовое значение символа
 * @return     канонический класс сочетания символа
 *
 */
uint8_t awh::unicode::combining(const uint32_t code) noexcept {
	// Получаем наименьший индекс просматриваемого участка таблицы
	size_t lower = 0;
	// Получаем наибольший индекс просматриваемого участка таблицы
	size_t upper = COMBINING_COUNT;
	/**
	 * Выполняем двоичный поиск диапазона в таблице классов сочетания
	 */
	while(lower < upper) {
		// Получаем индекс середины просматриваемого участка
		const size_t middle = (lower + ((upper - lower) / 2));
		/**
		 * Если кодовое значение находится левее диапазона
		 */
		if(code < COMBINING[middle].begin)
			// Продолжаем поиск в левой половине участка
			upper = middle;
		/**
		 * Если кодовое значение находится правее диапазона
		 */
		else if(code > COMBINING[middle].end)
			// Продолжаем поиск в правой половине участка
			lower = (middle + 1);
		// Выводим канонический класс сочетания символа
		else return static_cast <uint8_t> (COMBINING[middle].value);
	}
	// Выводим класс сочетания начального символа
	return 0;
}
/**
 * @brief Функция разложения символа набором кодовых значений
 *
 * @param code   кодовое значение разлагаемого символа
 * @param compat признак применения разложений совместимости
 * @param result набор кодовых значений разложения символа
 *
 */
void awh::unicode::decompose(const uint32_t code, const bool compat, vector <uint32_t> & result) noexcept {
	/**
	 * Если разлагаемый символ является слогом хангыля
	 *
	 * @details Слоги хангыля разлагаются вычислением, что предписано приложением
	 *          по нормализации стандарта Юникода.
	 *
	 */
	if((code >= HANGUL_BASE) && (code < (HANGUL_BASE + HANGUL_COUNT))) {
		// Получаем порядковый номер слога хангыля
		const uint32_t index = (code - HANGUL_BASE);
		// Выполняем добавление начальной части слога хангыля
		result.push_back(LEADING_BASE + (index / VOWEL_SPAN));
		// Выполняем добавление гласной части слога хангыля
		result.push_back(VOWEL_BASE + ((index % VOWEL_SPAN) / TRAILING_COUNT));
		// Получаем номер конечной части слога хангыля
		const uint32_t trailing = (index % TRAILING_COUNT);
		/**
		 * Если слог хангыля содержит конечную часть
		 */
		if(trailing != 0)
			// Выполняем добавление конечной части слога хангыля
			result.push_back(TRAILING_BASE + trailing);
		// Выходим из функции разложения символа
		return;
	}
	// Выполняем поиск разложения символа в таблице разложений
	const decomposition_t * item = search(code);
	/**
	 * Если разложение символа не обнаружено либо разложение совместимости не применяется
	 */
	if((item == nullptr) || (item->compat && !compat)) {
		// Выполняем добавление символа без изменений
		result.push_back(code);
		// Выходим из функции разложения символа
		return;
	}
	/**
	 * Выполняем разложение символов обнаруженного разложения
	 *
	 * @details Разложение выполняется полностью: символы разложения, разлагаемые
	 *          в свою очередь, разлагаются до неразложимых.
	 *
	 */
	for(size_t i = 0; i < item->length; i++)
		// Выполняем разложение очередного символа разложения
		decompose(DECOMPOSITION_SETS[item->offset + i], compat, result);
}
/**
 * @brief Функция канонического сочетания пары символов
 *
 * @param first  кодовое значение начального символа пары
 * @param second кодовое значение сочетающегося символа пары
 * @return       кодовое значение получившегося символа либо нулевое значение
 *
 */
uint32_t awh::unicode::compose(const uint32_t first, const uint32_t second) noexcept {
	/**
	 * Если пара образует слог хангыля из начальной и гласной частей
	 */
	if((first >= LEADING_BASE) && (first < (LEADING_BASE + LEADING_COUNT)) &&
	 (second >= VOWEL_BASE) && (second < (VOWEL_BASE + VOWEL_COUNT)))
		// Выводим вычисленное кодовое значение слога хангыля
		return (HANGUL_BASE + (((first - LEADING_BASE) * VOWEL_COUNT + (second - VOWEL_BASE)) * TRAILING_COUNT));
	/**
	 * Если пара образует слог хангыля присоединением конечной части
	 */
	if((first >= HANGUL_BASE) && (first < (HANGUL_BASE + HANGUL_COUNT)) &&
	 (second > TRAILING_BASE) && (second < (TRAILING_BASE + TRAILING_COUNT)) &&
	 (((first - HANGUL_BASE) % TRAILING_COUNT) == 0))
		// Выводим вычисленное кодовое значение слога хангыля
		return (first + (second - TRAILING_BASE));
	// Получаем наименьший индекс просматриваемого участка таблицы
	size_t lower = 0;
	// Получаем наибольший индекс просматриваемого участка таблицы
	size_t upper = COMPOSITIONS_COUNT;
	/**
	 * Выполняем двоичный поиск сочетания пары символов в таблице
	 */
	while(lower < upper) {
		// Получаем индекс середины просматриваемого участка
		const size_t middle = (lower + ((upper - lower) / 2));
		/**
		 * Если искомая пара предшествует паре таблицы
		 */
		if((first < COMPOSITIONS[middle].first) ||
		 ((first == COMPOSITIONS[middle].first) && (second < COMPOSITIONS[middle].second)))
			// Продолжаем поиск в левой половине участка
			upper = middle;
		/**
		 * Если искомая пара следует за парой таблицы
		 */
		else if((first > COMPOSITIONS[middle].first) ||
		 ((first == COMPOSITIONS[middle].first) && (second > COMPOSITIONS[middle].second)))
			// Продолжаем поиск в правой половине участка
			lower = (middle + 1);
		// Выводим кодовое значение получившегося символа
		else return COMPOSITIONS[middle].code;
	}
	// Выводим отсутствие сочетания пары символов
	return 0;
}
/**
 * @brief Функция приведения текста к нормальному представлению
 *
 * @param text   набор кодовых значений приводимого текста
 * @param form   вид нормального представления текста
 * @param result набор кодовых значений приведённого текста
 *
 */
void awh::unicode::normalize(const vector <uint32_t> & text, const form_t form, vector <uint32_t> & result) noexcept {
	// Выполняем очистку набора кодовых значений приведённого текста
	result.clear();
	/**
	 * Если приводить оказалось нечего
	 */
	if(text.empty())
		// Выходим из функции приведения текста
		return;
	// Получаем признак применения разложений совместимости
	const bool compat = ((form == form_t::NFKD) || (form == form_t::NFKC));
	// Выполняем предварительное выделение памяти под приведённый текст
	result.reserve(text.size());
	/**
	 * Выполняем полное разложение символов приводимого текста
	 */
	for(auto & code : text)
		// Выполняем разложение очередного символа текста
		decompose(code, compat, result);
	/**
	 * Выполняем каноническое упорядочение сочетающихся знаков
	 *
	 * @details Упорядочение выполняется устойчивой пузырьковой сортировкой по
	 *          каноническому классу сочетания: знаки одного класса обязаны сохранить
	 *          взаимный порядок, а перестановке подлежат лишь соседние знаки.
	 *
	 */
	for(size_t i = 1; i < result.size(); i++) {
		// Получаем канонический класс сочетания очередного символа
		const uint8_t current = combining(result.at(i));
		/**
		 * Если очередной символ является начальным символом
		 */
		if(current == 0)
			// Переходим к следующему символу текста
			continue;
		/**
		 * Выполняем перемещение знака к его месту в порядке классов сочетания
		 */
		for(size_t j = i; j > 0; j--) {
			// Получаем канонический класс сочетания предшествующего символа
			const uint8_t previous = combining(result.at(j - 1));
			/**
			 * Если предшествующий символ занимает своё место
			 */
			if((previous == 0) || (previous <= current))
				// Прекращаем перемещение знака
				break;
			// Выполняем перестановку знака с предшествующим символом
			swap(result.at(j), result.at(j - 1));
		}
	}
	/**
	 * Если сочетание символов не требуется
	 */
	if((form == form_t::NFD) || (form == form_t::NFKD))
		// Выходим из функции приведения текста
		return;
	/**
	 * Выполняем каноническое сочетание символов приведённого текста
	 */
	// Индекс начального символа, к которому присоединяются сочетающиеся знаки
	size_t starter = 0;
	// Индекс, по которому записывается очередной символ приведённого текста
	size_t position = 1;
	/**
	 * Канонический класс сочетания предшествующего рассмотренного символа
	 *
	 * @details Текст, начинающийся не с начального символа, а со знака, сочетанию
	 *          не подлежит до появления начального символа: заслоняющий класс задаётся
	 *          значением, превышающим любой класс сочетания.
	 *
	 */
	int32_t last = static_cast <int32_t> (combining(result.at(0)));
	// Если текст начинается со знака, а не с начального символа
	if(last != 0)
		// Устанавливаем заслоняющий класс сочетания
		last = 256;
	/**
	 * Выполняем обход символов приведённого текста
	 */
	for(size_t i = 1; i < result.size(); i++) {
		// Получаем кодовое значение очередного символа текста
		const uint32_t code = result.at(i);
		// Получаем канонический класс сочетания очередного символа
		const int32_t current = static_cast <int32_t> (combining(code));
		/**
		 * Если очередной символ сочетается с начальным символом
		 *
		 * @details Сочетание допускается лишь тогда, когда очередной символ не заслонён
		 *          предшествующим знаком: класс сочетания обязан возрасти, а начальный
		 *          символ — следовать непосредственно перед знаком.
		 *
		 */
		if((last < current) || ((last == 0) && (current == 0))) {
			// Выполняем сочетание начального символа с очередным символом
			const uint32_t composed = compose(result.at(starter), code);
			/**
			 * Если сочетание пары символов выполнено
			 */
			if(composed != 0) {
				// Выполняем замену начального символа получившимся символом
				result.at(starter) = composed;
				// Переходим к следующему символу текста
				continue;
			}
		}
		/**
		 * Если очередной символ является начальным символом
		 */
		if(current == 0)
			// Выполняем запоминание положения начального символа
			starter = position;
		// Выполняем запоминание класса сочетания рассмотренного символа
		last = current;
		// Выполняем запись очередного символа приведённого текста
		result.at(position++) = code;
	}
	// Выполняем усечение приведённого текста до записанной длины
	result.resize(position);
}
