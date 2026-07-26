/**
 * @file: quic.cpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация окружения бенчмарков протокола QUIC —
 *        генерация самоподписанного сертификата и ключа средствами BoringSSL,
 *        настройка контекста TLS и подготовка пары клиентского и серверного соединений для замеров
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
 * Подключаем заголовочные файлы проекта
 */
#include "quic.hpp"
#include "../../../include/proto/quic/params.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние счётчики выделений памяти
 *
 * @details Операторы выделения памяти перегружаются глобально: учёт нужен, чтобы
 *          отличить регрессию по количеству выделений от регрессии по времени.
 *          Учёт включается только на время измерения, поэтому подготовка
 *          сценария в статистику не попадает
 *
 */
namespace {
	// Количество выполненных выделений памяти
	static size_t gCount = 0;
	// Суммарный объём выделенной памяти в октетах
	static size_t gBytes = 0;
	// Флаг активности учёта выделений памяти
	static bool gEnabled = false;
};

/**
 * @brief Оператор выделения памяти с учётом статистики
 *
 * @param size размер выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
void * operator new (size_t size){
	// Если учёт выделений памяти активен
	if(gEnabled){
		// Считаем выполненное выделение памяти
		gCount++;
		// Суммируем объём выделенной памяти
		gBytes += size;
	}
	// Выполняем выделение памяти
	void * result = ::malloc(size > 0 ? size : 1);
	// Если память не выделена
	if(result == nullptr)
		// Завершаем приложение - обработка нехватки памяти в бенчмарке не предусмотрена
		::abort();
	// Выводим указатель на выделенную память
	return result;
}
/**
 * @brief Оператор выделения памяти под массив с учётом статистики
 *
 * @param size размер выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
void * operator new [] (size_t size){
	// Выполняем выделение памяти
	return operator new (size);
}
/**
 * @brief Оператор освобождения памяти
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete (void * ptr) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти массива
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete [] (void * ptr) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти с указанием размера
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete (void * ptr, size_t) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти массива с указанием размера
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete [] (void * ptr, size_t) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}

/**
 * @brief Функция подсчёта выделений памяти
 *
 * @param count количество выполненных выделений
 * @param bytes суммарный объём выделенной памяти в октетах
 *
 */
void awh::benchmark::quic::allocations(size_t & count, size_t & bytes) noexcept {
	// Выводим количество выполненных выделений памяти
	count = gCount;
	// Выводим суммарный объём выделенной памяти
	bytes = gBytes;
}
/**
 * @brief Функция управления учётом выделений памяти
 *
 * @param mode режим учёта выделений памяти
 *
 */
void awh::benchmark::quic::counting(const bool mode) noexcept {
	// Если учёт выделений памяти включается
	if(mode){
		// Обнуляем количество выполненных выделений памяти
		gCount = 0;
		// Обнуляем суммарный объём выделенной памяти
		gBytes = 0;
	}
	// Устанавливаем режим учёта выделений памяти
	gEnabled = mode;
}
/**
 * @brief Внутренние вспомогательные средства окружения безопасности
 *
 */
namespace {
	/**
	 * @brief Функция получения пути к временному каталогу
	 *
	 * @return путь к временному каталогу
	 *
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
	/**
	 * @brief Функция генерации самоподписанного сертификата в памяти
	 *
	 * @param certificate сертификат в формате PEM
	 * @param privateKey  приватный ключ в формате PEM
	 * @return            результат генерации
	 *
	 */
	static bool makeCertificate(std::string & certificate, std::string & privateKey) noexcept {
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
				::X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast <const uint8_t *> ("localhost"), -1, -1, 0);
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
					// Устанавливаем сертификат в формате PEM
					certificate.assign(data, length);
					// Извлекаем данные приватного ключа
					length = ::BIO_get_mem_data(kbio, &data);
					// Устанавливаем приватный ключ в формате PEM
					privateKey.assign(data, length);
					// Определяем результат генерации сертификата
					result = (!certificate.empty() && !privateKey.empty());
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
};
/**
 * @brief Метод доступа к объекту кодера транспортной безопасности
 *
 * @return объект кодера транспортной безопасности
 *
 */
awh::tls::Coder & awh::benchmark::quic::Security::coder() noexcept {
	// Выводим объект кодера транспортной безопасности
	return this->_coder;
}
/**
 * @brief Метод извлечения шаблона контекста безопасности роли эндпоинта
 *
 * @param endpoint роль эндпоинта
 * @return         идентификатор шаблона контекста безопасности
 *
 */
