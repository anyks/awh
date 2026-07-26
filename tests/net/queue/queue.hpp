/**
 * @file: queue.hpp
 * @date: 2026-02-07
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
 
#ifndef __AWH_NETWORK_QUEUE_TESTS__
#define __AWH_NETWORK_QUEUE_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/net/queue.hpp"

/**
 * @brief Класс фикстуры для тестов очереди сетевых событий
 *
 */
class NetworkQueueFixture : public testing::Test {
	protected:
		// Объекты фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
		// Объект очереди
		std::unique_ptr <awh::net_queue_t> _queue;
	public:
		/**
		 * @brief Метод инициализации тестовой среды
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестовой среды
		 *
		 */
		void TearDown();
};

#endif // __AWH_NETWORK_QUEUE_TESTS__
