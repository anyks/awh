/**
 * @file facade.cpp
 * @date 2026-11-25
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
 * @brief Интеграционные тесты фасада клиента HTTP-прокси поверх TLS к прокси —
 *        проверка режимов CONNECT и FORWARD по реальным TCP-сокетам в одном
 *        процессе (loopback), где роль прокси исполняет тестовый сервер
 *
 * @details Главное, что здесь проверяется — расшифровка входящих данных фасада
 *          при установленном TLS к прокси: прежде фасад отдавал шифротекст
 *          парсеру обмена напрямую, и рукопожатие завершалось зависанием.
 *          Тестовый сервер принимает CONNECT и открывает туннель эхом, либо
 *          отвечает на пересылаемый запрос фиксированным ответом; принятое
 *          клиентом сверяется с эталоном побайтово
 *
 * @note База событий едина на весь процесс: цикл событий запускает и блокирует
 *       ровно один юнит-лаунчер. Поэтому сервер поднимается лаунчером в фоновом
 *       потоке, а клиент стартует из серверного статус-коллбэка (тот же поток
 *       цикла). Тестовый поток лишь ожидает завершения обмена и по сторожевому
 *       таймауту будит цикл вызовом stop()
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
 *
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Запрещаем включение wincrypt.h: его макросы X509_NAME, X509_EXTENSIONS
	 * и PKCS7_SIGNER_INFO ломают разбор заголовков OpenSSL
	 */
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
 * Подключаем заголовочный файл самоподписанного сертификата тестового узла
 */
#include "../../../cert.hpp"

/**
 * Подключаем заголовочные файлы фасадов клиента HTTP-прокси и сервера
 */
#include "../../../../include/client/http/proxy.hpp"
#include "../../../../include/server/server.hpp"
#include "../../../../include/unit/dns.hpp"
#include <sys/log.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён заполнителей связывания функций
 */
using namespace std::placeholders;

