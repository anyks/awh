/**
 * @file entrycost.cpp
 * @date 2026-09-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп платы за вызов и за заход в попытку сопоставления
 *
 * @details Шестнадцать строк набора, эталону уступающих, делятся надвое.
 *          У одних попыток много, а работы на попытку мало, и время их -
 *          заходы в попытку. У других попытка одна, и время их - плата
 *          за вызов: отбор пути движком да подготовка исполнения. Щуп
 *          разводит эти платы:
 *
 *          - вызов через движок и вызов исполнения с возвратом напрямую на
 *            одном и том же сопоставлении: разность есть отбор пути движком;
 *          - наклон по числу попыток: «(a)b» на тексте из N букв «a» с «b»
 *            в конце делает N + 1 попыток, и разность времён при двух N,
 *            делённая на разность N, есть цена попытки;
 *          - три отстающие строки набора их же текстами - мера того, что
 *            делает с ними правка, щупом судимая.
 *
 *          Разложение платы по статьям ведётся погашенными сборками: копия
 *          дерева с одной статьёй, выключенной правкой, собирается тем же
 *          стендом, и разность времён с неизменённой есть цена статьи.
 *
 * @note Собирать надлежит БЕЗ учёта: учёт вносит меры работы сложением
 *       атомарным на всяком вызове, и щуп мерил бы тогда и его.
 *
 * Сборка и запуск: PROBING=нет tools/regex/probe.sh entrycost
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <regex/regex.hpp>
#include <regex/backtrack.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>

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
 * @brief Функция замера лучшего из кругов
 *
 * @param repeats количество повторений в круге
 * @param body    замеряемое действие
 * @return        лучшее время одного повторения в наносекундах
 *
 */
template <typename Body>
static double fastest(const size_t repeats, Body && body) noexcept {
	// Лучшее время повторения
	double result = 0.;
	/**
	 * Выполняем круги замера
	 *
	 * @details Берётся лучший круг: ядра этой машины двумодальны, и середина
	 *          внутри прогона смещена целиком, а не отдельными выбросами
	 *
	 */
	for(uint32_t round = 0; round < 12; round++) {
		// Получаем отметку времени начала круга
		const auto begin = chrono::steady_clock::now();
		/**
		 * Выполняем повторения замеряемого действия
		 */
		for(size_t i = 0; i < repeats; i++)
			// Выполняем замеряемое действие
			body();
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
 * @brief Функция запуска щупа
 *
 * @return результат исполнения щупа
 *
 */
int main() noexcept {
	// Создаём движок сопоставления
	awh::regex::engine_t engine;
	// Создаём объект исполнения с возвратом
	awh::regex::backtrack_t backtrack;
	// Создаём набор границ совпадения
	vector <pair <size_t, size_t>> captures;
	// Создаём собранное выражение с группой захвата
	awh::regex::expression_t expression;
	/**
	 * Если выражение не собрано
	 */
	if(!engine.build("(a)b", 0, expression)) {
		// Выводим сообщение об отказе сборки
		::printf("ОТКАЗ СБОРКИ «(a)b»\n");
		// Выходим с кодом отказа
		return 1;
	}
	// Выводим заголовок таблицы щупа
	::printf("%-44s %10s\n", "СТАТЬЯ", "нс");
	// Получаем текст единственной попытки
	const string single = "ab";
	// Выполняем замер вызова через движок
	const double viaEngine = ::fastest(400000, [&]() noexcept -> void {
		engine.exec(expression, single, 0, captures);
	});
	// Выполняем замер вызова исполнения с возвратом напрямую
	const double direct = ::fastest(400000, [&]() noexcept -> void {
		backtrack.exec(expression.forward, single, 0, captures);
	});
	// Выводим время вызова через движок
	::printf("%-44s %10.2f\n", "вызов через движок, одна попытка", viaEngine);
	// Выводим время вызова напрямую
	::printf("%-44s %10.2f\n", "вызов исполнения напрямую, одна попытка", direct);
	// Выводим разность вызовов - отбор пути движком
	::printf("%-44s %10.2f\n", "  из них отбор пути движком", (viaEngine - direct));
	// Получаем тексты двух длин для наклона по попыткам
	const string few = (string(16, 'a') + "b"), many = (string(64, 'a') + "b");
	// Выполняем замер шестнадцати попыток
	const double sixteen = ::fastest(100000, [&]() noexcept -> void {
		backtrack.exec(expression.forward, few, 0, captures);
	});
	// Выполняем замер шестидесяти четырёх попыток
	const double sixtyfour = ::fastest(25000, [&]() noexcept -> void {
		backtrack.exec(expression.forward, many, 0, captures);
	});
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
		// Создаём собранное выражение строки
		awh::regex::expression_t compiled;
		/**
		 * Если выражение не собрано
		 */
		if(!engine.build(row.pattern, 0, compiled)) {
			// Выводим сообщение об отказе сборки
			::printf("%-44s ОТКАЗ СБОРКИ\n", row.name);
			// Выполняем переход к строке следующей
			continue;
		}
		// Выполняем замер строки через движок
		const double spent = ::fastest(40000, [&]() noexcept -> void {
			engine.exec(compiled, * row.text, 0, captures);
		});
		// Выводим время строки
		::printf("строка %-37s %10.2f\n", row.name, spent);
	}
	// Выводим результат работы щупа
	return 0;
}
