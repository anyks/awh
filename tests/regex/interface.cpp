/**
 * @file interface.cpp
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
 * @brief Автоматические тесты открытого интерфейса модуля регулярных выражений —
 *        сборка выражений с кэшем собранного, сопоставление выражения с текстом,
 *        извлечение захваченных групп по номеру и по имени и разделение выражения
 *        несколькими потоками исполнения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <thread>
#include <utility>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../main.hpp"
#include <sys/log.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в
 *       одно, порождая порчу вдали от места её причины
 *
 */
namespace {
	/**
	 * @brief Объект журнала проверок с отключённым выводом
	 *
	 * @details Вывод отключается назначением пустого перечня приёмников: отказы
	 *          разбора проверки наводят намеренно, и журнал их засорял бы выдачу
	 *
	 */
	struct Silent {
		/**
		 * @brief Функция получения объекта фреймворка проверок
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          проверки, ибо фреймворк сам опирается на статику из библиотеки
		 *
		 * @return объект фреймворка проверок
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка проверок
			static awh::fmk_t fmk;
			// Выводим объект фреймворка проверок
			return fmk;
		}
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта журнала проверок
	 *
	 * @return объект журнала проверок
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект журнала проверок
		static Silent silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}
}

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Проверка сборки и сопоставления регулярного выражения
 *
 */
TEST(Regex, Interface) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("(\\w+)@(\\w+)\\.([a-z]{2,})");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Выполняем проверку отсутствия ошибки сборки регулярного выражения
	EXPECT_EQ(regexp.error(), regex::error_t::NONE);
	// Выполняем проверку количества захватывающих групп выражения
	EXPECT_EQ(regexp.captures(expression), static_cast <uint32_t> (3));
	// Выполняем проверку наличия совпадения в тексте
	EXPECT_TRUE(regexp.test("почта forman@anyks.com здесь", expression));
	// Выполняем проверку отсутствия совпадения в тексте
	EXPECT_FALSE(regexp.test("почты здесь нет", expression));
	// Получаем текст совпадения и захваченных групп
	const auto & result = regexp.exec("почта forman@anyks.com здесь", expression);
	// Выполняем проверку количества извлечённых значений
	ASSERT_EQ(result.size(), static_cast <size_t> (4));
	// Выполняем проверку текста совпадения целиком
	EXPECT_EQ(result.at(0), "forman@anyks.com");
	// Выполняем проверку текста первой захватывающей группы
	EXPECT_EQ(result.at(1), "forman");
	// Выполняем проверку текста второй захватывающей группы
	EXPECT_EQ(result.at(2), "anyks");
	// Выполняем проверку текста третьей захватывающей группы
	EXPECT_EQ(result.at(3), "com");
}

/**
 * @brief Проверка извлечения границ совпадения и захваченных групп
 *
 */
TEST(Regex, InterfaceBounds) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("a(b)?(c)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Получаем границы совпадения и захваченных групп
	const auto & bounds = regexp.match("xxac", expression);
	// Выполняем проверку количества извлечённых границ
	ASSERT_EQ(bounds.size(), static_cast <size_t> (3));
	// Выполняем проверку начальной границы совпадения
	EXPECT_EQ(bounds.at(0).first, static_cast <size_t> (2));
	// Выполняем проверку конечной границы совпадения
	EXPECT_EQ(bounds.at(0).second, static_cast <size_t> (4));
	/**
	 * Выполняем проверку невыполненного захвата первой группой
	 *
	 * @details Границы невыполненного захвата равны позиции отсутствующего символа,
	 *          что отличает невыполненный захват от захвата пустого текста.
	 *
	 */
	EXPECT_EQ(bounds.at(1).first, string_view::npos);
	// Выполняем проверку конечной границы невыполненного захвата
	EXPECT_EQ(bounds.at(1).second, string_view::npos);
	// Выполняем проверку начальной границы захвата второй группой
	EXPECT_EQ(bounds.at(2).first, static_cast <size_t> (3));
	// Выполняем проверку конечной границы захвата второй группой
	EXPECT_EQ(bounds.at(2).second, static_cast <size_t> (4));
	// Получаем текст совпадения и захваченных групп
	const auto & result = regexp.exec("xxac", expression);
	// Выполняем проверку количества извлечённых значений
	ASSERT_EQ(result.size(), static_cast <size_t> (3));
	// Выполняем проверку пустого текста невыполненного захвата
	EXPECT_TRUE(result.at(1).empty());
	// Выполняем проверку текста захвата второй группой
	EXPECT_EQ(result.at(2), "c");
}

/**
 * @brief Проверка извлечения номера именованной группы
 *
 */
TEST(Regex, InterfaceNames) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("(?<year>\\d{4})-(?<month>\\d{2})-(?<day>\\d{2})");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Выполняем проверку номера именованной группы года
	EXPECT_EQ(regexp.group(expression, "year"), static_cast <uint32_t> (1));
	// Выполняем проверку номера именованной группы месяца
	EXPECT_EQ(regexp.group(expression, "month"), static_cast <uint32_t> (2));
	// Выполняем проверку номера именованной группы дня
	EXPECT_EQ(regexp.group(expression, "day"), static_cast <uint32_t> (3));
	// Выполняем проверку отсутствия неизвестной именованной группы
	EXPECT_EQ(regexp.group(expression, "hour"), static_cast <uint32_t> (0));
	// Получаем текст совпадения и захваченных групп
	const auto & result = regexp.exec("дата 2026-07-31 здесь", expression);
	// Выполняем проверку количества извлечённых значений
	ASSERT_EQ(result.size(), static_cast <size_t> (4));
	// Выполняем проверку текста именованной группы года
	EXPECT_EQ(result.at(regexp.group(expression, "year")), "2026");
	// Выполняем проверку текста именованной группы месяца
	EXPECT_EQ(result.at(regexp.group(expression, "month")), "07");
	// Выполняем проверку текста именованной группы дня
	EXPECT_EQ(result.at(regexp.group(expression, "day")), "31");
}

/**
 * @brief Проверка глаголов управления возвратом
 *
 * @details Разряд четвёртый описи: глаголы, правящие движком возврата.
 *          Ожидания сняты с эталонной реализации опытом. Глагол «(*ACCEPT)»
 *          завершает сопоставление, закрывая группы открытые, а глаголы
 *          отсечения прекращают попытку сопоставления, различаясь тем,
 *          как её продолжать: отказом целиком, переносом на знак либо
 *          переносом в позицию глагола.
 *
 */
TEST(Regex, InterfaceControlBacktracking) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	/**
	 * Выполняем проверку завершения сопоставления глаголом
	 */
	{
		// Выполняем сборку выражения с глаголом завершения сопоставления
		const auto expression = regexp.build("x(a(b(*ACCEPT)c)d)e");
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (expression));
		// Выполняем проверку обнаружения совпадения
		ASSERT_TRUE(regexp.match("xabcde", expression, bounds));
		// Выполняем проверку количества границ совпадения
		ASSERT_EQ(bounds.size(), static_cast <size_t> (3));
		/**
		 * Выполняем проверку закрытия групп, захват не завершивших
		 *
		 * @details Глагол завершает сопоставление в текущей позиции, а группы,
		 *          захват начавшие и его не завершившие, закрываются ею же:
		 *          «xab» есть совпадение, «ab» и «b» - захваты групп.
		 *
		 */
		EXPECT_EQ(bounds.at(0).first, static_cast <size_t> (0));
		EXPECT_EQ(bounds.at(0).second, static_cast <size_t> (3));
		EXPECT_EQ(bounds.at(1).first, static_cast <size_t> (1));
		EXPECT_EQ(bounds.at(1).second, static_cast <size_t> (3));
		EXPECT_EQ(bounds.at(2).first, static_cast <size_t> (2));
		EXPECT_EQ(bounds.at(2).second, static_cast <size_t> (3));
	}
	/**
	 * @brief Ожидания сопоставления глаголов отсечения
	 *
	 */
	static const struct {
		// Выражение с глаголом управления возвратом
		const char * pattern;
		// Текст сопоставления выражения
		const char * text;
		// Ожидаемые границы совпадения либо отсутствие его
		bool matched;
		size_t begin;
		size_t finish;
	} SAMPLES[] = {
		// Глагол завершения сопоставления
		{"a(*ACCEPT)b", "ab", true, 0, 1},
		{"a(?:b(*ACCEPT))?c", "abc", true, 0, 2},
		// Глагол отказа сопоставления целиком
		{"a(*COMMIT)b", "xab", true, 1, 3},
		{"a+(*COMMIT)b", "aaac", false, 0, 0},
		{"(?:a(*COMMIT)b|ac)", "ac", false, 0, 0},
		// Глагол отказа попытки с переносом её на знак
		{"a(*PRUNE)b", "xab", true, 1, 3},
		{"(?:a(*PRUNE)b|ac)", "ac", false, 0, 0},
		{"a+(*PRUNE)b", "aaab", true, 0, 4},
		// Глагол отказа попытки с переносом её в позицию глагола
		{"a(*SKIP)b", "xab", true, 1, 3},
		{"(?:a(*SKIP)b|ac)", "ac", false, 0, 0},
		{"a+(*SKIP)b", "aaab", true, 0, 4},
		{"(?:aa(*SKIP)b|a)", "aaa", true, 2, 3},
		/**
		 * Отбор позиций начала совпадения глагол обходит
		 *
		 * @details Эталон ведёт себя так же: «(*COMMIT)a» на тексте «ba»
		 *          совпадение находит, поскольку отбор позиций к букве «a»
		 *          и переходит, глагола не достигая вовсе. Указание
		 *          «(*NO_START_OPT)» отбор снимает, и совпадения нет.
		 *
		 */
		{"(*COMMIT)a", "ba", true, 1, 2},
		{"(*NO_START_OPT)(*COMMIT)a", "ba", false, 0, 0}
	};
	/**
	 * Выполняем обход набора ожиданий сопоставления
	 */
	for(auto & sample : SAMPLES) {
		// Выполняем сборку выражения с глаголом управления
		const auto expression = regexp.build(sample.pattern);
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (expression)) << sample.pattern;
		// Выполняем проверку исхода сопоставления образца
		ASSERT_EQ(regexp.match(sample.text, expression, bounds), sample.matched)
		 << sample.pattern << " на тексте «" << sample.text << "»";
		/**
		 * Если совпадение обнаружено
		 */
		if(sample.matched) {
			// Выполняем проверку границ обнаруженного совпадения
			EXPECT_EQ(bounds.front().first, sample.begin) << sample.pattern;
			EXPECT_EQ(bounds.front().second, sample.finish) << sample.pattern;
		}
	}
	/**
	 * @brief Ожидания глагола перехода к ветви следующей
	 *
	 * @details Возврат в глагол отсекает точки, ветвью накопленные, и переходит
	 *          к ветви следующей охватывающей группы. Ветви охватывающей не имея,
	 *          глагол отказывает попытке целиком наравне с глаголом переноса
	 *          на знак: «a(*THEN)b» на тексте «xab» совпадение находит.
	 *
	 */
	static const struct {
		// Выражение с глаголом перехода к ветви следующей
		const char * pattern;
		// Текст сопоставления выражения
		const char * text;
		// Ожидаемые границы совпадения либо отсутствие его
		bool matched;
		size_t begin;
		size_t finish;
	} MOVING[] = {
		{"(?:a(*THEN)b|ac)", "ac", true, 0, 2},
		{"(?:a(*THEN)b|c)", "ac", true, 1, 2},
		{"a(*THEN)b|ac", "ac", true, 0, 2},
		{"(?:(?:a(*THEN)b)|ac)", "ac", true, 0, 2},
		{"a(*THEN)b", "xab", true, 1, 3},
		{"(?:a(*THEN)b|ad)", "ab", true, 0, 2},
		{"(?:a(*THEN)b|ac|ad)", "ad", true, 0, 2},
		{"(?:(a)(*THEN)b|ac)", "ac", true, 0, 2},
		{"^(?:a(*THEN)b|ac)$", "ac", true, 0, 2},
		{"(?:a+(*THEN)b|aac)", "aac", true, 0, 3}
	};
	/**
	 * Выполняем обход ожиданий глагола перехода к ветви следующей
	 */
	for(auto & sample : MOVING) {
		// Выполняем сборку выражения с глаголом перехода
		const auto expression = regexp.build(sample.pattern);
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (expression)) << sample.pattern;
		// Выполняем проверку исхода сопоставления образца
		ASSERT_EQ(regexp.match(sample.text, expression, bounds), sample.matched)
		 << sample.pattern << " на тексте «" << sample.text << "»";
		/**
		 * Если совпадение обнаружено
		 */
		if(sample.matched) {
			// Выполняем проверку границ обнаруженного совпадения
			EXPECT_EQ(bounds.front().first, sample.begin) << sample.pattern;
			EXPECT_EQ(bounds.front().second, sample.finish) << sample.pattern;
		}
	}
	// Выполняем проверку отказа квантора за глаголом управления
	EXPECT_FALSE(static_cast <bool> (regexp.build("(*ACCEPT)*")));
	EXPECT_FALSE(static_cast <bool> (regexp.build("a(*SKIP)?")));
	EXPECT_FALSE(static_cast <bool> (regexp.build("(*THEN)+")));
	/**
	 * @brief Ожидания глагола отметки ветви именем
	 *
	 * @details Имя выводится методом marker: по совпадении - отметка пути,
	 *          совпадение давшего, а по отказу - отметка последняя попытки
	 *          последней. Глагол отсечения отметку снимает, а сопоставление,
	 *          отбором позиций отвергнутое, отметки не даёт вовсе.
	 *
	 */
	static const struct {
		// Выражение с глаголом отметки ветви именем
		const char * pattern;
		// Текст сопоставления выражения
		const char * text;
		// Ожидаемый исход сопоставления
		bool matched;
		// Ожидаемое имя отметки совпадения
		const char * marker;
	} MARKERS[] = {
		{"a(*MARK:один)b", "ab", true, "один"},
		{"a(*MARK:один)b", "ax", false, ""},
		{"(*MARK:A)a|(*MARK:B)b", "b", true, "B"},
		{"(*MARK:A)a|(*MARK:B)b", "c", false, "B"},
		{"a(*:кратко)b", "ab", true, "кратко"},
		{"(*MARK:A)x", "ax", true, "A"},
		{"a(*MARK:A)b|ac", "ac", true, ""},
		{"(?:a(*MARK:A)(*THEN)b|ac)", "ac", true, ""},
		{"(*MARK:A)(*FAIL)|b", "b", true, ""},
		{"a(*MARK:A)(*COMMIT)b", "ax", false, ""},
		{"(*MARK:A)a(*MARK:B)b", "ab", true, "B"}
	};
	/**
	 * Выполняем обход ожиданий глагола отметки ветви именем
	 */
	for(auto & sample : MARKERS) {
		// Выполняем сборку выражения с глаголом отметки
		const auto expression = regexp.build(sample.pattern);
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (expression)) << sample.pattern;
		// Выполняем проверку исхода сопоставления образца
		ASSERT_EQ(regexp.match(sample.text, expression, bounds), sample.matched)
		 << sample.pattern << " на тексте «" << sample.text << "»";
		// Выполняем проверку имени отметки совпадения
		EXPECT_EQ(regexp.marker(), string(sample.marker))
		 << sample.pattern << " на тексте «" << sample.text << "»";
	}
	/**
	 * Выполняем проверку очистки отметки сопоставлением всяким
	 *
	 * @details Сопоставление, отбором позиций отвергнутое, попыток не делает
	 *          вовсе: отметка сопоставления прежнего осталась бы принятой
	 *          за отметку нынешнего, чего эталон не делает.
	 *
	 */
	{
		// Выполняем сборку выражения с глаголом отметки
		const auto expression = regexp.build("a(*MARK:один)b");
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (expression));
		// Выполняем проверку обнаружения совпадения
		ASSERT_TRUE(regexp.match("ab", expression, bounds));
		// Выполняем проверку имени отметки совпадения
		EXPECT_EQ(regexp.marker(), string("один"));
		// Выполняем проверку отсутствия совпадения на тексте ином
		ASSERT_FALSE(regexp.match("ax", expression, bounds));
		// Выполняем проверку очистки имени отметки сопоставлением
		EXPECT_TRUE(regexp.marker().empty());
	}
	// Выполняем проверку отказа глагола отметки без имени
	EXPECT_FALSE(static_cast <bool> (regexp.build("(*MARK)a")));
	EXPECT_FALSE(static_cast <bool> (regexp.build("(*MARK:)a")));
	// Выполняем проверку отказа квантора за глаголом отметки
	EXPECT_FALSE(static_cast <bool> (regexp.build("a(*MARK:A)*b")));
	/**
	 * Выполняем проверку отказа глаголов с именем, кроме глагола отметки
	 *
	 * @details Имя у них эталон принимает, а «(*SKIP:имя)» вдобавок переносит
	 *          попытку в позицию отметки того имени - устройство это остаётся
	 *          пробелом описанным. Глагол с именем отвергается, а не принимается
	 *          молча со смыслом иным.
	 *
	 */
	EXPECT_FALSE(static_cast <bool> (regexp.build("a(*SKIP:A)b|(*MARK:A)ac")));
	EXPECT_FALSE(static_cast <bool> (regexp.build("a(*COMMIT:A)b")));
}

