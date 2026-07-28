/**
 * @file: quic.hpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры протокола QUIC — объявление класса фикстуры Google Test,
 *        подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_PROTO_QUIC2_TESTS__
#define __AWH_PROTO_QUIC2_TESTS__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/fmk.hpp"
#include "../../../include/sys/log.hpp"
#include "../../../include/net/addr.hpp"
#include "../../../include/net/tls/coder.hpp"
#include "../../../include/proto/quic2/quic.hpp"
#include "../../../include/proto/quic2/frame.hpp"
#include "../../../include/proto/quic2/packet.hpp"
#include "../../../include/proto/quic2/varint.hpp"

/**
 * @brief Предварительное объявление класса тестового окружения транспортной безопасности
 *
 */
class Quic2Security;

/**
 * @brief Класс фикстуры для тестов подмодуля протокола QUIC
 *
 */
class Quic2Fixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект для работы с логами
		std::unique_ptr <awh::log_t> _log;
		// Объект разбора сетевых адресов
		std::unique_ptr <awh::net_addr_t> _addr;
		// Тестовое окружение транспортной безопасности
		std::unique_ptr <Quic2Security> _security;
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
		 * @brief Метод формирования опакового представления пути удалённого эндпоинта
		 *
		 * @note Повторяет формирование опакового пути в connection_t::address() (чистые
		 *       байты адреса и порта) для сверки с результатом connection_t::path()
		 *
		 * @param host адрес хоста удалённого эндпоинта
		 * @param port порт удалённого эндпоинта
		 * @return     опаковое представление пути соединения
		 *
		 */
		std::string makePath(const std::string & host, const uint16_t port) const noexcept;
	protected:
		/**
		 * @brief Метод преобразования шестнадцатеричной строки в бинарный буфер
		 *
		 * @param hex шестнадцатеричная строка
		 * @return    бинарный буфер
		 *
		 */
		std::string unhex(const std::string & hex) const noexcept;
		/**
		 * @brief Метод преобразования бинарного буфера в шестнадцатеричную строку
		 *
		 * @param data бинарный буфер
		 * @return     шестнадцатеричная строка
		 *
		 */
		std::string hex(const std::string & data) const noexcept;
		/**
		 * @brief Метод создания идентификатора соединения из бинарного буфера
		 *
		 * @param data бинарный буфер идентификатора
		 * @return     сформированный идентификатор соединения
		 *
		 */
		awh::quic2::cid_t makeCid(const std::string & data) const noexcept;
};

/**
 * @brief Класс тестового окружения транспортной безопасности
 *
 * @details Криптография соединения QUIC задаётся шаблоном контекста кодера,
 *          поэтому тесты нуждаются в готовой паре шаблонов - клиентском и
 *          серверном. Окружение создаётся однократно на весь прогон: генерация
 *          самоподписанного сертификата и построение контекста стоят заметно
 *          дороже самих проверок
 *
 */
class Quic2Security {
	private:
		// Путь к файлу сертификата тестового узла
		std::string _certificate;
		// Путь к файлу приватного ключа тестового узла
		std::string _privateKey;
	private:
		// Идентификатор шаблона контекста безопасности клиента
		awh::tls::Coder::id_t _client;
		// Идентификатор шаблона контекста безопасности сервера
		awh::tls::Coder::id_t _server;
	private:
		// Объект кодера транспортной безопасности
		awh::tls::Coder _coder;
	public:
		/**
		 * @brief Метод доступа к объекту кодера транспортной безопасности
		 *
		 * @return объект кодера транспортной безопасности
		 *
		 */
		awh::tls::Coder & coder() noexcept;
		/**
		 * @brief Метод извлечения шаблона контекста безопасности роли эндпоинта
		 *
		 * @param endpoint роль эндпоинта
		 * @return         идентификатор шаблона контекста безопасности
		 *
		 */
		awh::tls::Coder::id_t context(const awh::quic2::endpoint_t endpoint) const noexcept;
		/**
		 * @brief Метод создания отдельного шаблона контекста безопасности
		 *
		 * @note Требуется тестам, которым нужна настройка, отличная от общей:
		 *       несовпадающий список ALPN-протоколов либо включённая проверка
		 *       сертификата удалённого узла
		 *
		 * @param endpoint  роль эндпоинта
		 * @param protocols список поддерживаемых ALPN-протоколов
		 * @param validate  режим проверки сертификата удалённого узла
		 * @return          идентификатор созданного шаблона контекста безопасности
		 *
		 */
		awh::tls::Coder::id_t make(const awh::quic2::endpoint_t endpoint, const std::vector <awh::tls::Coder::alpn_t> & protocols, const bool validate = false) noexcept;
	public:
		/**
		 * Запрещаем копирование и перемещение (окружение владеет контекстами кодера)
		 */
		Quic2Security(const Quic2Security &) = delete;
		Quic2Security(Quic2Security &&) = delete;
		Quic2Security & operator = (const Quic2Security &) = delete;
		Quic2Security & operator = (Quic2Security &&) = delete;
	public:
		/**
		 * @brief Конструктор
		 *
		 * @param fmk объект фреймворка
		 * @param log объект для работы с логами
		 *
		 */
		explicit Quic2Security(const awh::fmk_t * fmk, const awh::log_t * log) noexcept;
		/**
		 * @brief Деструктор
		 *
		 */
		~Quic2Security() noexcept;
};

#endif // __AWH_PROTO_QUIC2_TESTS__
