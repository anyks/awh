/**
 * @file: main.hpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

#ifndef __AWH_BENCHMARK__
#define __AWH_BENCHMARK__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <functional>

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён бенчмарков
	 *
	 * @details Бенчмарк - это сценарий, измеряющий одну характеристику и
	 *          сравнивающий её с порогом. Отклонение от порога завершает запуск
	 *          с ненулевым кодом, поэтому набор бенчмарков пригоден как шаг
	 *          сборки: регрессия производительности останавливает конвейер.
	 *          Сценарии регистрируются статически, поэтому добавление нового
	 *          бенчмарка сводится к добавлению файла с его реализацией
	 */
	namespace benchmark {
		/**
		 * @brief Направление сравнения измеренного значения с порогом
		 *
		 */
		enum class bound_t : uint8_t {
			MINIMUM = 0x00, // Значение не должно опускаться ниже порога
			MAXIMUM = 0x01  // Значение не должно превышать порог
		};

		/**
		 * @brief Структура результата измерения
		 *
		 */
		typedef struct Result {
			// Измеренное значение характеристики
			double value;
			// Дополнительные сведения о прогоне для вывода
			std::string details;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Result() noexcept;
		} result_t;

		/**
		 * @brief Структура описания сценария бенчмарка
		 *
		 */
		typedef struct Scenario {
			// Название сценария
			std::string name;
			// Единица измерения характеристики
			std::string units;
			// Пороговое значение характеристики
			double threshold;
			// Направление сравнения с порогом
			bound_t bound;
			// Функция выполнения сценария
			std::function <result_t ()> run;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Scenario() noexcept;
		} scenario_t;

		/**
		 * @brief Функция регистрации сценария бенчмарка
		 *
		 * @note Вызывается из статического инициализатора файла сценария:
		 *       собранные сценарии выполняются точкой входа в порядке регистрации
		 *
		 * @param name      название сценария
		 * @param units     единица измерения характеристики
		 * @param threshold пороговое значение характеристики
		 * @param bound     направление сравнения с порогом
		 * @param run       функция выполнения сценария
		 * @return          признак успешной регистрации (для статической инициализации)
		 */
		bool add(const std::string & name, const std::string & units, const double threshold, const bound_t bound, std::function <result_t ()> run) noexcept;

		/**
		 * @brief Функция получения списка зарегистрированных сценариев
		 *
		 * @return список зарегистрированных сценариев
		 */
		const std::vector <scenario_t> & scenarios() noexcept;
	};
};

#endif // __AWH_BENCHMARK__
