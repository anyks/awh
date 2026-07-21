/**
 * @file: quic.hpp
 * @date: 2026-07-21
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

#ifndef __AWH_PROTO_QUIC_TESTS__
#define __AWH_PROTO_QUIC_TESTS__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/proto/quic/quic.hpp"
#include "../../../include/proto/quic/frame.hpp"
#include "../../../include/proto/quic/packet.hpp"
#include "../../../include/proto/quic/varint.hpp"

/**
 * @brief Класс фикстуры для тестов подмодуля протокола QUIC
 *
 */
class QuicFixture : public testing::Test {
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
		 * @brief Метод преобразования шестнадцатеричной строки в бинарный буфер
		 *
		 * @param hex шестнадцатеричная строка
		 * @return    бинарный буфер
		 */
		std::string unhex(const std::string & hex) const noexcept;
		/**
		 * @brief Метод преобразования бинарного буфера в шестнадцатеричную строку
		 *
		 * @param data бинарный буфер
		 * @return     шестнадцатеричная строка
		 */
		std::string hex(const std::string & data) const noexcept;
		/**
		 * @brief Метод создания идентификатора соединения из бинарного буфера
		 *
		 * @param data бинарный буфер идентификатора
		 * @return     сформированный идентификатор соединения
		 */
		awh::quic::cid_t makeCid(const std::string & data) const noexcept;
		/**
		 * @brief Метод генерации самоподписанного сертификата в памяти
		 *
		 * @param certificate сертификат в формате PEM
		 * @param privateKey  приватный ключ в формате PEM
		 * @return            результат генерации
		 */
		bool makeCertificate(std::string & certificate, std::string & privateKey) const noexcept;
};

#endif // __AWH_PROTO_QUIC_TESTS__
