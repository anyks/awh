/**
 * @file cert.hpp
 * @date 2026-11-19
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
 * @brief Общий заголовочный файл самоподписанного сертификата тестового узла —
 *        сертификат и приватный ключ в формате PEM для заведения контекстов TLS в тестах
 *
 * @copyright Copyright © 2026
 *
 */

#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <string_view>

/**
 * @brief Функция получения самоподписанного сертификата тестового узла
 *
 * @details Сертификат ECDSA на кривой P-256 с общим именем «localhost» и дополнительным
 *          именем DNS:localhost, срок действия 2026-2126. Лежит статической фикстурой:
 *          тест не зависит ни от стороннего криптопровайдера, ни от раскладки файлов репозитория
 *
 * @return сертификат в формате PEM
 *
 */
static inline std::string_view testCertificate() noexcept {
	// Выводим самоподписанный сертификат тестового узла
	return R"PEM(-----BEGIN CERTIFICATE-----
MIIBlDCCATugAwIBAgIUKbB0jPVKqTDsGDF/aL0zVvGmVi4wCgYIKoZIzj0EAwIw
FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MTExOTIwNTUzOFoYDzIxMjYxMDI2
MjA1NTM4WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO
PQMBBwNCAAS5X21MfaUjBzjcwyYMqf9puZbqlWf26fUVZdk2wfBLUTKyMizRCEzE
j+3cV1bTvu1BE81mu17gAjQSYu0kTt+4o2kwZzAdBgNVHQ4EFgQUQ11TC6tt2cJx
1dFOfWQwV4uudTcwHwYDVR0jBBgwFoAUQ11TC6tt2cJx1dFOfWQwV4uudTcwDwYD
VR0TAQH/BAUwAwEB/zAUBgNVHREEDTALgglsb2NhbGhvc3QwCgYIKoZIzj0EAwID
RwAwRAIgM1RcmK9Zir9nZh96MG1JGaQwxrx0U1zwzPdOZ4lnBBACIBLwr+ZlSBaf
gnttla0Vx/lg+2SPgM8BXrQkcM48PYBR
-----END CERTIFICATE-----
)PEM";
}

/**
 * @brief Функция получения приватного ключа тестового узла
 *
 * @details Ключ ECDSA на кривой P-256, парный сертификату testCertificate().
 *          Лежит статической фикстурой: тест не зависит ни от стороннего
 *          криптопровайдера, ни от раскладки файлов репозитория
 *
 * @return приватный ключ в формате PEM
 *
 */
static inline std::string_view testPrivateKey() noexcept {
	// Выводим приватный ключ тестового узла
	return R"PEM(-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgkMB11qRN1u6YnqDa
ef+q30LMw1zw/5gJlqeaeAhRA5+hRANCAAS5X21MfaUjBzjcwyYMqf9puZbqlWf2
6fUVZdk2wfBLUTKyMizRCEzEj+3cV1bTvu1BE81mu17gAjQSYu0kTt+4
-----END PRIVATE KEY-----
)PEM";
}
