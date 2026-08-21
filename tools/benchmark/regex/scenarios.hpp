/**
 * @file scenarios.hpp
 * @date 2026-07-31
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
 * @brief Общие данные стендов сравнения модуля регулярных выражений
 *
 * @details Выражения, тексты и количества повторений вынесены сюда, чтобы у обеих
 *          сравниваемых реализаций они были буквально одни и те же. Стенд,
 *          заводящий их у себя, рано или поздно разойдётся с соседним - и
 *          сравнение начнёт мерить разницу образцов, а не реализаций
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_REGEX_SCENARIOS__
#define __AWH_BENCHMARK_REGEX_SCENARIOS__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstddef>

/**
 * @brief Инкапсулируем общие данные стендов в пространство имён
 *
 */
namespace scenarios {
	/**
	 * @brief Количество повторений сценариев на коротком тексте
	 *
	 * @details Одно сопоставление обходится в сотни наносекунд, и на меньшем
	 *          количестве повторений замер измерял бы разрешение часов
	 *
	 */
	static constexpr size_t SHORT_ROUNDS = 200000;

	/**
	 * @brief Количество повторений сценариев на длинном тексте
	 *
	 * @details Проход длинного текста на три порядка дороже прохода короткого,
	 *          поэтому количество повторений уменьшено соразмерно
	 *
	 */
	static constexpr size_t LONG_ROUNDS = 2000;

	/**
	 * @brief Количество повторений сценариев на тексте среднем
	 *
	 */
	static constexpr size_t MEDIUM_ROUNDS = 50000;

	/**
	 * @brief Количество повторений сценариев исполнения с возвратом
	 *
	 * @details Исполнение с возвратом обходится дороже прочих способов
	 *          и на общем количестве повторений затянуло бы прогон
	 *
	 */
	static constexpr size_t HEAVY_ROUNDS = 20000;

	/**
	 * @brief Количество повторений прогрева сценариев
	 *
	 * @details Первый прогон после сборки систематически ниже остальных:
	 *          распределитель памяти выходит на рабочий объём, а предсказатель
	 *          переходов - на установившийся режим
	 *
	 */
	static constexpr size_t WARMUP = 2000;

	/**
	 * @brief Длина длинного текста сопоставления в октетах
	 *
	 */
	static constexpr size_t LONG_LENGTH = 262144;

	/**
	 * @brief Длина текста среднего в октетах
	 *
	 */
	static constexpr size_t MEDIUM_LENGTH = 2048;

	/**
	 * @brief Вид сценария сравнения
	 *
	 */
	enum class kind_t : uint8_t {
		SHORT  = 0x00, // Сопоставление на коротком тексте
		LONG   = 0x01, // Сопоставление на длинном тексте
		HEAVY  = 0x02, // Сопоставление исполнением с возвратом
		MEDIUM = 0x03  // Сопоставление на тексте среднего размера
	};

	/**
	 * @brief Сценарий сравнения реализаций
	 *
	 */
	typedef struct Scenario {
		// Название сценария сравнения
		const char * name;
		// Текст регулярного выражения
		const char * pattern;
		// Вид сценария сравнения
		kind_t kind;
		// Признак наличия совпадения в тексте сопоставления
		bool matches;
	} scenario_t;