/**
 * @brief Проверка соглашений о переводе строки
 *
 * @details Разряд третий описи глаголов управления: соглашение о переводе
 *          строки правит точкой, привязками к границам строк и привязкой
 *          конца текста, а охват «\R» правится указаниями «(*BSR_*)»
 *          отдельно от него. Правила сняты с эталонной реализации опытом
 *          матрицею семи соглашений на девять знаков.
 *
 */
TEST(Regex, InterfaceNewlineConventions) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Получаем признак сборки выражения с разбором последовательностей UTF-8
	const uint32_t wide = static_cast <uint32_t> (regex::flag_t::UTF);
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	/**
	 * @brief Ожидания сопоставления соглашений о переводе строки
	 *
	 */
	static const struct {
		// Выражение с указанием соглашения о переводе строки
		const char * pattern;
		// Текст сопоставления выражения
		const char * text;
		// Ожидаемый исход сопоставления
		bool expected;
	} SAMPLES[] = {
		// Точка перевод строки не сопоставляет, а возврат каретки сопоставляет
		{"^.$", "\n", false}, {"^.$", "\r", true},
		// Соглашение возврата каретки правило обращает
		{"(*CR)^.$", "\n", true}, {"(*CR)^.$", "\r", false},
		// Соглашение пары возврата каретки одиночных завершений не знает
		{"(*CRLF)^.$", "\n", true}, {"(*CRLF)^.$", "\r", true},
		// Соглашение возврата каретки либо перевода исключает оба
		{"(*ANYCRLF)^.$", "\n", false}, {"(*ANYCRLF)^.$", "\r", false},
		// Табуляция вертикальная и подача страницы завершениями его не являются
		{"(*ANYCRLF)^.$", "\v", true}, {"(*ANYCRLF)^.$", "\f", true},
		// Соглашение всякого перевода Юникода исключает и их
		{"(*ANY)^.$", "\v", false}, {"(*ANY)^.$", "\f", false},
		{"(*ANY)^.$", "\u0085", false}, {"(*ANY)^.$", "\u2028", false},
		// Соглашение нулевого знака исключает его одного
		{"(*NUL)^.$", "\n", true}, {"(*NUL)^.$", "\v", true},
		// Привязка к началу строки следует соглашению
		{"(?m)^b", "a\nb", true}, {"(?m)^b", "a\rb", false},
		{"(*CR)(?m)^b", "a\rb", true}, {"(*CR)(?m)^b", "a\nb", false},
		{"(*CRLF)(?m)^b", "a\r\nb", true}, {"(*CRLF)(?m)^b", "a\nb", false},
		{"(*ANYCRLF)(?m)^b", "a\rb", true}, {"(*ANYCRLF)(?m)^b", "a\vb", false},
		{"(*ANY)(?m)^b", "a\vb", true}, {"(*ANY)(?m)^b", "a\u2028b", true},
		{"(*NUL)(?m)^b", "a\nb", false},
		// Привязка конца текста следует соглашению наравне с началом строки
		{"a$", "a\n", true}, {"a$", "a\r", false},
		{"(*CR)a$", "a\r", true}, {"(*CR)a$", "a\n", false},
		{"(*CRLF)a$", "a\r\n", true}, {"(*CRLF)a$", "a\r", false},
		/**
		 * Охват последовательности перевода строки от соглашения не зависит
		 *
		 * @details Указания «(*BSR_*)» правят им отдельно: «(*BSR_ANYCRLF)»
		 *          сводит охват к возврату каретки, переводу строки и паре их.
		 *
		 */
		{"(*CR)^\\R$", "\n", true}, {"(*CR)^\\R$", "\v", true},
		{"(*BSR_ANYCRLF)^\\R$", "\v", false}, {"(*BSR_ANYCRLF)^\\R$", "\r\n", true},
		{"(*BSR_UNICODE)^\\R$", "\v", true}, {"(*ANY)(*BSR_ANYCRLF)^\\R$", "\u2028", false}
	};
	/**
	 * Выполняем обход набора ожиданий сопоставления
	 */
	for(auto & sample : SAMPLES) {
		// Выполняем сборку выражения с указанием соглашения
		const auto expression = regexp.build(sample.pattern, wide);
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (expression)) << sample.pattern;
		// Получаем текст сопоставления образца
		const string text(sample.text);
		// Выполняем проверку исхода сопоставления образца
		EXPECT_EQ(regexp.match(text, expression, bounds), sample.expected)
		 << sample.pattern << " на тексте длиной " << text.size();
	}
	/**
	 * Выполняем проверку соглашения нулевого знака
	 *
	 * @details Тексты с нулевым знаком заводятся отдельно: запись строковая
	 *          на нём обрывается, и в набор ожиданий они не ложатся.
	 *
	 */
	{
		// Получаем текст сопоставления с нулевым знаком внутри
		const string text = (string("a") + string(1, '\0') + string("b"));
		// Выполняем сборку выражения с соглашением нулевого знака
		const auto expression = regexp.build("(*NUL)(?m)^b", wide);
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (expression));
		// Выполняем проверку разделения строк нулевым знаком
		EXPECT_TRUE(regexp.match(text, expression, bounds));
		// Выполняем сборку выражения с соглашением умолчания
		const auto plain = regexp.build("(?m)^b", wide);
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (plain));
		// Выполняем проверку отсутствия разделения строк нулевым знаком
		EXPECT_FALSE(regexp.match(text, plain, bounds));
		// Получаем текст сопоставления с нулевым знаком завершающим
		const string tail = (string("a") + string(1, '\0'));
		// Выполняем сборку выражения с привязкой конца текста
		const auto ending = regexp.build("(*NUL)a$", wide);
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (ending));
		// Выполняем проверку привязки конца текста при соглашении нулевого знака
		EXPECT_TRUE(regexp.match(tail, ending, bounds));
	}
}

/**
 * @brief Проверка пределов и выключателей оптимизаций
 *
 * @details Разряд второй описи глаголов управления: пределы, выражением
 *          заданные, и выключатели оптимизаций. Предел выражения предел
 *          вызывающей стороны понижает, но не повышает, - правило снято
 *          с эталонной реализации.
 *
 */
TEST(Regex, InterfaceExpressionLimits) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	// Получаем текст сопоставления, возврат нагружающий
	const string text(4000, 'a');
	// Выполняем сборку выражения без предела шагов сопоставления
	const auto free = regexp.build("(a|aa)+\\1");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (free));
	// Выполняем проверку обнаружения совпадения без предела
	EXPECT_TRUE(regexp.match(text, free, bounds));
	// Выполняем сборку выражения с пределом шагов сопоставления малым
	const auto limited = regexp.build("(*LIMIT_MATCH=5)(a|aa)+\\1");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (limited));
	// Выполняем проверку отказа сопоставления по исчерпании предела
	EXPECT_FALSE(regexp.match(text, limited, bounds));
	// Выполняем проверку довода отказа сопоставления
	EXPECT_EQ(regexp.error(), regex::error_t::BUDGET_EXCEEDED);
	/**
	 * Выполняем проверку предела, работы не допускающего вовсе
	 *
	 * @details Отказ даётся до выбора пути исполнения: пути, линейные
	 *          по построению, шагов не считают, и предел на них иначе
	 *          не сказался бы вовсе.
	 *
	 */
	const auto forbidden = regexp.build("(*LIMIT_MATCH=0)a");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (forbidden));
	// Выполняем проверку отказа сопоставления работы не допускающего
	EXPECT_FALSE(regexp.match("a", forbidden, bounds));
	// Выполняем проверку довода отказа сопоставления
	EXPECT_EQ(regexp.error(), regex::error_t::BUDGET_EXCEEDED);
	// Выполняем сборку выражения с пределом объёма памяти сопоставления
	const auto memory = regexp.build("(*LIMIT_HEAP=1)(a|aa)+\\1");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (memory));
	// Выполняем проверку отказа сопоставления по исчерпании памяти
	EXPECT_FALSE(regexp.match(text, memory, bounds));
	// Выполняем проверку довода отказа сопоставления
	EXPECT_EQ(regexp.error(), regex::error_t::BUDGET_EXCEEDED);
	// Выполняем сборку выражения с рекурсией без предела глубины
	const auto recursion = regexp.build("(a(?R)?)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (recursion));
	// Выполняем проверку обнаружения совпадения без предела глубины
	EXPECT_TRUE(regexp.match("aaa", recursion, bounds));
	// Выполняем сборку того же выражения с пределом глубины малым
	const auto deep = regexp.build("(*LIMIT_DEPTH=2)(a(?R)?)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (deep));
	// Выполняем проверку отказа сопоставления по исчерпании глубины
	EXPECT_FALSE(regexp.match("aaa", deep, bounds));
	// Выполняем проверку довода отказа сопоставления
	EXPECT_EQ(regexp.error(), regex::error_t::BUDGET_EXCEEDED);
	// Выполняем сборку того же выражения с пределом глубины достаточным
	const auto shallow = regexp.build("(*LIMIT_DEPTH=10)(a(?R)?)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (shallow));
	// Выполняем проверку обнаружения совпадения при достаточном пределе
	EXPECT_TRUE(regexp.match("aaa", shallow, bounds));
	// Выполняем проверку отказа записи предела с посторонним знаком
	EXPECT_FALSE(static_cast <bool> (regexp.build("(*LIMIT_MATCH=x)a")));
	// Выполняем проверку отказа записи предела пустой
	EXPECT_FALSE(static_cast <bool> (regexp.build("(*LIMIT_MATCH=)a")));
	// Выполняем проверку отказа записи предела свыше разрядности
	EXPECT_FALSE(static_cast <bool> (regexp.build("(*LIMIT_MATCH=4294967296)a")));
	// Выполняем сборку выражения с выключателями оптимизаций
	const auto plain = regexp.build("(*NO_START_OPT)(*NO_AUTO_POSSESS)abc");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (plain));
	/**
	 * Выполняем проверку сохранения исхода при снятых оптимизациях
	 *
	 * @details Отбор позиций исхода сопоставления не меняет, а меняет одну лишь
	 *          скорость: снятие его выражается пустым отбором, и исход остаётся
	 *          тем же, что и при отборе действующем.
	 *
	 */
	ASSERT_TRUE(regexp.match("xxabc", plain, bounds));
	// Выполняем проверку границ обнаруженного совпадения
	EXPECT_EQ(bounds.front().first, static_cast <size_t> (2));
	EXPECT_EQ(bounds.front().second, static_cast <size_t> (5));
	// Выполняем сборку того же выражения с оптимизациями действующими
	const auto optimized = regexp.build("abc");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (optimized));
	// Выполняем проверку совпадения границ обоих выражений
	ASSERT_TRUE(regexp.match("xxabc", optimized, bounds));
	// Выполняем проверку границ обнаруженного совпадения
	EXPECT_EQ(bounds.front().first, static_cast <size_t> (2));
	EXPECT_EQ(bounds.front().second, static_cast <size_t> (5));
}

/**
 * @brief Проверка глагола отказа и начальных указаний
 *
 * @details Разряд первый описи глаголов управления: глагол отказа, совпадения
 *          не дающий никогда, и указания начальные, ложащиеся на признаки
 *          сборки, модулем заведённые. Прочие глаголы - «(*ACCEPT)», «(*SKIP)»,
 *          «(*PRUNE)», «(*COMMIT)», «(*THEN)», «(*MARK)» - и соглашения
 *          о переводе строки остаются пробелом описанным.
 *
 */
