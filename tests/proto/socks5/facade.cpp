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
 * @brief Интеграционные тесты фасада клиента SOCKS5 — проверка туннеля CONNECT
 *        по реальным TCP-сокетам в одном процессе (loopback): клиент SOCKS5
 *        подключается через серверный фасад SOCKS5 к тестовому серверу-эхо
 *
 * @details Проверяется полный обмен: согласование методов и аутентификация по
 *          имени пользователя и паролю, запрос CONNECT, рукопожатие TLS к
 *          конечному узлу сквозь открытый туннель и побайтовое эхо прикладных
 *          данных. Отдельной проверкой идёт отказ аутентификации: сервер
 *          отклоняет пару имя/пароль, и клиент обязан откликнуться неудачей
 *          подключения, а не зависнуть
 *
 * @note База событий едина на весь процесс: цикл событий запускает и блокирует
 *       ровно один юнит-лаунчер. Поэтому сервер-эхо поднимается лаунчером в
 *       фоновом потоке, прокси стартует из статус-коллбэка сервера-эхо, а клиент
 *       стартует из статус-коллбэка прокси (всё тот же поток цикла). Тестовый
 *       поток лишь ожидает завершения обмена и по сторожевому таймауту будит
 *       цикл вызовом stop()
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
#include <tuple>
#include <utility>
#include <vector>

/**
 * Подключаем заголовочные файлы тестового фреймворка
 */
#include <gtest/gtest.h>

/**
 * Подключаем заголовочный файл самоподписанного сертификата тестового узла
 */
#include "../../cert.hpp"

/**
 * Подключаем заголовочные файлы фасадов клиента SOCKS5, сервера SOCKS5 и сервера
 */
