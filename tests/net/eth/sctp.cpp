/**
 * @file: sctp.cpp
 * @date: 2026-02-06
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

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

#if __FreeBSD__ || __Linux__

/**
 * @brief Тест функций SCTP
 *
 */
TEST_F(EthFixture, SctpFunctionsTest){
	// Создаём сокет (SCTP может требовать lksctp-tools / ядра с поддержкой)
	// Используем обычный UDP/TCP сокет для проверки API методов (они должны возвращать ошибку корректно, а не падать)
	// Или пытаемся создать IPPROTO_SCTP сокет если поддерживается.
	
	awh::net::socket_t sock = ::socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	bool sctp_supported = (sock != -1);
	if (!sctp_supported) {
		// Fallback для проверки API на обычном сокете
		sock = ::socket(AF_INET, SOCK_DGRAM, 0); 
	}
	ASSERT_NE(sock, awh::net::invalid_socket_t);

	// Status
	awh::net::sctp::status_t status;
	this->_eth->sctp.status(sock, status);

	// Init Messages
	awh::net::sctp::initmsg_t initmsg;
	initmsg.attempts = 4;
	initmsg.ostreams = 5;
	initmsg.istreams = 5;
	this->_eth->sctp.initMessages(sock, initmsg);

	// Events Subscribe
	this->_eth->sctp.eventsSubscribe(sock, {
		awh::net::sctp::event_type_t::ASSOC_CHANGE,
		awh::net::sctp::event_type_t::SHUTDOWN_EVENT
	});

	// Authenticate Support Algorithms (нужен sctp auth support)
	std::vector<awh::net::sctp::auth_type_t> auths;
	// auths.push_back(RTC_AUTH_HMAC_SHA1); // Зависит от констант
	this->_eth->sctp.authenticateSupportAlgorithms(sock, auths);

	// Authenticate Key
	this->_eth->sctp.authenticateKey(sock, 1, "secret");
	this->_eth->sctp.authenticateKey(sock, awh::net::socket_mode_t::ENABLED, 0, 1);

	// Authenticate Chunks
	std::vector<awh::net::sctp::auth_chunk_t> chunks;
	this->_eth->sctp.authenticateChunks(sock, chunks);
	
	// Timeout
	this->_eth->sctp.timeout(sock, 0, awh::net::sctp::timeout_t::ASSOC, 1000);
	this->_eth->sctp.timeout(sock, 0, awh::net::sctp::timeout_t::ASSOC);

	::close(sock);
}

#endif
