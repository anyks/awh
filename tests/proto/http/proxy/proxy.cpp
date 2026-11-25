/**
 * @file proxy.cpp
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
 * @brief Реализация тестовой фикстуры клиента HTTP-прокси —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include "proxy.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void HttpProxyFixture::SetUp(){
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void HttpProxyFixture::TearDown() {}

/**
 * @brief Фабричный метод создания клиента HTTP-прокси
 *
 * @return сформированный объект клиента HTTP-прокси
 *
 */
std::unique_ptr <awh::http::proxy::client_t> HttpProxyFixture::make() const noexcept {
	// Создаём и возвращаем объект клиента HTTP-прокси
	return std::make_unique <awh::http::proxy::client_t> ();
}

/**
 * @brief Метод подписки сборщика событий на функции обратного вызова клиента
 *
 * @param proxy  объект клиента HTTP-прокси
 * @param events объект сборщика событий
 *
 */
void HttpProxyFixture::attach(awh::http::proxy::client_t & proxy, events_t & events) const noexcept {
	// Подписываем сборщик событий на обработку тела ответа прокси
	proxy.on(awh::http::parser_http_t::data_callback_t([&events](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Собираем фрагмент тела ответа прокси
		events.body.append(static_cast <const char *> (buffer), size);
		// Продолжаем разбор
		return true;
	}));
	// Подписываем сборщик событий на готовность головы ответа прокси
	proxy.on(awh::http::proxy::client_t::response_callback_t([&events](const uint16_t code, const std::vector <std::pair <std::string, std::string>> & headers) noexcept {
		// Помечаем что голова ответа выдана
		events.headFired = true;
		// Сохраняем код состояния ответа прокси
		events.code = code;
		// Сохраняем заголовки ответа прокси
		events.headers = headers;
	}));
	// Подписываем сборщик событий на готовность трейлеров ответа прокси
	proxy.on(awh::http::proxy::client_t::trailer_callback_t([&events](const std::vector <std::pair <std::string, std::string>> & trailers) noexcept {
		// Сохраняем трейлеры ответа прокси
		events.trailers = trailers;
	}));
}

/**
 * @brief Метод установки целевого хоста в контекст обмена
 *
 * @param ctx  объект контекста обмена
 * @param addr адрес целевого хоста (IP-адрес либо доменное имя)
 * @param port порт целевого хоста
 *
 */
void HttpProxyFixture::host(awh::http::proxy::client_t::ctx_t & ctx, const std::string & addr, const uint16_t port) const noexcept {
	// Объект сетевых адресов для распознавания адреса хоста
	awh::net_addr_t parser;
	// Если адрес распознан как IP-адрес
	if(parser.parse(addr)){
		// Создаём объект параметров сетевого адреса хоста
		ctx.host = std::make_unique <awh::net::attr_net_t> ();
		// Устанавливаем тип адреса хоста по распознанному адресу
		ctx.host->type = ((parser.type() == awh::net_addr_t::type_t::IPV4) ? awh::net::type_t::IPV4 : awh::net::type_t::IPV6);
		// Устанавливаем порт хоста
		awh_cast <awh::net::attr_net_t *> (ctx.host.get())->port = port;
		// Устанавливаем IP-адрес хоста
		awh_cast <awh::net::attr_net_t *> (ctx.host.get())->ip = parser.source(awh::net_addr_t::endian_t::LITTLE);
	// Если адрес не распознан как IP-адрес — считаем его доменным именем
	} else {
		// Создаём объект параметров FQDN-адреса хоста
		ctx.host = std::make_unique <awh::net::attr_fqdn_t> ();
		// Устанавливаем тип адреса хоста
		ctx.host->type = awh::net::type_t::FQDN;
		// Устанавливаем порт хоста
		awh_cast <awh::net::attr_fqdn_t *> (ctx.host.get())->port = port;
		// Устанавливаем доменное имя хоста
		awh_cast <awh::net::attr_fqdn_t *> (ctx.host.get())->domain = addr;
	}
}

/**
 * @brief Метод извлечения сформированного запроса к прокси
 *
 * @param proxy объект клиента HTTP-прокси
 * @param ctx   объект контекста обмена
 * @return      сформированный запрос к прокси (пустая строка, если запрос не собран)
 *
 */
std::string HttpProxyFixture::request(awh::http::proxy::client_t & proxy, awh::http::proxy::client_t::ctx_t & ctx) const noexcept {
	// Буфер сформированного запроса
	uint8_t * buffer = nullptr;
	// Размер сформированного запроса
	size_t size = 0;
	// Если извлечение буфера запроса выполнено успешно
	if(proxy.buffer(&buffer, size, ctx)){
		// Копируем сформированный запрос в строку
		const std::string result(reinterpret_cast <const char *> (buffer), size);
		// Освобождаем отправленные байты запроса
		proxy.consume(size);
		// Возвращаем сформированный запрос
		return result;
	}
	// Возвращаем пустую строку
	return "";
}
