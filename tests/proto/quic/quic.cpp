/**
 * @file: quic.cpp
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

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Заголовочные файлы BoringSSL (генерация тестового сертификата)
 */
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/nid.h>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "quic.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void QuicFixture::SetUp(){}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void QuicFixture::TearDown(){}

/**
 * @brief Метод преобразования шестнадцатеричной строки в бинарный буфер
 *
 * @param hex шестнадцатеричная строка
 * @return    бинарный буфер
 */
std::string QuicFixture::unhex(const std::string & hex) const noexcept {
	// Результирующий бинарный буфер
	std::string result;
	// Резервируем память под результат
	result.reserve(hex.size() / 2);
	/**
	 * Перебираем шестнадцатеричную строку парами символов
	 */
	for(size_t i = 0; (i + 1) < hex.size(); i += 2)
		// Преобразуем пару символов в октет
		result.push_back(static_cast <char> (std::stoi(hex.substr(i, 2), nullptr, 16)));
	// Выводим бинарный буфер
	return result;
}
/**
 * @brief Метод преобразования бинарного буфера в шестнадцатеричную строку
 *
 * @param data бинарный буфер
 * @return     шестнадцатеричная строка
 */
std::string QuicFixture::hex(const std::string & data) const noexcept {
	// Таблица шестнадцатеричных символов
	static const char SYMBOLS[] = "0123456789abcdef";
	// Результирующая шестнадцатеричная строка
	std::string result;
	// Резервируем память под результат
	result.reserve(data.size() * 2);
	/**
	 * Перебираем бинарный буфер
	 */
	for(size_t i = 0; i < data.size(); i++){
		// Октет бинарного буфера
		const uint8_t byte = static_cast <uint8_t> (data[i]);
		// Дописываем старший полуоктет
		result.push_back(SYMBOLS[byte >> 4]);
		// Дописываем младший полуоктет
		result.push_back(SYMBOLS[byte & 0x0F]);
	}
	// Выводим шестнадцатеричную строку
	return result;
}
/**
 * @brief Метод создания идентификатора соединения из бинарного буфера
 *
 * @param data бинарный буфер идентификатора
 * @return     сформированный идентификатор соединения
 */
awh::quic::cid_t QuicFixture::makeCid(const std::string & data) const noexcept {
	// Результирующий идентификатор соединения
	awh::quic::cid_t result;
	// Устанавливаем длину идентификатора соединения
	result.size = data.size();
	// Если буфер не пустой
	if(!data.empty())
		// Копируем данные идентификатора соединения
		::memcpy(result.data, data.data(), data.size());
	// Выводим идентификатор соединения
	return result;
}
/**
 * @brief Метод генерации самоподписанного сертификата в памяти
 *
 * @param certificate сертификат в формате PEM
 * @param privateKey  приватный ключ в формате PEM
 * @return            результат генерации
 */
bool QuicFixture::makeCertificate(std::string & certificate, std::string & privateKey) const noexcept {
	// Результат работы функции
	bool result = false;
	// Объект приватного ключа
	EVP_PKEY * pkey = nullptr;
	// Создаём контекст генерации ключа на эллиптической кривой P-256
	EVP_PKEY_CTX * pctx = ::EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
	// Если контекст генерации ключа создан
	if(pctx != nullptr){
		// Выполняем генерацию приватного ключа
		if((::EVP_PKEY_keygen_init(pctx) == 1) &&
		   (::EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1) == 1) &&
		   (::EVP_PKEY_keygen(pctx, &pkey) == 1)){
			// Создаём объект сертификата
			X509 * x509 = ::X509_new();
			// Если объект сертификата создан
			if(x509 != nullptr){
				// Устанавливаем серийный номер сертификата
				::ASN1_INTEGER_set(::X509_get_serialNumber(x509), 1);
				// Устанавливаем начало срока действия сертификата
				::X509_gmtime_adj(::X509_getm_notBefore(x509), 0);
				// Устанавливаем окончание срока действия сертификата (один час)
				::X509_gmtime_adj(::X509_getm_notAfter(x509), 3600);
				// Устанавливаем публичный ключ сертификата
				::X509_set_pubkey(x509, pkey);
				// Получаем объект субъекта сертификата
				X509_NAME * name = ::X509_get_subject_name(x509);
				// Устанавливаем общее имя субъекта сертификата
				::X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast <const uint8_t *> ("localhost"), -1, -1, 0);
				// Устанавливаем издателя сертификата (самоподписанный)
				::X509_set_issuer_name(x509, name);
				// Подписываем сертификат приватным ключом
				if(::X509_sign(x509, pkey, ::EVP_sha256()) > 0){
					// Создаём буфер BIO для записи сертификата
					BIO * cbio = ::BIO_new(::BIO_s_mem());
					// Создаём буфер BIO для записи приватного ключа
					BIO * kbio = ::BIO_new(::BIO_s_mem());
					// Если буферы BIO созданы и записаны
					if((cbio != nullptr) && (kbio != nullptr) &&
					   (::PEM_write_bio_X509(cbio, x509) == 1) &&
					   (::PEM_write_bio_PrivateKey(kbio, pkey, nullptr, nullptr, 0, nullptr, nullptr) == 1)){
						// Данные буфера BIO
						char * data = nullptr;
						// Извлекаем данные сертификата
						const long csize = ::BIO_get_mem_data(cbio, &data);
						// Устанавливаем сертификат в формате PEM
						certificate.assign(data, static_cast <size_t> (csize));
						// Извлекаем данные приватного ключа
						const long ksize = ::BIO_get_mem_data(kbio, &data);
						// Устанавливаем приватный ключ в формате PEM
						privateKey.assign(data, static_cast <size_t> (ksize));
						// Устанавливаем положительный результат
						result = (!certificate.empty() && !privateKey.empty());
					}
					// Если буфер BIO сертификата создан, освобождаем его
					if(cbio != nullptr)
						// Освобождаем буфер BIO сертификата
						::BIO_free(cbio);
					// Если буфер BIO приватного ключа создан, освобождаем его
					if(kbio != nullptr)
						// Освобождаем буфер BIO приватного ключа
						::BIO_free(kbio);
				}
				// Освобождаем объект сертификата
				::X509_free(x509);
			}
		}
		// Освобождаем контекст генерации ключа
		::EVP_PKEY_CTX_free(pctx);
	}
	// Если приватный ключ создан, освобождаем его
	if(pkey != nullptr)
		// Освобождаем объект приватного ключа
		::EVP_PKEY_free(pkey);
	// Выводим результат генерации
	return result;
}
