/**
 * @file: awh.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Стенд сравнения модуля регулярных выражений библиотеки AWH
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы стендов
 */
#include "driver.hpp"

/**
 * Подключаем заголовочный файлы проекта
 */
#include <regex/regex.hpp>

/**
 * @brief Функция запуска стенда сравнения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода стенда сравнения
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	// Получаем фильтр названий выполняемых сценариев
	const char * mask = rival::filter(argc, argv);
	// Создаём объект работы с регулярными выражениями
	awh::regexp_t regexp;
	/**
	 * Выполняем прогон каждого сценария сравнения
	 */
	for(const auto & scenario : scenarios::SCENARIOS) {
		// Выполняем сборку регулярного выражения сценария
		const auto expression = regexp.build(scenario.pattern);
		/**
		 * Если сборка регулярного выражения не выполнена
		 */
		if(!expression) {
			// Выводим сообщение об отказе сборки регулярного выражения
			::printf("сборка отказана: %s (%s)\n", scenario.pattern, regexp.message().c_str());
			// Переходим к следующему сценарию сравнения
			continue;
		}
		// Получаем текст сопоставления сценария
		const std::string & text = scenarios::text(scenario.kind);
		/**
		 * Если наличие совпадения ожиданию сценария не отвечает
		 *
		 * @details Сценарий, сопоставляющийся не так, как задумано, мерил бы
		 *          не тот способ исполнения, ради которого заведён
		 *
		 */
		if(regexp.test(text, expression) != scenario.matches) {
			// Выводим сообщение о несоответствии сценария ожиданию
			::printf("сценарий не отвечает ожиданию: %s\n", scenario.name);
			// Переходим к следующему сценарию сравнения
			continue;
		}
		/**
		 * Создаём набор границ совпадения и захваченных групп
		 *
		 * @details Набор переиспользуется между сопоставлениями: соперник
		 *          переиспользует свой, и размещение памяти на каждом
		 *          сопоставлении мерило бы разницу способов её отведения
		 *
		 */
		static std::vector <std::pair <size_t, size_t>> bounds;
		// Выполняем прогон сценария сравнения
		driver::execute(scenario, mask, [&regexp, &expression, &text]() noexcept -> uint64_t {
			// Выполняем извлечение границ совпадения и захваченных групп
			regexp.match(text, expression, bounds);
			// Выводим итог сопоставления регулярного выражения
			return (bounds.empty() ? 0 : static_cast <uint64_t> (bounds.front().second));
		});
	}
	// Выводим накопленные итоги работы сценариев
	driver::digest(argc, argv);
	// Выводим успешный код выхода
	return 0;
}
