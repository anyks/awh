/**
 * @file: sctp.hpp
 * @date: 2026-01-28
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

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SCTP__
#define __AWH_SCTP__

/**
 * Наши модули
 */
#include "../net.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён Ethernet протоколов
	 *
	 */
	namespace eth {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс управления протоколом передачи с управлением потоком
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Stream_Control_Transmission_Protocol  {
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод получения статуса SCTP сокета
				 *
				 * @param sock   сетевой сокет
				 * @param status объект для извлечения статуса инициализации SCTP сокета
				 * @return       результат работы функции
				 */
				bool status(const net::socket_t sock, net::sctp::status_t & status) const noexcept;
			public:
				/**
				 * @brief Метод инициализации SCTP сокета
				 *
				 * @param sock    сетевой сокет
				 * @param initmsg параметры инициализации SCTP сокета
				 * @return        результат работы функции
				 */
				bool initMessages(const net::socket_t sock, const net::sctp::initmsg_t & initmsg) const noexcept;
			public:
				/**
				 * @brief Метод подписки на SCTP события
				 *
				 * @param sock   сетевой сокет
				 * @param events список событий SCTP для активации
				 * @return       результат работы функции
				 */
				bool eventsSubscribe(const net::socket_t sock, const net::sctp::event_types_t & events) const noexcept;
			public:
				/**
				 * @brief Метод установки поддерживаемых алгоритмов аутентификации SCTP сокета
				 *
				 * @param sock  сетевой сокет
				 * @param types список поддерживаемых алгоритмов аутентификации
				 * @return      результат работы функции
				 */
				bool authenticateSupportAlgorithms(const net::socket_t sock, const vector <net::sctp::auth_type_t> & types) const noexcept;
			public:
				/**
				 * @brief Метод установки ключа аутентификации SCTP сокета
				 *
				 * @param sock сетевой сокет
				 * @param num  номер ключа аутентификации
				 * @param key  ключ аутентификации
				 * @return     результат работы функции
				 */
				bool authenticateKey(const net::socket_t sock, const uint16_t num, string_view key) const noexcept;
				/**
				 * @brief Метод активации/деактивации ключа аутентификации SCTP сокета
				 *
				 * @param sock сетевой сокет
				 * @param mode режим установки типа сокета
				 * @param id   идентификатор ассоциации
				 * @param num  номер ключа аутентификации
				 * @return     результат работы функции
				 */
				bool authenticateKey(const net::socket_t sock, const net::socket_mode_t mode, const uint32_t id, const uint16_t num) const noexcept;
			public:
				/**
				 * @brief Метод установки чанков аутентификации SCTP сокета
				 *
				 * @param sock   сетевой сокет
				 * @param chunks список чанков подлежащих аутентификации
				 * @return       результат работы функции
				 */
				bool authenticateChunks(const net::socket_t sock, const vector <net::sctp::auth_chunk_t> & chunks) const noexcept;
				/**
				 * @brief Метод извлечения чанков аутентификации SCTP сокета
				 *
				 * @param sock   сетевой сокет
				 * @param origin источник события
				 * @param id     идентификатор ассоциации
				 * @param chunks список чанков подлежащих аутентификации
				 * @return       результат работы функции
				 */
				bool authenticateChunks(const net::socket_t sock, const event::origin_t origin, const uint32_t id, vector <net::sctp::auth_chunk_t> & chunks) const noexcept;
			public:
				/**
				 * @brief Метод получения таймаута SCTP сокета
				 *
				 * @param sock сетевой сокет
				 * @param id   идентификатор ассоциации
				 * @param type тип таймаута
				 * @param ctx  контекст установки таймаута
				 * @return     значение таймаута в миллисекундах
				 */
				uint32_t timeout(const net::socket_t sock, const uint32_t id, const net::sctp::timeout_t type, void * ctx = nullptr) const noexcept;
				/**
				 * @brief Метод установки таймаута SCTP сокета
				 *
				 * @param sock    сетевой сокет
				 * @param id      идентификатор ассоциации
				 * @param type    тип таймаута
				 * @param timeout значение таймаута в миллисекундах
				 * @param ctx     контекст установки таймаута
				 * @return        результат работы функции
				 */
				bool timeout(const net::socket_t sock, const uint32_t id, const net::sctp::timeout_t type, const uint32_t timeout, void * ctx = nullptr) const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 */
				Stream_Control_Transmission_Protocol(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
				/**
				 * @brief Деструктор
				 *
				 */
				~Stream_Control_Transmission_Protocol() noexcept {}
		} sctp_t;
	};
};

#endif // __AWH_SCTP__
