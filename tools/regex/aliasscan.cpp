/**
 * @file aliasscan.cpp
 * @date 2026-10-01
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп зависимости порождённого сопоставителя от адресов текста и стека
 *
 * @details Выражение и текст берутся из штатного набора. Текст перемещается
 *          внутри страницы независимо от стека вызывающего кода. Каждая пара
 *          положений измеряется трижды; сохраняется наименьшее время вызова.
 *          Перед измерением и после него проверяются вердикт, границы и отказ.
 *          Вердикты всех повторений суммируются и проверяются после прохода.
 *          Щуп вызывает codegen_t::exec напрямую, как колонка кода стенда.
 *
 *          Доводы: имя сценария, шаг текста, шаг стека, число повторений.
 *          По умолчанию шаги равны 256 и 16 байтам, число повторений — 2000.
 *          Шаги должны делить страницу без остатка, шаг стека кратен 16.
 *          Вывод — CSV со всеми положениями, без отбрасывания медленных ячеек.
 *
 * @note Собирать без AWH_REGEX_PROBING: счётчики меняют цену вызова.
 *
 * Сборка и запуск:
 *   PROBING=нет FLAGS="-I tools/benchmark/syscount" tools/regex/probe.sh aliasscan address-short 256 16 20000
 *
 * Коды завершения: 1 — неверные доводы, 2 — ошибка сборки выражения либо JIT,
 * 3 — неверный исходный сценарий, 4 — ошибка проверки измеряемого вызова.
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта и штатного набора
 */
#include <regex/engine.hpp>
#include <regex/codegen.hpp>
#include "../../benchmark/regex/matching/matching.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string_view>
#include <vector>
#include <utility>
#include <limits>
#include <unistd.h>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Количество проходов по сетке положений
 */
static constexpr size_t PASSES = 3;
/**
 * Приёмник прокладки, не позволяющий удалить её при оптимизации
 */
static void (* volatile sink)(uint8_t *) noexcept = [](uint8_t *) noexcept -> void {};

/**
 * @brief Функция измерения вызова при заданном положении стека
 *
 * @param shift    величина прокладки стека
 * @param rounds   количество повторений
 * @param codegen  порождённый сопоставитель
 * @param text     участок текста
 * @param expected ожидаемые границы совпадения
 * @param matches  ожидаемый вердикт
 * @param bounds   набор границ для повторного использования
 * @param position адрес прокладки стека
 * @return         время вызова в наносекундах, отрицательное при ошибке
 *
 */
static AWH_REGEX_NOINLINE double pass(const size_t shift, const size_t rounds, const awh::regex::codegen_t & codegen,
 const string_view text, const vector <pair <size_t, size_t>> & expected, const bool matches,
 vector <pair <size_t, size_t>> & bounds, uintptr_t & position) noexcept {
	// Отводим прокладку, сдвигающую кадры всех последующих вызовов
	uint8_t * pad = static_cast <uint8_t *> (__builtin_alloca(shift + 16));
	// Передаём прокладку непрозрачному для собирателя приёмнику
	::sink(pad);
	// Запоминаем фактический адрес прокладки
	position = reinterpret_cast <uintptr_t> (pad);
	// Признак отказа порождённого сопоставителя
	bool refused = false;
	// Если вердикт либо границы неверны, измерение неприменимо
	if((codegen.exec(text, 0, bounds, refused) != matches) || refused || (bounds != expected))
		// Выводим признак отказа проверки до измерения
		return -1.;
	// Количество успешных сопоставлений
	size_t hits = 0;
	// Получаем время начала прохода
	const auto begin = chrono::steady_clock::now();
	/**
	 * Выполняем повторения сопоставления
	 */
	for(size_t i = 0; i < rounds; i++)
		// Вызываем порождённый сопоставитель обычным открытым методом
		hits += (codegen.exec(text, 0, bounds) ? 1 : 0);
	// Получаем среднее время вызова в проходе
	const double spent = (chrono::duration <double, nano> (chrono::steady_clock::now() - begin).count() / static_cast <double> (rounds));
	// Если хотя бы один вердикт неверен, измерение неприменимо
	if(hits != (matches ? rounds : 0))
		// Выводим признак отказа проверки вердиктов
		return -3.;
	// Если вердикт либо границы изменились, измерение неприменимо
	if((codegen.exec(text, 0, bounds, refused) != matches) || refused || (bounds != expected))
		// Выводим признак отказа проверки после измерения
		return -2.;
	// Выводим время вызова
	return spent;
}