awh::tls::Coder::id_t awh::benchmark::quic::Security::context(const awh::quic::endpoint_t endpoint) const noexcept {
	// Выводим шаблон контекста безопасности запрошенной роли
	return ((endpoint == awh::quic::endpoint_t::CLIENT) ? this->_client : this->_server);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::benchmark::quic::Security::Security(const awh::fmk_t * fmk, const awh::log_t * log) noexcept :
 _client(0), _server(0), _coder(fmk, log) {
	// Сертификат узла бенчмарка в формате PEM
	std::string certificate = "";
	// Приватный ключ узла бенчмарка в формате PEM
	std::string privateKey = "";
	// Если генерация самоподписанного сертификата не выполнена
	if(!::makeCertificate(certificate, privateKey))
		// Выходим из конструктора - контексты останутся несозданными
		return;
	// Формируем основу пути к временным файлам
	const std::string base = (::directory() + "/awh-quic-benchmark-" + std::to_string(reinterpret_cast <uintptr_t> (this)));
	// Устанавливаем путь к файлу сертификата
	this->_certificate = (base + ".crt");
	// Устанавливаем путь к файлу приватного ключа
	this->_privateKey = (base + ".key");
	// Если запись сертификата и приватного ключа во временные файлы не выполнена
	if(!::store(this->_certificate, certificate) || !::store(this->_privateKey, privateKey))
		// Выходим из конструктора - контексты останутся несозданными
		return;
	// Создаём шаблоны контекста безопасности протокола QUIC
	this->_client = this->_coder.context(event::node_t::CLIENT, event::protocol_t::QUIC);
	// Создаём шаблон контекста безопасности сервера
	this->_server = this->_coder.context(event::node_t::SERVER, event::protocol_t::QUIC);
	// Если шаблоны контекста безопасности не созданы
	if((this->_client == 0) || (this->_server == 0))
		// Выходим из конструктора
		return;
	// Устанавливаем список поддерживаемых ALPN-протоколов клиента (RFC 9001 §8.1)
	this->_coder.alpn(this->_client, {tls::Coder::alpn_t{0, "h3"}});
	// Устанавливаем список поддерживаемых ALPN-протоколов сервера
	this->_coder.alpn(this->_server, {tls::Coder::alpn_t{0, "h3"}});
	// Устанавливаем доменное имя удалённого узла на клиенте
	this->_coder.serverNameIndication(this->_client, "localhost");
	// Снимаем проверку сертификата удалённого узла на клиенте
	this->_coder.validateServerNameIndication(this->_client, false);
	// Устанавливаем сертификат узла бенчмарка на сервере
	this->_coder.certificate(this->_server, this->_certificate);
	// Устанавливаем приватный ключ узла бенчмарка на сервере
	this->_coder.privateKey(this->_server, this->_privateKey);
	/**
	 * Снимаем проверку на сервере: шаблон контекста создаётся с включённой
	 * проверкой, а на серверном узле это означает требование клиентского
	 * сертификата, то есть взаимную аутентификацию
	 */
	this->_coder.validateServerNameIndication(this->_server, false);
}
/**
 * @brief Деструктор
 *
 */
awh::benchmark::quic::Security::~Security() noexcept {
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
 * @brief Функция получения окружения транспортной безопасности бенчмарка
 *
 * @return окружение транспортной безопасности бенчмарка
 *
 */
awh::benchmark::quic::Security & awh::benchmark::quic::security() noexcept {
	// Объект фреймворка окружения
	static awh::fmk_t fmk;
	// Объект логирования окружения
	static awh::log_t log(&fmk);
	// Окружение транспортной безопасности бенчмарка
	static Security result(&fmk, &log);
	// Выводим окружение транспортной безопасности бенчмарка
	return result;
}
/**
 * @brief Функция подготовки соединения к бенчмарку
 *
 * @param connection объект соединения
 *
 */
void awh::benchmark::quic::configure(awh::quic::connection_t & connection) noexcept {
	// Транспортные параметры эндпоинта
	awh::quic::params::params_t params;
	/**
	 * Лимиты выбраны заведомо широкими: бенчмарк измеряет стоимость обработки,
	 * а не поведение flow control, поэтому упираться в окна он не должен
	 */
	params.initialMaxData = 1073741824;
	// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiLocal = 268435456;
	// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiRemote = 268435456;
	// Устанавливаем лимит данных однонаправленных потоков
	params.initialMaxStreamDataUni = 268435456;
	// Устанавливаем лимит числа двунаправленных потоков
	params.initialMaxStreamsBidi = 1024;
	// Устанавливаем лимит числа однонаправленных потоков
	params.initialMaxStreamsUni = 1024;
	// Устанавливаем транспортные параметры
	connection.params(params);
}
/**
 * @brief Функция выполнения хендшейка между клиентом и сервером
 *
 * @param client эндпоинт клиента
 * @param server эндпоинт сервера
 * @param now    текущее время тестовых часов в миллисекундах
 * @return       результат установления соединения
 *
 */
bool awh::benchmark::quic::establish(awh::quic::connection_t & client, awh::quic::connection_t & server, uint64_t & now) noexcept {
	// Буфер передаваемой датаграммы
	std::string datagram = "";
	/**
	 * Выполняем обмен датаграммами до установления соединения
	 */
	for(size_t i = 0; i < 16; i++){
		/**
		 * Передаём датаграммы клиента серверу
		 */
		while(client.write(datagram, now)){
			// Продвигаем часы бенчмарка
			now += 1;
			// Передаём датаграмму серверу
			server.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now);
		}
		/**
		 * Передаём датаграммы сервера клиенту
		 */
		while(server.write(datagram, now)){
			// Продвигаем часы бенчмарка
			now += 1;
			// Передаём датаграмму клиенту
			client.read(reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size(), now);
		}
		// Если соединение установлено на обоих эндпоинтах
		if((client.state() == awh::quic::connection_t::state_t::CONNECTED) &&
		   (server.state() == awh::quic::connection_t::state_t::CONNECTED))
			// Выводим положительный результат
			return true;
	}
	// Выводим отрицательный результат - обмен не сошёлся
	return false;
}
