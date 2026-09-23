/**
 * @file entryref.cpp
 * @date 2026-09-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп платы за вызов и за попытку у эталона PCRE2 без машинного кода
 *
 * @details Двойник щупа «entrycost»: тот же порядок замера, те же выражения
 *          и тексты, но меряется разбор эталона - сопоставление с признаком
 *          «PCRE2_NO_JIT». Числа двух щупов сличимы строка в строку и отвечают
 *          на вопрос, какая статья платы у нас дороже, чем у эталона.
 *
 *          Эталон зовёт свой исполнитель «match()» на КАЖДОЙ позиции начала,
 *          как и мы, - цикл попыток у него тоже снаружи. Ячеек захвата он
 *          при этом не сбрасывает: «Foffset_top = 0» помечает их пустыми
 *          одним присвоением.
 *
 * @note Эталон собирается общим сценарием стендов сличения, а не системный:
 *       версия системного случайна.
 *
 * Сборка и запуск:
 *   REF=$(sh/reference/pcre2.sh /tmp/awh-reference-pcre2)
 *   c++ -std=c++17 -O2 -DPCRE2_STATIC -I "$(echo "$REF" | head -1)" \
 *    tools/regex/entryref.cpp "$(echo "$REF" | tail -1)" -o /tmp/entryref && /tmp/entryref
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Эталон подключается восьмиразрядным
 */
#define PCRE2_CODE_UNIT_WIDTH 8

/**
 * Подключаем заголовочный файл эталона
 */
#include <pcre2.h>

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstdio>
#include <string>
#include <cstdint>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Короткий текст набора замеров
 *
 * @details Снят дословно с набора: обмен по протоколу HTTP
 *
 */
static const string SHORT = (
	"GET /index.html HTTP/1.1\r\nHost: 192.168.001.100\r\n"
	"Content-Length: 4096\r\nUser-Agent: forman@anyks.com\r\n"
);

/**
 * @brief Функция получения текста набора среднего размера
 *
 * @return текст сопоставления среднего размера, снятый с набора дословно
 *
 */