TEST(Regex, InterfaceControlVerbs) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	// Выполняем сборку выражения с глаголом отказа в ветви первой
	const auto refusal = regexp.build("a(*FAIL)|b");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (refusal));
	/**
	 * Выполняем проверку отказа ветви, глагол несущей
	 *
	 * @details Ветвь первая совпадения не даёт никогда, и сопоставление
	 *          переходит к ветви второй.
	 *
	 */
	ASSERT_TRUE(regexp.match("ab", refusal, bounds));
	// Выполняем проверку границ обнаруженного совпадения
	EXPECT_EQ(bounds.front().first, static_cast <size_t> (1));
	EXPECT_EQ(bounds.front().second, static_cast <size_t> (2));
	// Выполняем сборку выражения с глаголом отказа кратким
	const auto brief = regexp.build("a(*F)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (brief));
	// Выполняем проверку отсутствия совпадения вовсе
	EXPECT_FALSE(regexp.match("a", brief, bounds));
	// Выполняем проверку принятия глагола отказа с меткой
	EXPECT_TRUE(static_cast <bool> (regexp.build("x(*FAIL:метка)|y")));
	// Выполняем проверку отказа квантора за глаголом управления
	EXPECT_FALSE(static_cast <bool> (regexp.build("(*FAIL)*")));
	EXPECT_FALSE(static_cast <bool> (regexp.build("a(*F)?")));
	// Выполняем проверку отказа глагола, набором не заведённого
	EXPECT_FALSE(static_cast <bool> (regexp.build("(*XXX)a")));
	// Выполняем проверку отказа глагола, буквой строчной записанного
	EXPECT_FALSE(static_cast <bool> (regexp.build("(*fail)")));
	// Выполняем сборку выражения с указанием разбора последовательностей UTF-8
	const auto wide = regexp.build("(*UTF)^.$");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (wide));
	/**
	 * Выполняем проверку действия указания начального
	 *
	 * @details Указание задаёт разбор последовательностей UTF-8, и точка
	 *          сопоставляет символ целиком, а не единицу кодирования.
	 *
	 */
	ASSERT_TRUE(regexp.match("\u0439", wide, bounds));
	// Выполняем проверку границ обнаруженного совпадения
	EXPECT_EQ(bounds.front().second, static_cast <size_t> (2));
	// Выполняем сборку выражения без указания разбора последовательностей UTF-8
	const auto plain = regexp.build("^.$");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (plain));
	// Выполняем проверку сопоставления единицы кодирования вне режима UTF-8
	EXPECT_FALSE(regexp.match("\u0439", plain, bounds));
	// Выполняем сборку выражения с указанием учёта свойств Юникода
	const auto property = regexp.build("(*UTF)(*UCP)^\\w$");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (property));
	// Выполняем проверку действия двух указаний подряд
	EXPECT_TRUE(regexp.match("\u0439", property, bounds));
	// Выполняем сборку выражения с указанием запрета пустого совпадения
	const auto notempty = regexp.build("(*NOTEMPTY)a*");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (notempty));
	// Выполняем проверку запрета пустого совпадения
	EXPECT_FALSE(regexp.match("b", notempty, bounds));
	// Выполняем проверку сохранения совпадения непустого
	EXPECT_TRUE(regexp.match("aa", notempty, bounds));
	/**
	 * Выполняем проверку отказа указания вне начала выражения
	 *
	 * @details Указания начальные размещаются в начале выражения и только там:
	 *          «a(*UTF)» эталон отвергает наравне с глаголом неизвестным.
	 *
	 */
	EXPECT_FALSE(static_cast <bool> (regexp.build("a(*UTF)")));
	EXPECT_FALSE(static_cast <bool> (regexp.build("(?i)(*UTF)a")));
	// Выполняем проверку принятия указания перед встроенными настройками
	EXPECT_TRUE(static_cast <bool> (regexp.build("(*UTF)(?i)a")));
	// Выполняем сборку выражения с глаголом отказа и захватывающей группой
	const auto captured = regexp.build("(a)(*FAIL)|(b)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (captured));
	/**
	 * Выполняем проверку количества захватывающих групп выражения
	 *
	 * @details Глагол управления захватывающей группой не является
	 *          и в счёт их не идёт.
	 *
	 */
	EXPECT_EQ(regexp.captures(captured), static_cast <uint32_t> (2));
}

/**
 * @brief Проверка задания символа кодовым значением Юникода
 *
 * @details Последовательность «\N{U+HHHH}» задаёт символ значением его кодовым
 *          и доступна лишь при разборе последовательностей UTF-8, как то
 *          и у эталонной реализации. Модуль её не поддерживал вовсе, отвергая
 *          наравне с заданием символа именем, эталоном не поддерживаемым.
 *
 */
TEST(Regex, InterfaceNamedCodepoint) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Получаем признаки сборки выражения с разбором последовательностей UTF-8
	const uint32_t wide = (static_cast <uint32_t> (regex::flag_t::UTF) | static_cast <uint32_t> (regex::flag_t::UCP));
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	// Выполняем сборку выражения с символом, заданным кодовым значением
	const auto latin = regexp.build("\\N{U+41}", wide);
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (latin));
	// Выполняем проверку совпадения с заданным символом
	EXPECT_TRUE(regexp.match("A", latin, bounds));
	// Выполняем проверку отсутствия совпадения с символом иным
	EXPECT_FALSE(regexp.match("B", latin, bounds));
	// Выполняем сборку выражения с символом вне таблицы ASCII
	const auto wideness = regexp.build("\\N{U+439}", wide);
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (wideness));
	// Выполняем проверку совпадения с символом вне таблицы ASCII
	ASSERT_TRUE(regexp.match("\u0439", wideness, bounds));
	// Выполняем проверку границ обнаруженного совпадения
	EXPECT_EQ(bounds.front().second, static_cast <size_t> (2));
	// Выполняем сборку выражения с диапазоном из символов заданных
	const auto range = regexp.build("[\\N{U+41}-\\N{U+5A}]", wide);
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (range));
	// Выполняем проверку действия диапазона внутри класса символов
	EXPECT_TRUE(regexp.match("M", range, bounds));
	// Выполняем проверку отсутствия совпадения вне диапазона
	EXPECT_FALSE(regexp.match("m", range, bounds));
	/**
	 * Выполняем проверку отказа вне разбора последовательностей UTF-8
	 *
	 * @details Задание символа кодовым значением доступно лишь при разборе
	 *          последовательностей UTF-8: вне его текст ведётся октетами.
	 *
	 */
	EXPECT_FALSE(static_cast <bool> (regexp.build("\\N{U+41}")));
	// Выполняем проверку отказа задания символа именем Юникода
	EXPECT_FALSE(static_cast <bool> (regexp.build("\\N{LATIN SMALL LETTER A}", wide)));
	// Выполняем проверку отказа задания без цифр кодового значения
	EXPECT_FALSE(static_cast <bool> (regexp.build("\\N{U+}", wide)));
	// Выполняем проверку отказа значения, отведённого суррогатной паре
	EXPECT_FALSE(static_cast <bool> (regexp.build("\\N{U+D800}", wide)));
	// Выполняем проверку отказа значения свыше предела Юникода
	EXPECT_FALSE(static_cast <bool> (regexp.build("\\N{U+110000}", wide)));
	// Выполняем сборку выражения с любым символом, кроме перевода строки
	const auto any = regexp.build("\\N", wide);
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (any));
	// Выполняем проверку сохранения прежнего значения последовательности
	EXPECT_TRUE(regexp.match("a", any, bounds));
	// Выполняем проверку отсутствия совпадения с переводом строки
	EXPECT_FALSE(regexp.match("\n", any, bounds));
}

/**
 * @brief Проверка правил разбора квантора повторения
 *
 * @details Правила эти сняты с эталонной реализации опытом: внутри фигурных
 *          скобок допускаются пробел и знак табуляции, прочие же пробельные
 *          символы квантора не образуют, а квантор, ни одной цифры не несущий,
 *          остаётся последовательностью литералов.
 *
 */
TEST(Regex, InterfaceRepeatBraces) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	// Выполняем сборку выражения с пробелами внутри квантора повторения
	const auto spaced = regexp.build("a{ 2 , 3 }");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (spaced));
	// Выполняем проверку действия квантора с пробелами
	ASSERT_TRUE(regexp.match("aaa", spaced, bounds));
	// Выполняем проверку границ обнаруженного совпадения
	EXPECT_EQ(bounds.front().second, static_cast <size_t> (3));
	// Выполняем сборку выражения со знаком табуляции внутри квантора
	const auto tabbed = regexp.build("a{2\t,\t3}");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (tabbed));
	// Выполняем проверку действия квантора со знаком табуляции
	EXPECT_TRUE(regexp.match("aaa", tabbed, bounds));
	// Выполняем сборку выражения с переводом строки внутри фигурных скобок
	const auto broken = regexp.build("a{\n2}");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (broken));
	/**
	 * Выполняем проверку отсутствия квантора при переводе строки
	 *
	 * @details Перевод строки квантора не образует, и скобки фигурные
	 *          остаются литералами выражения.
	 *
	 */
	EXPECT_FALSE(regexp.match("aa", broken, bounds));
	// Выполняем проверку совпадения выражения с ним же самим
	EXPECT_TRUE(regexp.match("a{\n2}", broken, bounds));
	// Выполняем сборку выражения с квантором без цифр вовсе
	const auto empty = regexp.build("x{,}");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (empty));
	/**
	 * Выполняем проверку отсутствия квантора без цифр вовсе
	 *
	 * @details Квантор «{,}» ни одной цифры не несёт и квантором не является:
	 *          «x{,}» есть последовательность литералов, а не «x» произвольное
	 *          число раз.
	 *
	 */
	EXPECT_FALSE(regexp.match("x", empty, bounds));
	// Выполняем проверку совпадения выражения с ним же самим
	EXPECT_TRUE(regexp.match("x{,}", empty, bounds));
	// Выполняем сборку выражения с опущенным наименьшим числом повторений
	const auto least = regexp.build("a{,2}");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (least));
	// Выполняем проверку действия квантора с опущенной границей нижней
	ASSERT_TRUE(regexp.match("aa", least, bounds));
	// Выполняем проверку границ обнаруженного совпадения
	EXPECT_EQ(bounds.front().second, static_cast <size_t> (2));
}

/**
 * @brief Проверка правил состава класса символов
 *
 * @details Правила эти сняты с эталонной реализации опытом и закрывают пять
 *          расхождений разом: значение кодовое свыше 0xFF вне разбора UTF-8,
 *          класс POSIX нижним краем диапазона, элемент сортировки в классе
 *          отрицающем и края диапазона у дословной последовательности.
 *
 */
TEST(Regex, InterfaceClassRules) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Получаем признаки сборки выражения с разбором последовательностей UTF-8
	const uint32_t wide = (static_cast <uint32_t> (regex::flag_t::UTF) | static_cast <uint32_t> (regex::flag_t::UCP));
	/**
	 * Выполняем проверку предела кодового значения вне разбора UTF-8
	 *
	 * @details Текст вне разбора последовательностей UTF-8 ведётся октетами,
	 *          и значение свыше 0xFF в нём не выражается вовсе. Модуль такое
	 *          значение принимал, собирая выражение, совпадения не дающее
	 *          никогда, - отказ молчаливый.
	 *
	 */
	EXPECT_FALSE(static_cast <bool> (regexp.build("[\\x{439}]")));
	EXPECT_FALSE(static_cast <bool> (regexp.build("\\x{100}")));
	EXPECT_FALSE(static_cast <bool> (regexp.build("\\o{400}")));
	// Выполняем проверку принятия кодового значения предельного
	EXPECT_TRUE(static_cast <bool> (regexp.build("[\\x{FF}]")));
	// Выполняем проверку принятия значения свыше предела при разборе UTF-8
	EXPECT_TRUE(static_cast <bool> (regexp.build("[\\x{439}]", wide)));
	/**
	 * Выполняем проверку отказа класса POSIX краем диапазона
	 *
	 * @details Класс символов POSIX задаёт набор символов и краем диапазона
	 *          выступать не может ни нижним, ни верхним.
	 *
	 */
	EXPECT_FALSE(static_cast <bool> (regexp.build("[[:alpha:]-z]")));
	EXPECT_FALSE(static_cast <bool> (regexp.build("[.-[:punct:]]")));
	/**
	 * Выполняем проверку элемента сортировки в классе отрицающем
	 *
	 * @details Знак вводящий элемент сортировки открывает лишь класс,
	 *          отрицанием не являющийся: «[..]» отвергается, а «[^..]»
	 *          есть класс из знаков точки.
	 *
	 */
	EXPECT_FALSE(static_cast <bool> (regexp.build("[..]")));
	EXPECT_TRUE(static_cast <bool> (regexp.build("[^..]")));
	// Выполняем сборку выражения с дословной последовательностью краем нижним
	const auto lower = regexp.build("[\\Qa\\E-z]");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (lower));
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	/**
	 * Выполняем проверку края нижнего у дословной последовательности
	 *
	 * @details Краем нижним диапазона выступает символ последовательности
	 *          последний: «[\Qa\E-z]» есть диапазон от «a» до «z».
	 *
	 */
	EXPECT_TRUE(regexp.match("m", lower, bounds));
	// Выполняем сборку выражения с дословной последовательностью краем верхним
	const auto upper = regexp.build("[a-\\Qzc\\E]");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (upper));
	/**
	 * Выполняем проверку края верхнего у дословной последовательности
	 *
	 * @details Краем верхним диапазона выступает символ последовательности
	 *          первый, а прочие входят в класс наравне с остальными:
	 *          «[a-\Qzc\E]» есть диапазон от «a» до «z».
	 *
	 */
	EXPECT_TRUE(regexp.match("m", upper, bounds));
	// Выполняем проверку отказа диапазона с краями в обратном порядке
	EXPECT_FALSE(static_cast <bool> (regexp.build("[]-\\Q-]\\E]")));
	// Выполняем сборку выражения с дословной последовательностью пустой
	const auto empty = regexp.build("[a-\\Q\\E]");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (empty));
	/**
	 * Выполняем проверку прозрачности последовательности пустой
	 *
	 * @details Последовательность «\Q\E» символов не несёт и краем диапазона
	 *          не выступает: знак переноса остаётся литералом класса.
	 *
	 */
	EXPECT_TRUE(regexp.match("-", empty, bounds));
	// Выполняем проверку отсутствия диапазона в собранном классе символов
	EXPECT_FALSE(regexp.match("m", empty, bounds));
}

/**
 * @brief Проверка сброса внутристрочных признаков
 *
 * @details Сброс «(?^)» снимает признаки «i», «m», «n», «s» и «x», а признаков
 *          «U» и «J» не касается: правило это снято с эталонной реализации
 *          опытом. Снималась прежде обратная жадность, сбросом не управляемая,
 *          а отмена захвата круглыми скобками не снималась вовсе.
 *
 */