#include "../../../include/client/socks5.hpp"
#include "../../../include/server/socks5.hpp"
#include "../../../include/server/server.hpp"
#include "../../../include/unit/dns.hpp"
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
 * @brief Внутренние средства интеграционного окружения фасада SOCKS5
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
			// Выводим признак неудачной записи
			return false;
		// Записываем данные в файл
		const size_t written = ::fwrite(data.data(), 1, data.size(), file);
		// Закрываем файл
		::fclose(file);
		// Выводим признак записи данных целиком
		return (written == data.size());
	}
	/**
	 * @brief Функция генерации самоподписанного сертификата тестового узла
	 *
	 * @param certificate вывод сертификата в формате PEM
	 * @param privateKey  вывод приватного ключа в формате PEM
	 * @return            результат генерации
	 *
	 */
	static bool __awh_make_certificate__(std::string & certificate, std::string & privateKey) noexcept {
		// Получаем сертификат тестового узла в формате PEM
		certificate = testCertificate();
		// Получаем приватный ключ тестового узла в формате PEM
		privateKey = testPrivateKey();
		// Выводим признак наличия обоих материалов
		return (!certificate.empty() && !privateKey.empty());
	}
	/**
	 * @brief Класс тестового окружения транспортной безопасности
	 *
	 * @details Владеет временными файлами сертификата и приватного ключа тестового
	 *          узла, а также шаблонами контекста безопасности сервера и клиента.
	 *          Шаблоны живут всё время прогона, транспортные уровни заводятся
	 *          от них отдельными вызовами
	 *
	 */
	class Security {
		private:
			// Путь к файлу сертификата тестового узла
			std::string _certificate;
			// Путь к файлу приватного ключа тестового узла
			std::string _privateKey;
		private:
			// Опознаватель шаблона контекста безопасности клиента
			tls::coder::id_t _client;
			// Опознаватель шаблона контекста безопасности сервера
			tls::coder::id_t _server;
		public:
			/**
			 * @brief Метод получения шаблона контекста безопасности клиента
			 *
			 * @return опознаватель шаблона контекста безопасности клиента
			 *
			 */
			tls::coder::id_t client() const noexcept {
				// Выводим шаблон контекста безопасности клиента
				return this->_client;
			}
			/**
			 * @brief Метод получения шаблона контекста безопасности сервера
			 *
			 * @return опознаватель шаблона контекста безопасности сервера
			 *
			 */
			tls::coder::id_t server() const noexcept {
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
				const std::string base = (::__awh_directory__() + "/awh-socks5-" + std::to_string(reinterpret_cast <uintptr_t> (this)));
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
	 * @brief Итоги прогона обмена через SOCKS5-прокси
	 *
	 */
	struct result_t {
		// Клиент установил туннель через прокси (и рукопожатие TLS к цели, если оно было)
		bool connected = false;
		// Обмен завершён штатно (не по сторожевому таймауту)
		bool completed = false;
		// Прогон прерван сторожевым таймаутом
		bool timedOut = false;
		// Цикл событий не откликнулся на остановку и был оставлен работать
		bool unstoppable = false;
		// Прокси вызвал коллбэк аутентификации клиента
		bool authSeen = false;
		// Принятое клиентом побайтово совпало с эталоном
		bool matched = false;
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
	static std::vector <std::tuple <std::unique_ptr <awh::server_t>, std::unique_ptr <awh::server::socks5_t>, std::unique_ptr <awh::client::socks5_t>>> & abandoned() noexcept {
		// Хранилище фасадов, оставленных работать до конца работы процесса
		static std::vector <std::tuple <std::unique_ptr <awh::server_t>, std::unique_ptr <awh::server::socks5_t>, std::unique_ptr <awh::client::socks5_t>>> result;
		// Выводим хранилище фасадов
		return result;
	}
	/**
	 * @brief Класс интеграционного окружения фасада SOCKS5 (loopback клиент ↔ прокси ↔ эхо)
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
			// Порт прослушивания сервера-эхо на локальной петле
			uint16_t _originPort;
			// Порт прослушивания SOCKS5-прокси на локальной петле
			uint16_t _proxyPort;
			// Признак TLS к конечному узлу сквозь туннель
			bool _tls;
			// Признак предъявления клиентом верной пары имя/пароль
			bool _authOk;
		private:
			// Объект DNS-резолвера клиента (обязан переживать фасад клиента)
			awh::unit::dns_t _dns;
		private:
			// Фасад сервера-эхо в роли конечного узла туннеля
			std::unique_ptr <awh::server_t> _origin;
			// Фасад сервера SOCKS5 в роли прокси
			std::unique_ptr <awh::server::socks5_t> _proxy;
			// Фасад клиента SOCKS5
			std::unique_ptr <awh::client::socks5_t> _client;
		private:
			// Опознаватель транспортного уровня клиента, заведённого прогоном
			tls::coder::id_t _ctl;
		private:
			// Отправленный клиентом запрос (эталон эха сквозь туннель)
			std::string _payload;
			// Ожидаемое принятое клиентом (эхо запроса)
			std::string _expected;
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
					// Останавливаем событие сервера SOCKS5 в роли прокси
					this->_proxy->stop();
					// Останавливаем работу цикла событий сервера-лаунчера
					this->_origin->stop();
				}
			}
		private:
			/**
			 * @brief Метод обработки изменения статуса сервера-эхо
			 *
			 * @param status новый статус сервера
			 *
			 */
			void originStatus(const event::status_t status) noexcept {
				// Если сервер-эхо перешёл в рабочее состояние (лаунчер запустил цикл событий)
				if(status == event::status_t::LAUNCHED){
					// Переводим сервер-эхо в режим прослушивания входящих соединений
					this->_origin->listen(100);
					// Поднимаем прокси: его статус-коллбэк продолжит цепочку запуска
					this->_proxy->start();
				}
			}
			/**
			 * @brief Метод обработки приёма подключения сервером-эхо
			 *
			 * @note Сокет принятого клиента переводится в неблокирующий режим по устройству
			 *       самой проверки: все стороны обмена живут в одном процессе на одном
			 *       потоке цикла, и блокирующая запись встала бы намертво
			 *
			 * @param cid идентификатор принятого клиента
			 *
			 */
			void originAccept(const event::id_t, const event::id_t cid, const tls::coder::id_t) noexcept {
				// Устанавливаем опции принятого клиента
				this->_origin->setOptions(cid, (event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::TCP_NO_DELAY));
			}
			/**
			 * @brief Метод обработки принятых сервером-эхо расшифрованных данных
			 *
			 * @param eid    идентификатор клиента
			 * @param buffer буфер принятых данных
			 * @param size   размер принятых данных
			 *
			 */
			void originRead(const event::id_t eid, const uint8_t * buffer, const size_t size, void *) noexcept {
				// Возвращаем принятое эхом отправителю
				this->_origin->send(eid, buffer, size);
			}
		private:
			/**
			 * @brief Метод обработки изменения статуса сервера SOCKS5 в роли прокси
			 *
			 * @param status новый статус сервера
			 *
			 */
			void proxyStatus(const event::status_t status) noexcept {
				// Если прокси перешёл в рабочее состояние
				if(status == event::status_t::LAUNCHED){
					// Переводим прокси в режим прослушивания входящих соединений
					this->_proxy->listen(100);
					// Поднимаем клиента: фасад SOCKS5 подключается к прокси сам на LAUNCHED
					this->_client->start();
				}
			}
			/**
			 * @brief Метод обработки аутентификации клиента прокси
			 *
			 * @param username имя пользователя, предъявленное клиентом
			 * @param password пароль, предъявленный клиентом
			 * @return         результат аутентификации
			 *
			 */
			bool proxyAuth(const std::string & username, const std::string & password) noexcept {
				// Запоминаем факт вызова коллбэка аутентификации
				this->_result.authSeen = true;
				// Выводим результат сверки предъявленной пары с эталонной
				return ((username == "testuser") && (password == "testpass"));
			}
			/**
			 * @brief Метод обработки приёма подключения клиента прокси
			 *
			 * @param cid идентификатор принятого клиента
			 *
			 */
			void proxyAccept(const event::id_t, const event::id_t cid) noexcept {
				// Устанавливаем опции принятого клиента
				this->_proxy->setOptions(cid, (event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::TCP_NO_DELAY));
			}
		private:
			/**
			 * @brief Метод обработки установки туннеля клиентом через прокси
			 *
			 * @param ok результат установки туннеля (и рукопожатия TLS к цели, если оно было)
			 *
			 */
			void clientConnect(const bool ok) noexcept {
				// Запоминаем результат установки туннеля
				this->_result.connected = ok;
				// Если туннель через прокси не установлен
				if(!ok){
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
					// Выходим из метода
					return;
				}
				// Отправляем запрос пользователя сквозь туннель
				this->_result.sendReturn = this->_client->send(this->_payload.data(), this->_payload.size());
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
			 * @brief Метод выполнения прогона обмена через SOCKS5-прокси
			 *
			 * @param timeout сторожевой таймаут прогона в миллисекундах
			 * @return        итоги прогона
			 *
			 */
			result_t execute(const uint32_t timeout) noexcept {
				// Создаём фасад сервера-эхо: с TLS-контекстом либо без него по устройству прогона
				if(this->_tls)
					// Создаём фасад сервера-эхо на серверном шаблоне контекста безопасности
					this->_origin = std::make_unique <awh::server_t> (this->_security->server());
				// Прогон без TLS к конечному узлу
				else this->_origin = std::make_unique <awh::server_t> ();
				// Создаём событие сервера-эхо потокового транспорта поверх TCP
				this->_origin->init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
				// Устанавливаем хост сервера-эхо на локальной петле
				this->_origin->setHost("127.0.0.1");
				// Устанавливаем порт прослушивания сервера-эхо
				this->_origin->setPort(this->_originPort);
				// Регистрируем коллбэк изменения статуса сервера-эхо
				this->_origin->on <void (const event::status_t)> ("status", &Harness::originStatus, this, _1);
				// Регистрируем коллбэк приёма подключения клиента сервером-эхо
				this->_origin->on <void (const event::id_t, const event::id_t, const tls::coder::id_t)> ("accept", &Harness::originAccept, this, _1, _2, _3);
				// Регистрируем коллбэк принятых сервером-эхо расшифрованных данных
				this->_origin->on <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", &Harness::originRead, this, _1, _2, _3, _4);
				/**
				 * Создаём фасад сервера SOCKS5 без DNS-резолвера: конечный узел туннеля
				 * задан IP-литералом локальной петли, и разрешение имён прокси не нужно
				 */
				this->_proxy = std::make_unique <awh::server::socks5_t> ();
				// Создаём событие прокси потокового транспорта поверх TCP
				this->_proxy->init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
				// Устанавливаем хост прокси на локальной петле
				this->_proxy->setHost("127.0.0.1");
				// Устанавливаем порт прослушивания прокси
				this->_proxy->setPort(this->_proxyPort);
				// Регистрируем коллбэк изменения статуса прокси
				this->_proxy->on <void (const event::status_t)> ("status", &Harness::proxyStatus, this, _1);
				// Регистрируем коллбэк аутентификации клиента прокси
				this->_proxy->on <bool (const std::string &, const std::string &)> ("auth", &Harness::proxyAuth, this, _1, _2);
				// Регистрируем коллбэк приёма подключения клиента прокси
				this->_proxy->on <void (const event::id_t, const event::id_t)> ("accept", &Harness::proxyAccept, this, _1, _2);
				/**
				 * Опознаватель транспортного уровня запоминается: реестр участников
				 * живёт весь процесс, и заведённый прогоном уровень обязан быть снят
				 * этим же прогоном - иначе он копится от проверки к проверке вместе с
				 * откликами, привязанными к фасаду клиента, который прогон разрушит
				 */
				// Если прогон с TLS к конечному узлу сквозь туннель
				if(this->_tls)
					// Заводим транспортный уровень клиента по шаблону
					this->_ctl = awh::tls::coder::transport(this->_security->client());
				// Создаём фасад клиента SOCKS5 на транспорте клиентского шаблона контекста безопасности
				this->_client = std::make_unique <awh::client::socks5_t> (this->_ctl, &this->_dns);
				// Создаём событие клиента потокового транспорта поверх TCP
				this->_client->init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
				// Устанавливаем опции клиента: все стороны на одном потоке цикла, блокирующая запись встала бы намертво
				this->_client->setOptions(event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::TCP_NO_DELAY);
				// Устанавливаем адрес прокси на локальной петле
				this->_client->setTarget("127.0.0.1");
				// Устанавливаем порт прокси
				this->_client->setTargetPort(this->_proxyPort);
				// Устанавливаем пару имя/пароль для аутентификации на прокси (верную либо заведомо неверную)
				this->_client->setUser("testuser", (this->_authOk ? "testpass" : "wrongpass"));
				// Устанавливаем конечную точку туннеля: сервер-эхо на локальной петле
				this->_client->endpoint("127.0.0.1", this->_originPort);
				// Регистрируем коллбэк установки туннеля клиентом через прокси
				this->_client->on <void (const bool)> ("connect", &Harness::clientConnect, this, _1);
				// Регистрируем коллбэк принятого клиентом расшифрованного ответа
				this->_client->on <void (const uint8_t *, const size_t)> ("read", &Harness::clientRead, this, _1, _2);
				// Обещание завершения работы фонового потока цикла событий
				std::promise <void> finished;
				// Ожидание завершения работы фонового потока цикла событий
				std::future <void> waiter = finished.get_future();
				// Запускаем сервер-лаунчер в фоновом потоке (start блокирует до остановки цикла)
				std::thread worker([this, &finished]() noexcept {
					// Запускаем цикл событий сервера-эхо (блокирует поток до вызова stop)
					this->_origin->start();
					// Сигнализируем завершение работы фонового потока
					finished.set_value();
				});
				// Ожидаем завершения обмена в пределах сторожевого таймаута
				if(waiter.wait_for(std::chrono::milliseconds(timeout)) == std::future_status::timeout){
					// Помечаем прерывание прогона сторожевым таймаутом
					this->_result.timedOut = true;
					// Останавливаем событие клиента (останавливает и DNS-резолвер)
					this->_client->stop();
					// Останавливаем событие сервера SOCKS5 в роли прокси
					this->_proxy->stop();
					// Будим цикл событий сервера-лаунчера для завершения фонового потока
					this->_origin->stop();
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
						abandoned().emplace_back(std::move(this->_origin), std::move(this->_proxy), std::move(this->_client));
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
			 * @param tls        признак TLS к конечному узлу сквозь туннель
			 * @param authOk     признак предъявления клиентом верной пары имя/пароль
			 * @param originPort порт прослушивания сервера-эхо на локальной петле
			 * @param proxyPort  порт прослушивания прокси на локальной петле
			 * @param security   тестовое окружение транспортной безопасности
			 *
			 */
			Harness(const bool tls, const bool authOk, const uint16_t originPort, const uint16_t proxyPort, Security * security) noexcept :
			 _security(security), _originPort(originPort), _proxyPort(proxyPort),
			 _tls(tls), _authOk(authOk), _dns(awh::event::family_t::IPV4), _ctl(0),
			 _payload{""}, _expected{""}, _finishing(false) {
				// Устанавливаем запрос пользователя сквозь туннель
				this->_payload = "GET / HTTP/1.1\r\nHost: tunnel.target\r\nConnection: close\r\n\r\n";
				// Устанавливаем эталон принятого: эхо запроса сквозь туннель
				this->_expected = this->_payload;
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
				// Разрушаем фасад клиента SOCKS5
				this->_client.reset(nullptr);
				// Разрушаем фасад сервера SOCKS5 в роли прокси
				this->_proxy.reset(nullptr);
				// Разрушаем фасад сервера-эхо
				this->_origin.reset(nullptr);
			}
	};
	/**
	 * @brief Класс интеграционного окружения фасада SOCKS5 UDP ASSOCIATE (loopback клиент ↔ прокси ↔ эхо)
	 *
	 */
	class UdpHarness {
		private:
			// Итоги прогона
			result_t _result;
		private:
			// Порт прослушивания сервера-эхо UDP на локальной петле
			uint16_t _originPort;
			// Порт прослушивания SOCKS5-прокси на локальной петле
			uint16_t _proxyPort;
			// Начальный порт диапазона UDP-ретрансляторов прокси
			uint16_t _udpBegin;
			// Конечный порт диапазона UDP-ретрансляторов прокси
			uint16_t _udpEnd;
			// Фиксированный порт источника UDP клиента на локальной петле
			uint16_t _clientPort;
			// Признак согласования UDP ASSOCIATE с нулевым портом источника (эфемерный порт)
			bool _ephemeral;
			// Признак согласования UDP ASSOCIATE с незаданным адресом источника (все нули)
			bool _wildcard;
		private:
			// Объект DNS-резолвера клиента (обязан переживать фасад клиента)
			awh::unit::dns_t _dns;
		private:
			// Фасад сервера-эхо UDP в роли конечного узла ассоциации
			std::unique_ptr <awh::server_t> _origin;
			// Фасад сервера SOCKS5 в роли прокси
			std::unique_ptr <awh::server::socks5_t> _proxy;
			// Фасад клиента SOCKS5
			std::unique_ptr <awh::client::socks5_t> _client;
		private:
			// Отправленная клиентом датаграмма (эталон эха сквозь ассоциацию)
			std::string _payload;
			// Ожидаемое принятое клиентом (эхо датаграммы)
			std::string _expected;
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
					// Останавливаем событие сервера SOCKS5 в роли прокси
					this->_proxy->stop();
					// Останавливаем работу цикла событий сервера-лаунчера
					this->_origin->stop();
				}
			}
		private:
			/**
			 * @brief Метод обработки изменения статуса сервера-эхо UDP
			 *
			 * @param status новый статус сервера
			 *
			 */
			void originStatus(const event::status_t status) noexcept {
				// Если сервер-эхо перешёл в рабочее состояние (лаунчер запустил цикл событий)
				if(status == event::status_t::LAUNCHED){
					/**
					 * Дейтаграммный сервер слушания не объявляет: приём датаграмм
					 * начинается сразу после запуска, а listen() к UDP неприменим
					 * и вернул бы отказ. Поднимаем прокси: его статус-коллбэк
					 * продолжит цепочку запуска
					 */
					this->_proxy->start();
				}
			}
			/**
			 * @brief Метод обработки приёма первого пакета от пира сервером-эхо UDP
			 *
			 * @note Сокет принятого пира переводится в неблокирующий режим по устройству
			 *       самой проверки: все стороны обмена живут в одном процессе на одном
			 *       потоке цикла, и блокирующая запись встала бы намертво
			 *
			 * @param cid идентификатор принятого пира
			 *
			 */
			void originAccept(const event::id_t, const event::id_t cid, const tls::coder::id_t) noexcept {
				// Устанавливаем опции принятого пира
				this->_origin->setOptions(cid, (event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK));
			}
			/**
			 * @brief Метод обработки принятых сервером-эхо UDP данных
			 *
			 * @param eid    идентификатор пира
			 * @param buffer буфер принятых данных
			 * @param size   размер принятых данных
			 *
			 */
			void originRead(const event::id_t eid, const uint8_t * buffer, const size_t size, void *) noexcept {
				// Возвращаем принятое эхом отправителю
				this->_origin->send(eid, buffer, size);
			}
		private:
			/**
			 * @brief Метод обработки изменения статуса сервера SOCKS5 в роли прокси
			 *
			 * @param status новый статус сервера
			 *
			 */
			void proxyStatus(const event::status_t status) noexcept {
				// Если прокси перешёл в рабочее состояние
				if(status == event::status_t::LAUNCHED){
					// Переводим прокси в режим прослушивания входящих соединений
					this->_proxy->listen(100);
					// Поднимаем клиента: фасад SOCKS5 подключается к прокси сам на LAUNCHED
					this->_client->start();
				}
			}
			/**
			 * @brief Метод обработки аутентификации клиента прокси
			 *
			 * @param username имя пользователя, предъявленное клиентом
			 * @param password пароль, предъявленный клиентом
			 * @return         результат аутентификации
			 *
			 */
			bool proxyAuth(const std::string & username, const std::string & password) noexcept {
				// Запоминаем факт вызова коллбэка аутентификации
				this->_result.authSeen = true;
				// Выводим результат сверки предъявленной пары с эталонной
				return ((username == "testuser") && (password == "testpass"));
			}
			/**
			 * @brief Метод обработки приёма подключения клиента прокси
			 *
			 * @param cid идентификатор принятого клиента
			 *
			 */
			void proxyAccept(const event::id_t, const event::id_t cid) noexcept {
				// Устанавливаем опции принятого клиента
				this->_proxy->setOptions(cid, (event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::TCP_NO_DELAY));
			}
		private:
			/**
			 * @brief Метод обработки установки ассоциации UDP клиентом через прокси
			 *
			 * @param ok результат установки ассоциации UDP
			 *
			 */
			void clientConnect(const bool ok) noexcept {
				// Запоминаем результат установки ассоциации UDP
				this->_result.connected = ok;
				// Если ассоциация UDP через прокси не установлена
				if(!ok){
					// Завершаем обмен и останавливаем цикл событий
					this->finish();
					// Выходим из метода
					return;
				}
				// Отправляем датаграмму пользователя сквозь ассоциацию
				this->_result.sendReturn = this->_client->send(this->_payload.data(), this->_payload.size());
			}
			/**
			 * @brief Метод обработки принятого клиентом ответа ассоциации UDP
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
			 * @brief Метод выполнения прогона обмена через ассоциацию UDP SOCKS5-прокси
			 *
			 * @param timeout сторожевой таймаут прогона в миллисекундах
			 * @return        итоги прогона
			 *
			 */
			result_t execute(const uint32_t timeout) noexcept {
				// Создаём фасад сервера-эхо UDP без транспортного шифрования
				this->_origin = std::make_unique <awh::server_t> ();
				// Создаём событие сервера-эхо дейтаграммного транспорта поверх UDP
				this->_origin->init(event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
				// Устанавливаем хост сервера-эхо на локальной петле
				this->_origin->setHost("127.0.0.1");
				// Устанавливаем порт приёма сервера-эхо
				this->_origin->setPort(this->_originPort);
				// Регистрируем коллбэк изменения статуса сервера-эхо
				this->_origin->on <void (const event::status_t)> ("status", &UdpHarness::originStatus, this, _1);
				// Регистрируем коллбэк приёма первого пакета от пира сервером-эхо
				this->_origin->on <void (const event::id_t, const event::id_t, const tls::coder::id_t)> ("accept", &UdpHarness::originAccept, this, _1, _2, _3);
				// Регистрируем коллбэк принятых сервером-эхо данных
				this->_origin->on <void (const event::id_t, const uint8_t *, const size_t, void *)> ("read", &UdpHarness::originRead, this, _1, _2, _3, _4);
				/**
				 * Создаём фасад сервера SOCKS5 без DNS-резолвера: конечный узел ассоциации
				 * задан IP-литералом локальной петли, и разрешение имён прокси не нужно
				 */
				this->_proxy = std::make_unique <awh::server::socks5_t> ();
				// Создаём событие прокси потокового транспорта поверх TCP
				this->_proxy->init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
				// Устанавливаем хост прокси на локальной петле
				this->_proxy->setHost("127.0.0.1");
				// Устанавливаем порт прослушивания прокси
				this->_proxy->setPort(this->_proxyPort);
				// Выделяем один UDP-ретранслятор прокси из заданного диапазона портов на локальной петле
				this->_proxy->udp(1, this->_udpBegin, this->_udpEnd, "127.0.0.1");
				// Регистрируем коллбэк изменения статуса прокси
				this->_proxy->on <void (const event::status_t)> ("status", &UdpHarness::proxyStatus, this, _1);
				// Регистрируем коллбэк аутентификации клиента прокси
				this->_proxy->on <bool (const std::string &, const std::string &)> ("auth", &UdpHarness::proxyAuth, this, _1, _2);
				// Регистрируем коллбэк приёма подключения клиента прокси
				this->_proxy->on <void (const event::id_t, const event::id_t)> ("accept", &UdpHarness::proxyAccept, this, _1, _2);
				// Создаём фасад клиента SOCKS5 без транспортного шифрования
				this->_client = std::make_unique <awh::client::socks5_t> (0, &this->_dns);
				// Создаём событие клиента потокового транспорта поверх TCP (управляющее соединение)
				this->_client->init(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
				// Устанавливаем опции клиента: все стороны на одном потоке цикла, блокирующая запись встала бы намертво
				this->_client->setOptions(event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::TCP_NO_DELAY);
				/**
				 * Заводим событие клиента дейтаграммного транспорта поверх UDP: в режиме
				 * wildcard согласуем ASSOCIATE с незаданным адресом источника (все нули),
				 * в режиме ephemeral — с нулевым портом (клиент выбирает порт сам),
				 * иначе привязываем фиксированный порт для сопоставления по точному адресу/порту
				 */
				if(this->_wildcard)
					// Согласуем ассоциацию с незаданным адресом: прокси принимает адрес TCP-соединения управления
					this->_client->udp("0.0.0.0", 0);
				else if(this->_ephemeral)
					// Согласуем ассоциацию с нулевым портом: клиент выбирает эфемерный порт источника
					this->_client->udp("127.0.0.1", 0);
				else
					// Привязываем фиксированный порт источника UDP клиента на локальной петле
					this->_client->udp("127.0.0.1", this->_clientPort);
				// Устанавливаем адрес прокси на локальной петле
				this->_client->setTarget("127.0.0.1");
				// Устанавливаем порт прокси
				this->_client->setTargetPort(this->_proxyPort);
				// Устанавливаем верную пару имя/пароль для аутентификации на прокси
				this->_client->setUser("testuser", "testpass");
				// Устанавливаем конечную точку ассоциации UDP: сервер-эхо на локальной петле
				this->_client->endpoint("127.0.0.1", this->_originPort);
				// Регистрируем коллбэк установки ассоциации UDP клиентом через прокси
				this->_client->on <void (const bool)> ("connect", &UdpHarness::clientConnect, this, _1);
				// Регистрируем коллбэк принятого клиентом ответа ассоциации UDP
				this->_client->on <void (const uint8_t *, const size_t)> ("read", &UdpHarness::clientRead, this, _1, _2);
				// Обещание завершения работы фонового потока цикла событий
				std::promise <void> finished;
				// Ожидание завершения работы фонового потока цикла событий
				std::future <void> waiter = finished.get_future();
				// Запускаем сервер-лаунчер в фоновом потоке (start блокирует до остановки цикла)
				std::thread worker([this, &finished]() noexcept {
					// Запускаем цикл событий сервера-эхо (блокирует поток до вызова stop)
					this->_origin->start();
					// Сигнализируем завершение работы фонового потока
					finished.set_value();
				});
				// Ожидаем завершения обмена в пределах сторожевого таймаута
				if(waiter.wait_for(std::chrono::milliseconds(timeout)) == std::future_status::timeout){
					// Помечаем прерывание прогона сторожевым таймаутом
					this->_result.timedOut = true;
					// Останавливаем событие клиента (останавливает и DNS-резолвер)
					this->_client->stop();
					// Останавливаем событие сервера SOCKS5 в роли прокси
					this->_proxy->stop();
					// Будим цикл событий сервера-лаунчера для завершения фонового потока
					this->_origin->stop();
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
						abandoned().emplace_back(std::move(this->_origin), std::move(this->_proxy), std::move(this->_client));
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
			 * @param originPort порт приёма сервера-эхо UDP на локальной петле
			 * @param proxyPort  порт прослушивания прокси на локальной петле
			 * @param udpBegin   начальный порт диапазона UDP-ретрансляторов прокси
			 * @param udpEnd     конечный порт диапазона UDP-ретрансляторов прокси
			 * @param clientPort фиксированный порт источника UDP клиента на локальной петле
			 * @param ephemeral  признак согласования ASSOCIATE с нулевым портом источника
			 * @param wildcard   признак согласования ASSOCIATE с незаданным адресом источника
			 *
			 */
			UdpHarness(const uint16_t originPort, const uint16_t proxyPort, const uint16_t udpBegin, const uint16_t udpEnd, const uint16_t clientPort, const bool ephemeral = false, const bool wildcard = false) noexcept :
			 _originPort(originPort), _proxyPort(proxyPort), _udpBegin(udpBegin), _udpEnd(udpEnd), _clientPort(clientPort), _ephemeral(ephemeral), _wildcard(wildcard),
			 _dns(awh::event::family_t::IPV4), _payload{""}, _expected{""}, _finishing(false) {
				// Устанавливаем датаграмму пользователя сквозь ассоциацию UDP
				this->_payload = "AWH SOCKS5 UDP ASSOCIATE loopback echo probe";
				// Устанавливаем эталон принятого: эхо датаграммы сквозь ассоциацию
				this->_expected = this->_payload;
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~UdpHarness() noexcept {
				// Разрушаем фасад клиента SOCKS5
				this->_client.reset(nullptr);
				// Разрушаем фасад сервера SOCKS5 в роли прокси
				this->_proxy.reset(nullptr);
				// Разрушаем фасад сервера-эхо UDP
				this->_origin.reset(nullptr);
			}
	};
};

/**
 * @brief Класс фикстуры интеграционных тестов фасада клиента SOCKS5
 *
 */
class Socks5FacadeTest : public testing::Test {
	protected:
		// Тестовое окружение транспортной безопасности
		std::unique_ptr <Security> _security;
	protected:
		/**
		 * @brief Метод подбора порта прослушивания на локальной петле
		 *
		 * @note Порт выводится из идентификатора процесса, чтобы снизить вероятность
		 *       коллизии при параллельных прогонах на одной машине. Диапазон отличается
		 *       от диапазона тестов фасадов HTTP-прокси: наборы идут в одном процессе,
		 *       а слушающий сокет предыдущей проверки уходит в TIME_WAIT
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
			// Выводим порт в диапазоне 53100..53999 на основе идентификатора процесса
			return static_cast <uint16_t> (53100 + ((static_cast <uint32_t> (::getpid()) + (offset++)) % 900));
		}
		/**
		 * @brief Метод подбора базового порта диапазона UDP на локальной петле
		 *
		 * @note Диапазон отличается и от TCP-портов этого набора, и от диапазона
		 *       тестов фасадов HTTP-прокси: UDP-ретранслятор прокси и сервер-эхо
		 *       обязаны получить свободные порты, не пересекающиеся с TCP-стороной
		 *
		 * @return подобранный базовый порт диапазона UDP
		 *
		 */
		uint16_t pickUdpPort() const noexcept {
			/**
			 * Счётчик выданных базовых портов: связанный UDP-сокет предыдущей
			 * проверки может ещё удерживать порт, и повторная привязка к тому же
			 * порту не проходит
			 */
			static uint16_t offset = 0;
			// Выводим базовый порт в диапазоне 54100..54899 на основе идентификатора процесса
			return static_cast <uint16_t> (54100 + ((static_cast <uint32_t> (::getpid()) + (offset++)) % 800));
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
 * @brief Тест туннеля CONNECT через SOCKS5-прокси с TLS к конечному узлу
 *
 * @details Проверяет полный обмен: аутентификацию по имени пользователя и паролю,
 *          запрос CONNECT, рукопожатие TLS к конечному узлу сквозь открытый
 *          туннель и побайтовое эхо прикладных данных. TLS здесь установлен к
 *          конечному узлу, а не к прокси: само согласование SOCKS5 идёт открытым
 *          текстом, и шифротекст обязан дойти до прикладного обмена расшифрованным
 *
 */
TEST_F(Socks5FacadeTest, TunnelOverSocks5Tls){
	// Проверяем готовность тестового окружения транспортной безопасности
	ASSERT_TRUE(this->_security->ready()) << "security environment is not ready";
	// Создаём интеграционное окружение фасада с TLS к конечному узлу сквозь туннель
	Harness harness(true, true, this->pickPort(), this->pickPort(), this->_security.get());
	// Запускаем прогон обмена через SOCKS5-прокси
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "socks5 negotiation or tunnel exchange timed out";
	// Проверяем, что туннель через прокси установлен
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что прокси вызвал коллбэк аутентификации клиента
	EXPECT_TRUE(result.authSeen);
	// Проверяем, что клиентский send() принял запрос целиком
	EXPECT_GT(result.sendReturn, 0u);
	// Проверяем, что принятое клиентом эхо туннеля побайтово совпало с запросом
	EXPECT_TRUE(result.matched);
}

/**
 * @brief Тест туннеля CONNECT через SOCKS5-прокси без TLS
 *
 * @details Проверяет тот же обмен без транспортного шифрования: аутентификацию,
 *          запрос CONNECT и побайтовое эхо прикладных данных открытым текстом.
 *          Проверка отделяет работу согласования SOCKS5 и туннеля от механики TLS
 *
 */
TEST_F(Socks5FacadeTest, TunnelOverSocks5Plain){
	// Проверяем готовность тестового окружения транспортной безопасности
	ASSERT_TRUE(this->_security->ready()) << "security environment is not ready";
	// Создаём интеграционное окружение фасада без TLS к конечному узлу
	Harness harness(false, true, this->pickPort(), this->pickPort(), this->_security.get());
	// Запускаем прогон обмена через SOCKS5-прокси
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "socks5 negotiation or tunnel exchange timed out";
	// Проверяем, что туннель через прокси установлен
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что прокси вызвал коллбэк аутентификации клиента
	EXPECT_TRUE(result.authSeen);
	// Проверяем, что клиентский send() принял запрос целиком
	EXPECT_GT(result.sendReturn, 0u);
	// Проверяем, что принятое клиентом эхо туннеля побайтово совпало с запросом
	EXPECT_TRUE(result.matched);
}

/**
 * @brief Тест отказа аутентификации на SOCKS5-прокси
 *
 * @details Клиент предъявляет заведомо неверный пароль, прокси отклоняет пару
 *          имя/пароль на этапе аутентификации. Клиент обязан откликнуться
 *          неудачей подключения, а не зависнуть в ожидании ответа CONNECT:
 *          отказ аутентификации приходит раньше запроса CONNECT
 *
 */
TEST_F(Socks5FacadeTest, AuthFailure){
	// Проверяем готовность тестового окружения транспортной безопасности
	ASSERT_TRUE(this->_security->ready()) << "security environment is not ready";
	// Создаём интеграционное окружение фасада с заведомо неверным паролем
	Harness harness(false, false, this->pickPort(), this->pickPort(), this->_security.get());
	// Запускаем прогон обмена через SOCKS5-прокси
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "auth failure was not reported by the client";
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что прокси вызвал коллбэк аутентификации клиента
	EXPECT_TRUE(result.authSeen);
	// Проверяем, что клиент откликнулся неудачей установки туннеля
	EXPECT_FALSE(result.connected);
	// Проверяем, что клиент не отправлял прикладные данные без установленного туннеля
	EXPECT_EQ(result.sendReturn, 0u);
}

/**
 * @brief Тест ассоциации UDP ASSOCIATE через SOCKS5-прокси
 *
 * @details Проверяет полный обмен дейтаграммами: аутентификацию по имени
 *          пользователя и паролю на управляющем TCP-соединении, запрос
 *          UDP ASSOCIATE, отправку датаграммы через выделенный прокси
 *          UDP-ретранслятор и побайтовое эхо прикладных данных от сервера-эхо
 *          UDP. Клиентский send() обязан обрамить датаграмму заголовком
 *          SOCKS5 UDP, а read() - снять его перед выдачей приложению
 *
 * @note Клиент привязывает UDP к фиксированному порту источника — путь точного
 *       сопоставления сессии по адресу/порту. Эфемерный порт источника покрыт
 *       отдельным тестом UdpAssociateEphemeralPort
 *
 */
TEST_F(Socks5FacadeTest, UdpAssociateOverSocks5){
	// Подбираем базовый порт диапазона UDP: на нём слушает сервер-эхо, выше - ретранслятор прокси
	const uint16_t udpBase = this->pickUdpPort();
	// Создаём интеграционное окружение фасада ассоциации UDP (порт источника клиента отделён от диапазона ретранслятора)
	UdpHarness harness(udpBase, this->pickPort(), static_cast <uint16_t> (udpBase + 1), static_cast <uint16_t> (udpBase + 11), static_cast <uint16_t> (udpBase + 50));
	// Запускаем прогон обмена через ассоциацию UDP SOCKS5-прокси
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "socks5 udp associate exchange timed out";
	// Проверяем, что ассоциация UDP через прокси установлена
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что прокси вызвал коллбэк аутентификации клиента
	EXPECT_TRUE(result.authSeen);
	// Проверяем, что клиентский send() принял датаграмму целиком
	EXPECT_GT(result.sendReturn, 0u);
	// Проверяем, что принятое клиентом эхо ассоциации побайтово совпало с отправленным
	EXPECT_TRUE(result.matched);
}

/**
 * @brief Тест ассоциации UDP ASSOCIATE с эфемерным портом источника
 *
 * @details Проверяет обмен дейтаграммами, когда клиент не привязывает фиксированный
 *          порт источника UDP (нулевой порт в udp()): клиент выбирает порт сам,
 *          а прокси сопоставляет датаграмму с сессией ASSOCIATE по адресу/порту
 *          источника. Покрывает клиентский выбор эфемерного порта и серверное
 *          сопоставление сессии для него
 *
 * @note Адрес источника клиента — петля с нулевым портом: клиент сообщает прокси
 *       фактический эфемерный порт, сопоставление идёт по точному адресу/порту.
 *       Истинный wildcard (незаданный адрес источника 0.0.0.0) покрыт отдельным
 *       тестом UdpAssociateWildcardAddress
 *
 */
TEST_F(Socks5FacadeTest, UdpAssociateEphemeralPort){
	// Подбираем базовый порт диапазона UDP: на нём слушает сервер-эхо, выше - ретранслятор прокси
	const uint16_t udpBase = this->pickUdpPort();
	// Создаём окружение фасада ассоциации UDP с эфемерным портом источника (нулевой порт в udp())
	UdpHarness harness(udpBase, this->pickPort(), static_cast <uint16_t> (udpBase + 1), static_cast <uint16_t> (udpBase + 11), 0, true);
	// Запускаем прогон обмена через ассоциацию UDP SOCKS5-прокси
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "socks5 udp ephemeral associate exchange timed out";
	// Проверяем, что ассоциация UDP через прокси установлена
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что прокси вызвал коллбэк аутентификации клиента
	EXPECT_TRUE(result.authSeen);
	// Проверяем, что клиентский send() принял датаграмму целиком
	EXPECT_GT(result.sendReturn, 0u);
	// Проверяем, что принятое клиентом эхо ассоциации побайтово совпало с отправленным
	EXPECT_TRUE(result.matched);
}

/**
 * @brief Тест ассоциации UDP ASSOCIATE с незаданным адресом источника
 *
 * @details Проверяет обмен дейтаграммами, когда клиент не задаёт исходящий адрес
 *          UDP (все нули в udp()): по RFC 1928 §6 клиент передаёт прокси нулевой
 *          адрес, а прокси принимает адрес TCP-соединения управления и сопоставляет
 *          датаграммы с сессией ASSOCIATE по этому адресу. Покрывает клиентскую
 *          передачу нулевого адреса и серверное сопоставление сессии для него
 *
 * @note Движок подставляет адрес устройства по умолчанию вместо нулевого адреса
 *       источника (требование PCP), поэтому клиент SOCKS5 передаёт прокси именно
 *       заданный нулевой адрес, а не результат getAddress: иначе адрес сессии
 *       не совпал бы с адресом датаграмм на петле
 *
 */
TEST_F(Socks5FacadeTest, UdpAssociateWildcardAddress){
	// Подбираем базовый порт диапазона UDP: на нём слушает сервер-эхо, выше - ретранслятор прокси
	const uint16_t udpBase = this->pickUdpPort();
	// Создаём окружение фасада ассоциации UDP с незаданным адресом источника (все нули в udp())
	UdpHarness harness(udpBase, this->pickPort(), static_cast <uint16_t> (udpBase + 1), static_cast <uint16_t> (udpBase + 11), 0, false, true);
	// Запускаем прогон обмена через ассоциацию UDP SOCKS5-прокси
	const result_t result = harness.execute(20000);
	// Проверяем, что прогон не прерван сторожевым таймаутом
	ASSERT_FALSE(result.timedOut) << "socks5 udp wildcard associate exchange timed out";
	// Проверяем, что ассоциация UDP через прокси установлена
	ASSERT_TRUE(result.connected);
	// Проверяем, что обмен завершён штатно
	ASSERT_TRUE(result.completed);
	// Проверяем, что прокси вызвал коллбэк аутентификации клиента
	EXPECT_TRUE(result.authSeen);
	// Проверяем, что клиентский send() принял датаграмму целиком
	EXPECT_GT(result.sendReturn, 0u);
	// Проверяем, что принятое клиентом эхо ассоциации побайтово совпало с отправленным
	EXPECT_TRUE(result.matched);
}