	/**
	 * @brief Набор сценариев сравнения реализаций
	 *
	 * @details Набор покрывает способы исполнения выражения по отдельности:
	 *          отбор по обязательному литералу, детерминированное исполнение,
	 *          исполнение без возврата с захватом групп и исполнение с возвратом.
	 *          Сценарии с отсутствующим совпадением отделены намеренно: проход
	 *          текста целиком стоит иного, нежели прекращение на первом совпадении
	 *
	 */
	static const std::vector <scenario_t> SCENARIOS = {
		// Отбор позиций сопоставления по обязательному литералу
		{"literal-short",      "Content-Length",                          kind_t::SHORT, true},
		{"literal-long",       "needle-in-haystack",                      kind_t::LONG,  true},
		{"literal-absent",     "no-such-sequence-here",                   kind_t::LONG,  false},
		// Классы символов и кванторы повторения
		{"digits-short",       "[0-9]{3,5}",                              kind_t::SHORT, true},
		{"digits-long",        "[0-9]{3,5}",                              kind_t::LONG,  true},
		{"word-long",          "\\w+@\\w+\\.\\w+",                        kind_t::LONG,  true},
		{"dotstar-long",       ".*needle",                                kind_t::LONG,  true},
		// Выбор одной из ветвей
		{"alternate-short",    "GET|POST|PUT|DELETE|HEAD|OPTIONS",        kind_t::SHORT, true},
		{"alternate-long",     "alpha|bravo|charlie|delta|echo|foxtrot",  kind_t::LONG,  true},
		// Привязки к позиции в тексте
		{"anchored-short",     "^GET /\\S+ HTTP/1\\.1",                    kind_t::SHORT, true},
		{"boundary-long",      "\\bneedle\\b",                            kind_t::LONG,  true},
		// Захват групп
		{"captures-short",     "([A-Za-z0-9-]+): (.+)",                   kind_t::SHORT, true},
		{"captures-long",      "(\\w+)@(\\w+)\\.(\\w+)",                  kind_t::LONG,  true},
		// Разбор, встречающийся при работе с протоколами
		{"request-short",      "^(GET|POST) (\\S+) HTTP/(\\d)\\.(\\d)",   kind_t::SHORT, true},
		{"address-short",      "(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})", kind_t::SHORT, true},
		// Исполнение с возвратом
		{"backref-heavy",      "(\\w+) \\1",                              kind_t::HEAVY, true},
		{"lookahead-heavy",    "\\w+(?=@)",                               kind_t::HEAVY, true},
		{"lookbehind-heavy",   "(?<=@)\\w+",                              kind_t::HEAVY, true},
		{"atomic-heavy",       "(?>\\w+)@\\w+",                           kind_t::HEAVY, true},
		{"recurse-heavy",      "\\((?:[^()]|(?R))*\\)",                   kind_t::HEAVY, true},
		/**
		 * Повторение над областью инструкций, ленивое и жадное
		 *
		 * @details Строки эти набором замеров покрыты, а отдельным стендом
		 *          не были: он отстал на пятнадцать строк, отчего замеры
		 *          по стендам шли подмножеством устаревшим. Пороги набора
		 *          калибруются по стенду самому медленному, и калибровать
		 *          их без этих строк было нечем.
		 *
		 */
		{"anchored-absent",       "^[A-Za-z0-9-]+: .+$",                         kind_t::SHORT, false},
		{"request-absent",        "^(GET|POST) (\\S+) HTTP/(\\d)\\.(\\d)$",      kind_t::SHORT, false},
		{"address-absent",        "^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$", kind_t::SHORT, false},
		{"lazy-short",            "\\w+?@\\w+?\\.",                              kind_t::SHORT, true},
		{"lazy-long",             "\\w+?@\\w+?\\.",                              kind_t::LONG,  true},
		{"lazy-dotstar",          ".*?needle",                                   kind_t::LONG,  true},
		{"region-fixed-short",    "(?:HT|TP)+/",                                 kind_t::SHORT, true},
		{"region-absent-short",   "(?:ab|cd)+z",                                 kind_t::SHORT, false},
		{"region-fixed-long",     "(?:\\w\\w)+ 4096",                            kind_t::LONG,  true},
		{"region-varied-long",    "(?:[a-z]+ )+dog",                             kind_t::LONG,  true},
		{"region-nested-heavy",   "\\((?:[^()]|\\([^()]*\\))*\\)",               kind_t::HEAVY, true},
		{"region-capture-heavy",  "(?:(\\w+) )+forman",                          kind_t::HEAVY, true},
		/**
		 * Отбор позиций начала попытки по обязательному литералу
		 *
		 * @details Выражения дают удаление литерала малое и ограниченное:
		 *          ветви выбора постоянной длины стоят перед литералом,
		 *          а повторения перед ним нет вовсе. Прочие строки набора
		 *          удаления ограниченного не дают ни одна.
		 *
		 */
		{"bounded-short",         "(?:HT|TP)/1",                                kind_t::SHORT, true},
		{"bounded-long",          "(?:fox|dog)trot",                            kind_t::LONG,  true},
		{"bounded-absent",        "(?:fox|dog)trap",                            kind_t::LONG,  false},
		/**
		 * Сопоставление на тексте среднего размера
		 *
		 */
		{"literal-medium",        "needle-in-haystack",                         kind_t::MEDIUM, true},
		{"alternate-medium",      "alpha|bravo|charlie|delta|echo|foxtrot",     kind_t::MEDIUM, true},
		{"captures-medium",       "(\\w+)@(\\w+)\\.(\\w+)",                     kind_t::MEDIUM, true},
		{"bounded-medium",        "(?:fox|dog)trot",                            kind_t::MEDIUM, true},
		{"region-medium",         "(?:[a-z]+/)+v1",                             kind_t::MEDIUM, true},
		{"region-lazy-short",     "(?:HT|TP)+?/",                                kind_t::SHORT, true},
		{"region-lazy-long",      "(?:[a-z]+ )+?dog",                            kind_t::LONG,  true},
		/**
		 * Повторения, тело каких пусто совпасть способно
		 *
		 * @details Тело такое ведётся сторожем: положение начала прохода
		 *          кладётся в ячейку состояния, а проверка продвижения
		 *          повторение завершает, когда проход по тексту не двинулся.
		 *          Ячейки состояния размещаются за ячейками захвата, и проверка
		 *          применимости прежде принимала их за выход за пределы набора
		 *          границ - отчего порождение кода отменялось у всякого такого
		 *          выражения. Набор устройства этого не мерил вовсе, оттого
		 *          изъян и жил незамеченным.
		 *
		 */
		{"region-empty-long",     "(?:[a-z]* ?)*dog",                            kind_t::LONG,  true},
		{"region-empty-lazy",     "(?:[a-z]* ?)*?dog",                           kind_t::LONG,  true},
		{"region-lazy-heavy",     "\\((?:[^()]|\\([^()]*\\))*?\\)",              kind_t::HEAVY, true},
	};