TEST(Regex, InterfaceInlineReset) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку выражения с обратной жадностью и сбросом признаков
	const auto ungreedy = regexp.build("(?U)(?^)a*?");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (ungreedy));
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	// Выполняем проверку сохранения обратной жадности сбросом признаков
	ASSERT_TRUE(regexp.match("aaa", ungreedy, bounds));
	// Выполняем проверку границ обнаруженного совпадения
	EXPECT_EQ(bounds.front().second, static_cast <size_t> (3));
	// Выполняем сборку выражения с отменой захвата и сбросом признаков
	const auto capturing = regexp.build("(?n)(?^)(a)b");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (capturing));
	// Выполняем проверку снятия отмены захвата сбросом признаков
	EXPECT_EQ(regexp.captures(capturing), static_cast <uint32_t> (1));
	// Выполняем сборку выражения с отменой захвата без сброса признаков
	const auto nocapture = regexp.build("(?n)(a)b");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (nocapture));
	// Выполняем проверку действия отмены захвата круглыми скобками
	EXPECT_EQ(regexp.captures(nocapture), static_cast <uint32_t> (0));
	// Выполняем сборку выражения с повторным объявлением имён и сбросом признаков
	const auto duplicates = regexp.build("(?J)(?^)(?<v>a)(?<v>b)");
	// Выполняем проверку сохранения повторного объявления имён сбросом признаков
	EXPECT_TRUE(static_cast <bool> (duplicates));
	// Выполняем сборку выражения со сбросом признака учёта регистра
	const auto caseless = regexp.build("(?i)(?^)A");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (caseless));
	// Выполняем проверку снятия сопоставления без учёта регистра
	EXPECT_FALSE(regexp.match("a", caseless, bounds));
}

/**
 * @brief Проверка обращения по имени к одной из одноимённых групп
 *
 * @details Правила эти сняты с эталонной реализации: ссылка по имени
 *          обращается к первой из одноимённых групп, захват выполнившей,
 *          и к прочим не возвращается вовсе, а условие по имени выполнено
 *          при захвате любой из них. Разбор выводит по имени номер первой
 *          объявленной, отчего оба правила нарушались: ссылка не находила
 *          захвата, выполненного группою второй, а условие им не выполнялось.
 *
 */
TEST(Regex, InterfaceDuplicateReference) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	/**
	 * Выполняем проверку обоих способов сопоставления
	 */
	for(uint8_t index = 0; index < 2; index++) {
		// Получаем признаки сборки регулярного выражения
		const uint32_t flags = (static_cast <uint32_t> (regex::flag_t::DUPNAMES) |
		 ((index > 0) ? static_cast <uint32_t> (regex::flag_t::JIT) : 0));
		// Выполняем сборку выражения со ссылкою на имя одноимённых групп
		const auto reference = regexp.build("(?<v>a)|(?<v>b)\\k{v}", flags);
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (reference));
		// Набор границ совпадения и захваченных групп
		vector <pair <size_t, size_t>> bounds;
		// Выполняем проверку обращения ссылки к захвату группы второй
		ASSERT_TRUE(regexp.match("bb", reference, bounds));
		// Выполняем проверку границ обнаруженного совпадения
		EXPECT_EQ(bounds.front().first, static_cast <size_t> (0));
		EXPECT_EQ(bounds.front().second, static_cast <size_t> (2));
		// Выполняем сборку выражения со ссылкою при обеих группах захвативших
		const auto first = regexp.build("(?<v>a)(?<v>b)\\k{v}c", flags);
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (first));
		// Выполняем проверку обращения ссылки к первой захватившей группе
		EXPECT_TRUE(regexp.match("abac", first, bounds));
		/**
		 * Выполняем проверку отказа от возврата к прочим одноимённым группам
		 *
		 * @details Захват выполнен обеими группами, и ссылка берёт первую из них:
		 *          отказ сопоставления к группе второй не возвращается.
		 *
		 */
		EXPECT_FALSE(regexp.match("abbc", first, bounds));
		// Выполняем сборку выражения с условием по имени одноимённых групп
		const auto condition = regexp.build("(?<v>a)?(?<v>b)?(?(<v>)x|y)", flags);
		// Выполняем проверку выполнения сборки регулярного выражения
		ASSERT_TRUE(static_cast <bool> (condition));
		// Выполняем проверку выполнения условия захватом группы второй
		ASSERT_TRUE(regexp.match("bx", condition, bounds));
		// Выполняем проверку границ обнаруженного совпадения
		EXPECT_EQ(bounds.front().first, static_cast <size_t> (0));
		EXPECT_EQ(bounds.front().second, static_cast <size_t> (2));
		// Выполняем проверку невыполнения условия при захвате не выполненном
		EXPECT_TRUE(regexp.match("y", condition, bounds));
	}
}

/**
 * @brief Проверка извлечения захваченного текста по имени группы
 *
 */
TEST(Regex, InterfaceCapture) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("(?P<host>[\\w.]+):(?P<port>\\d+)(?<tail>/\\w+)?");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	// Выполняем проверку обнаружения совпадения
	ASSERT_TRUE(regexp.match("узел anyks.com:8080 здесь", expression, bounds));
	// Выполняем проверку текста, захваченного именованной группой узла
	EXPECT_EQ(regexp.capture("узел anyks.com:8080 здесь", bounds, expression, "host"), "anyks.com");
	// Выполняем проверку текста, захваченного именованной группой порта
	EXPECT_EQ(regexp.capture("узел anyks.com:8080 здесь", bounds, expression, "port"), "8080");
	/**
	 * Выполняем проверку невыполненного захвата именованной группой
	 *
	 * @details Невыполненный захват от захвата пустого текста отличается тем,
	 *          что выводимый вид на текст сопоставления не ссылается.
	 *
	 */
	EXPECT_EQ(regexp.capture("узел anyks.com:8080 здесь", bounds, expression, "tail").data(), nullptr);
	// Выполняем проверку отсутствия неизвестной именованной группы
	EXPECT_EQ(regexp.capture("узел anyks.com:8080 здесь", bounds, expression, "path").data(), nullptr);
	// Выполняем проверку соответствия имён групп наборам их номеров
	EXPECT_EQ(regexp.groups(expression).size(), static_cast <size_t> (3));
}

/**
 * @brief Проверка извлечения именованных групп сопоставлением
 *
 */
TEST(Regex, InterfaceNamed) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("(?<method>[A-Z]+) (?<path>\\S+) HTTP/(?<version>\\d\\.\\d)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Соответствие имён именованных групп захваченному тексту
	unordered_map <string, string> result;
	// Выполняем проверку обнаружения совпадения
	ASSERT_TRUE(regexp.exec("GET /index.html HTTP/1.1", expression, result));
	// Выполняем проверку количества извлечённых именованных групп
	ASSERT_EQ(result.size(), static_cast <size_t> (3));
	// Выполняем проверку текста именованной группы способа запроса
	EXPECT_EQ(result.at("method"), "GET");
	// Выполняем проверку текста именованной группы пути запроса
	EXPECT_EQ(result.at("path"), "/index.html");
	// Выполняем проверку текста именованной группы издания протокола
	EXPECT_EQ(result.at("version"), "1.1");
	// Выполняем проверку отсутствия совпадения в ином тексте
	EXPECT_FALSE(regexp.exec("совпадения здесь нет", expression, result));
	// Выполняем проверку очистки соответствия при отсутствии совпадения
	EXPECT_TRUE(result.empty());
}

/**
 * @brief Проверка одноимённых групп регулярного выражения
 *
 */
TEST(Regex, InterfaceDuplicates) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения с одноимёнными группами
	const auto expression = regexp.build("(?J)(?<n>a)|(?<n>b)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Выполняем проверку количества групп, объявленных одним именем
	ASSERT_EQ(regexp.groups(expression).at("n").size(), static_cast <size_t> (2));
	// Набор границ совпадения и захваченных групп
	vector <pair <size_t, size_t>> bounds;
	// Выполняем проверку обнаружения совпадения первой ветвью
	ASSERT_TRUE(regexp.match("a", expression, bounds));
	/**
	 * Выполняем проверку выбора группы, выполнившей захват
	 *
	 * @details Одно имя объявлено двумя группами, и захват выполняет та из них,
	 *          что участвовала в совпадении. Так же поступает эталонная реализация.
	 *
	 */
	EXPECT_EQ(regexp.capture("a", bounds, expression, "n"), "a");
	// Выполняем проверку обнаружения совпадения второй ветвью
	ASSERT_TRUE(regexp.match("b", expression, bounds));
	// Выполняем проверку выбора группы, выполнившей захват
	EXPECT_EQ(regexp.capture("b", bounds, expression, "n"), "b");
	// Выполняем проверку отказа сборки одноимённых групп вне режима «DUPNAMES»
	EXPECT_FALSE(static_cast <bool> (regexp.build("(?<n>a)(?<n>b)")));
}

/**
 * @brief Проверка установки режимов сборки регулярного выражения
 *
 */
TEST(Regex, InterfaceFlags) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения с учётом регистра символов
	const auto sensitive = regexp.build("ПРИВЕТ", static_cast <uint32_t> (regex::flag_t::UTF));
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (sensitive));
	// Выполняем проверку отсутствия совпадения при учёте регистра символов
	EXPECT_FALSE(regexp.test("привет", sensitive));
	// Выполняем сборку регулярного выражения без учёта регистра символов
	const auto insensitive = regexp.build("ПРИВЕТ", {regex::flag_t::UTF, regex::flag_t::CASELESS});
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (insensitive));
	// Выполняем проверку наличия совпадения без учёта регистра символов
	EXPECT_TRUE(regexp.test("привет", insensitive));
	// Выполняем сборку регулярного выражения с привязкой к границам строк
	const auto multiline = regexp.build("^b", {regex::flag_t::MULTILINE});
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (multiline));
	// Выполняем проверку наличия совпадения с привязкой к границам строк
	EXPECT_TRUE(regexp.test("a\nb", multiline));
}

/**
 * @brief Проверка отказа сборки ошибочного регулярного выражения
 *
 */
TEST(Regex, InterfaceFailure) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку ошибочного регулярного выражения
	const auto expression = regexp.build("a(b");
	// Выполняем проверку отказа сборки регулярного выражения
	EXPECT_FALSE(static_cast <bool> (expression));
	// Выполняем проверку установки кода ошибки сборки выражения
	EXPECT_NE(regexp.error(), regex::error_t::NONE);
	// Выполняем проверку установки текста ошибки сборки выражения
	EXPECT_FALSE(regexp.message().empty());
	// Выполняем проверку отсутствия совпадения несобранного выражения
	EXPECT_FALSE(regexp.test("ab", expression));
	// Выполняем проверку отсутствия захватывающих групп несобранного выражения
	EXPECT_EQ(regexp.captures(expression), static_cast <uint32_t> (0));
}

/**
 * @brief Проверка кэша собранных регулярных выражений
 *
 */
TEST(Regex, InterfaceCache) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения
	const auto first = regexp.build("[a-z]+\\d+");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (first));
	// Выполняем повторную сборку того же регулярного выражения
	const auto second = regexp.build("[a-z]+\\d+");
	/**
	 * Выполняем проверку вывода собранного ранее выражения
	 *
	 * @details Кэш избавляет от повторной сборки того же выражения, пока собранное
	 *          ранее выражение удерживается вызывающей стороной.
	 *
	 */
	EXPECT_EQ(first.get(), second.get());
	// Выполняем сборку того же выражения с иным набором режимов
	const auto third = regexp.build("[a-z]+\\d+", {regex::flag_t::CASELESS});
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (third));
	/**
	 * Выполняем проверку сборки отдельного выражения для иного набора режимов
	 *
	 * @details Ключом кэша является пара из набора режимов и текста выражения,
	 *          поэтому режимы разделяют собранные выражения между собой.
	 *
	 */
	EXPECT_NE(first.get(), third.get());
	// Выполняем проверку сопоставления собранного ранее выражения
	EXPECT_TRUE(regexp.test("abc123", second));
	// Выполняем очистку кэша собранных выражений
	regexp.clear();
	// Выполняем проверку сопоставления выражения после очистки кэша
	EXPECT_TRUE(regexp.test("abc123", first));
}

/**
 * @brief Проверка сопоставления выражения несколькими потоками исполнения
 *
 * @details Согласования доступа модуль не ведёт вовсе, и проверка закрепляет,
 *          что оно и не требуется: собранное выражение после сборки не
 *          изменяется, а рабочее состояние сопоставления хранится отдельно
 *          для каждого потока исполнения. Сборка же выражения идёт до потоков -
 *          кэш её поле объекта, и защищать объект волен сам потребитель
 *
 */
TEST(Regex, InterfaceThreads) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("(\\w+)=(\\d+)");
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Количество потоков исполнения сопоставления
	constexpr size_t count = 8;
	// Количество сопоставлений в каждом потоке исполнения
	constexpr size_t rounds = 1000;
	// Набор потоков исполнения сопоставления
	vector <thread> threads;
	// Набор количества совпадений каждого потока исполнения
	vector <size_t> matched(count, 0);
	// Выполняем размещение набора потоков исполнения
	threads.reserve(count);
	/**
	 * Выполняем запуск потоков исполнения сопоставления
	 *
	 * @details Собранное выражение после сборки не изменяется и разделяется
	 *          потоками исполнения, тогда как рабочее состояние сопоставления
	 *          хранится отдельно для каждого потока исполнения.
	 *
	 */
	for(size_t i = 0; i < count; i++) {
		// Выполняем запуск потока исполнения сопоставления
		threads.emplace_back([&regexp, &expression, &matched, i]() noexcept {
			/**
			 * Выполняем сопоставление выражения с текстом
			 */
			for(size_t j = 0; j < rounds; j++) {
				// Получаем текст совпадения и захваченных групп
				const auto & result = regexp.exec("параметр width=1024 задан", expression);
				/**
				 * Если текст совпадения и захваченных групп извлечён
				 */
				if((result.size() == 3) && (result.at(1).compare("width") == 0) && (result.at(2).compare("1024") == 0))
					// Увеличиваем количество совпадений потока исполнения
					matched.at(i)++;
			}
		});
	}
	/**
	 * Выполняем ожидание завершения потоков исполнения
	 */
	for(auto & item : threads)
		// Выполняем ожидание завершения потока исполнения
		item.join();
	/**
	 * Выполняем проверку количества совпадений каждого потока исполнения
	 */
	for(size_t i = 0; i < count; i++)
		// Выполняем проверку количества совпадений потока исполнения
		EXPECT_EQ(matched.at(i), rounds) << "Поток: " << i;
}
/**
 * @brief Проверка сопоставления порождённым машинным кодом несколькими потоками
 *
 * @details Заголовок движка объявляет владение сопоставителем разделяемым:
 *          порождённый код исполняется несколькими потоками одновременно,
 *          поскольку рабочего состояния он не несёт. Заявление это закреплено
 *          не было - проверка соседняя собирает выражение без признака «JIT»
 *          и оттого испытывает один лишь толкователь.
 *
 *          Проверка утверждает и сам факт порождения кода: без этого сужение
 *          подмножества, кодогенерацию получающего, обратило бы её во вторую
 *          проверку толкователя, зелёной при том оставшуюся.
 *
 */