/**
 * @brief Внутренние средства интеграционного окружения фасада HTTP-прокси
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
	static std::string __awh_directory__() noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Буфер под путь к временному каталогу
			char buffer[MAX_PATH + 1]{0};
			// Получаем путь к временному каталогу у системы
			const DWORD size = ::GetTempPathA(static_cast <DWORD> (sizeof(buffer)), buffer);
			// Если путь к временному каталогу получен
			if((size > 0) && (size <= MAX_PATH)){
				// Формируем путь к временному каталогу
				std::string result(buffer, static_cast <size_t> (size));
				/**
				 * Удаляем завершающий разделитель, добавляемый системой
				 */
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
	static bool __awh_store__(const std::string & path, const std::string & data) noexcept {
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
	 * @brief Функция получения самоподписанного сертификата тестового узла
	 *
	 * @param certificate сертификат в формате PEM
	 * @param privateKey  приватный ключ в формате PEM
	 * @return            результат получения
	 *
	 */
	static bool __awh_make_certificate__(std::string & certificate, std::string & privateKey) noexcept {
		// Устанавливаем самоподписанный сертификат тестового узла
		certificate = std::string(::testCertificate());
		// Устанавливаем приватный ключ тестового узла
		privateKey = std::string(::testPrivateKey());
		// Выводим положительный результат
		return (!certificate.empty() && !privateKey.empty());
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
			awh::tls::coder::id_t _client;
			// Идентификатор шаблона контекста безопасности сервера
			awh::tls::coder::id_t _server;
		public:
			/**
			 * @brief Метод извлечения шаблона контекста безопасности клиента
			 *
			 * @return идентификатор шаблона контекста безопасности
			 *
			 */
			awh::tls::coder::id_t client() const noexcept {
				// Выводим шаблон контекста безопасности клиента
				return this->_client;
			}
			/**
			 * @brief Метод извлечения шаблона контекста безопасности сервера
			 *
			 * @return идентификатор шаблона контекста безопасности
			 *
			 */
			awh::tls::coder::id_t server() const noexcept {
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
			// Запрещаем конструктор перемещения
			Security(Security &&) = delete;
			// Запрещаем копирующее присваивание
			Security & operator = (const Security &) = delete;
			// Запрещаем перемещающее присваивание
			Security & operator = (Security &&) = delete;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Security() noexcept :
			 _client(0), _server(0) {
				// Сертификат тестового узла в формате PEM
				std::string certificate = "";
				// Приватный ключ тестового узла в формате PEM
				std::string privateKey = "";
				// Если генерация самоподписанного сертификата не выполнена
				if(!::__awh_make_certificate__(certificate, privateKey))
					// Выходим из конструктора - контексты останутся несозданными
					return;
				// Формируем основу пути к временным файлам
				const std::string base = (::__awh_directory__() + "/awh-httpproxy-" + std::to_string(reinterpret_cast <uintptr_t> (this)));
				// Устанавливаем путь к файлу сертификата
				this->_certificate = (base + ".crt");
				// Устанавливаем путь к файлу приватного ключа
				this->_privateKey = (base + ".key");
				// Если запись сертификата и приватного ключа во временные файлы не выполнена
				if(!::__awh_store__(this->_certificate, certificate) || !::__awh_store__(this->_privateKey, privateKey))
					// Выходим из конструктора - контексты останутся несозданными
					return;
				// Создаём шаблон контекста безопасности сервера поверх TCP
				this->_server = awh::tls::coder::context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
				// Если шаблон контекста безопасности сервера создан
				if(this->_server > 0){
					// Устанавливаем сертификат тестового узла
					awh::tls::coder::certificate(this->_server, this->_certificate);
					// Устанавливаем приватный ключ тестового узла
					awh::tls::coder::privateKey(this->_server, this->_privateKey);
					/**
					 * Снимаем проверку на сервере: шаблон контекста создаётся с включённой
					 * проверкой, а на серверном узле это означает требование клиентского
					 * сертификата, то есть взаимную аутентификацию. Проверяется односторонний
					 * TLS, поэтому требование снимается явно
					 */
					awh::tls::coder::validateServerNameIndication(this->_server, false);
				}
				// Создаём шаблон контекста безопасности клиента поверх TCP
				this->_client = awh::tls::coder::context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
				// Если шаблон контекста безопасности клиента создан
				if(this->_client > 0){
					// Устанавливаем доменное имя удалённого узла
					awh::tls::coder::serverNameIndication(this->_client, "localhost");
					// Устанавливаем доверенный центр сертификации тестового узла
					awh::tls::coder::ca(this->_client, this->_certificate);
					// Снимаем проверку сертификата удалённого узла (сертификат самоподписанный)
					awh::tls::coder::validateServerNameIndication(this->_client, false);
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
	 * @brief Итоги прогона обмена через прокси поверх TLS
	 *
	 */
	struct result_t {
		// Клиент установил соединение с прокси
		bool connected = false;
		// Обмен завершён штатно (не по сторожевому таймауту)
		bool completed = false;
		// Прогон прерван сторожевым таймаутом
		bool timedOut = false;
		// Цикл событий не откликнулся на остановку и был оставлен работать
		bool unstoppable = false;
		// Прокси принял запрос CONNECT в режиме туннеля
		bool connectSeen = false;
		// Прокси принял запрос в absolute-form в режиме пересылки
		bool forwardSeen = false;
		// Принятое клиентом побайтово совпало с эталоном
		bool matched = false;
		// Код состояния ответа прокси в режиме пересылки
		uint16_t code = 0;
		// Возвращённое клиентским send() число октетов
		size_t sendReturn = 0;
		// Накопленное клиентом принятое
		std::string received;
	};
	// Предел ожидания отклика цикла событий на остановку в секундах
	static constexpr uint32_t SHUTDOWN_TIMEOUT = 5;
	/**
	 * @brief Метод доступа к хранилищу неостановленных фасадов
	 *
	 * @details Фасад, чей цикл событий не откликнулся на остановку, разрушать нельзя:
	 *          его поток продолжает обращаться к полям объекта. Такие фасады
	 *          складываются сюда и живут до конца работы процесса
	 *
	 * @note Хранилище намеренно не опустошается: это не утечка по недосмотру, а
	 *       единственный безопасный исход, когда поток остановить не удалось
	 *
	 * @return хранилище фасадов, оставленных работать
	 *
	 */
	static std::vector <std::pair <std::unique_ptr <awh::server_t>, std::unique_ptr <awh::client::http_proxy_t>>> & abandoned() noexcept {
		// Хранилище фасадов, оставленных работать до конца работы процесса
		static std::vector <std::pair <std::unique_ptr <awh::server_t>, std::unique_ptr <awh::client::http_proxy_t>>> result;
		// Выводим хранилище фасадов
		return result;
	}
	/**
	 * @brief Класс интеграционного окружения фасада HTTP-прокси (loopback клиент ↔ прокси)
	 *
	 */
	class Harness {
		private:
			// Тестовое окружение транспортной безопасности
			Security * _security;
		private:
			// Итоги прогона
			result_t _result;
		private:
			// Порт прослушивания прокси на локальной петле
			uint16_t _port;
			// Режим работы клиента через прокси
			awh::http::proxy::client_t::mode_t _mode;
		private:
			// Объект DNS-резолвера клиента (обязан переживать фасад клиента)
			awh::unit::dns_t _dns;
		private:
			// Фасад сервера в роли прокси
			std::unique_ptr <awh::server_t> _server;
			// Фасад клиента HTTP-прокси
			std::unique_ptr <awh::client::http_proxy_t> _client;
		private:
			// Опознаватель транспортного уровня клиента, заведённого прогоном
			tls::coder::id_t _ctl;
		private:
			// Отправленный клиентом запрос (эталон эха в режиме туннеля)
			std::string _payload;
			// Ожидаемое принятое клиентом (эхо запроса либо тело ответа прокси)
			std::string _expected;
			// Накопленное прокси принятое до конца головы запроса
			std::string _serverBuf;
			// Флаг открытого туннеля прокси
			bool _tunnelOpen;
			// Флаг отправленного ответа прокси в режиме пересылки
			bool _answered;
		private:
			// Флаг однократной остановки цикла (защита от повторного stop)
			std::atomic <bool> _finishing;
		private:
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
					// Останавливаем событие клиента (останавливает и DNS-резолвер)
					this->_client->stop();
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
			 *       самой проверки: обе стороны обмена живут в одном процессе на одном
			 *       потоке цикла, и блокирующая запись встала бы намертво
			 *
			 * @param cid идентификатор принятого клиента
			 *
			 */
			void serverAccept(const event::id_t, const event::id_t cid, const tls::coder::id_t) noexcept {
				// Устанавливаем опции принятого клиента
				this->_server->setOptions(cid, (event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::TCP_NO_DELAY));
			}
			/**
			 * @brief Метод обработки принятых прокси расшифрованных данных
			 *
			 * @param eid    идентификатор клиента
			 * @param buffer буфер принятых данных
			 * @param size   размер принятых данных
			 *
			 */
			void serverRead(const event::id_t eid, const uint8_t * buffer, const size_t size, void *) noexcept {
				// Накапливаем принятое до конца головы запроса
				this->_serverBuf.append(reinterpret_cast <const char *> (buffer), size);
				// Если режим работы — туннель
				if(this->_mode == awh::http::proxy::client_t::mode_t::TUNNEL){
					// Если туннель ещё не открыт
					if(!this->_tunnelOpen){
						// Позиция конца головы запроса CONNECT
						const size_t pos = this->_serverBuf.find("\r\n\r\n");
						// Если голова запроса принята целиком
						if(pos != std::string::npos){
							// Помечаем туннель открытым
							this->_tunnelOpen = true;
							// Запоминаем наличие метода CONNECT в запросе
							this->_result.connectSeen = (this->_serverBuf.compare(0, 8, "CONNECT ") == 0);
							// Ответ прокси, открывающий туннель
							const std::string reply = "HTTP/1.1 200 Connection established\r\n\r\n";
							// Отправляем ответ, открывающий туннель
							this->_server->send(eid, reply.data(), reply.size());
							// Если данные пришли вместе с головой запроса (ранние данные)
							if(this->_serverBuf.size() > (pos + 4))
								// Возвращаем ранние данные эхом
								this->_server->send(eid, this->_serverBuf.data() + pos + 4, this->_serverBuf.size() - pos - 4);
							// Очищаем буфер накопления
							this->_serverBuf.clear();
						}
					// Если туннель открыт, то возвращаем принятое эхом
					} else this->_server->send(eid, buffer, size);
				// Режим работы — пересылка
				} else {
					// Если ответ ещё не отправлен и голова запроса принята целиком
					if(!this->_answered && (this->_serverBuf.find("\r\n\r\n") != std::string::npos)){
						// Помечаем ответ отправленным
						this->_answered = true;
						// Запоминаем наличие цели в absolute-form в запросе
						this->_result.forwardSeen = ((this->_serverBuf.compare(0, 4, "GET ") == 0) && (this->_serverBuf.find(" http://") != std::string::npos));
						// Ответ прокси на пересылаемый запрос
						const std::string reply = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nPROXY-BODY";
						// Отправляем ответ на пересылаемый запрос
						this->_server->send(eid, reply.data(), reply.size());
					}
				}
			}
		private:
			/**
			 * @brief Метод обработки изменения статуса клиента
			 *
			 * @param status новый статус клиента
			 *
			 */
			void clientStatus(const event::status_t status) noexcept {
				// Если клиент перешёл в рабочее состояние - подключаемся к прокси
				if(status == event::status_t::LAUNCHED)
					// Подключаем клиента к прокси на локальной петле
					this->_client->connect();
			}
			/**
			 * @brief Метод обработки подключения клиента к прокси
			 *
			 * @param ok результат подключения к прокси
			 *
			 */
			void clientConnect(const bool ok) noexcept {
				// Запоминаем результат установки соединения
				this->_result.connected = ok;
				// Если подключение к прокси не выполнено
				if(!ok){
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
					// Выходим из метода
					return;
				}
				// Отправляем запрос пользователя через прокси
				this->_result.sendReturn = this->_client->send(this->_payload.data(), this->_payload.size());
			}
			/**
			 * @brief Метод обработки готовности головы ответа прокси
			 *
			 * @param code код состояния ответа прокси
			 *
			 */
			void clientResponse(const uint16_t code, const std::vector <std::pair <std::string, std::string>> &) noexcept {
				// Запоминаем код состояния ответа прокси
				this->_result.code = code;
			}
			/**
			 * @brief Метод обработки принятого клиентом расшифрованного ответа
			 *
			 * @param buffer буфер принятых данных
			 * @param size   размер принятых данных
			 *
			 */
			void clientRead(const uint8_t * buffer, const size_t size) noexcept {
				// Накапливаем принятое клиентом
				this->_result.received.append(reinterpret_cast <const char *> (buffer), size);
				// Если клиент принял всё ожидаемое целиком
				if(this->_result.received.size() >= this->_expected.size()){
					// Запоминаем совпадение принятого клиентом с эталоном
					this->_result.matched = (this->_result.received == this->_expected);
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
				}
			}
		public:
			/**
			 * @brief Метод выполнения прогона обмена через прокси поверх TLS
			 *
			 * @param timeout сторожевой таймаут прогона в миллисекундах
			 * @return        итоги прогона
			 *
			 */
			result_t execute(const uint32_t timeout) noexcept {
				// Создаём фасад сервера на серверном шаблоне контекста безопасности
				this->_server = std::make_unique <awh::server_t> (this->_security->server());
				// Создаём событие сервера потокового транспорта поверх TCP
				this->_server->init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
				// Устанавливаем хост сервера на локальной петле
				this->_server->setHost("127.0.0.1");
				// Устанавливаем порт прослушивания сервера
				this->_server->setPort(this->_port);
				// Регистрируем коллбэк изменения статуса сервера
				this->_server->on <void (const event::status_t)> ("status", &Harness::serverStatus, this, _1);
				// Регистрируем коллбэк приёма подключения клиента сервером
				this->_server->on <void (const event::id_t, const event::id_t, const tls::coder::id_t)> ("accept", &Harness::serverAccept, this, _1, _2, _3);
				// Регистрируем коллбэк принятых сервером расшифрованных данных
				this->_server->on <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", &Harness::serverRead, this, _1, _2, _3, _4);
				/**
				 * Опознаватель транспортного уровня запоминается: реестр участников
				 * живёт весь процесс, и заведённый прогоном уровень обязан быть снят
				 * этим же прогоном - иначе он копится от проверки к проверке вместе с
				 * откликами, привязанными к фасаду клиента, который прогон разрушит
				 */
				// Заводим транспортный уровень клиента по шаблону
				this->_ctl = awh::tls::coder::transport(this->_security->client());
				// Создаём фасад клиента HTTP-прокси на транспорте клиентского шаблона контекста безопасности
				this->_client = std::make_unique <awh::client::http_proxy_t> (this->_ctl, &this->_dns);
				// Создаём событие клиента потокового транспорта поверх TCP
				this->_client->init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
				// Устанавливаем опции клиента: обе стороны на одном потоке цикла, блокирующая запись встала бы намертво
				this->_client->setOptions(event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::TCP_NO_DELAY);
				// Устанавливаем адрес прокси на локальной петле
				this->_client->setTarget("127.0.0.1");
				// Устанавливаем порт прокси
				this->_client->setTargetPort(this->_port);
				// Если режим работы — туннель
				if(this->_mode == awh::http::proxy::client_t::mode_t::TUNNEL)
					// Устанавливаем конечную точку туннеля
					this->_client->endpoint("127.0.0.1", 9);
				// Если режим работы — пересылка
				else this->_client->mode(awh::http::proxy::client_t::mode_t::FORWARD);
				// Регистрируем коллбэк изменения статуса клиента
				this->_client->on <void (const event::status_t)> ("status", &Harness::clientStatus, this, _1);
				// Регистрируем коллбэк подключения клиента к прокси
				this->_client->on <void (const bool)> ("connect", &Harness::clientConnect, this, _1);
				// Регистрируем коллбэк готовности головы ответа прокси
				this->_client->on <void (const uint16_t, const std::vector <std::pair <std::string, std::string>> &)> ("response", &Harness::clientResponse, this, _1, _2);
				// Регистрируем коллбэк принятого клиентом расшифрованного ответа
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
					// Останавливаем событие клиента (останавливает и DNS-резолвер)
					this->_client->stop();
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
			 * @param mode     режим работы клиента через прокси
			 * @param port     порт прослушивания прокси на локальной петле
			 * @param security тестовое окружение транспортной безопасности
			 *
			 */
			Harness(const awh::http::proxy::client_t::mode_t mode, const uint16_t port, Security * security) noexcept :
			 _security(security), _port(port), _mode(mode),
			 _dns(awh::event::family_t::IPV4), _ctl(0),
			 _payload{""}, _expected{""}, _serverBuf{""},
			 _tunnelOpen(false), _answered(false), _finishing(false) {
				// Если режим работы — туннель
				if(this->_mode == awh::http::proxy::client_t::mode_t::TUNNEL){
					// Устанавливаем запрос пользователя через туннель
					this->_payload = "GET / HTTP/1.1\r\nHost: tunnel.target\r\nConnection: close\r\n\r\n";
					// Устанавливаем эталон принятого: эхо запроса через туннель
					this->_expected = this->_payload;
				// Режим работы — пересылка
				} else {
					// Устанавливаем запрос пользователя на пересылку
					this->_payload = "GET http://target.example/path HTTP/1.1\r\nHost: target.example\r\n\r\n";
					// Устанавливаем эталон принятого: тело ответа прокси
					this->_expected = "PROXY-BODY";
				}
			}
			/**
			 * @brief Деструктор
			 *
			 * @details Снимает транспортный уровень, заведённый прогоном. Порядок обязателен и
			 *          именно таков: сперва снимается участник, и лишь затем разрушаются
			 *          фасады. Снятие зовёт отклик состояния, а отклик этот привязан к фасаду
			 *          клиента - разрушь мы фасад первым, снятие ушло бы в освобождённую
			 *          память
			 *
			 */
			~Harness() noexcept {
				// Если транспортный уровень заводился прогоном
				if(this->_ctl > 0)
					// Снимаем транспортный уровень клиента
					awh::tls::coder::destroy(this->_ctl);
				// Разрушаем фасад клиента HTTP-прокси
				this->_client.reset(nullptr);
				// Разрушаем фасад сервера-прокси
				this->_server.reset(nullptr);
			}
	};
};

/**
 * @brief Класс фикстуры интеграционных тестов фасада HTTP-прокси поверх TLS
 *
 */
class HttpProxyFacadeTest : public testing::Test {
	protected:
		// Тестовое окружение транспортной безопасности
		std::unique_ptr <Security> _security;
	protected:
		/**
		 * @brief Метод подбора порта прослушивания на локальной петле
		 *
		 * @note Порт выводится из идентификатора процесса, чтобы снизить вероятность
		 *       коллизии при параллельных прогонах на одной машине. Диапазон отличается
		 *       от диапазона тестов фасадов TLS: наборы идут в одном процессе, а слушающий
		 *       сокет предыдущей проверки уходит в TIME_WAIT
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
			// Выводим порт в диапазоне 53000..53999 на основе идентификатора процесса
			return static_cast <uint16_t> (53000 + ((static_cast <uint32_t> (::getpid()) + (offset++)) % 1000));
		}
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp() override {
			// Отключаем вывод логов в тестовом окружении
			awh::log::level(awh::log::level_t::NONE);
			// Инициализируем тестовое окружение транспортной безопасности
			this->_security = std::make_unique <Security> ();
		}
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown() override {}
};

/**
 * @brief Тест туннеля CONNECT через прокси поверх TLS
 *
 * @details Проверяет расшифровку входящих данных фасада при TLS к прокси: ответ
 *          на CONNECT и эхо туннеля приходят шифротекстом и обязаны дойти до
 *          прикладного обмена расшифрованными. Прежде фасад отдавал шифротекст
 *          парсеру обмена напрямую, и прогон зависал на рукопожатии
 *
 */
TEST_F(HttpProxyFacadeTest, TunnelOverTls){
	// Проверяем готовность тестового окружения транспортной безопасности
	ASSERT_TRUE(this->_security->ready()) << "security environment is not ready";
	// Создаём интеграционное окружение фасада в режиме туннеля
	Harness harness(awh::http::proxy::client_t::mode_t::TUNNEL, this->pickPort(), this->_security.get());
	// Запускаем прогон обмена через прокси
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "handshake or tunnel exchange timed out";
	// Проверяем, что соединение с прокси установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что прокси принял запрос CONNECT
	EXPECT_TRUE(result.connectSeen);
	// Проверяем, что клиентский send() принял запрос целиком
	EXPECT_GT(result.sendReturn, 0u);
	// Проверяем, что принятое клиентом эхо туннеля побайтово совпало с запросом
	EXPECT_TRUE(result.matched);
}

/**
 * @brief Тест пересылки FORWARD через прокси поверх TLS
 *
 * @details Проверяет расшифровку входящих данных фасада при TLS к прокси в режиме
 *          пересылки: голова и тело ответа прокси приходят шифротекстом и обязаны
 *          дойти до прикладного обмена расшифрованными
 *
 */
TEST_F(HttpProxyFacadeTest, ForwardOverTls){
	// Проверяем готовность тестового окружения транспортной безопасности
	ASSERT_TRUE(this->_security->ready()) << "security environment is not ready";
	// Создаём интеграционное окружение фасада в режиме пересылки
	Harness harness(awh::http::proxy::client_t::mode_t::FORWARD, this->pickPort(), this->_security.get());
	// Запускаем прогон обмена через прокси
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "handshake or forward exchange timed out";
	// Проверяем, что соединение с прокси установлено
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что прокси принял запрос в absolute-form
	EXPECT_TRUE(result.forwardSeen);
	// Проверяем, что клиентский send() принял запрос целиком
	EXPECT_GT(result.sendReturn, 0u);
	// Проверяем код состояния ответа прокси
	EXPECT_EQ(result.code, 200);
	// Проверяем, что принятое клиентом тело ответа побайтово совпало с эталоном
	EXPECT_TRUE(result.matched);
}