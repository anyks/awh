/**
 * @file: pcre2.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Стенд сравнения эталонной реализации регулярных выражений PCRE2
 *
 * @details Стенд собирается с компиляцией выражения в машинный код, если она
 *          доступна: сравнивать реализацию с эталоном, работающим не в полную
 *          силу, бессмысленно. Признак применения компиляции выводится прогоном
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы стендов
 */
#include "driver.hpp"

/**
 * Устанавливаем ширину единицы кодирования эталонной реализации
 */
#define PCRE2_CODE_UNIT_WIDTH 8

/**
 * Подключаем заголовочный файл эталонной реализации
 */
#include <pcre2.h>

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
	// Признак применения компиляции выражения в машинный код
	bool compiled = false;
	/**
	 * Выполняем прогон каждого сценария сравнения
	 */
	for(const auto & scenario : scenarios::SCENARIOS) {
		// Код ошибки сборки регулярного выражения
		int32_t code = 0;
		// Смещение ошибки сборки регулярного выражения
		PCRE2_SIZE offset = 0;
		// Выполняем сборку регулярного выражения сценария
		pcre2_code * expression = ::pcre2_compile(
			reinterpret_cast <PCRE2_SPTR> (scenario.pattern),
			PCRE2_ZERO_TERMINATED, 0, &code, &offset, nullptr
		);
		/**
		 * Если сборка регулярного выражения не выполнена
		 */
		if(expression == nullptr) {
			// Выводим сообщение об отказе сборки регулярного выражения
			::printf("сборка отказана: %s\n", scenario.pattern);
			// Переходим к следующему сценарию сравнения
			continue;
		}
		/**
		 * Если компиляция выражения в машинный код выполнена
		 */
		if(::pcre2_jit_compile(expression, PCRE2_JIT_COMPLETE) == 0)
			// Выполняем установку признака применения компиляции
			compiled = true;
		// Создаём набор границ совпадения и захваченных групп
		pcre2_match_data * data = ::pcre2_match_data_create_from_pattern(expression, nullptr);
		// Получаем текст сопоставления сценария
		const std::string & text = scenarios::text(scenario.kind);
		// Получаем наличие совпадения регулярного выражения в тексте
		const bool matches = (::pcre2_match(
			expression, reinterpret_cast <PCRE2_SPTR> (text.c_str()),
			text.size(), 0, 0, data, nullptr
		) > 0);
		/**
		 * Если наличие совпадения ожиданию сценария не отвечает
		 *
		 * @details Сценарий, сопоставляющийся не так, как задумано, мерил бы
		 *          не тот способ исполнения, ради которого заведён
		 *
		 */
		if(matches != scenario.matches) {
			// Выводим сообщение о несоответствии сценария ожиданию
			::printf("сценарий не отвечает ожиданию: %s\n", scenario.name);
			// Выполняем освобождение набора границ совпадения
			::pcre2_match_data_free(data);
			// Выполняем освобождение собранного регулярного выражения
			::pcre2_code_free(expression);
			// Переходим к следующему сценарию сравнения
			continue;
		}
		// Выполняем прогон сценария сравнения
		driver::execute(scenario, mask, [expression, data, &text]() noexcept -> uint64_t {
			// Выполняем сопоставление регулярного выражения с текстом
			const int32_t count = ::pcre2_match(
				expression, reinterpret_cast <PCRE2_SPTR> (text.c_str()),
				text.size(), 0, 0, data, nullptr
			);
			/**
			 * Если совпадение в тексте не обнаружено
			 */
			if(count <= 0)
				// Выводим итог сопоставления регулярного выражения
				return 0;
			// Получаем набор границ совпадения и захваченных групп
			const PCRE2_SIZE * bounds = ::pcre2_get_ovector_pointer(data);
			// Выводим итог сопоставления регулярного выражения
			return static_cast <uint64_t> (bounds[1]);
		});
		// Выполняем освобождение набора границ совпадения
		::pcre2_match_data_free(data);
		// Выполняем освобождение собранного регулярного выражения
		::pcre2_code_free(expression);
	}
	// Выводим признак применения компиляции выражения в машинный код
	::printf("\nкомпиляция в машинный код: %s\n", (compiled ? "применена" : "недоступна"));
	// Выводим накопленные итоги работы сценариев
	driver::digest(argc, argv);
	// Выводим успешный код выхода
	return 0;
}