static const string & medium() noexcept {
	// Текст сопоставления среднего размера
	static const string result = []() noexcept -> string {
		// Создаём основу текста сопоставления
		string outcome;
		// Номер порождаемого обмена по протоколу
		size_t number = 0;
		/**
		 * Выполняем наполнение текста до заданной длины
		 */
		while(outcome.size() < 2048) {
			// Выполняем добавление строки запроса обмена по протоколу
			outcome.append("GET /api/v1/items/");
			// Выполняем добавление номера запрашиваемого ресурса
			outcome.append(to_string(number++));
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
 * @brief Функция замера лучшего из кругов разбора эталона
 *
 * @param code    собранное выражение эталона
 * @param data    блок итогов сопоставления эталона
 * @param text    текст сопоставления
 * @param repeats количество повторений в круге
 * @return        лучшее время одного повторения в наносекундах
 *
 */
static double fastest(const pcre2_code * code, pcre2_match_data * data, const string & text, const size_t repeats) noexcept {
	// Лучшее время повторения
	double result = 0.;
	/**
	 * Выполняем круги замера
	 */
	for(uint32_t round = 0; round < 12; round++) {
		// Получаем отметку времени начала круга
		const auto begin = chrono::steady_clock::now();
		/**
		 * Выполняем повторения сопоставления разбором эталона
		 */
		for(size_t i = 0; i < repeats; i++)
			// Выполняем сопоставление без порождённого кода
			::pcre2_match(code, reinterpret_cast <PCRE2_SPTR> (text.data()), text.size(), 0, PCRE2_NO_JIT, data, nullptr);
		// Получаем время одного повторения в наносекундах
		const double spent = (static_cast <double> (chrono::duration_cast <chrono::nanoseconds> (
		 chrono::steady_clock::now() - begin).count()) / static_cast <double> (repeats));
		/**
		 * Если круг выполнен быстрее прежних
		 */
		if((result == 0.) || (spent < result))
			// Выполняем установку времени круга
			result = spent;
	}
	// Выводим лучшее время повторения
	return result;
}

/**
 * @brief Функция сборки выражения эталоном
 *
 * @param pattern текст регулярного выражения
 * @return        собранное выражение либо пустой указатель
 *
 */
static pcre2_code * build(const char * pattern) noexcept {
	// Код ошибки сборки
	int error = 0;
	// Смещение ошибки сборки
	PCRE2_SIZE offset = 0;
	// Выводим собранное выражение
	return ::pcre2_compile(reinterpret_cast <PCRE2_SPTR> (pattern), PCRE2_ZERO_TERMINATED, 0, &error, &offset, nullptr);
}

/**
 * @brief Функция запуска щупа
 *
 * @return результат исполнения щупа
 *
 */
int main() noexcept {
	// Выполняем сборку выражения с группой захвата
	pcre2_code * code = ::build("(a)b");
	/**
	 * Если выражение не собрано
	 */
	if(code == nullptr) {
		// Выводим сообщение об отказе сборки
		::printf("ОТКАЗ СБОРКИ «(a)b»\n");
		// Выходим с кодом отказа
		return 1;
	}
	// Создаём блок итогов сопоставления
	pcre2_match_data * data = ::pcre2_match_data_create_from_pattern(code, nullptr);
	// Выводим заголовок таблицы щупа
	::printf("%-44s %10s\n", "СТАТЬЯ ЭТАЛОНА", "нс");
	// Выполняем замер вызова с единственной попыткой
	::printf("%-44s %10.2f\n", "вызов, одна попытка", ::fastest(code, data, "ab", 400000));
	// Получаем тексты двух длин для наклона по попыткам
	const string few = (string(16, 'a') + "b"), many = (string(64, 'a') + "b");
	// Выполняем замер шестнадцати попыток
	const double sixteen = ::fastest(code, data, few, 100000);
	// Выполняем замер шестидесяти четырёх попыток
	const double sixtyfour = ::fastest(code, data, many, 25000);
	// Выводим цену попытки
	::printf("%-44s %10.2f\n", "попытка «(a)b» по наклону 17..65 попыток", ((sixtyfour - sixteen) / 48.));
	/**
	 * @brief Отстающая строка набора
	 *
	 */
	struct row_t {
		// Имя строки набора замеров
		const char * name;
		// Текст регулярного выражения
		const char * pattern;
		// Текст сопоставления
		const string * text;
	};
	// Набор отстающих строк, щупом замеряемых
	const row_t ROWS[] = {
		{"alternate-short", "GET|POST|PUT|DELETE|HEAD|OPTIONS", &SHORT},
		{"region-medium",   "(?:[a-z]+/)+v1",                    &medium()},
		{"digits-short",    "[0-9]{3,5}",                        &SHORT},
		{"captures-short",  "([A-Za-z0-9-]+): (.+)",             &SHORT}
	};
	/**
	 * Выполняем перебор отстающих строк
	 */
	for(const auto & row : ROWS) {
		// Выполняем сборку выражения строки
		pcre2_code * compiled = ::build(row.pattern);
		/**
		 * Если выражение не собрано
		 */
		if(compiled == nullptr) {
			// Выводим сообщение об отказе сборки
			::printf("%-44s ОТКАЗ СБОРКИ\n", row.name);
			// Выполняем переход к строке следующей
			continue;
		}
		// Создаём блок итогов сопоставления строки
		pcre2_match_data * block = ::pcre2_match_data_create_from_pattern(compiled, nullptr);
		// Выводим время строки
		::printf("строка %-37s %10.2f\n", row.name, ::fastest(compiled, block, * row.text, 40000));
		// Выполняем освобождение блока итогов строки
		::pcre2_match_data_free(block);
		// Выполняем освобождение выражения строки
		::pcre2_code_free(compiled);
	}
	// Выполняем освобождение блока итогов
	::pcre2_match_data_free(data);
	// Выполняем освобождение выражения
	::pcre2_code_free(code);
	// Выводим результат работы щупа
	return 0;
}
