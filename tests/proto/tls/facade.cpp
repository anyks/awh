/**
 * @file facade.cpp
 * @date 2026-08-14
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
 * @brief Интеграционные тесты фасадов клиента и сервера поверх TLS - проверка сквозного
 *        обмена шифрованными данными по реальным TCP-сокетам в одном процессе (loopback).
 *
 * @details Главное, что здесь проверяется - сохранность записей TLS при переполнении
 *          очереди отправки. Шифротекст выдаётся сетевому движку вытягивающей моделью:
 *          движок забирает его ровно тогда, когда готов отправить. Прежде шифротекст
 *          писался в сокет напрямую, и при переполнении очереди запись терялась молча.
 *          Потеря даже одной записи рвёт поток шифрования: удалённая сторона получает
 *          следующую запись с неверным счётчиком и обрывает соединение. Поэтому объём
 *          нагрузки берётся заведомо больше очереди отправки, а принятое сверяется
 *          с отправленным побайтово в обе стороны.
 *
 * @note База событий едина на весь процесс: цикл событий запускает и блокирует ровно
 *       один юнит-лаунчер. Поэтому сервер поднимается лаунчером в фоновом потоке,
 *       а клиент стартует и подключается уже из серверного статус-коллбэка (тот же
 *       поток цикла) - так весь обмен остаётся на одном потоке. Тестовый поток лишь
 *       ожидает завершения обмена и по сторожевому таймауту будит цикл вызовом stop()
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы операционной системы MS Windows
 *
 * @note Путь к временному каталогу берётся у самой системы вызовом GetTempPathA,
 *       а он вместе с MAX_PATH и DWORD объявлен здесь: без этого заголовка ветка
 *       Windows не собирается вовсе, тогда как прочие системы её не разбирают
 *
 * @note Признак NOCRYPT обязателен: без него заголовок тянет wincrypt.h, а тот
 *       объявляет X509_NAME, X509_EXTENSIONS и PKCS7_SIGNER_INFO макросами, и
 *       заголовки OpenSSL следом за ним не разбираются вовсе
 */
#if _WIN32 || _WIN64
	#define NOCRYPT
	#include <windows.h>
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

/**
 * Подключаем заголовочные файлы тестового фреймворка
 */
#include <gtest/gtest.h>

/**
 * Подключаем заголовочные файлы OpenSSL для генерации самоподписанного сертификата
 */
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

/**
 * Подключаем заголовочные файлы фасадов клиента и сервера
 */
#include "../../../include/client/client.hpp"
#include "../../../include/server/server.hpp"

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён заполнителей связывания функций
 */
using namespace std::placeholders;

/**
 * @brief Внутренние средства интеграционного окружения фасадов TLS
 *
 */