/**
 * @brief Функция обхода положений текста и стека
 *
 * @param scenario  сценарий штатного набора
 * @param page      размер страницы системы
 * @param textStep  шаг положений текста
 * @param stackStep шаг положений стека
 * @param rounds    количество повторений в одном проходе
 * @return          код завершения измерений
 *
 */
static int measure(const awh::benchmark::matching::scenario_t & scenario, const size_t page,
 const size_t textStep, const size_t stackStep, const size_t rounds) noexcept {
	// Получаем исходный текст из штатного набора
	const string & body = awh::benchmark::matching::text(scenario.kind);
	// Создаём движок, программу и порождённый сопоставитель
	awh::regex::engine_t engine;
	awh::regex::expression_t expression;
	awh::regex::codegen_t codegen;
	// Если сборка либо порождение не выполнены, измерение невозможно
	if(!engine.build(scenario.pattern, 0, expression) || !codegen.compile(expression.forward))
		// Выводим признак отказа подготовки
		return 2;
	// Получаем ожидаемые границы исполнением программы
	vector <pair <size_t, size_t>> expected, bounds;
	// Если вердикт штатному сценарию не отвечает, измерение невозможно
	if(engine.exec(expression, body, 0, expected) != scenario.matches)
		// Выводим признак отказа исходного сценария
		return 3;
	// Отводим место для выравнивания и всех смещений текста
	vector <char> storage(body.size() + (page * 2));
	// Получаем исходный адрес участка памяти
	const uintptr_t address = reinterpret_cast <uintptr_t> (storage.data());
	// Получаем начало текста, выровненное на страницу
	char * aligned = (storage.data() + ((page - (address % page)) % page));
	// Получаем количество положений стека и размер сетки
	const size_t stacks = (page / stackStep), count = ((page / textStep) * stacks);
	// Заводим лучшие времена и фактические положения стека
	vector <double> times(count, 0.);
	vector <size_t> places(count, 0);
	// Выводим параметры порождённого сопоставителя отдельно от таблицы
	::fprintf(stderr, "scenario=%s page=%zu rounds=%zu code=%zu filter=%u feasible=%u skipping=%u\n",
	 scenario.name, page, rounds, codegen.length(), static_cast <unsigned> (codegen.filter()),
	 static_cast <unsigned> (codegen.feasibility()), static_cast <unsigned> (codegen.skipping()));
	/**
	 * Выполняем проходы по всей сетке
	 */
	for(size_t attempt = 0; attempt < PASSES; attempt++){
		/**
		 * Перебираем положения текста
		 */
		for(size_t offset = 0; offset < page; offset += textStep){
			// Размещаем точную копию штатного текста
			::memcpy(aligned + offset, body.data(), body.size());
			// Создаём представление текста с заданным адресом
			const string_view text(aligned + offset, body.size());
			/**
			 * Перебираем положения стека
			 */
			for(size_t index = 0; index < stacks; index++){
				// Чередуем направление обхода, чтобы дрейф не совпадал с адресом
				const size_t stack = (((attempt % 2) == 0) ? index : (stacks - index - 1));
				// Получаем индекс ячейки сетки
				const size_t cell = (((offset / textStep) * stacks) + stack);
				// Фактический адрес прокладки стека
				uintptr_t position = 0;
				// Измеряем текущую пару положений
				const double spent = ::pass(stack * stackStep, rounds, codegen, text, expected, scenario.matches, bounds, position);
				// Если проверка вердикта либо границ отказала, прекращаем измерения
				if(spent <= 0.)
					// Выводим признак отказа измерения
					return 4;
				/**
				 * Если получено первое либо лучшее время ячейки
				 */
				if((times[cell] == 0.) || (spent < times[cell])){
					// Сохраняем время и соответствующее ему положение стека
					times[cell] = spent;
					places[cell] = static_cast <size_t> (position % page);
				}
			}
		}
	}
	// Выводим заголовок таблицы
	::printf("scenario,text,stack,ns\n");
	/**
	 * Выводим все ячейки, включая медленные
	 */
	for(size_t offset = 0; offset < page; offset += textStep){
		/**
		 * Перебираем положения стека для текущего текста
		 */
		for(size_t stack = 0; stack < stacks; stack++){
			// Получаем индекс ячейки
			const size_t cell = (((offset / textStep) * stacks) + stack);
			// Выводим адреса по модулю страницы и время вызова
			::printf("%s,%zu,%zu,%.3f\n", scenario.name, offset, places[cell], times[cell]);
		}
	}
	// Выводим признак успешного обхода
	return 0;
}