TEST(Regex, InterfaceThreadsCodegen) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения в режиме порождения машинного кода
	const auto expression = regexp.build("(\\w+)=(\\d+)", static_cast <uint32_t> (regex::flag_t::JIT));
	// Выполняем проверку выполнения сборки регулярного выражения
	ASSERT_TRUE(static_cast <bool> (expression));
	// Выполняем проверку порождения сопоставителя в виде машинного кода
	ASSERT_NE(expression->machine, nullptr) << "машинный код выражением не порождён";
	// Количество потоков исполнения сопоставления
	constexpr size_t count = 8;
	// Количество сопоставлений в каждом потоке исполнения
	constexpr size_t rounds = 1000;
	// Набор потоков исполнения сопоставления
	vector <thread> threads;
	// Набор количества совпадений каждого потока исполнения
	vector <size_t> matched(count, 0);
	// Выполняем размещение набора потоков исполнения
	threads.reserve(count);
	/**
	 * Выполняем запуск потоков исполнения сопоставления
	 */
	for(size_t i = 0; i < count; i++) {
		// Выполняем запуск потока исполнения сопоставления
		threads.emplace_back([&regexp, &expression, &matched, i]() noexcept {
			/**
			 * Выполняем сопоставление выражения с текстом
			 */
			for(size_t j = 0; j < rounds; j++) {
				// Получаем текст совпадения и захваченных групп
				const auto & result = regexp.exec("параметр width=1024 задан", expression);
				/**
				 * Если текст совпадения и захваченных групп извлечён
				 */
				if((result.size() == 3) && (result.at(1).compare("width") == 0) && (result.at(2).compare("1024") == 0))
					// Увеличиваем количество совпадений потока исполнения
					matched.at(i)++;
			}
		});
	}
	/**
	 * Выполняем ожидание завершения потоков исполнения
	 */
	for(auto & item : threads)
		// Выполняем ожидание завершения потока исполнения
		item.join();
	/**
	 * Выполняем проверку количества совпадений каждого потока исполнения
	 */
	for(size_t i = 0; i < count; i++)
		// Выполняем проверку количества совпадений потока исполнения
		EXPECT_EQ(matched.at(i), rounds) << "Поток: " << i;
}
/**
 * @brief Тест соответствия классов POSIX режиму соответствия Юникоду
 *
 * @details Режим «UCP» заменяет наборы символов ASCII, задающие классы POSIX,
 *          свойствами Юникода - ровно так же, как поступает он с сокращёнными
 *          классами. Состав каждого класса сличён с эталонной реализацией по
 *          всем кодовым значениям, а тест закрепляет проверенные образцы,
 *          включая классы «punct», «graph», «print» и «xdigit», общей категории
 *          Юникода не отвечающие вовсе.
 *
 */
TEST(Regex, InterfacePosixUnicode) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	/**
	 * @brief Образец проверки класса символов POSIX
	 *
	 */
	struct sample_t {
		// Название класса символов POSIX
		const char * name;
		// Текст для сопоставления
		const char * text;
		// Ожидаемый результат под режимом соответствия Юникоду
		bool unicode;
		// Ожидаемый результат без режима соответствия Юникоду
		bool plain;
	};
	// Набор образцов проверки классов символов POSIX
	const sample_t samples[] = {
		{"alpha", "ы", true, false},
		{"alnum", "ы", true, false},
		{"alnum", "٤", true, false},
		{"digit", "٤", true, false},
		{"digit", "7", true, true},
		{"lower", "ы", true, false},
		{"upper", "Ы", true, false},
		{"upper", "ы", false, false},
		{"word", "ы", true, false},
		{"word", "_", true, true},
		{"space", "\xC2\x85", true, false},
		{"punct", "«", true, false},
		{"punct", "$", true, true},
		{"graph", "ы", true, false},
		{"print", "ы", true, false},
		{"print", " ", true, true},
		{"graph", " ", false, false},
		{"xdigit", "\xEF\xBC\x91", true, false},
		{"xdigit", "f", true, true},
		{"ascii", "ы", false, false},
		{"ascii", "z", true, true},
		{"blank", "\xC2\xA0", true, false}
	};
	/**
	 * Выполняем перебор набора образцов проверки классов символов POSIX
	 */
	for(const auto & sample : samples) {
		// Получаем текст выражения класса символов POSIX
		const string pattern = (string("^[[:") + sample.name + ":]]$");
		// Выполняем сборку выражения под режимом соответствия Юникоду
		const auto unicode = regexp.build(pattern, {regexp_t::flag_t::UTF, regexp_t::flag_t::UCP});
		// Выполняем проверку сборки выражения
		ASSERT_TRUE(!!unicode) << "Класс \"" << sample.name << "\" не собран";
		// Выполняем проверку соответствия текста классу символов POSIX
		EXPECT_EQ(regexp.test(sample.text, unicode), sample.unicode)
		 << "Класс \"" << sample.name << "\" под режимом Юникода на тексте \"" << sample.text << "\"";
		// Выполняем сборку выражения без режима соответствия Юникоду
		const auto plain = regexp.build(pattern);
		// Выполняем проверку сборки выражения
		ASSERT_TRUE(!!plain) << "Класс \"" << sample.name << "\" не собран";
		// Выполняем проверку соответствия текста классу символов POSIX
		EXPECT_EQ(regexp.test(sample.text, plain), sample.plain)
		 << "Класс \"" << sample.name << "\" без режима Юникода на тексте \"" << sample.text << "\"";
	}
	/**
	 * Выполняем проверку отрицания классов символов POSIX
	 */
	const auto negated = regexp.build("^[[:^alpha:]]$", {regexp_t::flag_t::UTF, regexp_t::flag_t::UCP});
	// Выполняем проверку сборки выражения
	ASSERT_TRUE(!!negated);
	// Выполняем проверку отрицания класса символов POSIX
	EXPECT_FALSE(regexp.test("ы", negated));
	EXPECT_TRUE(regexp.test("7", negated));
}
/**
 * @brief Тест отказа сопоставления по тексту с неверной записью UTF-8
 *
 * @details Под режимом «UTF» текст разбирается посимвольно, и запись, кодировке
 *          UTF-8 не отвечающая, разбору не поддаётся. Эталонная реализация
 *          отвечает на такой текст отказом, и модуль отвечает им же. Без режима
 *          «UTF» текст разбирается побайтно, и проверка не ведётся вовсе.
 *
 */
TEST(Regex, InterfaceBadUtf8Subject) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	// Выполняем сборку выражения под режимом разбора текста посимвольно
	const auto unicode = regexp.build("\\w+|.", {regexp_t::flag_t::UTF, regexp_t::flag_t::UCP});
	// Выполняем проверку сборки выражения
	ASSERT_TRUE(!!unicode);
	// Выполняем проверку сопоставления по тексту с правильной записью
	EXPECT_TRUE(regexp.test("узел", unicode));
	// Выполняем проверку отсутствия ошибки сопоставления
	EXPECT_EQ(regexp.error(), regexp_t::error_t::NONE);
	/**
	 * @brief Набор текстов с неверной записью UTF-8
	 *
	 */
	const vector <string> samples = {
		string("\xD1", 1),
		string("\xFF\xFE", 2),
		string("\xED\xA0\x80", 3),
		string("\xC0\xAF", 2)
	};
	/**
	 * Выполняем перебор набора текстов с неверной записью UTF-8
	 */
	for(const auto & sample : samples) {
		// Выполняем проверку отказа сопоставления по тексту
		EXPECT_FALSE(regexp.test(sample, unicode));
		// Выполняем проверку ошибки неверной записи текста сопоставления
		EXPECT_EQ(regexp.error(), regexp_t::error_t::BAD_UTF8_SUBJECT);
		// Набор границ совпадения и захваченных групп
		vector <pair <size_t, size_t>> bounds;
		// Выполняем проверку отказа извлечения границ совпадения
		EXPECT_FALSE(regexp.match(sample, unicode, bounds));
		// Выполняем проверку пустоты набора границ совпадения
		EXPECT_TRUE(bounds.empty());
	}
	/**
	 * Выполняем проверку снятия проверки записи режимом «UNCHECKED»
	 *
	 * @details Проверка проходит текст целиком, и проход по тексту повторными
	 *          вызовами от очередной позиции обходится квадратично, поэтому
	 *          там, где текст проверен единожды снаружи, проверка снимается.
	 */
	const auto unchecked = regexp.build("\\w+|.", {regexp_t::flag_t::UTF, regexp_t::flag_t::UCP, regexp_t::flag_t::UNCHECKED});
	// Выполняем проверку сборки выражения
	ASSERT_TRUE(!!unchecked);
	// Выполняем проверку отсутствия проверки записи текста
	EXPECT_TRUE(regexp.test(string("\xFF\xFE", 2), unchecked));
	// Выполняем сборку выражения без режима разбора текста посимвольно
	const auto plain = regexp.build("\\w+|.");
	// Выполняем проверку сборки выражения
	ASSERT_TRUE(!!plain);
	/**
	 * Выполняем проверку отсутствия проверки записи без режима «UTF»
	 */
	EXPECT_TRUE(regexp.test(string("\xFF\xFE", 2), plain));
}
/**
 * @brief Тест отказа разбора выражения с неправильной записью UTF-8
 *
 */
TEST(Regex, InterfaceMalformedUTF) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	/**
	 * @brief Набор выражений с записью UTF-8 неправильной
	 *
	 * @details Разбор ведётся с режимом UTF намеренно: без него байты
	 *          выражения разбираются поодиночке, и правильность записи
	 *          не проверяется вовсе. Всякий образец обязан быть отвергнут -
	 *          принятие его означало бы разбор по неправильной записи,
	 *          а он ведёт к кодовым значениям несообразным.
	 *
	 */
	const struct {
		const char * name;
		const string pattern;
	} samples[] = {
		{"обрыв двухбайтовой последовательности", string("a\xD0", 2)},
		{"негодный байт продолжения", string("a\xD0\x41", 3)},
		{"избыточная запись значения", string("a\xC0\x80", 3)},
		{"суррогатное значение", string("a\xED\xA0\x80", 4)},
		{"значение сверх предела", string("a\xF5\x80\x80\x80", 5)},
		{"одинокий байт продолжения", string("a\x80", 2)},
		{"обрыв последовательности в классе", string("[\xD0", 2)},
		{"суррогатное значение в классе", string("[\xED\xA0\x80]", 5)},
		{"обрыв последовательности в повторении", string("a{2,3}\xD0", 7)},
		{"негодный байт продолжения за экранированием", string("\\\xD0\x41", 3)}
	};
	/**
	 * Выполняем обход образцов записи неправильной
	 */
	for(const auto & sample : samples) {
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build(sample.pattern, {regexp_t::flag_t::UTF});
		// Выполняем проверку отказа сборки регулярного выражения
		EXPECT_FALSE(!!expression) << sample.name;
	}
	/**
	 * Выполняем проверку сборки выражения с записью правильной
	 *
	 * @details Проверка отсекает отказ огульный: разбор обязан отвергать
	 *          запись неправильную, а правильную принимать.
	 *
	 */
	{
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build("[а-яё]+", {regexp_t::flag_t::UTF});
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(!!expression);
		// Выполняем проверку наличия совпадения в тексте
		EXPECT_TRUE(regexp.test("привет", expression));
	}
}
/**
 * @brief Тест отказа разбора выражения с записью неверной
 *
 */
TEST(Regex, InterfaceMalformedSyntax) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	/**
	 * @brief Набор выражений с записью неверной
	 *
	 * @details Всякий образец обязан быть отвергнут разбором: обрыв записи
	 *          числа, значение сверх предела, свойство неведомое, класс POSIX
	 *          неведомый и ссылка на группу несуществующую - всё это ошибки
	 *          записи, и принятие их вело бы к выражению, значащему не то,
	 *          что записано.
	 *
	 */
	const char * samples[] = {
		"\\x{", "\\x{}", "\\x{12", "\\x{110000}", "\\x{FFFFFFFF}",
		"\\o{", "\\o{}", "\\o{777", "\\o{7777777777}",
		"\\p{", "\\p{}", "\\p{Nonesuch}", "\\p{L", "\\P{Nonesuch}",
		"\\N{", "\\N{U+", "\\N{U+110000}", // именование символа Юникода не поддерживается намеренно
		"[[:", "[[:nonesuch:]]", "[[:alpha]",
		"\\3", "(a)\\2", "(?3)", "(a)(?2)", "\\k<none>", "(?&none)"
	};
	/**
	 * Выполняем обход образцов записи неверной
	 */
	for(const char * pattern : samples) {
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build(pattern, {regexp_t::flag_t::UTF});
		// Выполняем проверку отказа сборки регулярного выражения
		EXPECT_FALSE(!!expression) << pattern;
	}
	/**
	 * @brief Набор выражений, образцам неверным сродных, но записанных верно
	 *
	 * @details Отсекает отказ огульный: разбор обязан отвергать запись
	 *          неверную, а сродную ей верную - принимать.
	 *
	 */
	const char * correct[] = {
		"\\x{41}", "\\o{101}", "\\p{L}", "\\P{L}",
		"[[:alpha:]]", "(a)\\1", "(a)(?1)", "(?<name>a)\\k<name>", "(?<name>a)(?&name)"
	};
	/**
	 * Выполняем обход образцов записи верной
	 */
	for(const char * pattern : correct) {
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build(pattern, {regexp_t::flag_t::UTF});
		// Выполняем проверку сборки регулярного выражения
		EXPECT_TRUE(!!expression) << pattern;
	}
}
/**
 * @brief Тест разбора выражений с записью пограничной
 *
 */
TEST(Regex, InterfaceMalformedTokens) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	/**
	 * @brief Набор выражений, разбором отвергаемых
	 *
	 */
	const struct {
		const char * name;
		const string pattern;
	} refused[] = {
		{"экранирование управляющего без символа", "\\c"},
		{"экранирование управляющего сверх ASCII", string("\\c\xD0\x90")},
		{"имя группы пустое", "(?<>a)"},
		{"имя группы, начатое цифрой", "(?<1a>a)"},
		{"имя группы без закрытия", "(?<name a)"},
		{"ссылка по имени без закрытия", "(?<name>a)\\k<name"},
		{"повторитель убывающий", "a{3,2}"},
		{"класс без закрытия", "[a-z"},
		{"диапазон класса убывающий", "[z-a]"},
		{"группа без закрытия", "(a"},
		{"скобка закрывающая лишняя", "a)"},
		{"свойство с именем непомерным", "\\p{" + string(200, 'X') + "}"},
		{"выражение длиною свыше предела", string(0x100001, 'a')}
	};
	/**
	 * Выполняем обход выражений, разбором отвергаемых
	 */
	for(const auto & sample : refused) {
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build(sample.pattern, {regexp_t::flag_t::UTF});
		// Выполняем проверку отказа сборки регулярного выражения
		EXPECT_FALSE(!!expression) << sample.name;
	}
	/**
	 * @brief Набор выражений с повторителем несостоявшимся
	 *
	 * @details Скобки, повторителя не образующие, разбираются как знаки
	 *          обычные - тем же порядком, что и у эталона. Проверка
	 *          закрепляет именно это: не отказ, а сопоставление текстом.
	 *
	 */
	const char * literals[] = {"a{2,3", "a{}", "a{,3}"};
	/**
	 * Выполняем обход выражений с повторителем несостоявшимся
	 */
	for(const char * pattern : literals) {
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build(pattern, {regexp_t::flag_t::UTF});
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(!!expression) << pattern;
		// Выполняем проверку сопоставления выражения с текстом его же
		EXPECT_TRUE(regexp.test(pattern, expression)) << pattern;
	}
}
/**
 * @brief Тест отказа сборки выражения, размах программы превышающего
 *
 */