namespace {
	/**
	 * @brief Функция получения пути к временному каталогу
	 *
	 * @note У MS Windows путь берётся у самой системы, а не из окружения: оболочка MSYS2
	 *       выставляет TEMP и TMP по правилам POSIX, а собранный под MinGW двоичный файл
	 *       родной для Windows, и такой путь ему непонятен
	 *
	 * @return путь к временному каталогу
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
	/**
	 * @brief Функция генерации самоподписанного сертификата в памяти
	 *
	 * @param certificate сертификат в формате PEM
	 * @param privateKey  приватный ключ в формате PEM
	 * @return            результат генерации
	 *
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
						// Если буфер BIO сертификата создан
						if(cbio != nullptr)
							// Освобождаем буфер BIO сертификата
							::BIO_free(cbio);
						// Если буфер BIO приватного ключа создан
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
		// Если приватный ключ создан
		if(pkey != nullptr)
			// Освобождаем объект приватного ключа
			::EVP_PKEY_free(pkey);
		// Выводим результат генерации
		return result;
	}
	/**
	 * @brief Класс тестового окружения транспортной безопасности поверх TCP
	 *
	 */
	class Security {
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
			awh::tls::Coder & coder() noexcept {
				// Выводим объект кодера транспортной безопасности
				return this->_coder;
			}
			/**
			 * @brief Метод извлечения шаблона контекста безопасности клиента
			 *
			 * @return идентификатор шаблона контекста безопасности
			 *
			 */
			awh::tls::Coder::id_t client() const noexcept {
				// Выводим шаблон контекста безопасности клиента
				return this->_client;
			}
			/**
			 * @brief Метод извлечения шаблона контекста безопасности сервера
			 *
			 * @return идентификатор шаблона контекста безопасности
			 *
			 */
			awh::tls::Coder::id_t server() const noexcept {
				// Выводим шаблон контекста безопасности сервера
				return this->_server;
			}
			/**
			 * @brief Метод готовности тестового окружения безопасности
			 *
			 * @return признак готовности окружения
			 *
			 */
			bool ready() const noexcept {
				// Выводим признак создания обоих шаблонов контекста безопасности
				return ((this->_client > 0) && (this->_server > 0));
			}
		public:
			/**
			 * Запрещаем копирование и перемещение (окружение владеет контекстами кодера)
			 */
			Security(const Security &) = delete;
			Security(Security &&) = delete;
			Security & operator = (const Security &) = delete;
			Security & operator = (Security &&) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Security(const awh::fmk_t * fmk, const awh::log_t * log) noexcept :
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
				const std::string base = (::directory() + "/awh-tls-" + std::to_string(reinterpret_cast <uintptr_t> (this)));
				// Устанавливаем путь к файлу сертификата
				this->_certificate = (base + ".crt");
				// Устанавливаем путь к файлу приватного ключа
				this->_privateKey = (base + ".key");
				// Если запись сертификата и приватного ключа во временные файлы не выполнена
				if(!::store(this->_certificate, certificate) || !::store(this->_privateKey, privateKey))
					// Выходим из конструктора - контексты останутся несозданными
					return;
				// Создаём шаблон контекста безопасности сервера поверх TCP
				this->_server = this->_coder.context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
				// Если шаблон контекста безопасности сервера создан
				if(this->_server > 0){
					// Устанавливаем сертификат тестового узла
					this->_coder.certificate(this->_server, this->_certificate);
					// Устанавливаем приватный ключ тестового узла
					this->_coder.privateKey(this->_server, this->_privateKey);
					/**
					 * Снимаем проверку на сервере: шаблон контекста создаётся с включённой
					 * проверкой, а на серверном узле это означает требование клиентского
					 * сертификата, то есть взаимную аутентификацию. Проверяется односторонний
					 * TLS, поэтому требование снимается явно
					 */
					this->_coder.validateServerNameIndication(this->_server, false);
				}
				// Создаём шаблон контекста безопасности клиента поверх TCP
				this->_client = this->_coder.context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
				// Если шаблон контекста безопасности клиента создан
				if(this->_client > 0){
					// Устанавливаем доменное имя удалённого узла
					this->_coder.serverNameIndication(this->_client, "localhost");
					// Устанавливаем доверенный центр сертификации тестового узла
					this->_coder.ca(this->_client, this->_certificate);
					// Снимаем проверку сертификата удалённого узла (сертификат самоподписанный)
					this->_coder.validateServerNameIndication(this->_client, false);
				}
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Security() noexcept {
				// Если файл сертификата создан
				if(!this->_certificate.empty())
					// Удаляем файл сертификата
					::remove(this->_certificate.c_str());
				// Если файл приватного ключа создан
				if(!this->_privateKey.empty())
					// Удаляем файл приватного ключа
					::remove(this->_privateKey.c_str());
			}
	};
	/**
	 * @brief Итоги прогона сквозного обмена
	 *
	 */
	struct result_t {
		// Клиент установил соединение с сервером
		bool connected = false;
		// Обмен завершён штатно (не по сторожевому таймауту)
		bool completed = false;
		// Прогон прерван сторожевым таймаутом
		bool timedOut = false;
		// Цикл событий не откликнулся на остановку и был оставлен работать
		bool unstoppable = false;
		// Сервер принял всю нагрузку и она побайтово совпала с отправленной
		bool serverMatched = false;
		// Клиент принял всё эхо и оно побайтово совпало с отправленной нагрузкой
		bool clientMatched = false;
		// Число октетов, принятых сервером
		size_t serverBytes = 0;
		// Число октетов, принятых клиентом
		size_t clientBytes = 0;
		// Возвращённое клиентским send() число октетов
		size_t sendReturn = 0;
		// Число вызовов приёма на клиенте (эхо приходит частями)
		size_t clientReads = 0;
	};
	// Предел ожидания отклика цикла событий на остановку в секундах
	static constexpr uint32_t SHUTDOWN_TIMEOUT = 5;