	/**
	 * @brief Функция получения короткого текста сопоставления
	 *
	 * @return короткий текст сопоставления
	 *
	 */
	static inline const std::string & shortText() noexcept {
		// Короткий текст сопоставления, снятый с обычного обмена по протоколу HTTP
		static const std::string result(
			"GET /index.html HTTP/1.1\r\nHost: 192.168.001.100\r\n"
			"Content-Length: 4096\r\nUser-Agent: forman@anyks.com\r\n"
		);
		// Выводим короткий текст сопоставления
		return result;
	}
	/**
	 * @brief Функция получения длинного текста сопоставления
	 *
	 * @details Искомые последовательности размещены у конца текста: реализация,
	 *          прекращающая проход на первом совпадении, обязана пройти текст
	 *          почти целиком, и отбор позиций сопоставления работы не лишается
	 *
	 * @return длинный текст сопоставления
	 *
	 */
	static inline const std::string & longText() noexcept {
		// Длинный текст сопоставления
		static const std::string result = []() noexcept -> std::string {
			// Создаём основу длинного текста сопоставления
			std::string outcome;
			// Выполняем размещение длинного текста сопоставления
			outcome.reserve(LONG_LENGTH + 128);
			/**
			 * Выполняем наполнение текста до заданной длины
			 */
			while(outcome.size() < LONG_LENGTH)
				// Выполняем добавление очередного участка текста
				outcome.append("the quick brown fox jumps over the lazy dog 1234 ");
			// Выполняем добавление искомых последовательностей у конца текста
			outcome.append("needle-in-haystack foxtrot forman@anyks.com needle 4096 ");
			// Выводим длинный текст сопоставления
			return outcome;
		}();
		// Выводим длинный текст сопоставления
		return result;
	}
	/**
	 * @brief Функция получения текста сопоставления среднего размера
	 *
	 * @details Текст сложен обменом по протоколу HTTP, а не связной прозой:
	 *          доля байтов, совпадению начало дающих, у прозы и у протокола
	 *          разная, и отбор позиций на них ведёт себя по-разному
	 *
	 * @return текст сопоставления среднего размера
	 *
	 */
	static inline const std::string & mediumText() noexcept {
		// Текст сопоставления среднего размера
		static const std::string result = []() noexcept -> std::string {
			// Создаём основу текста сопоставления
			std::string outcome;
			// Выполняем размещение текста сопоставления
			outcome.reserve(MEDIUM_LENGTH + 128);
			// Номер порождаемого обмена по протоколу
			size_t number = 0;
			/**
			 * Выполняем наполнение текста до заданной длины
			 */
			while(outcome.size() < MEDIUM_LENGTH) {
				// Выполняем добавление строки запроса обмена по протоколу
				outcome.append("GET /api/v1/items/");
				// Выполняем добавление номера запрашиваемого ресурса
				outcome.append(std::to_string(number++));
				// Выполняем добавление заголовков обмена по протоколу
				outcome.append(" HTTP/1.1\r\nHost: node-07.anyks.com\r\n"
				 "User-Agent: awh/5.0\r\nAccept: application/json\r\n"
				 "X-Request-Id: 7f3a9c2e-41bd-4e88-9a1f-0c5d6e8b2a34\r\n"
				 "Content-Length: 512\r\n\r\n");
			}
			// Выполняем добавление искомых последовательностей у конца текста
			outcome.append("needle-in-haystack foxtrot forman@anyks.com needle 4096 ");
			// Выводим текст сопоставления среднего размера
			return outcome;
		}();
		// Выводим текст сопоставления среднего размера
		return result;
	}
	/**
	 * @brief Функция получения текста сопоставления исполнения с возвратом
	 *
	 * @return текст сопоставления исполнения с возвратом
	 *
	 */
	static inline const std::string & heavyText() noexcept {
		// Текст сопоставления исполнения с возвратом
		static const std::string result(
			"lorem ipsum dolor sit amet consectetur adipiscing elit sed do "
			"eiusmod tempor incididunt ut labore et dolore magna aliqua (nested "
			"(parenthesis (here)) done) repeat repeat forman@anyks.com tail"
		);
		// Выводим текст сопоставления исполнения с возвратом
		return result;
	}
	/**
	 * @brief Функция получения текста сопоставления сценария
	 *
	 * @param kind вид сценария сравнения
	 * @return     текст сопоставления сценария
	 *
	 */
	static inline const std::string & text(const kind_t kind) noexcept {
		/**
		 * Определяем вид сценария сравнения
		 */
		switch(static_cast <uint8_t> (kind)) {
			// Выводим длинный текст сопоставления
			case static_cast <uint8_t> (kind_t::LONG): return longText();
			// Выводим текст сопоставления исполнения с возвратом
			case static_cast <uint8_t> (kind_t::HEAVY): return heavyText();
			// Выводим текст сопоставления среднего размера
			case static_cast <uint8_t> (kind_t::MEDIUM): return mediumText();
		}
		// Выводим короткий текст сопоставления
		return shortText();
	}
	/**
	 * @brief Функция получения количества повторений сценария
	 *
	 * @param kind вид сценария сравнения
	 * @return     количество повторений сценария
	 *
	 */
	static inline size_t rounds(const kind_t kind) noexcept {
		/**
		 * Определяем вид сценария сравнения
		 */
		switch(static_cast <uint8_t> (kind)) {
			// Выводим количество повторений сценария на длинном тексте
			case static_cast <uint8_t> (kind_t::LONG): return LONG_ROUNDS;
			// Выводим количество повторений сценария исполнения с возвратом
			case static_cast <uint8_t> (kind_t::HEAVY): return HEAVY_ROUNDS;
			// Выводим количество повторений сценария на тексте среднем
			case static_cast <uint8_t> (kind_t::MEDIUM): return MEDIUM_ROUNDS;
		}
		// Выводим количество повторений сценария на коротком тексте
		return SHORT_ROUNDS;
	}
};

#endif // __AWH_BENCHMARK_REGEX_SCENARIOS__
