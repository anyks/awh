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
#include <cstdio>
#include <cstdlib>
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
 * @brief Внутренние вспомогательные средства тестового окружения
 *
 */
namespace {
	/**
	 * @brief Функция получения пути к временному каталогу
	 *
	 * @return путь к временному каталогу
	 */
	static std::string directory() noexcept {
		// Получаем путь к временному каталогу из окружения
		const char * result = ::getenv("TMPDIR");
		// Выводим путь к временному каталогу
		return ((result != nullptr) ? std::string(result) : std::string("/tmp"));
	}
	/**
	 * @brief Функция записи данных во временный файл
	 *
	 * @param path путь к создаваемому файлу
	 * @param data записываемые данные
	 * @return     результат записи
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
	/**
	 * @brief Функция генерации самоподписанного сертификата в памяти
	 *
	 * @param certificate сертификат в формате PEM
	 * @param privateKey  приватный ключ в формате PEM
	 * @return            результат генерации
	 */
	static bool makeCertificate(std::string & certificate, std::string & privateKey) noexcept {
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
};

/**
 * @brief Метод доступа к объекту кодера транспортной безопасности
 *
 * @return объект кодера транспортной безопасности
 */
awh::tls::Coder & QuicSecurity::coder() noexcept {
	// Выводим объект кодера транспортной безопасности
	return this->_coder;
}
/**
 * @brief Метод извлечения шаблона контекста безопасности роли эндпоинта
 *
 * @param endpoint роль эндпоинта
 * @return         идентификатор шаблона контекста безопасности
 */
awh::tls::Coder::id_t QuicSecurity::context(const awh::quic::endpoint_t endpoint) const noexcept {
	// Выводим шаблон контекста безопасности запрошенной роли
	return ((endpoint == awh::quic::endpoint_t::CLIENT) ? this->_client : this->_server);
}
/**
 * @brief Метод создания отдельного шаблона контекста безопасности
 *
 * @param endpoint  роль эндпоинта
 * @param protocols список поддерживаемых ALPN-протоколов
 * @param validate  режим проверки сертификата удалённого узла
 * @return          идентификатор созданного шаблона контекста безопасности
 */
awh::tls::Coder::id_t QuicSecurity::make(const awh::quic::endpoint_t endpoint, const std::vector <awh::tls::Coder::alpn_t> & protocols, const bool validate) noexcept {
	// Определяем роль узла кодера
	const awh::event::node_t node = ((endpoint == awh::quic::endpoint_t::CLIENT) ? awh::event::node_t::CLIENT : awh::event::node_t::SERVER);
	// Создаём шаблон контекста безопасности протокола QUIC
	const awh::tls::Coder::id_t result = this->_coder.context(node, awh::event::protocol_t::QUIC);
	// Если шаблон контекста безопасности не создан
	if(result == 0)
		// Выводим отрицательный результат
		return result;
	// Устанавливаем список поддерживаемых ALPN-протоколов
	this->_coder.alpn(result, protocols);
	// Если эндпоинт является сервером
	if(endpoint == awh::quic::endpoint_t::SERVER){
		// Устанавливаем сертификат тестового узла
		this->_coder.certificate(result, this->_certificate);
		// Устанавливаем приватный ключ тестового узла
		this->_coder.privateKey(result, this->_privateKey);
		/**
		 * Снимаем проверку на сервере: шаблон контекста создаётся с включённой
		 * проверкой, а на серверном узле это означает требование клиентского
		 * сертификата, то есть взаимную аутентификацию. Тесты проверяют
		 * односторонний TLS, поэтому требование снимается явно
		 */
		this->_coder.validateServerNameIndication(result, false);
	// Если эндпоинт является клиентом
	} else {
		// Устанавливаем доменное имя удалённого узла
		this->_coder.serverNameIndication(result, "localhost");
		// Устанавливаем доверенный центр сертификации тестового узла
		this->_coder.ca(result, this->_certificate);
		// Устанавливаем режим проверки сертификата удалённого узла
		this->_coder.validateServerNameIndication(result, validate);
	}
	// Выводим созданный шаблон контекста безопасности
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
QuicSecurity::QuicSecurity(const awh::fmk_t * fmk, const awh::log_t * log) noexcept :
 _client(0), _server(0), _coder(fmk, log) {
	// Сертификат тестового узла в формате PEM
	std::string certificate = "";
	// Приватный ключ тестового узла в формате PEM
	std::string privateKey = "";
	// Если генерация самоподписанного сертификата не выполнена
	if(!::makeCertificate(certificate, privateKey))
		// Выходим из конструктора - контексты останутся несозданными
		return;
	// Формируем основу пути к временным файлам
	const std::string base = (::directory() + "/awh-quic-" + std::to_string(reinterpret_cast <uintptr_t> (this)));
	// Устанавливаем путь к файлу сертификата
	this->_certificate = (base + ".crt");
	// Устанавливаем путь к файлу приватного ключа
	this->_privateKey = (base + ".key");
	// Если запись сертификата и приватного ключа во временные файлы не выполнена
	if(!::store(this->_certificate, certificate) || !::store(this->_privateKey, privateKey))
		// Выходим из конструктора - контексты останутся несозданными
		return;
	// Создаём шаблон контекста безопасности клиента
	this->_client = this->make(awh::quic::endpoint_t::CLIENT, {awh::tls::Coder::alpn_t{0, "h3"}});
	// Создаём шаблон контекста безопасности сервера
	this->_server = this->make(awh::quic::endpoint_t::SERVER, {awh::tls::Coder::alpn_t{0, "h3"}});
}
/**
 * @brief Деструктор
 *
 */
QuicSecurity::~QuicSecurity() noexcept {
	// Если файл сертификата создан
	if(!this->_certificate.empty())
		// Удаляем файл сертификата
		::remove(this->_certificate.c_str());
	// Если файл приватного ключа создан
	if(!this->_privateKey.empty())
		// Удаляем файл приватного ключа
		::remove(this->_privateKey.c_str());
}
/**
 * @brief Функция получения объекта фреймворка
 *
 * @return объект фреймворка
 */
awh::fmk_t & framework() noexcept {
	// Объект фреймворка
	static awh::fmk_t result;
	// Выводим объект фреймворка
	return result;
}
/**
 * @brief Функция получения объекта логирования
 *
 * @return объект логирования
 */
awh::log_t & logger() noexcept {
	// Объект логирования
	static awh::log_t result(&::framework());
	// Выводим объект логирования
	return result;
}
/**
 * @brief Функция получения тестового окружения транспортной безопасности
 *
 * @return тестовое окружение транспортной безопасности
 */
QuicSecurity & security() noexcept {
	// Тестовое окружение транспортной безопасности
	static QuicSecurity result(&::framework(), &::logger());
	// Выводим тестовое окружение транспортной безопасности
	return result;
}