	/**
	 * @brief Метод доступа к хранилищу неостановленных фасадов
	 *
	 * @details Фасад, чей цикл событий не откликнулся на остановку, разрушать нельзя:
	 *          его поток продолжает обращаться к полям объекта. Такие фасады
	 *          складываются сюда и живут до конца работы процесса.
	 *
	 * @note Хранилище намеренно не опустошается: это не утечка по недосмотру, а
	 *       единственный безопасный исход, когда поток остановить не удалось
	 *
	 * @return хранилище фасадов, оставленных работать
	 *
	 */
	static std::vector <std::pair <std::unique_ptr <awh::server_t>, std::unique_ptr <awh::client_t>>> & abandoned() noexcept {
		// Хранилище фасадов, оставленных работать до конца работы процесса
		static std::vector <std::pair <std::unique_ptr <awh::server_t>, std::unique_ptr <awh::client_t>>> result;
		// Выводим хранилище фасадов
		return result;
	}

	/**
	 * @brief Класс интеграционного окружения фасадов TLS (loopback клиент ↔ сервер)
	 *
	 */
	class Harness {
		private:
			// Объект фреймворка
			awh::fmk_t * _fmk;
			// Объект для работы с логами
			awh::log_t * _log;
			// Тестовое окружение транспортной безопасности
			Security * _security;
		private:
			// Итоги прогона
			result_t _result;
		private:
			// Размер полезной нагрузки обмена (октеты)
			size_t _length;
			// Порт прослушивания сервера на локальной петле
			uint16_t _port;
		private:
			// Фасад сервера
			std::unique_ptr <awh::server_t> _server;
			// Фасад клиента
			std::unique_ptr <awh::client_t> _client;
		private:
			// Опознаватель транспортного уровня клиента, заведённого прогоном
			tls::coder_t::id_t _ctl;
		private:
			// Отправленная клиентом нагрузка (эталон для сверки)
			std::string _payload;
			// Накопленное сервером принятое (эталон сверяется побайтово по мере приёма)
			bool _serverOk;
			// Накопленное клиентом эхо совпадает с отправленной нагрузкой
			bool _clientOk;
		private:
			// Флаг однократной остановки цикла (защита от повторного stop)
			std::atomic <bool> _finishing;
		private:
			/**
			 * @brief Метод формирования детерминированной нагрузки заданного размера
			 *
			 * @param size размер нагрузки
			 * @return     сформированная нагрузка
			 *
			 */
			std::string makePayload(const size_t size) const noexcept {
				// Результирующая нагрузка
				std::string result;
				// Резервируем память под нагрузку
				result.resize(size);
				// Заполняем нагрузку детерминированным образцом
				for(size_t i = 0; i < size; i++)
					// Записываем октет образца
					result[i] = static_cast <char> ((i * 31 + 7) & 0xFF);
				// Выводим сформированную нагрузку
				return result;
			}
			/**
			 * @brief Метод завершения обмена и остановки цикла событий
			 *
			 * @note Вызывается из коллбэка на потоке цикла: помечает обмен завершённым
			 *       и будит базу через stop(). Защищён от повторного вызова
			 *
			 */
			void finish() noexcept {
				// Ожидаемое состояние флага завершения (обмен ещё не завершён)
				bool expected = false;
				// Атомарно захватываем однократное завершение обмена
				if(this->_finishing.compare_exchange_strong(expected, true)){
					// Помечаем обмен завершённым штатно
					this->_result.completed = true;
					// Останавливаем работу цикла событий сервера-лаунчера
					this->_server->stop();
				}
			}
		private:
			/**
			 * @brief Метод обработки изменения статуса сервера
			 *
			 * @param status новый статус сервера
			 *
			 */
			void serverStatus(const event::status_t status) noexcept {
				// Если сервер перешёл в рабочее состояние (лаунчер запустил цикл событий)
				if(status == event::status_t::LAUNCHED){
					// Переводим сервер в режим прослушивания входящих соединений
					this->_server->listen(100);
					// Поднимаем клиента: статус-коллбэк клиента инициирует подключение
					this->_client->start();
				}
			}
			/**
			 * @brief Метод обработки приёма подключения клиента сервером
			 *
			 * @note Сокет принятого клиента переводится в неблокирующий режим по устройству
			 *       самой проверки, а не потому, что так положено фасаду: обе стороны обмена
			 *       живут в одном процессе на одном потоке цикла, и блокирующая запись встала
			 *       бы намертво - вычерпать приёмный буфер должен тот самый поток, который
			 *       стоит в записи. У настоящего потребителя удалённая сторона читает своим
			 *       ходом, и блокирующий сокет работает как задумано
			 *
			 * @param cid идентификатор принятого клиента
			 *
			 */
			void serverAccept(const event::id_t, const event::id_t cid, const tls::coder_t::id_t) noexcept {
				// Устанавливаем опции принятого клиента
				this->_server->setOptions(cid, (event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::TCP_NO_DELAY));
			}
			/**
			 * @brief Метод обработки принятых сервером расшифрованных данных
			 *
			 * @param eid    идентификатор клиента
			 * @param buffer буфер принятых данных
			 * @param size   размер принятых данных
			 *
			 */
			void serverRead(const event::id_t eid, const uint8_t * buffer, const size_t size, void *) noexcept {
				/**
				 * Сверяем принятое с эталоном по мере приёма: данные приходят частями,
				 * поэтому сверка идёт по абсолютному смещению принятого
				 */
				for(size_t i = 0; i < size; i++){
					// Абсолютное смещение принятого октета
					const size_t offset = (this->_result.serverBytes + i);
					// Если принятый октет вышел за пределы эталона либо не совпал с ним
					if((offset >= this->_payload.size()) || (static_cast <char> (buffer[i]) != this->_payload[offset])){
						// Помечаем несовпадение принятого с эталоном
						this->_serverOk = false;
						// Прерываем сверку
						break;
					}
				}
				// Накапливаем число принятых сервером октетов
				this->_result.serverBytes += size;
				// Возвращаем принятое обратно клиенту эхом (обратное направление шифрования)
				this->_server->send(eid, buffer, size);
				// Если сервер принял всю ожидаемую нагрузку
				if(this->_result.serverBytes >= this->_payload.size())
					// Запоминаем совпадение принятого сервером с эталоном
					this->_result.serverMatched = this->_serverOk;
			}
		private:
			/**
			 * @brief Метод обработки изменения статуса клиента
			 *
			 * @param status новый статус клиента
			 *
			 */
			void clientStatus(const event::status_t status) noexcept {
				// Если клиент перешёл в рабочее состояние - подключаемся к серверу
				if(status == event::status_t::LAUNCHED)
					// Выполняем подключение клиента к удалённому серверу
					this->_client->connect();
			}
			/**
			 * @brief Метод обработки подключения клиента к серверу
			 *
			 * @param ok результат подключения к серверу
			 *
			 */
			void clientConnect(const bool ok) noexcept {
				// Запоминаем результат установки соединения
				this->_result.connected = ok;
				// Если подключение к серверу не выполнено
				if(!ok){
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
					// Выходим из метода
					return;
				}
				/**
				 * Отправляем всю нагрузку одним заходом: объём заведомо превышает очередь
				 * отправки, поэтому шифротекст уходит не разом, а вытягивается движком
				 * по мере освобождения места
				 */
				this->_result.sendReturn = this->_client->send(this->_payload.data(), this->_payload.size());
			}
			/**
			 * @brief Метод обработки принятого клиентом расшифрованного эхо-ответа
			 *
			 * @param buffer буфер принятых данных
			 * @param size   размер принятых данных
			 *
			 */
			void clientRead(const uint8_t * buffer, const size_t size) noexcept {
				// Накапливаем число вызовов приёма
				this->_result.clientReads++;
				/**
				 * Сверяем принятое эхо с эталоном по мере приёма: эхо приходит частями,
				 * поэтому сверка идёт по абсолютному смещению принятого
				 */
				for(size_t i = 0; i < size; i++){
					// Абсолютное смещение принятого октета
					const size_t offset = (this->_result.clientBytes + i);
					// Если принятый октет вышел за пределы эталона либо не совпал с ним
					if((offset >= this->_payload.size()) || (static_cast <char> (buffer[i]) != this->_payload[offset])){
						// Помечаем несовпадение принятого с эталоном
						this->_clientOk = false;
						// Прерываем сверку
						break;
					}
				}
				// Накапливаем число принятых клиентом октетов
				this->_result.clientBytes += size;
				// Если клиент принял всё эхо целиком
				if(this->_result.clientBytes >= this->_payload.size()){
					// Запоминаем совпадение принятого клиентом с эталоном
					this->_result.clientMatched = this->_clientOk;
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
				}
			}
		public:
			/**
			 * @brief Метод выполнения прогона сквозного обмена
			 *
			 * @param timeout сторожевой таймаут прогона в миллисекундах
			 * @return        итоги прогона
			 *
			 */
			result_t execute(const uint32_t timeout) noexcept {
				// Формируем нагрузку обмена заданного размера
				this->_payload = this->makePayload(this->_length);
				// Создаём фасад сервера на серверном шаблоне контекста безопасности
				this->_server = std::make_unique <awh::server_t> (this->_security->server(), &this->_security->coder(), this->_fmk, this->_log);
				// Создаём событие сервера потокового транспорта поверх TCP
				this->_server->init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
				// Устанавливаем хост сервера на локальной петле
				this->_server->setHost("127.0.0.1");
				// Устанавливаем порт прослушивания сервера
				this->_server->setPort(this->_port);
				// Регистрируем коллбэк изменения статуса сервера
				this->_server->on <void (const event::status_t)> ("status", &Harness::serverStatus, this, _1);
				// Регистрируем коллбэк приёма подключения клиента сервером
				this->_server->on <void (const event::id_t, const event::id_t, const tls::coder_t::id_t)> ("accept", &Harness::serverAccept, this, _1, _2, _3);
				// Регистрируем коллбэк принятых сервером расшифрованных данных
				this->_server->on <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", &Harness::serverRead, this, _1, _2, _3, _4);
				/**
				 * Опознаватель транспортного уровня запоминается: реестр участников
				 * живёт весь процесс, и заведённый прогоном уровень обязан быть снят
				 * этим же прогоном - иначе он копится от проверки к проверке вместе с
				 * откликами, привязанными к фасаду клиента, который прогон разрушит
				 */
				// Выполняем заведение транспортного уровня клиента
				this->_ctl = this->_security->coder().transport(this->_security->client());
				// Создаём фасад клиента на транспорте клиентского шаблона контекста безопасности
				this->_client = std::make_unique <awh::client_t> (this->_ctl, &this->_security->coder(), this->_fmk, this->_log);
				// Создаём событие клиента потокового транспорта поверх TCP
				this->_client->init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
				// Устанавливаем опции клиента: обе стороны на одном потоке цикла, блокирующая запись встала бы намертво
				this->_client->setOptions(event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::TCP_NO_DELAY);
				// Устанавливаем адрес удалённого сервера на локальной петле
				this->_client->setTarget("127.0.0.1");
				// Устанавливаем порт удалённого сервера
				this->_client->setTargetPort(this->_port);
				// Регистрируем коллбэк изменения статуса клиента
				this->_client->on <void (const event::status_t)> ("status", &Harness::clientStatus, this, _1);
				// Регистрируем коллбэк подключения клиента к серверу
				this->_client->on <void (const bool)> ("connect", &Harness::clientConnect, this, _1);
				// Регистрируем коллбэк принятого клиентом расшифрованного эхо-ответа
				this->_client->on <void (const uint8_t *, const size_t)> ("read", &Harness::clientRead, this, _1, _2);
				// Обещание завершения работы фонового потока цикла событий
				std::promise <void> finished;
				// Ожидание завершения работы фонового потока цикла событий
				std::future <void> waiter = finished.get_future();
				// Запускаем сервер-лаунчер в фоновом потоке (start блокирует до остановки цикла)
				std::thread worker([this, &finished]() noexcept {
					// Запускаем цикл событий сервера (блокирует поток до вызова stop)
					this->_server->start();
					// Сигнализируем завершение работы фонового потока
					finished.set_value();
				});
				// Ожидаем завершения обмена в пределах сторожевого таймаута
				if(waiter.wait_for(std::chrono::milliseconds(timeout)) == std::future_status::timeout){
					// Помечаем прерывание прогона сторожевым таймаутом
					this->_result.timedOut = true;
					// Будим цикл событий сервера-лаунчера для завершения фонового потока
					this->_server->stop();
					/**
					 * Ожидаем фактического завершения работы фонового потока, но не бесконечно:
					 * цикл событий может не откликнуться на остановку, и безусловное ожидание
					 * превращало бы сторожевой таймаут в вечное зависание всего набора
					 */
					if(waiter.wait_for(std::chrono::seconds(SHUTDOWN_TIMEOUT)) == std::future_status::timeout){
						// Помечаем неостановимый цикл событий
						this->_result.unstoppable = true;
						// Отпускаем фоновый поток, не дожидаясь его завершения
						worker.detach();
						// Продлеваем жизнь фасадов до конца работы процесса
						abandoned().emplace_back(std::move(this->_server), std::move(this->_client));
						/**
						 * Транспортный уровень снимать нельзя: фасады оставлены работать,
						 * и снятие отдало бы память участника, которым они ещё пользуются
						 */
						// Забываем опознаватель транспортного уровня, не снимая его
						this->_ctl = 0;
						// Выводим итоги прогона
						return this->_result;
					}
				}
				// Дожидаемся завершения фонового потока
				worker.join();
				// Выводим итоги прогона
				return this->_result;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param length   размер полезной нагрузки обмена
			 * @param port     порт прослушивания сервера на локальной петле
			 * @param fmk      объект фреймворка
			 * @param log      объект для работы с логами
			 * @param security тестовое окружение транспортной безопасности
			 *
			 */
			Harness(const size_t length, const uint16_t port, awh::fmk_t * fmk, awh::log_t * log, Security * security) noexcept :
			 _fmk(fmk), _log(log), _security(security), _length(length), _port(port),
			 _ctl(0), _payload{""}, _serverOk(true), _clientOk(true), _finishing(false) {}
			/**
			 * @brief Деструктор
			 *
			 * @details Снимает транспортный уровень, заведённый прогоном. Порядок обязателен и
			 *          именно таков: сперва снимается участник, и лишь затем разрушаются
			 *          фасады. Снятие зовёт отклик состояния, а отклик этот привязан к фасаду
			 *          клиента - разрушь мы фасад первым, снятие ушло бы в освобождённую
			 *          память. Проверено: обратный порядок валит прогон немедленно
			 *
			 */
			~Harness() noexcept {
				// Если транспортный уровень заводился прогоном
				if(this->_ctl > 0)
					// Выполняем снятие транспортного уровня
					this->_security->coder().destroy(this->_ctl);
				// Выполняем разрушение фасада клиента
				this->_client.reset(nullptr);
				// Выполняем разрушение фасада сервера
				this->_server.reset(nullptr);
			}
	};
};

/**
 * @brief Класс фикстуры интеграционных тестов фасадов TLS
 *
 */
class TlsFacadeTest : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект для работы с логами
		std::unique_ptr <awh::log_t> _log;
		// Тестовое окружение транспортной безопасности
		std::unique_ptr <Security> _security;
	protected:
		/**
		 * @brief Метод подбора порта прослушивания на локальной петле
		 *
		 * @note Порт выводится из идентификатора процесса, чтобы снизить вероятность
		 *       коллизии при параллельных прогонах на одной машине
		 *
		 * @return подобранный порт
		 *
		 */
		uint16_t pickPort() const noexcept {
			/**
			 * Счётчик выданных портов: слушающий сокет предыдущей проверки уходит
			 * в состояние TIME_WAIT, и повторная привязка к тому же порту не проходит,
			 * а отказ привязки движок обрабатывает уходом из приложения
			 */
			static uint16_t offset = 0;
			// Выводим порт в диапазоне 50000..59999 на основе идентификатора процесса
			return static_cast <uint16_t> (50000 + ((static_cast <uint32_t> (::getpid()) + (offset++)) % 10000));
		}
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp() override {
			// Инициализируем объект фреймворка
			this->_fmk = std::make_unique <awh::fmk_t> ();
			// Инициализируем объект логирования
			this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
			// Отключаем вывод логов в тестовом окружении
			this->_log->level(awh::log_t::level_t::NONE);
			// Инициализируем тестовое окружение транспортной безопасности
			this->_security = std::make_unique <Security> (this->_fmk.get(), this->_log.get());
		}
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown() override {}
};