TEST(Regex, InterfaceProgramLimit) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	/**
	 * @brief Набор выражений, размах программы превышающих
	 *
	 * @details Размах программы ограничен, и превышение его обязано быть
	 *          отвергнуто сборкой, а не обращено программой усечённой.
	 *          Превышается он тремя путями разными: длиною самого выражения,
	 *          повторением развёрнутым и повторением вложенным - отказ обязан
	 *          быть при всяком.
	 *
	 */
	/**
	 * @brief Набивка, подводящая программу к пределу вплотную
	 *
	 * @details Количество подобрано опытом: набивка сама по себе собирается,
	 *          а всякий хвост, к ней приставленный, предел уже превышает.
	 *          Знаки подряд для того негодны - сборка сводит их в узел строки
	 *          один, и предела ими не достать вовсе.
	 *
	 */
	constexpr const char * FILLER = "(?:(?:a|b){1000}){262}(?:a|b){141}";
	const struct {
		const char * name;
		const string pattern;
	} refused[] = {
		{"цепочка знаков сверх предела", string(0x50000, 'a')},
		{"повторение развёрнутое сверх предела", "(?:abcd){70000}"},
		{"повторение вложенное сверх предела", "(?:a{1000}){1000}"},
		/**
		 * @details Хвостовые образцы превышают предел не сами по себе,
		 *          а сборкою хвоста своего: набивка подводит программу
		 *          к пределу вплотную, и отказ случается уже внутри сборки
		 *          разветвления, повторения, проверки либо группы атомарной -
		 *          то есть на пути своём у всякого вида узла.
		 */
		{"разветвление у предела", string(FILLER) + "(?:x|y|z)"},
		{"повторение у предела", string(FILLER) + "(?:xy)+"},
		{"проверка у предела", string(FILLER) + "(?=xyz)w"},
		{"группа атомарная у предела", string(FILLER) + "(?>xyz)"},
		{"захват у предела", string(FILLER) + "(xyz)\\1"},
		{"класс у предела", string(FILLER) + "[a-z]{100}"},
		{"знак единственный у предела", string(FILLER) + "x"}
	};
	/**
	 * Выполняем обход выражений, размах программы превышающих
	 */
	for(const auto & sample : refused) {
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build(sample.pattern);
		// Выполняем проверку отказа сборки регулярного выражения
		EXPECT_FALSE(!!expression) << sample.name;
	}
	/**
	 * Выполняем проверку сборки выражения, предела не превышающего
	 *
	 * @details Отсекает отказ огульный: выражение, размахом в предел
	 *          вмещающееся, обязано быть собрано и сопоставлено.
	 *
	 */
	{
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build("(?:abcd){1000}");
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(!!expression);
		// Создаём текст из тысячи повторений
		string text;
		/**
		 * Выполняем сборку текста из тысячи повторений
		 */
		for(size_t i = 0; i < 1000; i++)
			// Выполняем добавление очередного повторения текста
			text.append("abcd");
		// Выполняем проверку наличия совпадения в тексте
		EXPECT_TRUE(regexp.test(text, expression));
	}
}
/**
 * @brief Тест предела длины ретроспективной проверки
 *
 */
TEST(Regex, InterfaceLookbehindBounds) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	/**
	 * @brief Набор выражений с ретроспективой длины беспредельной
	 *
	 * @details Ретроспектива сопоставляется от места назад, и без предела
	 *          длины отступать назад пришлось бы до начала текста при всяком
	 *          месте. Потому беспредельная отвергается сборкой - тем же
	 *          порядком, что и у эталона.
	 *
	 */
	const char * refused[] = {"(?<=a+)b", "(?<=ab*)c", "(?<=(?:ab)+)c", "(?<!a+)b"};
	/**
	 * Выполняем обход выражений с ретроспективой беспредельной
	 */
	for(const char * pattern : refused) {
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build(pattern, {regexp_t::flag_t::UTF});
		// Выполняем проверку отказа сборки регулярного выражения
		EXPECT_FALSE(!!expression) << pattern;
	}
	/**
	 * @brief Набор выражений с ретроспективой длины предельной
	 *
	 */
	const struct {
		const char * pattern;
		const char * text;
	} accepted[] = {
		{"(?<=a{1,4})b", "aaab"},
		{"(?<=ab)c", "abc"},
		{"(?<=a|bc)d", "bcd"},
		{"(?<!x)y", "ay"}
	};
	/**
	 * Выполняем обход выражений с ретроспективой предельной
	 */
	for(const auto & sample : accepted) {
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build(sample.pattern, {regexp_t::flag_t::UTF});
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(!!expression) << sample.pattern;
		// Выполняем проверку наличия совпадения в тексте
		EXPECT_TRUE(regexp.test(sample.text, expression)) << sample.pattern;
	}
}
/**
 * @brief Тест настройки глубины рекурсии и снятия кода ошибки фасадом
 *
 * @details Фасад настройку держит своим полем и ставит движку перед каждым
 *          сопоставлением, а код ошибки с движка снимает: без снятия отказ
 *          по исчерпанию объёма работы был бы неотличим от отсутствия
 *          совпадения, а с потребителя спрашивалось бы различать их
 *          гаданием.
 *
 */
TEST(Regex, InterfaceNestingAndError) {
	// Создаём объект работы с регулярными выражениями
	regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения с рекурсивным вызовом
	const auto expression = regexp.build("^(?<r>a(?&r)?b)$");
	// Выполняем проверку сборки регулярного выражения
	ASSERT_TRUE(!!expression);
	/**
	 * Выполняем проверку отсутствия ошибки при отсутствии совпадения
	 *
	 * @details Текст совпадения не даёт вовсе, и отказ этот ошибкою
	 *          не является: код обязан остаться пустым.
	 *
	 */
	{
		// Выполняем проверку отсутствия совпадения в тексте
		EXPECT_FALSE(regexp.test("zzz", expression));
		// Выполняем проверку отсутствия кода ошибки сопоставления
		EXPECT_EQ(regexp.error(), regex::error_t::NONE);
	}
	/**
	 * Выполняем проверку сопоставления при глубине умолчания
	 */
	{
		// Создаём текст сопоставления глубины, умолчанию отвечающей
		const string text = (string(300, 'a') + string(300, 'b'));
		// Выполняем проверку наличия совпадения в тексте
		EXPECT_TRUE(regexp.test(text, expression));
		// Выполняем проверку отсутствия кода ошибки сопоставления
		EXPECT_EQ(regexp.error(), regex::error_t::NONE);
	}
	/**
	 * Выполняем проверку отказа при глубине сверх установленной
	 *
	 * @details Отказ этот отсутствием совпадения не является: совпадение
	 *          в тексте есть, а движок глубины не осилил, - и код ошибки
	 *          обязан быть выставлен, иначе потребитель принял бы отказ
	 *          за отсутствие совпадения.
	 *
	 */
	{
		// Выполняем установку наибольшей допустимой глубины вызовов
		regexp.nesting(64);
		// Создаём текст сопоставления глубины сверх установленной
		const string text = (string(300, 'a') + string(300, 'b'));
		// Выполняем проверку отказа сопоставления текста
		EXPECT_FALSE(regexp.test(text, expression));
		// Выполняем проверку установки кода ошибки превышения объёма работы
		EXPECT_EQ(regexp.error(), regex::error_t::BUDGET_EXCEEDED);
		// Выполняем проверку установки текста ошибки сопоставления
		EXPECT_EQ(regexp.message(), "match budget exceeded");
	}
	/**
	 * Выполняем проверку восстановления глубины умолчания
	 */
	{
		// Выполняем восстановление наибольшей допустимой глубины вызовов
		regexp.nesting(0);
		// Создаём текст сопоставления глубины, умолчанию отвечающей
		const string text = (string(300, 'a') + string(300, 'b'));
		// Выполняем проверку наличия совпадения в тексте
		EXPECT_TRUE(regexp.test(text, expression));
		// Выполняем проверку отсутствия кода ошибки сопоставления
		EXPECT_EQ(regexp.error(), regex::error_t::NONE);
	}
}
/**
 * @brief Тест выдачи текста ошибки по всякому её коду
 *
 */
TEST(Regex, InterfaceMessages) {
	/**
	 * @brief Набор кодов ошибок модуля
	 *
	 * @details Перечень кодов выписан целиком намеренно: обход по значению
	 *          от нуля до последнего кода прошёл бы и мимо кода добавленного,
	 *          а выписанный перечень требует правки при всяком его пополнении.
	 *
	 */
	const regex::error_t ERRORS[] = {
		regex::error_t::NONE, regex::error_t::INTERNAL,
		regex::error_t::TRAILING_BACKSLASH, regex::error_t::UNKNOWN_ESCAPE,
		regex::error_t::UNMATCHED_PAREN, regex::error_t::UNMATCHED_BRACKET,
		regex::error_t::UNMATCHED_BRACE, regex::error_t::BAD_QUANTIFIER,
		regex::error_t::QUANTIFIER_NO_ATOM, regex::error_t::QUANTIFIER_TOO_BIG,
		regex::error_t::BAD_CLASS_RANGE, regex::error_t::BAD_ESCAPE_HEX,
		regex::error_t::BAD_ESCAPE_OCTAL, regex::error_t::BAD_PROPERTY,
		regex::error_t::BAD_POSIX_CLASS, regex::error_t::BAD_GROUP_SYNTAX,
		regex::error_t::BAD_GROUP_NAME, regex::error_t::DUPLICATE_NAME,
		regex::error_t::BAD_BACKREFERENCE, regex::error_t::BAD_CONDITION,
		regex::error_t::BAD_RECURSION, regex::error_t::BAD_OPTIONS,
		regex::error_t::NESTING_TOO_DEEP, regex::error_t::PATTERN_TOO_LARGE,
		regex::error_t::BAD_UTF8, regex::error_t::LOOKBEHIND_INVALID,
		regex::error_t::UNSUPPORTED, regex::error_t::BUDGET_EXCEEDED,
		regex::error_t::NESTED_RECURSION, regex::error_t::BAD_UTF8_SUBJECT
	};
	// Набор выданных текстов ошибок
	set <string> messages;
	/**
	 * Выполняем обход набора кодов ошибок модуля
	 */
	for(const regex::error_t error : ERRORS) {
		// Получаем текст ошибки по её коду
		const string message = regex::parser_t::message(error);
		// Выполняем проверку выдачи текста ошибки непустого
		EXPECT_FALSE(message.empty()) << static_cast <uint16_t> (error);
		/**
		 * Выполняем проверку выдачи текста ошибки осмысленного
		 *
		 * @details Заслон разбора кода неведомого выдаёт текст свой, и выдача
		 *          его по коду ведомому означала бы пропуск кода в перечне.
		 *
		 */
		EXPECT_NE(message, "unknown error") << static_cast <uint16_t> (error);
		// Выполняем добавление выданного текста ошибки в набор
		messages.emplace(message);
	}
	/**
	 * Выполняем проверку различия выданных текстов ошибок
	 *
	 * @details Текст ошибки выводится журналом потребителю, и совпадение
	 *          текстов у кодов различных лишает вывод смысла.
	 *
	 */
	EXPECT_EQ(messages.size(), (sizeof(ERRORS) / sizeof(ERRORS[0])));
	/**
	 * Выполняем проверку выдачи текста своего по коду неведомому
	 */
	EXPECT_EQ(regex::parser_t::message(static_cast <regex::error_t> (0xFF)), "unknown error");
}
/**
 * @brief Тест отказа сборки выражений негодных
 *
 */
