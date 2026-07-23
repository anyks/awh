/**
 * @file: tls.hpp
 * @date: 2026-07-22
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

#ifndef __AWH_TLS_CODER_TESTS__
#define __AWH_TLS_CODER_TESTS__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/sys/fmk.hpp"
#include "../../../include/sys/log.hpp"
#include "../../../include/net/tls/coder.hpp"

/**
 * @brief Класс фикстуры для тестов кодера транспортной безопасности
 *
 * @details Набор характеризационный: он фиксирует наблюдаемое поведение кодера
 *          таким, какое оно есть, чтобы последующие изменения модуля опирались
 *          на проверяемый контракт, а не на чтение тринадцати тысяч строк
 */
class TlsFixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект для работы с логами
		std::unique_ptr <awh::log_t> _log;
		// Объект кодера транспортной безопасности
		std::unique_ptr <awh::tls::Coder> _coder;
	protected:
		// Путь к файлу сертификата тестового узла
		std::string _certificate;
		// Путь к файлу приватного ключа тестового узла
		std::string _privateKey;
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
		 * @brief Метод генерации самоподписанного сертификата во временных файлах
		 *
		 * @param certificate путь к созданному файлу сертификата
		 * @param privateKey  путь к созданному файлу приватного ключа
		 * @param host        доменное имя субъекта сертификата
		 * @return            результат генерации
		 */
		bool makeCertificate(std::string & certificate, std::string & privateKey, const std::string & host = "localhost") const noexcept;
};

#endif // __AWH_TLS_CODER_TESTS__