/**
 * @brief Тест сквозного эхо небольшой нагрузки поверх TLS
 *
 * @note Нагрузка помещается в очередь отправки целиком: проверяется сам обмен
 *       шифрованными данными, без переполнения очереди
 *
 */
TEST_F(TlsFacadeTest, ConnectAndEcho){
	// Проверяем готовность тестового окружения транспортной безопасности
	ASSERT_TRUE(this->_security->ready()) << "security environment is not ready";
	// Размер полезной нагрузки обмена
	const size_t length = 4096;
	// Создаём интеграционное окружение фасадов
	Harness harness(length, this->pickPort(), this->_fmk.get(), this->_log.get(), this->_security.get());
	// Выполняем прогон сквозного обмена
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "handshake or echo timed out";
	// Проверяем, что соединение установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что клиентский send() принял всю нагрузку целиком
	EXPECT_EQ(result.sendReturn, length);
	// Проверяем, что сервер принял всю нагрузку
	EXPECT_EQ(result.serverBytes, length);
	// Проверяем, что принятое сервером побайтово совпало с отправленным
	EXPECT_TRUE(result.serverMatched);
	// Проверяем, что клиент принял всё эхо
	EXPECT_EQ(result.clientBytes, length);
	// Проверяем, что принятое клиентом эхо побайтово совпало с отправленным
	EXPECT_TRUE(result.clientMatched);
}

