/**
 * @file matching.hpp
 * @date 2026-08-02
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
 * @brief Заголовочный файл бенчмарков сопоставления регулярных выражений — выражения,
 *        тексты сопоставления и количества повторений сценариев сравнения
 *
 * @details Выражения, тексты и количества повторений вынесены сюда, чтобы у обеих
 *          сравниваемых реализаций они были буквально одни и те же. Стенд,
 *          заводящий их у себя, рано или поздно разойдётся с соседним - и
 *          сравнение начнёт мерить разницу образцов, а не реализаций
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_REGEX_MATCHING__
#define __AWH_BENCHMARK_REGEX_MATCHING__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/regex/regex.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён бенчмарков
	 *
	 */
	namespace benchmark {
		/**
		 * @brief Пространство имён бенчмарков сопоставления регулярных выражений
		 *
		 */
		namespace matching {
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
				 * @brief Количество проходов набора повторений сценария
				 *
				 * @details Из нескольких проходов принимается наименьшее время: помехи
				 *          окружения время лишь увеличивают, поэтому наименьшее из
				 *          измеренных ближе прочих к времени самой операции
				 *
				 */
				static constexpr size_t ATTEMPTS = 5;

				/**
				 * @brief Длина длинного текста сопоставления в октетах
				 *
				 */
				static constexpr size_t LONG_LENGTH = 262144;

				/**
				 * @brief Вид сценария сравнения
				 *
				 */
				enum class kind_t : uint8_t {
					SHORT = 0x00, // Сопоставление на коротком тексте
					LONG  = 0x01, // Сопоставление на длинном тексте
					HEAVY = 0x02  // Сопоставление исполнением с возвратом
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
					/**
					 * Пороговая пропускная способность сценария в совпадениях за секунду
					 *
					 * @details Порог ловит утрату способа исполнения целиком, а не колебания
					 *          окружения. Способ исполнения у каждого сценария свой, поэтому
					 *          и порог у каждого свой.
					 *
					 *          Калибруется порог **по самому медленному стенду**, а не по
					 *          рабочей машине, с запасом в полтора раза. Прежде он брался
					 *          от рабочей машины с запасом вчетверо, и правило это оказалось
					 *          негодным: стенды идут медленнее неравномерно - от 1.26 раза
					 *          у сценария одного до 14 раз у другого, - отчего единая доля
					 *          от рабочей машины давала одним сценариям запас излишний,
					 *          а другим не давала никакого. Набор, на стенде запущенный,
					 *          отчитывался отказом по половине сценариев, и отказ этот был
					 *          ложным: медленнее была машина, а не код.
					 *
					 *          Снято 17.08.2026 щупом, собираемым теми же исходными текстами,
					 *          что и переносимая проверка, на трёх стендах x86-64:
					 *          FreeBSD, OpenBSD и NetBSD. Пороги 26 сценариев прежних
					 *          пересчитаны тем же правилом заодно с шестью новыми
					 *
					 */
					double threshold;
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
					{"literal-short",      "Content-Length",                          kind_t::SHORT, true, 8200000.0},
					{"literal-long",       "needle-in-haystack",                      kind_t::LONG,  true, 3900.0},
					{"literal-absent",     "no-such-sequence-here",                   kind_t::LONG,  false, 3900.0},
					// Классы символов и кванторы повторения
					{"digits-short",       "[0-9]{3,5}",                              kind_t::SHORT, true, 1300000.0},
					{"digits-long",        "[0-9]{3,5}",                              kind_t::LONG,  true, 1800000.0},
					{"word-long",          "\\w+@\\w+\\.\\w+",                        kind_t::LONG,  true, 240.0},
					{"dotstar-long",       ".*needle",                                kind_t::LONG,  true, 2800.0},
					// Выбор одной из ветвей
					{"alternate-short",    "GET|POST|PUT|DELETE|HEAD|OPTIONS",        kind_t::SHORT, true, 3000000.0},
					{"alternate-long",     "alpha|bravo|charlie|delta|echo|foxtrot",  kind_t::LONG,  true, 290.0},
					// Привязки к позиции в тексте
					{"anchored-short",     "(?m)^[A-Za-z0-9-]+: .+$",                 kind_t::SHORT, true, 250000.0},
					{"anchored-absent",    "^[A-Za-z0-9-]+: .+$",                     kind_t::SHORT, false, 6300000.0},
					{"boundary-long",      "\\bneedle\\b",                            kind_t::LONG,  true, 6600.0},
					// Захват групп
					{"captures-short",     "([A-Za-z0-9-]+): (.+)",                   kind_t::SHORT, true, 160000.0},
					{"captures-long",      "(\\w+)@(\\w+)\\.(\\w+)",                  kind_t::LONG,  true, 240.0},
					// Разбор, встречающийся при работе с протоколами
					{"request-short",      "(?m)^(GET|POST) (\\S+) HTTP/(\\d)\\.(\\d)\\r?$", kind_t::SHORT, true, 980000.0},
					{"request-absent",     "^(GET|POST) (\\S+) HTTP/(\\d)\\.(\\d)$",  kind_t::SHORT, false, 3400000.0},
					{"address-short",      "(?m)^Host: (\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\r?$", kind_t::SHORT, true, 950000.0},
					{"address-absent",     "^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$", kind_t::SHORT, false, 5900000.0},
					// Ленивые повторения
					{"lazy-short",         "\\w+?@\\w+?\\.",                        kind_t::SHORT, true, 44000.0},
					{"lazy-long",          "\\w+?@\\w+?\\.",                        kind_t::LONG,  true, 240.0},
					{"lazy-dotstar",       ".*?needle",                               kind_t::LONG,  true, 70.0},
					/**
					 * Повторение над областью инструкций
					 *
					 * @details Повторение над областью ведётся двумя способами, и сценарии
					 *          покрывают оба: тело постоянной длины без записи границ групп
					 *          обходится местами кадра, тогда как тело переменной длины
					 *          либо границы пишущее получает запись кадра на каждый проход.
					 *          Отделён намеренно сценарий без совпадения: повторение,
					 *          отказывающее быстро, платит за подготовку прохода, ничего
					 *          ею не выигрывая, и остаётся местом самым слабым.
					 *
					 */
					{"region-fixed-short", "(?:HT|TP)+/",                             kind_t::SHORT, true, 1300000.0},
					{"region-absent-short","(?:ab|cd)+z",                             kind_t::SHORT, false, 6600000.0},
					{"region-fixed-long",  "(?:\\w\\w)+ 4096",                        kind_t::LONG,  true, 290.0},
					{"region-varied-long", "(?:[a-z]+ )+dog",                         kind_t::LONG,  true, 100000.0},
					{"region-nested-heavy","\\((?:[^()]|\\([^()]*\\))*\\)",             kind_t::HEAVY, true, 420000.0},
					{"region-capture-heavy","(?:(\\w+) )+forman",                     kind_t::HEAVY, true, 1300.0},
					/**
					 * Устройства, порождению машинного кода поддавшиеся не сразу
					 *
					 * @details Атомарные группы и обе проверки окружения порождению
					 *          поддались: набора точек возврата произвольной глубины
					 *          не требует ни одна из них. Обратные ссылки и рекурсия
					 *          исполняются разбором программы с возвратом.
					 *
					 */
					{"backref-heavy",      "(\\w+) \\1",                              kind_t::HEAVY, true, 56000.0},
					{"lookahead-heavy",    "\\w+(?=@)",                               kind_t::HEAVY, true, 24000.0},
					{"lookbehind-heavy",   "(?<=@)\\w+",                              kind_t::HEAVY, true, 61000.0},
					{"atomic-heavy",       "(?>\\w+)@\\w+",                           kind_t::HEAVY, true, 18000.0},
					{"recurse-heavy",      "\\((?:[^()]|(?R))*\\)",                   kind_t::HEAVY, true, 280000.0}
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
					}
					// Выводим количество повторений сценария на коротком тексте
					return SHORT_ROUNDS;
				}
		};
	};
};

#endif // __AWH_BENCHMARK_REGEX_MATCHING__
