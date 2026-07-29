/**
 * @file: chrono.hpp
 * @date: 2025-12-10
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры модуля работы с датой и временем —
 *        объявление класса фикстуры Google Test, подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2025
 *
 */
 
#ifndef __AWH_CHRONO_TESTS__
#define __AWH_CHRONO_TESTS__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/log.hpp"
#include "../../../include/sys/chrono.hpp"

/**
 * @brief Класс фикстуры для тестов модуля работы с датой и временем
 *
 * @details Фикстура закрепляет временную зону процесса на UTC на время выполнения
 *          теста и восстанавливает исходную по его завершении. Без этого набор
 *          проверяет не модуль, а настройки машины: методы формирования и разбора
 *          берут смещение зоны из окружения, когда формат его не задаёт, и
 *          ожидаемые значения расходятся на величину этого смещения. Набор,
 *          написанный без закрепления зоны, зеленел только в UTC+3 и падал
 *          шестьюдесятью тестами в любой другой зоне
 *
 */
class ChronoFixture : public testing::Test {
	protected:
		// Признак наличия временной зоны в окружении до начала теста
		bool _restore;
		// Временная зона окружения, действовавшая до начала теста
		std::string _timezone;
	protected:
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
		// Объект работы с датой и временем
		std::unique_ptr <awh::chrono_t> _chrono;
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown();
};

#endif // __AWH_CHRONO_TESTS__
