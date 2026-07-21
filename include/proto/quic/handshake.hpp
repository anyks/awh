/**
 * @file: handshake.hpp
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

#ifndef __AWH_PROTO_QUIC_HANDSHAKE__
#define __AWH_PROTO_QUIC_HANDSHAKE__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "quic.hpp"
#include "crypto.hpp"
#include "params.hpp"
#include "../../sys/global.hpp"

/**
 * Предварительные объявления типов BoringSSL (реализация в handshake.cpp)
 */
struct ssl_st;
struct ssl_ctx_st;

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён транспортного протокола QUIC
	 *
	 */
	namespace quic {
		/**
		 * Предварительное объявление класса доступа обратных вызовов BoringSSL
		 */
		class HandshakeHook;

		/**
		 * @brief Класс криптографического хендшейка QUIC (RFC 9001 §4)
		 *
		 * @details Машина TLS 1.3 хендшейка поверх QUIC API BoringSSL (SSL_QUIC_METHOD).
		 *          Работает без ввода-вывода (sans-IO): принимает данные CRYPTO-фреймов
		 *          через crypto(), накапливает исходящие CRYPTO-данные по уровням
		 *          шифрования (извлекаются через data()) и выводит ключи защиты пакетов
		 *          каждого уровня по мере поступления секретов от TLS-стека.
		 *          Транспортные параметры передаются в расширении quic_transport_parameters
		 *          как непрозрачные байты - сериализация модулем params.
		 *          Сборка/разбор пакетов и фреймов, потери и таймеры - вне этого класса.
		 */
		typedef class __AWH_SHARED_EXPORT__ Handshake {
			public:
				/**
				 * @brief Состояния хендшейка
				 *
				 */
				enum class state_t : uint8_t {
					NONE      = 0x00, // Хендшейк не начат
					PROCESS   = 0x01, // Хендшейк выполняется
					COMPLETED = 0x02, // Хендшейк успешно завершён
					FAILED    = 0x03  // Хендшейк завершился ошибкой
				};
			public:
				/**
				 * @brief Количество уровней шифрования (RFC 9001 §4)
				 *
				 */
				static constexpr size_t LEVELS = 4;
			private:
				/**
				 * @brief Структура состояния одного уровня шифрования
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Level {
					// Флаг наличия ключей чтения
					bool hasRead;
					// Флаг наличия ключей записи
					bool hasWrite;
					// Исходящие CRYPTO-данные уровня (ожидают отправки)
					string data;
					// Ключи снятия защиты входящих пакетов уровня
					crypto::keys_t read;
					// Ключи защиты исходящих пакетов уровня
					crypto::keys_t write;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Level() noexcept;
				} level_data_t;
			private:
				// Роль локального эндпоинта на соединении
				endpoint_t _endpoint;
				// Состояние хендшейка
				state_t _state;
			private:
				// Код фатального TLS-алерта (RFC 9001 §4.8)
				uint8_t _alert;
				// Флаг получения фатального TLS-алерта
				bool _hasAlert;
			private:
				// Флаг проверки сертификата удалённого узла
				bool _verify;
			private:
				// Объект контекста TLS
				ssl_ctx_st * _ctx;
				// Объект TLS-соединения
				ssl_st * _ssl;
			private:
				// Доменное имя удалённого сервера (SNI)
				string _sni;
				// Сериализованные локальные транспортные параметры
				string _params;
				// Сертификат локального узла в формате PEM
				string _certificate;
				// Приватный ключ локального узла в формате PEM
				string _privateKey;
				// Список поддерживаемых ALPN-протоколов
				vector <string> _protocols;
			private:
				// Состояния уровней шифрования
				level_data_t _levels[LEVELS];
			private:
				/**
				 * Дружественный класс доступа обратных вызовов BoringSSL QUIC API
				 */
				friend class HandshakeHook;
			private:
				/**
				 * @brief Метод продвижения TLS-хендшейка
				 *
				 * @return результат продвижения (OK - хендшейк продолжается или завершён)
				 */
				status_t process() noexcept;
			public:
				/**
				 * @brief Метод установки списка поддерживаемых ALPN-протоколов
				 *
				 * @note Вызывается до начала хендшейка. Для QUIC согласование ALPN
				 *       обязательно (RFC 9001 §8.1) - список не может быть пустым
				 *
				 * @param protocols список поддерживаемых ALPN-протоколов (например "h3")
				 */
				void alpn(const vector <string> & protocols) noexcept;
				/**
				 * @brief Метод извлечения согласованного ALPN-протокола
				 *
				 * @return согласованный ALPN-протокол (пусто - согласование не выполнено)
				 */
				string alpn() const noexcept;
			public:
				/**
				 * @brief Метод установки доменного имени удалённого сервера (SNI)
				 *
				 * @note Вызывается клиентом до начала хендшейка
				 *
				 * @param sni доменное имя удалённого сервера
				 */
				void serverNameIndication(string_view sni) noexcept;
			public:
				/**
				 * @brief Метод установки локальных транспортных параметров (RFC 9000 §7.4)
				 *
				 * @note Вызывается до начала хендшейка. Параметры сериализуются
				 *       и передаются в расширении quic_transport_parameters
				 *
				 * @param params локальные транспортные параметры
				 * @return       результат установки (false - ошибка сериализации)
				 */
				bool params(const quic::params::params_t & params) noexcept;
				/**
				 * @brief Метод извлечения транспортных параметров удалённого узла (RFC 9000 §7.4)
				 *
				 * @note Параметры доступны после обработки ClientHello сервером
				 *       либо EncryptedExtensions клиентом
				 *
				 * @param params транспортные параметры удалённого узла
				 * @param error  код ошибки транспорта
				 * @return       результат извлечения (OK/INCOMPLETE/ERROR)
				 */
				status_t peer(quic::params::params_t & params, error_t & error) const noexcept;
			public:
				/**
				 * @brief Метод установки сертификата и приватного ключа локального узла
				 *
				 * @note Вызывается сервером до начала хендшейка (для клиента - при mTLS)
				 *
				 * @param certificate сертификат в формате PEM
				 * @param privateKey  приватный ключ в формате PEM
				 */
				void certificate(string_view certificate, string_view privateKey) noexcept;
			public:
				/**
				 * @brief Метод установки проверки сертификата удалённого узла
				 *
				 * @note По умолчанию проверка отключена - доверенные центры сертификации
				 *       настраиваются на уровне рабочего юнита
				 *
				 * @param mode режим проверки сертификата удалённого узла
				 */
				void verify(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод вывода ключей уровня Initial (RFC 9001 §5.2)
				 *
				 * @note Направления чтения/записи назначаются согласно роли эндпоинта:
				 *       клиент пишет клиентскими ключами и читает серверными, сервер - наоборот
				 *
				 * @param dcid идентификатор соединения получателя первого пакета Initial клиента
				 * @return     результат вывода (false - ошибка криптографической библиотеки)
				 */
				bool initial(const cid_t & dcid) noexcept;
			public:
				/**
				 * @brief Метод начала хендшейка
				 *
				 * @note Все настройки (ALPN, SNI, параметры, сертификат) должны быть
				 *       установлены до вызова. Клиент формирует ClientHello - исходящие
				 *       CRYPTO-данные появятся на уровне INITIAL. Сервер только
				 *       инициализирует TLS-стек и ожидает данных через crypto()
				 *
				 * @return результат начала хендшейка (OK/ERROR)
				 */
				status_t start() noexcept;
				/**
				 * @brief Метод обработки входящих данных CRYPTO-фреймов
				 *
				 * @note Данные должны передаваться в порядке смещений CRYPTO-потока уровня
				 *       (сборку по смещениям выполняет вызывающий код). После завершения
				 *       хендшейка обрабатывает post-handshake сообщения (NewSessionTicket)
				 *
				 * @param level уровень шифрования пакета с CRYPTO-фреймом
				 * @param data  данные CRYPTO-фрейма
				 * @param size  размер данных CRYPTO-фрейма
				 * @return      результат обработки (OK/ERROR)
				 */
				status_t crypto(const level_t level, const uint8_t * data, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод проверки наличия исходящих CRYPTO-данных уровня
				 *
				 * @param level уровень шифрования
				 * @return      результат проверки (true - есть данные для отправки)
				 */
				bool pending(const level_t level) const noexcept;
				/**
				 * @brief Метод извлечения исходящих CRYPTO-данных уровня
				 *
				 * @note Данные передаются вызывающему коду и удаляются из очереди уровня.
				 *       Вызывающий код упаковывает их в CRYPTO-фреймы, отслеживая смещения
				 *
				 * @param level уровень шифрования
				 * @return      исходящие CRYPTO-данные уровня
				 */
				string data(const level_t level) noexcept;
			public:
				/**
				 * @brief Метод извлечения ключей защиты исходящих пакетов уровня
				 *
				 * @param level уровень шифрования
				 * @return      ключи защиты пакетов либо nullptr если ключи ещё не выведены
				 */
				const crypto::keys_t * encryption(const level_t level) const noexcept;
				/**
				 * @brief Метод извлечения ключей снятия защиты входящих пакетов уровня
				 *
				 * @param level уровень шифрования
				 * @return      ключи защиты пакетов либо nullptr если ключи ещё не выведены
				 */
				const crypto::keys_t * decryption(const level_t level) const noexcept;
			public:
				/**
				 * @brief Метод замены ключей уровня шифрования (RFC 9001 §6)
				 *
				 * @note Используется слоем соединения при обновлении ключей (key update):
				 *       ключи уровня APPLICATION заменяются ключами следующей фазы
				 *
				 * @param level уровень шифрования
				 * @param read  ключи снятия защиты входящих пакетов уровня
				 * @param write ключи защиты исходящих пакетов уровня
				 */
				void install(const level_t level, const crypto::keys_t & read, const crypto::keys_t & write) noexcept;
			public:
				/**
				 * @brief Метод сброса ключей уровня шифрования (RFC 9001 §4.9)
				 *
				 * @note Ключи Initial сбрасываются после установки ключей Handshake,
				 *       ключи Handshake - после подтверждения хендшейка
				 *
				 * @param level уровень шифрования
				 */
				void discard(const level_t level) noexcept;
			public:
				/**
				 * @brief Метод получения состояния хендшейка
				 *
				 * @return состояние хендшейка
				 */
				state_t state() const noexcept;
				/**
				 * @brief Метод получения кода ошибки транспорта (RFC 9001 §4.8)
				 *
				 * @note При фатальном TLS-алерте возвращает CRYPTO_ERROR + код алерта,
				 *       при иных ошибках хендшейка - INTERNAL_ERROR
				 *
				 * @return код ошибки транспорта (NO_ERROR - ошибки нет)
				 */
				error_t error() const noexcept;
			public:
				/**
				 * Запрещаем копирование и перемещение (объект владеет TLS-соединением,
				 * на которое BoringSSL хранит обратный указатель)
				 */
				Handshake(const Handshake &) = delete;
				Handshake(Handshake &&) = delete;
				Handshake & operator = (const Handshake &) = delete;
				Handshake & operator = (Handshake &&) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param endpoint роль локального эндпоинта на соединении
				 */
				explicit Handshake(const endpoint_t endpoint) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Handshake() noexcept;
		} handshake_t;
	};
};

#endif // __AWH_PROTO_QUIC_HANDSHAKE__