/**
 * @brief Тест сохранности записей TLS при переполнении очереди отправки
 *
 * @details Объём нагрузки заведомо превышает очередь отправки в обе стороны: клиент
 *          шлёт её одним заходом, сервер возвращает эхом. Прежде шифротекст писался
 *          в сокет напрямую, и при переполнении очереди запись терялась - удалённая
 *          сторона получала следующую запись с неверным счётчиком и обрывала соединение.
 *          Потеря даже одного октета шифротекста здесь означает срыв прогона по
 *          сторожевому таймауту либо несовпадение принятого с эталоном
 *
 */
TEST_F(TlsFacadeTest, CiphertextSurvivesQueueOverflow){
	// Проверяем готовность тестового окружения транспортной безопасности
	ASSERT_TRUE(this->_security->ready()) << "security environment is not ready";
	// Размер полезной нагрузки обмена, заведомо превышающий очередь отправки
	const size_t length = (4 * 1024 * 1024);
	// Создаём интеграционное окружение фасадов
	Harness harness(length, this->pickPort(), this->_fmk.get(), this->_log.get(), this->_security.get());
	// Выполняем прогон сквозного обмена
	const result_t result = harness.execute(60000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "bulk ciphertext exchange timed out: server=" << result.serverBytes << " client=" << result.clientBytes << " send=" << result.sendReturn << " reads=" << result.clientReads;
	// Проверяем, что соединение установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что сервер принял всю нагрузку целиком
	EXPECT_EQ(result.serverBytes, length);
	// Проверяем, что принятое сервером побайтово совпало с отправленным
	EXPECT_TRUE(result.serverMatched);
	// Проверяем, что клиент принял всё эхо целиком
	EXPECT_EQ(result.clientBytes, length);
	// Проверяем, что принятое клиентом эхо побайтово совпало с отправленным
	EXPECT_TRUE(result.clientMatched);
	// Проверяем, что эхо пришло частями (иначе переполнения очереди не случилось)
	EXPECT_GT(result.clientReads, 1u);
}