/**
 * @brief Функция разбора положительного целого довода
 *
 * @param value  строка с числом
 * @param limit  допустимый предел числа
 * @param result результат разбора
 * @return       признак успешного разбора
 *
 */
static bool number(const char * value, const size_t limit, size_t & result) noexcept {
	// Заводим промежуточное значение довода
	size_t current = 0;
	/**
	 * Выполняем обход цифр довода
	 */
	for(const char * letter = value; *letter != '\0'; letter++){
		// Если очередной знак цифрой не является
		if((*letter < '0') || (*letter > '9'))
			// Выводим признак неверного довода
			return false;
		// Получаем значение очередной цифры
		const size_t digit = static_cast <size_t> (*letter - '0');
		// Если при добавлении цифры предел будет превышен
		if((digit > limit) || (current > ((limit - digit) / 10)))
			// Выводим признак превышения предела
			return false;
		// Добавляем очередную цифру довода
		current = ((current * 10) + digit);
	}
	// Если довод пуст либо равен нулю
	if(current == 0)
		// Выводим признак неверного довода
		return false;
	// Передаём разобранное значение
	result = current;
	// Выводим признак успешного разбора
	return true;
}

/**
 * @brief Функция запуска щупа
 *
 * @param count  количество доводов
 * @param values имя сценария и необязательные параметры сетки
 * @return       код завершения щупа
 *
 */
int main(const int count, char ** values) noexcept {
	// Если количество доводов неверно
	if((count < 2) || (count > 5)){
		// Выводим порядок доводов щупа
		::fprintf(stderr, "Использование: %s сценарий [шаг текста=256] [шаг стека=16] [повторения=2000]\n", values[0]);
		// Выводим признак ошибки доводов
		return 1;
	}
	// Получаем размер страницы системы
	const int size = ::getpagesize();
	// Если размер страницы не определён
	if(size <= 0)
		// Выводим признак ошибки параметров системы
		return 1;
	// Получаем размер страницы для расчёта сетки
	const size_t page = static_cast <size_t> (size);
	// Параметры обхода: шаг текста, шаг стека и число повторений
	size_t options[] = {256, 16, 2000};
	// Максимальное значение размера
	const size_t limit = numeric_limits <size_t>::max();
	/**
	 * Выполняем разбор заданных параметров обхода
	 */
	for(int i = 2; i < count; i++){
		// Если число не положительно либо превышает допустимый предел
		if(!::number(values[i], ((i < 4) ? page : limit), options[i - 2])){
			// Выводим сообщение о неверном числе
			::fprintf(stderr, "Неверный числовой довод: %s\n", values[i]);
			// Выводим признак ошибки доводов
			return 1;
		}
	}
	// Если шаги не делят страницу либо стек не соблюдает выравнивание
	if(((page % options[0]) != 0) || ((page % options[1]) != 0) || ((options[1] % 16) != 0)){
		// Выводим сообщение о неверной сетке
		::fprintf(stderr, "Шаги должны делить страницу %zu, шаг стека должен быть кратен 16\n", page);
		// Выводим признак ошибки доводов
		return 1;
	}
	// Если размер сетки не представим
	if((page / options[0]) > (limit / (page / options[1])))
		// Выводим признак ошибки размера сетки
		return 1;
	/**
	 * Выполняем поиск выбранного сценария
	 */
	for(const auto & scenario : awh::benchmark::matching::SCENARIOS){
		// Если имя совпало, выполняем измерение выбранного сценария
		if(::strcmp(values[1], scenario.name) == 0)
			// Выводим результат измерений
			return ::measure(scenario, page, options[0], options[1], options[2]);
	}
	// Выводим сообщение о неизвестном сценарии
	::fprintf(stderr, "Неизвестный сценарий: %s\n", values[1]);
	// Выводим признак неизвестного сценария
	return 1;
}
