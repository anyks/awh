/**
 * @file: tls.cpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры модуля транспортного уровня безопасности —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>

/**
 * Заголовочные файлы BoringSSL
 */
#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>
#endif

/**
 * Подключаем заголовочный файл тестов кодера
 */
#include "tls.hpp"

/**
 * @brief Внутренние вспомогательные средства тестов кодера
 *
 */
namespace {
	/**
	 * @brief Функция получения пути к временному каталогу
	 *
	 * @return путь к временному каталогу
	 *
	 * @note У MS Windows путь берётся у самой системы, а не из окружения. Оболочка MSYS2
	 *       выставляет там TEMP и TMP по правилам POSIX (значением "/tmp"), но собранный
	 *       под MinGW двоичный файл - родной для Windows, и такой путь ему непонятен:
	 *       он раскрывается в "C:\\tmp", какого на машине нет вовсе
	 *
	 */
	static std::string directory() noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Буфер под путь к временному каталогу
			char buffer[MAX_PATH + 1]{0};
			// Получаем путь к временному каталогу у системы
			const DWORD size = ::GetTempPathA(static_cast <DWORD> (sizeof(buffer)), buffer);
			// Если путь к временному каталогу получен
			if((size > 0) && (size <= MAX_PATH)){
				// Формируем путь к временному каталогу
				std::string result(buffer, static_cast <size_t> (size));
				// Удаляем завершающий разделитель, добавляемый системой
				while(!result.empty() && ((result.back() == '\\') || (result.back() == '/')))
					// Удаляем завершающий разделитель
					result.pop_back();
				// Выводим путь к временному каталогу
				return result;
			}
			// Выводим запасной путь к временному каталогу
			return std::string(".");
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Получаем путь к временному каталогу из окружения
			const char * result = ::getenv("TMPDIR");
			// Выводим путь к временному каталогу
			return ((result != nullptr) ? std::string(result) : std::string("/tmp"));
		#endif
	}
	/**
	 * @brief Функция записи данных во временный файл
	 *
	 * @param path путь к создаваемому файлу
	 * @param data записываемые данные
	 * @return     результат записи
	 *
	 */
	static bool store(const std::string & path, const std::string & data) noexcept {
		// Открываем файл на запись
		FILE * file = ::fopen(path.c_str(), "wb");
		// Если файл не открыт
		if(file == nullptr)
			// Выводим отрицательный результат
			return false;
		// Записываем данные в файл
		const bool result = (::fwrite(data.data(), 1, data.size(), file) == data.size());
		// Закрываем файл
		::fclose(file);
		// Выводим результат записи
		return result;
	}
};

/**
 * @brief Метод настройки тестового окружения
 *
 */
void TlsFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект для работы с логами
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	/**
	 * Отключаем вывод логов в тестах: часть проверок намеренно подаёт кодеру
	 * некорректные данные, и его диагностика забивала бы вывод теста
	 */
	this->_log->level(awh::log_t::level_t::NONE);
	// Создаём объект кодера транспортной безопасности
	this->_coder = std::make_unique <awh::tls::Coder> (this->_fmk.get(), this->_log.get());
	// Выполняем генерацию самоподписанного сертификата тестового узла
	this->makeCertificate(this->_certificate, this->_privateKey);
}
/**
 * @brief Метод очистки тестового окружения
 *
 */
void TlsFixture::TearDown(){
	// Если файл сертификата создан
	if(!this->_certificate.empty())
		// Удаляем файл сертификата
		::remove(this->_certificate.c_str());
	// Если файл приватного ключа создан
	if(!this->_privateKey.empty())
		// Удаляем файл приватного ключа
		::remove(this->_privateKey.c_str());
	// Освобождаем объект кодера транспортной безопасности
	this->_coder.reset(nullptr);
	// Освобождаем объект для работы с логами
	this->_log.reset(nullptr);
	// Освобождаем объект фреймворка
	this->_fmk.reset(nullptr);
}
/**
 * @brief Метод генерации самоподписанного сертификата во временных файлах
 *
 * @param certificate путь к созданному файлу сертификата
 * @param privateKey  путь к созданному файлу приватного ключа
 * @param host        доменное имя субъекта сертификата
 * @return            результат генерации
 *
 */
