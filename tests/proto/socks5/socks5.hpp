/**
 * @file: socks5.hpp
 * @date: 2026-07-20
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

#ifndef __AWH_PROTO_SOCKS5_TESTS__
#define __AWH_PROTO_SOCKS5_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/proto/socks5/client.hpp"
#include "../../../include/proto/socks5/server.hpp"

/**
 * @brief Класс фикстуры для тестов подмодуля протокола SOCKS5
 *
 */
class Socks5Fixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
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
	protected:
		/**
		 * @brief Фабричный метод создания клиента SOCKS5
		 *
		 * @return сформированный объект клиента SOCKS5
		 */
		std::unique_ptr <awh::proto::client_socks5_t> makeClient() const noexcept;
		/**
		 * @brief Фабричный метод создания сервера SOCKS5
		 *
		 * @return сформированный объект сервера SOCKS5
		 */
		std::unique_ptr <awh::proto::server_socks5_t> makeServer() const noexcept;
};

#endif // __AWH_PROTO_SOCKS5_TESTS__
