/**
 * @file: main.cpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Точка входа набора бенчмарков библиотеки — хранилище зарегистрированных сценариев,
 *        разбор параметров командной строки,
 *        отбор и последовательный прогон сценариев с выводом собранных показателей
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstring>

/**
 * Подключаем заголовочный файл главного модуля бенчмарков
 */
#include "main.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутреннее хранилище зарегистрированных сценариев
 *
 */
namespace {
	/**
	 * @brief Функция получения изменяемого списка сценариев
	 *
	 * @note Список создаётся при первом обращении: сценарии регистрируются
	 *       статическими инициализаторами, порядок которых между единицами
	 *       трансляции не определён
	 *
	 * @return изменяемый список зарегистрированных сценариев
	 *
	 */
	static std::vector <awh::benchmark::scenario_t> & registry() noexcept {
		// Список зарегистрированных сценариев
		static std::vector <awh::benchmark::scenario_t> result;
		// Выводим список зарегистрированных сценариев
		return result;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::benchmark::Result::Result() noexcept : value(0.0), details{""} {}

/**
 * @brief Конструктор
 *
 */
awh::benchmark::Scenario::Scenario() noexcept :
 name{""}, units{""}, threshold(0.0), bound(bound_t::MINIMUM) {}

/**
 * @brief Функция регистрации сценария бенчмарка
 *
 * @param name      название сценария
 * @param units     единица измерения характеристики
 * @param threshold пороговое значение характеристики
 * @param bound     направление сравнения с порогом
 * @param run       функция выполнения сценария
 * @return          признак успешной регистрации (для статической инициализации)
 *
 */
bool awh::benchmark::add(const std::string & name, const std::string & units, const double threshold, const bound_t bound, std::function <result_t ()> run) noexcept {
	// Формируем описание сценария бенчмарка
	scenario_t scenario;
	// Устанавливаем название сценария
	scenario.name = name;
	// Устанавливаем единицу измерения характеристики
	scenario.units = units;
	// Устанавливаем пороговое значение характеристики
	scenario.threshold = threshold;
	// Устанавливаем направление сравнения с порогом
	scenario.bound = bound;
	// Устанавливаем функцию выполнения сценария
	scenario.run = run;
	// Добавляем сценарий в список зарегистрированных
	::registry().push_back(scenario);
	// Выводим признак успешной регистрации
	return true;
}
/**
 * @brief Функция получения списка зарегистрированных сценариев
 *
 * @return список зарегистрированных сценариев
 *
 */
const std::vector <awh::benchmark::scenario_t> & awh::benchmark::scenarios() noexcept {
	// Выводим список зарегистрированных сценариев
	return ::registry();
}
/**
 * @brief Главная функция приложения бенчмарков
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char ** argv){
	// Фильтр названий выполняемых сценариев
	const char * filter = nullptr;
	// Флаг игнорирования порогов
	bool relaxed = false;
	/**
	 * Перебираем параметры запуска приложения
	 */
	for(int32_t i = 1; i < argc; i++){
		// Если задан фильтр названий выполняемых сценариев
		if((::strncmp(argv[i], "--filter=", 9) == 0))
			// Устанавливаем фильтр названий сценариев
			filter = (argv[i] + 9);
		// Если задано игнорирование порогов
		else if(::strcmp(argv[i], "--relaxed") == 0)
			// Устанавливаем флаг игнорирования порогов
			relaxed = true;
		// Если запрошена справка по параметрам запуска
		else if((::strcmp(argv[i], "--help") == 0) || (::strcmp(argv[i], "-h") == 0)){
			// Выводим справку по параметрам запуска
			::printf(
				"Использование: %s [параметры]\n\n"
				"  --filter=ПОДСТРОКА  выполнить только сценарии с подстрокой в названии\n"
				"  --relaxed           выполнить измерения без проверки порогов\n"
				"  --help              вывести эту справку\n",
				argv[0]
			);
			// Выводим успешный код выхода
			return 0;
		}
	}
	// Количество выполненных сценариев
	size_t executed = 0;
	// Количество сценариев, не уложившихся в порог
	size_t failed = 0;
	/**
	 * Выводим заголовок таблицы результатов: ширина поля форматирования считается
	 * в октетах, а кириллица в UTF-8 занимает по два октета на символ, поэтому
	 * выравнивание задаётся пробелами явно
	 */
	::printf("СЦЕНАРИЙ                               ИЗМЕРЕНО          ПОРОГ   РЕЗУЛЬТАТ\n");
	/**
	 * Перебираем список зарегистрированных сценариев
	 */
	for(auto & scenario : awh::benchmark::scenarios()){
		// Если название сценария не соответствует фильтру
		if((filter != nullptr) && (scenario.name.find(filter) == std::string::npos))
			// Переходим к следующему сценарию
			continue;
		// Выполняем сценарий бенчмарка
		const awh::benchmark::result_t result = scenario.run();
		// Считаем выполненный сценарий
		executed++;
		/**
		 * Определяем соответствие измеренного значения порогу
		 */
		const bool passed = (relaxed || ((scenario.bound == awh::benchmark::bound_t::MINIMUM) ?
		 (result.value >= scenario.threshold) : (result.value <= scenario.threshold)));
		// Если измеренное значение не уложилось в порог
		if(!passed)
			// Считаем сценарий, не уложившийся в порог
			failed++;
		// Выводим результат выполнения сценария
		::printf(
			"%-32s %14.2f %14.2f   %s  (%s)\n",
			scenario.name.c_str(), result.value, scenario.threshold,
			(passed ? "OK  " : "ХУЖЕ"), scenario.units.c_str()
		);
		// Если сценарий сообщил дополнительные сведения
		if(!result.details.empty())
			// Выводим дополнительные сведения о прогоне
			::printf("%34s%s\n", "", result.details.c_str());
	}
	// Если ни один сценарий не выполнялся
	if(executed == 0){
		// Выводим сообщение об отсутствии подходящих сценариев
		::printf("\nСценарии не найдены\n");
		// Выводим код выхода с ошибкой
		return 1;
	}
	// Если все сценарии уложились в пороги
	if(failed == 0)
		// Выводим итоговое сообщение об успехе
		::printf("\nВыполнено сценариев: %zu, все уложились в пороги\n", executed);
	// Если часть сценариев не уложилась в пороги
	else ::printf("\nВыполнено сценариев: %zu, не уложились в пороги: %zu\n", executed, failed);
	// Выводим код выхода по результатам проверки порогов
	return ((failed == 0) ? 0 : 1);
}