TEST(Regex, InterfaceMalformedCorpus) {
	/**
	 * @brief Устройство образца выражения негодного
	 *
	 */
	const struct {
		// Собираемое регулярное выражение
		const char * pattern;
		// Ожидаемый код ошибки сборки
		regex::error_t error;
	} SAMPLES[] = {
		// Свойства Юникода и классы символов POSIX
		{"\\p{Zzz}", regex::error_t::BAD_PROPERTY},
		{"[\\p{Zzz}]", regex::error_t::BAD_PROPERTY},
		{"[[:zzz:]]", regex::error_t::BAD_POSIX_CLASS},
		{"[[:", regex::error_t::BAD_POSIX_CLASS},
		{"[[:alpha:", regex::error_t::BAD_POSIX_CLASS},
		// Диапазоны класса символов
		{"[z-a]", regex::error_t::BAD_CLASS_RANGE},
		{"[\\d-z]", regex::error_t::BAD_CLASS_RANGE},
		{"[a", regex::error_t::UNMATCHED_BRACKET},
		// Экранированные последовательности
		{"\\", regex::error_t::TRAILING_BACKSLASH},
		{"[a\\", regex::error_t::TRAILING_BACKSLASH},
		{"\\c", regex::error_t::UNKNOWN_ESCAPE},
		{"\\99", regex::error_t::UNKNOWN_ESCAPE},
		{"\\x{", regex::error_t::BAD_ESCAPE_HEX},
		{"\\x{ZZ}", regex::error_t::BAD_ESCAPE_HEX},
		{"\\o", regex::error_t::BAD_ESCAPE_OCTAL},
		{"\\o{", regex::error_t::BAD_ESCAPE_OCTAL},
		{"\\o{9}", regex::error_t::BAD_ESCAPE_OCTAL},
		{"\\N{ZZ}", regex::error_t::UNSUPPORTED},
		// Имена именованных групп
		{"(?<>a)", regex::error_t::BAD_GROUP_NAME},
		{"(?<1a>b)", regex::error_t::BAD_GROUP_NAME},
		{"(?<a", regex::error_t::BAD_GROUP_NAME},
		{"(?'", regex::error_t::BAD_GROUP_NAME},
		{"(?'name", regex::error_t::BAD_GROUP_NAME},
		{"(?'1a')", regex::error_t::BAD_GROUP_NAME},
		{"(?P<", regex::error_t::BAD_GROUP_NAME},
		{"(?P=zzz", regex::error_t::BAD_GROUP_NAME},
		{"\\k<", regex::error_t::BAD_GROUP_NAME},
		{"\\k<>", regex::error_t::BAD_GROUP_NAME},
		{"\\k{", regex::error_t::BAD_GROUP_NAME},
		{"\\k'", regex::error_t::BAD_GROUP_NAME},
		{"\\g<>", regex::error_t::BAD_GROUP_NAME},
		// Ссылки на группы
		{"(?<name>a)\\k<other>", regex::error_t::BAD_BACKREFERENCE},
		{"(?P>zzz)", regex::error_t::BAD_BACKREFERENCE},
		{"\\g", regex::error_t::BAD_BACKREFERENCE},
		{"\\g{", regex::error_t::BAD_BACKREFERENCE},
		{"\\g{0}", regex::error_t::BAD_BACKREFERENCE},
		{"\\g{99}", regex::error_t::BAD_BACKREFERENCE},
		{"(?&zzz)", regex::error_t::BAD_BACKREFERENCE},
		{"(?1)", regex::error_t::BAD_BACKREFERENCE},
		{"(?99)", regex::error_t::BAD_BACKREFERENCE},
		// Условные шаблоны
		{"(?(", regex::error_t::BAD_CONDITION},
		{"(?()a)", regex::error_t::BAD_GROUP_NAME},
		{"(?(zzz)a)", regex::error_t::BAD_BACKREFERENCE},
		{"(?(1a)b)", regex::error_t::BAD_CONDITION},
		{"(?(?=a)b|c|d)", regex::error_t::BAD_CONDITION},
		{"(?(<name>)a)", regex::error_t::BAD_BACKREFERENCE},
		{"(?('name')a)", regex::error_t::BAD_BACKREFERENCE},
		// Встроенные настройки и устройство группы
		{"(?i-zz)", regex::error_t::BAD_OPTIONS},
		{"(?i", regex::error_t::BAD_OPTIONS},
		{"(?-zzz)", regex::error_t::BAD_OPTIONS},
		{"(?zz)", regex::error_t::BAD_GROUP_SYNTAX},
		{"(?", regex::error_t::BAD_GROUP_SYNTAX},
		{"(?P", regex::error_t::BAD_GROUP_SYNTAX},
		{"(?+zzz)", regex::error_t::BAD_GROUP_SYNTAX},
		{"(?~a)", regex::error_t::BAD_GROUP_SYNTAX},
		{"(?{code})", regex::error_t::BAD_GROUP_SYNTAX},
		{"(?C1)", regex::error_t::UNSUPPORTED},
		{"(?R99)", regex::error_t::BAD_RECURSION},
		// Непарные скобки
		{"a)", regex::error_t::UNMATCHED_PAREN},
		{"(a", regex::error_t::UNMATCHED_PAREN},
		{"(?#", regex::error_t::UNMATCHED_PAREN},
		{"(?<=a", regex::error_t::UNMATCHED_PAREN},
		{"(?|", regex::error_t::UNMATCHED_PAREN},
		// Кванторы повторения
		{"a{2,1}", regex::error_t::BAD_QUANTIFIER},
		{"a**", regex::error_t::BAD_QUANTIFIER},
		{"a{99999999}", regex::error_t::QUANTIFIER_TOO_BIG},
		{"*a", regex::error_t::QUANTIFIER_NO_ATOM},
		{"+a", regex::error_t::QUANTIFIER_NO_ATOM},
		{"?a", regex::error_t::QUANTIFIER_NO_ATOM},
		/**
		 * Глагол управления, набором не заведённый
		 *
		 * @details Отказ давался прежде отсутствием элемента повторяемого:
		 *          скобка глагола разбиралась группою, а знак его - квантором
		 *          при пустом её начале. С заведением разбора глаголов отказ
		 *          даётся доводом неподдерживаемой конструкции, что и ближе
		 *          к доводу эталона - «(*VERB) not recognized or malformed».
		 *
		 */
		{"(*)", regex::error_t::UNSUPPORTED},
		{"(?i)*", regex::error_t::QUANTIFIER_NO_ATOM},
		{"^*", regex::error_t::QUANTIFIER_NO_ATOM},
		{"|*", regex::error_t::QUANTIFIER_NO_ATOM},
		{"\\b*", regex::error_t::QUANTIFIER_NO_ATOM},
		{"\\A*", regex::error_t::QUANTIFIER_NO_ATOM},
		{"\\Z*", regex::error_t::QUANTIFIER_NO_ATOM},
		{"\\G*", regex::error_t::QUANTIFIER_NO_ATOM},
		{"(?:*)", regex::error_t::QUANTIFIER_NO_ATOM},
		{"\\Q\\E*", regex::error_t::QUANTIFIER_NO_ATOM},
		{"a{1}{2}", regex::error_t::QUANTIFIER_NO_ATOM},
		// Свойства, классы и ссылки, вторым щупом добытые
		{"\\p{}", regex::error_t::BAD_PROPERTY},
		{"\\p", regex::error_t::BAD_PROPERTY},
		{"[[:^zzz:]]", regex::error_t::BAD_POSIX_CLASS},
		{"[a-\\d]", regex::error_t::BAD_CLASS_RANGE},
		{"[]", regex::error_t::UNMATCHED_BRACKET},
		{"[^]", regex::error_t::UNMATCHED_BRACKET},
		{"[\\q]", regex::error_t::UNKNOWN_ESCAPE},
		{"\\k", regex::error_t::BAD_GROUP_NAME},
		{"\\kx", regex::error_t::BAD_GROUP_NAME},
		{"(?P>", regex::error_t::BAD_GROUP_NAME},
		{"(?P>)", regex::error_t::BAD_GROUP_NAME},
		{"\\g{+}", regex::error_t::BAD_BACKREFERENCE},
		{"\\g{-}", regex::error_t::BAD_BACKREFERENCE},
		{"\\g{1x", regex::error_t::BAD_BACKREFERENCE},
		{"\\g{-1}", regex::error_t::BAD_BACKREFERENCE},
		{"\\g+", regex::error_t::BAD_BACKREFERENCE},
		{"\\g-", regex::error_t::BAD_BACKREFERENCE},
		{"\\g<-1>", regex::error_t::BAD_BACKREFERENCE},
		{"\\g<+1>", regex::error_t::BAD_BACKREFERENCE},
		{"\\g{99999999999}", regex::error_t::BAD_BACKREFERENCE},
		// Проверки окружения и рекурсивные вызовы
		{"(?=a\\K)", regex::error_t::UNSUPPORTED},
		{"(?<=a\\K)", regex::error_t::UNSUPPORTED},
		{"(?<=a*)", regex::error_t::LOOKBEHIND_INVALID},
		{"(?<!a+)", regex::error_t::LOOKBEHIND_INVALID},
		{"(?1x", regex::error_t::BAD_RECURSION},
		{"(?-1)", regex::error_t::BAD_RECURSION},
		{"(?+0)", regex::error_t::BAD_RECURSION},
		{"(?-0)", regex::error_t::BAD_RECURSION},
		{"(?+)", regex::error_t::BAD_GROUP_SYNTAX},
		// Условные шаблоны, вторым щупом добытые
		{"(?(?i)a|b)", regex::error_t::BAD_CONDITION},
		{"(?(-)a)", regex::error_t::BAD_CONDITION},
		{"(?(-1)a)", regex::error_t::BAD_CONDITION},
		{"(?(+)a)", regex::error_t::BAD_CONDITION},
		{"(?(R&zzz)a)", regex::error_t::BAD_BACKREFERENCE},
		{"(?(1)a|b)", regex::error_t::BAD_BACKREFERENCE},
		{"(?(1)a|b)(?:x)", regex::error_t::BAD_BACKREFERENCE},
		// Встроенные настройки с повторным знаком снятия
		{"(?-i-m)", regex::error_t::BAD_OPTIONS},
		{"(?i-i-m)", regex::error_t::BAD_OPTIONS}
	};
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	/**
	 * Выполняем обход набора образцов выражений негодных
	 */
	for(auto & sample : SAMPLES) {
		// Выполняем сборку очередного регулярного выражения
		const auto expression = regexp.build(sample.pattern);
		// Выполняем проверку отказа сборки регулярного выражения
		EXPECT_FALSE(expression) << sample.pattern;
		/**
		 * Выполняем проверку установки кода ошибки сборки
		 *
		 * @details Проверка утверждает не отказ один, а повод его: отказ
		 *          с кодом чужим доносит потребителю не то, что случилось,
		 *          и правку выражения его не направляет.
		 *
		 */
		EXPECT_EQ(regexp.error(), sample.error)
		 << sample.pattern << ": " << regexp.message();
	}
}
/**
 * @brief Тест отказа сборки выражений с ломаной последовательностью UTF-8
 *
 */
TEST(Regex, InterfaceMalformedPattern) {
	/**
	 * @brief Набор выражений с ломаной последовательностью UTF-8
	 *
	 * @details Ломаный байт ставится в местах разбора различных: в тексте
	 *          простом, внутри класса символов, концом диапазона, внутри
	 *          дословной последовательности и одиночным байтом выражения.
	 *
	 */
	const string SAMPLES[] = {
		(string("a\xFF") + "b"), (string("[a\xFF") + "]"),
		(string("[a-\xFF") + "]"), (string("\\Q\xFF") + "\\E"),
		string("\xFF"), string("\xC3"), (string("[\xC3") + "]")
	};
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	/**
	 * Выполняем обход набора выражений с ломаной последовательностью
	 */
	for(const string & pattern : SAMPLES) {
		// Выполняем сборку очередного регулярного выражения
		const auto expression = regexp.build(pattern, {regexp_t::flag_t::UTF});
		// Выполняем проверку отказа сборки регулярного выражения
		EXPECT_FALSE(expression);
		// Выполняем проверку установки кода ошибки ломаной последовательности
		EXPECT_EQ(regexp.error(), regex::error_t::BAD_UTF8) << regexp.message();
	}
}
/**
 * @brief Тест отказа сборки выражения с вложенностью сверх допустимой
 *
 */
TEST(Regex, InterfaceNestingDepth) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	// Собираемое выражение с вложенностью сверх допустимой
	string deep;
	/**
	 * Выполняем сборку выражения с вложенностью сверх допустимой
	 */
	for(size_t i = 0; i < 4000; i++)
		// Выполняем добавление очередной открывающей скобки
		deep.append("(");
	/**
	 * Выполняем закрытие открытых скобок выражения
	 */
	for(size_t i = 0; i < 4000; i++)
		// Выполняем добавление очередной закрывающей скобки
		deep.append(")");
	// Выполняем проверку отказа сборки регулярного выражения
	EXPECT_FALSE(regexp.build(deep));
	// Выполняем проверку установки кода ошибки превышения вложенности
	EXPECT_EQ(regexp.error(), regex::error_t::NESTING_TOO_DEEP) << regexp.message();
	/**
	 * Выполняем проверку сборки выражения с вложенностью допустимой
	 *
	 * @details Проверка утверждает не отказ один: заслон, отвергающий и
	 *          вложенность допустимую, прошёл бы проверку отказа целиком.
	 *
	 */
	EXPECT_TRUE(regexp.build(string(64, '(') + "a" + string(64, ')')));
}
/**
 * @brief Тест проверки существования группы, условием указанной вперёд
 *
 */
TEST(Regex, InterfaceConditionForward) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	/**
	 * Выполняем проверку отказа при группе, вперёд указанной и не объявленной
	 *
	 * @details Номер группы, заданный относительно текущей позиции вперёд,
	 *          указывает на группу, объявляемую позже, и существование её
	 *          проверяется по завершении разбора. Проверка эта велась
	 *          для ссылок и рекурсивных вызовов, а для условий не велась:
	 *          выражение принималось, а условие считалось невыполненным.
	 *          Эталон PCRE2 такое выражение отвергает.
	 *
	 */
	EXPECT_FALSE(regexp.build("(?(+1)a|b)"));
	// Выполняем проверку установки кода ошибки ссылки на несуществующую группу
	EXPECT_EQ(regexp.error(), regex::error_t::BAD_BACKREFERENCE) << regexp.message();
	/**
	 * Выполняем проверку сборки при группе, вперёд указанной и объявленной
	 */
	{
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build("(?(+1)a|b)(c)");
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(expression);
		// Выполняем проверку наличия совпадения в тексте
		EXPECT_TRUE(regexp.test("bc", expression));
	}
	/**
	 * Выполняем проверку отказа при номере прямом сверх количества групп
	 *
	 * @details Вид задания номера различия не делает: эталон PCRE2 условие
	 *          принимает ровно тогда, когда номер не превышает общего
	 *          количества групп выражения.
	 *
	 */
	EXPECT_FALSE(regexp.build("(?(2)a|b)(x)"));
	// Выполняем проверку установки кода ошибки ссылки на несуществующую группу
	EXPECT_EQ(regexp.error(), regex::error_t::BAD_BACKREFERENCE) << regexp.message();
	/**
	 * Выполняем проверку сборки при номере прямом, группе отвечающем
	 *
	 * @details Проверка утверждает не отказ один: заслон, отвергающий всякое
	 *          условие по номеру, прошёл бы проверки отказа целиком.
	 *
	 */
	{
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build("(?(2)a|b)(x)(y)");
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(expression);
		// Выполняем проверку наличия совпадения в тексте
		EXPECT_TRUE(regexp.test("bxy", expression));
	}
}
/**
 * @brief Тест связи квантора повторения с элементом своего вызова
 *
 */
TEST(Regex, InterfaceQuantifierBinding) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	/**
	 * @brief Набор выражений с квантором в начале ветви
	 *
	 * @details Примечание, завершение экранирования и дословная
	 *          последовательность пустая символов не несут и элемента выражения
	 *          не образуют, отчего квантор за ними применяется к элементу
	 *          предшествующему. Элемент этот обязан принадлежать вызову своему:
	 *          ряд состава общий на все вызовы, и сличение его с пустотою
	 *          связывало квантор с элементом ветви соседней либо
	 *          последовательности объемлющей.
	 *
	 *          Встроенные же настройки символов связь эту разрывают: квантор
	 *          за ними считается ошибкой, и барьер этот элементом прозрачным
	 *          не снимается. Все три правила сверены с эталоном PCRE2
	 *          восемнадцатью выражениями.
	 *
	 */
	const char * REFUSED[] = {
		"a|(?#c)+", "a|(?i)+", "a|\\Q\\E+", "(a)((?#c)+)",
		"a|(?#c)*", "(?:a|(?i)?)", "a(?:(?#c)+)",
		"a(?i)+", "a(?i)\\Q\\E+", "a(?i)(?#c)+", "a\\Q\\E(?i)+",
		"(?:a(?i)*)", "(?)*"
	};
	/**
	 * Выполняем обход набора выражений с квантором в начале ветви
	 */
	for(const char * pattern : REFUSED) {
		// Выполняем проверку отказа сборки регулярного выражения
		EXPECT_FALSE(regexp.build(pattern)) << pattern;
		// Выполняем проверку установки кода ошибки кванторов повторения
		EXPECT_EQ(regexp.error(), regex::error_t::QUANTIFIER_NO_ATOM)
		 << pattern << ": " << regexp.message();
	}
	/**
	 * @brief Набор выражений с квантором за элементом своего вызова
	 *
	 */
	const char * ACCEPTED[] = {
		"a(?#c)+", "a\\E+", "a\\Q\\E+", "a\\Qb\\E+", "b|a(?#c)+",
		"(a(?#c)+)", "(a(?#c)+b)", "(?:)*"
	};
	/**
	 * Выполняем обход набора выражений с квантором за элементом своего вызова
	 *
	 * @details Проверка утверждает не отказ один: заслон, отвергающий квантор
	 *          за всяким примечанием, прошёл бы проверки отказа целиком.
	 *
	 */
	for(const char * pattern : ACCEPTED)
		// Выполняем проверку сборки регулярного выражения
		EXPECT_TRUE(regexp.build(pattern)) << pattern << ": " << regexp.message();
}
/**
 * @brief Тест прозрачных последовательностей в начале класса символов
 *
 */