bool TlsFixture::makeCertificate(std::string & certificate, std::string & privateKey, const std::string & host) const noexcept {
	// Результат генерации сертификата
	bool result = false;
	// Объект пары ключей
	EVP_PKEY * pkey = ::EVP_PKEY_new();
	// Объект ключа RSA
	RSA * rsa = ::RSA_new();
	// Объект открытой экспоненты
	BIGNUM * exponent = ::BN_new();
	// Если объекты созданы
	if((pkey != nullptr) && (rsa != nullptr) && (exponent != nullptr)){
		// Устанавливаем значение открытой экспоненты
		::BN_set_word(exponent, RSA_F4);
		// Выполняем генерацию пары ключей RSA
		::RSA_generate_key_ex(rsa, 2048, exponent, nullptr);
		// Передаём ключ RSA во владение паре ключей
		::EVP_PKEY_assign_RSA(pkey, rsa);
		// Объект сертификата
		X509 * x509 = ::X509_new();
		// Если объект сертификата создан
		if(x509 != nullptr){
			// Устанавливаем версию сертификата X.509v3
			::X509_set_version(x509, 2);
			// Устанавливаем серийный номер сертификата
			::ASN1_INTEGER_set(::X509_get_serialNumber(x509), 1);
			// Устанавливаем начало срока действия сертификата
			::X509_gmtime_adj(::X509_getm_notBefore(x509), 0);
			// Устанавливаем окончание срока действия сертификата
			::X509_gmtime_adj(::X509_getm_notAfter(x509), 31536000L);
			// Устанавливаем открытый ключ сертификата
			::X509_set_pubkey(x509, pkey);
			// Получаем объект имени субъекта сертификата
			X509_NAME * name = ::X509_get_subject_name(x509);
			// Устанавливаем доменное имя субъекта сертификата
			::X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast <const uint8_t *> (host.c_str()), -1, -1, 0);
			// Устанавливаем имя издателя сертификата
			::X509_set_issuer_name(x509, name);
			// Выполняем подпись сертификата
			::X509_sign(x509, pkey, ::EVP_sha256());
			// Буфер вывода сертификата
			BIO * cbio = ::BIO_new(::BIO_s_mem());
			// Буфер вывода приватного ключа
			BIO * kbio = ::BIO_new(::BIO_s_mem());
			// Если буферы вывода созданы
			if((cbio != nullptr) && (kbio != nullptr)){
				// Записываем сертификат в буфер вывода
				::PEM_write_bio_X509(cbio, x509);
				// Записываем приватный ключ в буфер вывода
				::PEM_write_bio_PrivateKey(kbio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
				// Данные буфера вывода
				char * data = nullptr;
				// Извлекаем данные сертификата
				long length = ::BIO_get_mem_data(cbio, &data);
				// Формируем содержимое сертификата
				const std::string cert(data, length);
				// Извлекаем данные приватного ключа
				length = ::BIO_get_mem_data(kbio, &data);
				// Формируем содержимое приватного ключа
				const std::string key(data, length);
				// Формируем основу пути к временным файлам
				const std::string base = (::directory() + "/awh-tls-" + host + "-" + std::to_string(reinterpret_cast <uintptr_t> (x509)));
				// Устанавливаем путь к файлу сертификата
				certificate = (base + ".crt");
				// Устанавливаем путь к файлу приватного ключа
				privateKey = (base + ".key");
				// Записываем сертификат и приватный ключ во временные файлы
				result = (::store(certificate, cert) && ::store(privateKey, key));
			}
			// Если буфер вывода сертификата создан
			if(cbio != nullptr)
				// Освобождаем буфер вывода сертификата
				::BIO_free(cbio);
			// Если буфер вывода приватного ключа создан
			if(kbio != nullptr)
				// Освобождаем буфер вывода приватного ключа
				::BIO_free(kbio);
			// Освобождаем объект сертификата
			::X509_free(x509);
		}
	}
	// Если объект открытой экспоненты создан
	if(exponent != nullptr)
		// Освобождаем объект открытой экспоненты
		::BN_free(exponent);
	// Если пара ключей создана
	if(pkey != nullptr)
		// Освобождаем пару ключей вместе с ключом RSA
		::EVP_PKEY_free(pkey);
	// Выводим результат генерации сертификата
	return result;
}
