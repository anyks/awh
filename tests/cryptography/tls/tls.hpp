/**
 * @file tls.hpp
 * @date 2026-07-22
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
 * @brief Заголовочный файл тестовой фикстуры модуля транспортного уровня безопасности —
 *        объявление класса фикстуры Google Test, подготавливающего и освобождающего тестовое окружение набора тестов
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

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
#include "../../../include/cryptography/tls/coder.hpp"

/**
 * @brief Класс фикстуры для тестов кодера транспортной безопасности
 *
 * @details Набор характеризационный: он фиксирует наблюдаемое поведение кодера
 *          таким, какое оно есть, чтобы последующие изменения модуля опирались
 *          на проверяемый контракт, а не на чтение тринадцати тысяч строк
 *
 */
class TlsFixture : public testing::Test {
	protected:
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
		 *
		 */
		bool makeCertificate(std::string & certificate, std::string & privateKey, const std::string & host = "localhost") const noexcept;
};