TEST(Regex, InterfaceClassTransparent) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	/**
	 * @brief Набор классов символов незавершённых
	 *
	 * @details Завершение экранирования и дословная последовательность пустая
	 *          символов не несут и элементами класса не являются, отчего знак
	 *          отрицания, за ними стоящий, отрицанием и остаётся, а скобка
	 *          закрывающая - первым элементом класса. Опознание отрицания
	 *          до пропуска этих последовательностей обращало «[\Q\E^]»
	 *          в класс из знака отрицания, эталоном же он отвергается.
	 *
	 */
	const char * REFUSED[] = {
		"[\\Q\\E^]", "[\\E^]", "[^\\Q\\E]", "[^\\E]", "[\\Q\\E]", "[\\E]"
	};
	/**
	 * Выполняем обход набора классов символов незавершённых
	 */
	for(const char * pattern : REFUSED) {
		// Выполняем проверку отказа сборки регулярного выражения
		EXPECT_FALSE(regexp.build(pattern)) << pattern;
		// Выполняем проверку установки кода ошибки незавершённого класса
		EXPECT_EQ(regexp.error(), regex::error_t::UNMATCHED_BRACKET)
		 << pattern << ": " << regexp.message();
	}
	/**
	 * @brief Набор классов символов завершённых
	 *
	 * @details Проверка утверждает не отказ один: заслон, отвергающий всякий
	 *          класс с последовательностью прозрачной, прошёл бы проверки
	 *          отказа целиком.
	 *
	 */
	const char * ACCEPTED[] = {
		"[\\Q\\E^a]", "[\\E^a]", "[\\Q\\E^]]", "[^\\Q\\E]]", "[\\Q\\Ea]", "[\\Q\\E]a]"
	};
	/**
	 * Выполняем обход набора классов символов завершённых
	 */
	for(const char * pattern : ACCEPTED)
		// Выполняем проверку сборки регулярного выражения
		EXPECT_TRUE(regexp.build(pattern)) << pattern << ": " << regexp.message();
	/**
	 * Выполняем проверку смысла отрицания за последовательностью прозрачной
	 *
	 * @details Отрицание обязано отрицать, а не стать знаком класса: сборка
	 *          принятая ещё не означает разбора верного.
	 *
	 */
	{
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build("[\\Q\\E^a]");
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(expression);
		// Выполняем проверку отсутствия совпадения знака отрицаемого
		EXPECT_FALSE(regexp.test("a", expression));
		// Выполняем проверку наличия совпадения знака иного
		EXPECT_TRUE(regexp.test("b", expression));
		/**
		 * Выполняем проверку наличия совпадения знака отрицания
		 *
		 * @details Знак отрицания знаком класса не стал, а отрицанием остался,
		 *          отчего класс есть «[^a]» и знаку «^» отвечает. Ожидание это
		 *          снято с эталона, а не выведено рассуждением.
		 *
		 */
		EXPECT_TRUE(regexp.test("^", expression));
	}
}
/**
 * @brief Тест признака отведения привязке одного конца текста
 *
 */
TEST(Regex, InterfaceDollarEnd) {
	/**
	 * @brief Устройство образца проверки привязки
	 *
	 */
	const struct {
		// Собираемое регулярное выражение
		const char * pattern;
		// Текст сопоставления
		const char * text;
		// Ожидаемое наличие совпадения
		bool matched;
		// Ожидаемая начальная граница совпадения
		size_t begin;
	} SAMPLES[] = {
		// Признак отменяет соответствие привязки переводу строки завершающему
		{"$", "\n", true, 1},
		{"$", "a\n", true, 2},
		{"a$", "a\n", false, 0},
		{"$", "a", true, 1},
		{"a$", "a", true, 0}
	};
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	/**
	 * Выполняем обход набора образцов проверки привязки
	 *
	 * @details Признак «DOLLAR_END» был объявлен открытым API, а не читался
	 *          нигде: потребитель его устанавливал, а получал поведение
	 *          обыкновенное - без отказа и без предупреждения. Сличение
	 *          с эталоном по режимам сборки его и вскрыло.
	 *
	 */
	for(auto & sample : SAMPLES) {
		/**
		 * Выполняем обход способов сопоставления образца
		 */
		for(const bool jit : {false, true}) {
			// Собираем набор признаков сборки выражения
			vector <regexp_t::flag_t> flags = {regexp_t::flag_t::DOLLAR_END};
			/**
			 * Если выражение собирается с порождением машинного кода
			 */
			if(jit)
				// Выполняем добавление признака порождения машинного кода
				flags.push_back(regexp_t::flag_t::JIT);
			// Выполняем сборку регулярного выражения
			const auto expression = regexp.build(sample.pattern, flags);
			// Выполняем проверку сборки регулярного выражения
			ASSERT_TRUE(expression) << sample.pattern;
			// Выполняем сопоставление текста регулярным выражением
			const auto bounds = regexp.match(sample.text, expression);
			// Выполняем проверку наличия совпадения в тексте
			ASSERT_EQ(!bounds.empty(), sample.matched)
			 << sample.pattern << " на «" << sample.text << "»" << (jit ? ", машинный код" : "");
			/**
			 * Если совпадение в тексте обнаружено
			 */
			if(sample.matched)
				// Выполняем проверку начальной границы совпадения
				EXPECT_EQ(bounds.front().first, sample.begin)
				 << sample.pattern << " на «" << sample.text << "»" << (jit ? ", машинный код" : "");
		}
	}
	/**
	 * Выполняем проверку бездействия признака при границах строк
	 *
	 * @details Эталон PCRE2 признак этот при режиме соответствия привязок
	 *          границам строк не применяет вовсе, и модуль поступает так же.
	 *
	 */
	{
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build("a$",
		 {regexp_t::flag_t::DOLLAR_END, regexp_t::flag_t::MULTILINE});
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(expression);
		// Выполняем проверку наличия совпадения в тексте
		EXPECT_TRUE(regexp.test("a\n", expression));
	}
	/**
	 * Выполняем проверку поведения обыкновенного без признака
	 *
	 * @details Проверка утверждает не отказ один: привязка, конца текста
	 *          требующая всегда, прошла бы проверки выше целиком.
	 *
	 */
	{
		// Выполняем сборку регулярного выражения
		const auto expression = regexp.build("a$");
		// Выполняем проверку сборки регулярного выражения
		ASSERT_TRUE(expression);
		// Выполняем проверку наличия совпадения в тексте
		EXPECT_TRUE(regexp.test("a\n", expression));
	}
}
/**
 * @brief Тест разрядов свойств Юникода без учёта регистра символов
 *
 */
TEST(Regex, InterfaceCaselessProperty) {
	/**
	 * @brief Устройство образца проверки разряда свойства
	 *
	 */
	const struct {
		// Собираемое регулярное выражение
		const char * pattern;
		// Текст сопоставления
		const char * text;
		// Ожидаемое совпадение с учётом регистра
		bool strict;
		// Ожидаемое совпадение без учёта регистра
		bool caseless;
	} SAMPLES[] = {
		// Разряды букв прописных, строчных и с заглавной первой частью
		{"\\p{Lu}", "а", false, true},
		{"\\p{Ll}", "А", false, true},
		{"[\\p{Lu}]", "а", false, true},
		{"\\p{Lt}", "a", false, true},
		{"\\p{Lt}", "A", false, true},
		{"\\p{Lu}", "ǅ", false, true},
		{"\\p{Ll}", "ǅ", false, true},
		// Буква без пары прописной разряду прописному всё же отвечает
		{"\\p{Lu}", "ﬁ", false, true},
		{"\\p{Lu}", "ı", false, true},
		// Разряды, регистра не меняющие, признаком не задеваются
		{"\\p{Lm}", "a", false, false},
		{"\\p{Lu}", "ʰ", false, false},
		{"\\p{Lu}", "5", false, false},
		{"\\p{Nd}", "5", true, true},
		{"\\p{L}", "а", true, true}
	};
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	/**
	 * Выполняем обход набора образцов проверки разряда свойства
	 *
	 * @details Разряды букв прописных, строчных и с заглавной первой частью
	 *          без учёта регистра взаимозаменяемы: эталон PCRE2 подменяет
	 *          всякий из них разрядом букв, регистр меняющих. Свёртка сама
	 *          тут негодна: лигатура «ﬁ» пары прописной не имеет вовсе,
	 *          а разряду прописному без учёта регистра отвечает.
	 *
	 */
	for(auto & sample : SAMPLES) {
		/**
		 * Выполняем обход способов сопоставления образца
		 */
		for(const bool jit : {false, true}) {
			/**
			 * Выполняем обход режимов учёта регистра символов
			 */
			for(const bool caseless : {false, true}) {
				// Собираем набор признаков сборки выражения
				vector <regexp_t::flag_t> flags = {regexp_t::flag_t::UTF, regexp_t::flag_t::UCP};
				/**
				 * Если сопоставление ведётся без учёта регистра символов
				 */
				if(caseless)
					// Выполняем добавление признака учёта регистра символов
					flags.push_back(regexp_t::flag_t::CASELESS);
				/**
				 * Если выражение собирается с порождением машинного кода
				 */
				if(jit)
					// Выполняем добавление признака порождения машинного кода
					flags.push_back(regexp_t::flag_t::JIT);
				// Выполняем сборку регулярного выражения
				const auto expression = regexp.build(sample.pattern, flags);
				// Выполняем проверку сборки регулярного выражения
				ASSERT_TRUE(expression) << sample.pattern;
				// Выполняем проверку наличия совпадения в тексте
				EXPECT_EQ(regexp.test(sample.text, expression),
				 (caseless ? sample.caseless : sample.strict))
				 << sample.pattern << " на «" << sample.text << "»"
				 << (caseless ? ", без учёта регистра" : "") << (jit ? ", машинный код" : "");
			}
		}
	}
}
/**
 * @brief Тест атомарности последовательности перевода строки
 *
 */
TEST(Regex, InterfaceLineBreakAtomic) {
	/**
	 * @brief Устройство образца проверки последовательности
	 *
	 */
	const struct {
		// Собираемое регулярное выражение
		const char * pattern;
		// Текст сопоставления
		const char * text;
		// Ожидаемое наличие совпадения
		bool matched;
		// Ожидаемая конечная граница совпадения
		size_t finish;
	} SAMPLES[] = {
		// Пара возврата каретки с переводом строки разрыву не подлежит
		{"\\R\\v", "\r\n", false, 0},
		{"\\R\\n", "\r\n", false, 0},
		{"\\R\\R", "\r\n", false, 0},
		// Сама последовательность паре отвечает целиком
		{"\\R", "\r\n", true, 2},
		{"\\R+", "\r\n", true, 2},
		{"\\Rb", "\r\nb", true, 3},
		{"\\R+b", "\r\n\r\nb", true, 5},
		// Знаки перевода строки одиночные последовательности отвечают
		{"\\R", "\n", true, 1},
		{"\\R", "\r", true, 1},
		{"\\R\\R", "\n\r", true, 2}
	};
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	/**
	 * Выполняем обход набора образцов проверки последовательности
	 *
	 * @details Последовательность «\R» атомарна: возврат внутрь неё запрещён,
	 *          и пара «CRLF» разрыву не подлежит. Прежде она собиралась
	 *          выбором одной из ветвей без атомарности, отчего «\R\v»
	 *          сопоставлялся паре - «\R» брал возврат каретки, а «\v» перевод
	 *          строки, - чему эталон PCRE2 отказывает.
	 *
	 */
	for(auto & sample : SAMPLES) {
		/**
		 * Выполняем обход способов сопоставления образца
		 */
		for(const bool jit : {false, true}) {
			// Собираем набор признаков сборки выражения
			vector <regexp_t::flag_t> flags;
			/**
			 * Если выражение собирается с порождением машинного кода
			 */
			if(jit)
				// Выполняем добавление признака порождения машинного кода
				flags.push_back(regexp_t::flag_t::JIT);
			// Выполняем сборку регулярного выражения
			const auto expression = regexp.build(sample.pattern, flags);
			// Выполняем проверку сборки регулярного выражения
			ASSERT_TRUE(expression) << sample.pattern;
			// Выполняем сопоставление текста регулярным выражением
			const auto bounds = regexp.match(sample.text, expression);
			// Выполняем проверку наличия совпадения в тексте
			ASSERT_EQ(!bounds.empty(), sample.matched)
			 << sample.pattern << (jit ? ", машинный код" : "");
			/**
			 * Если совпадение в тексте обнаружено
			 */
			if(sample.matched)
				// Выполняем проверку конечной границы совпадения
				EXPECT_EQ(bounds.front().second, sample.finish)
				 << sample.pattern << (jit ? ", машинный код" : "");
		}
	}
}
/**
 * @brief Тест разрыва графемного кластера у связки индийских письменностей
 *
 */
TEST(Regex, InterfaceGraphemeConjunct) {
	// Создаём объект работы с регулярными выражениями
	const regexp_t regexp(::logger());
	// Выполняем сборку регулярного выражения
	const auto expression = regexp.build("^\\X",
	 {regexp_t::flag_t::UTF, regexp_t::flag_t::UCP});
	// Выполняем проверку сборки регулярного выражения
	ASSERT_TRUE(expression);
	// Выполняем сопоставление связки согласных деванагари
	const auto bounds = regexp.match("\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7", expression);
	// Выполняем проверку наличия совпадения в тексте
	ASSERT_FALSE(bounds.empty());
	/**
	 * Выполняем проверку неразрывности связки согласных
	 *
	 * @details Правило «GB9c» стандарта UAX #29, введённое изданием 15.1,
	 *          запрещает разрыв связки «согласный - соединитель - согласный»:
	 *          три знака эти образуют один графемный кластер длиною девять
	 *          байтов. Эталон PCRE2 правила этого не применяет и разрывает
	 *          связку после соединителя, хотя таблицы несёт Юникода 17.0.
	 *          Расхождение это намеренно и записано: модуль следует стандарту.
	 *
	 */
	EXPECT_EQ(bounds.front().second, static_cast <size_t> (9));
}
