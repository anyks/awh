/**
 * @file: handshake.hpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
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
#include "../../net/tls/coder.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
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
				/**
				 * Контекст TLS, настроенный вызывающим кодом. Настройка шифров, групп,
				 * алгоритмов подписи, доверенных центров, списков отзыва и расширений
				 * выполняется на нём целиком, поэтому собственных настроек криптографии
				 * хендшейк не имеет. Владение контекстом остаётся за вызывающим кодом
				 */
				ssl_ctx_st * _ctx;
				// Объект TLS-соединения
				ssl_st * _ssl;
			private:
				// Сериализованные локальные транспортные параметры
				string _params;
				/**
				 * Контекст ранних данных: лимиты, под которые удалённый узел отправляет
				 * ранние данные. Билет возобновления действителен только при совпадении
				 * контекста, поэтому уникальные для соединения значения в него
				 * не входят (RFC 9001 §4.6.1)
				 */
				string _early;
				// Сериализованная сессия возобновления хендшейка (RFC 9001 §4.6)
				string _session;
				// Сериализованный билет возобновления, присланный удалённым узлом
				string _ticket;
				// Флаг отказа удалённого узла в ранних данных (RFC 9001 §4.6.2)
				bool _rejected;
				// Список поддерживаемых ALPN-протоколов из контекста TLS
				vector <tls::coder_t::alpn_t> _protocols;
			private:
				// Состояния уровней шифрования
				level_data_t _levels[LEVELS];
			private:
				// Объект для работы с логами
				const log_t * _log;
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
				 * @brief Метод извлечения согласованного ALPN-протокола
				 *
				 * @note Согласование ALPN для QUIC обязательно (RFC 9001 §8.1).
				 *       Список поддерживаемых протоколов задаётся на контексте TLS,
				 *       здесь выдаётся результат согласования с идентификатором
				 *       протокола из этого списка
				 *
				 * @return согласованный ALPN-протокол (пустое название - согласование не выполнено)
				 */
				tls::coder_t::alpn_t alpn() const noexcept;
			public:
				/**
				 * @brief Метод извлечения возобновляемой сессии хендшейка (RFC 9001 §4.6)
				 *
				 * @note Сессия становится доступна после приёма билета возобновления,
				 *       который сервер присылает уже после завершения хендшейка.
				 *       Сохранив её, вызывающий код возобновляет соединение с тем же
				 *       сервером и отправляет ранние данные, не дожидаясь хендшейка
				 *
				 * @return сериализованная сессия (пусто - сессия недоступна)
				 */
				string session() const noexcept;
				/**
				 * @brief Метод установки возобновляемой сессии хендшейка (RFC 9001 §4.6)
				 *
				 * @note Устанавливается до начала хендшейка и только на клиенте.
				 *       Сессия обязана относиться к тому же серверу: билет привязан
				 *       к его параметрам, и подмена приведёт к отказу возобновления
				 *
				 * @param session сериализованная сессия
				 * @return        результат установки
				 */
				bool session(string_view session) noexcept;
				/**
				 * @brief Метод проверки принятия ранних данных удалённым узлом
				 *
				 * @note Отказ означает, что отправленные ранние данные потеряны и
				 *       подлежат повторной отправке на уровне приложения (RFC 9001 §4.6.2)
				 *
				 * @return результат проверки
				 */
				bool early() const noexcept;
				/**
				 * @brief Метод проверки отказа удалённого узла в ранних данных (RFC 9001 §4.6.2)
				 *
				 * @note Отказ отказом хендшейка не является: он лишь означает, что
				 *       отправленные ранние данные потеряны и подлежат повторной
				 *       отправке после завершения хендшейка
				 *
				 * @return результат проверки
				 */
				bool rejected() const noexcept;
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
				 * @note Контекст TLS обязателен: вся настройка криптографии выполняется
				 *       на нём средствами модуля транспортной безопасности, поэтому
				 *       собственных настроек хендшейк не имеет. Контекст обязан
				 *       пережить объект хендшейка
				 *
				 * @param endpoint роль локального эндпоинта на соединении
				 * @param ctx      идентификатор шаблона контекста безопасности
				 * @param coder    объект кодера транспортной безопасности
				 * @param log      объект для работы с логами
				 */
				explicit Handshake(const endpoint_t endpoint, const tls::coder_t::id_t ctx, const tls::coder_t & coder, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Handshake() noexcept;
		} handshake_t;
	};
};

#endif // __AWH_PROTO_QUIC_HANDSHAKE__
