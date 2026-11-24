/**
 * @file proxy.hpp
 * @date 2026-11-23
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры клиента HTTP-прокси — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../../main.hpp"
#include "../../../../include/proto/http/proxy/client.hpp"

/**
 * @brief Класс фикстуры для тестов подмодуля клиента HTTP-прокси
 *
 */
class HttpProxyFixture : public testing::Test {
	public:
		/**
		 * @brief Структура сборщика событий клиента HTTP-прокси
		 *
		 */
		typedef struct Events {
			// Собранное тело ответа прокси
			std::string body;
			// Флаг вызова функции обратного вызова готовности головы ответа
			bool headFired = false;
			// Код состояния ответа прокси
			uint16_t code = 0;
			// Собранные заголовки ответа прокси
			std::vector <std::pair <std::string, std::string>> headers;
		} events_t;
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
		 * @brief Фабричный метод создания клиента HTTP-прокси
		 *
		 * @return сформированный объект клиента HTTP-прокси
		 *
		 */
		std::unique_ptr <awh::http::proxy::client_t> make() const noexcept;
		/**
		 * @brief Метод подписки сборщика событий на функции обратного вызова клиента
		 *
		 * @param proxy  объект клиента HTTP-прокси
		 * @param events объект сборщика событий
		 *
		 */
		void attach(awh::http::proxy::client_t & proxy, events_t & events) const noexcept;
		/**
		 * @brief Метод установки целевого хоста в контекст обмена
		 *
		 * @param ctx  объект контекста обмена
		 * @param addr адрес целевого хоста (IP-адрес либо доменное имя)
		 * @param port порт целевого хоста
		 *
		 */
		void host(awh::http::proxy::client_t::ctx_t & ctx, const std::string & addr, const uint16_t port) const noexcept;
		/**
		 * @brief Метод извлечения сформированного запроса к прокси
		 *
		 * @param proxy объект клиента HTTP-прокси
		 * @param ctx   объект контекста обмена
		 * @return      сформированный запрос к прокси (пустая строка, если запрос не собран)
		 *
		 */
		std::string request(awh::http::proxy::client_t & proxy, awh::http::proxy::client_t::ctx_t & ctx) const noexcept;
};
