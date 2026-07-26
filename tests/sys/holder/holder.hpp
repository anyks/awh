/**
 * @file: holder.hpp
 * @date: 2026-01-26
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
 
#ifndef __AWH_HOLDER_TESTS__
#define __AWH_HOLDER_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/holder.hpp"

/**
 * @brief Тестовый класс для работы с холдером
 *
 */
class HolderFixture : public testing::Test {
	public:
		/**
		 * @brief Статусы для тестирования холдера
		 *
		 */
		enum class status_t : uint8_t {
			NONE    = 0x00, // Статус не установлен
			STATUS1 = 0x01, // Первый статус
			STATUS2 = 0x02, // Второй статус
			STATUS3 = 0x03, // Третий статус
			STATUS4 = 0x04  // Четвёртый статус
		};
	protected:
		// Стек статусов холдера
		std::stack <status_t> _status;
	protected:
		// Внешний мьютекс для защиты стека статусов
		awh::lock_state_t <std::mutex> _mtx;
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

#endif // __AWH_HOLDER_TESTS__
