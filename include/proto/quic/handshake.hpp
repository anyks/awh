/**
 * @file handshake.hpp
 * @date 2026-07-21
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
 * \~russian
 * @brief Заголовочный файл криптографического рукопожатия QUIC — класс quic::Handshake,
 *        управляющий уровнями шифрования, обменом CRYPTO-данными с BoringSSL,
 *        установкой ключей и передачей транспортных параметров как непрозрачных байт
 *
 * \~english
 * @brief Header file of the QUIC cryptographic handshake — the quic::Handshake class,
 *        which manages the encryption levels, the exchange of the CRYPTO data with BoringSSL,
 *        the setting of the keys and the passing of the transport parameters as opaque bytes
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
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
#include "../../sys/log.hpp"
#include "../../sys/global.hpp"
#include "../../cryptography/tls/coder.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их pop.hpp в конце файла)
 */
#include "../../sys/push.hpp"

/**
 * Предварительные объявления типов BoringSSL (реализация в handshake.cpp)
 */
struct ssl_st;
struct ssl_ctx_st;

/**
 * \~russian
 * @brief основное пространство имён
 *
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён транспортного протокола QUIC
	 *
	 *
	 * \~english
	 * @brief QUIC transport protocol namespace
	 *
	 * \~
	 */
	namespace quic {
		/**
		 * Предварительное объявление класса доступа обратных вызовов BoringSSL
		 */
		class HandshakeHook;

		/**
		 * \~russian
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
		 *
		 * \~english
		 * @brief Class of the QUIC cryptographic handshake (RFC 9001 §4)
		 * @details A machine of the TLS 1.3 handshake on top of the QUIC API of BoringSSL (SSL_QUIC_METHOD).
		 *          Works without input-output (sans-IO): accepts the data of the CRYPTO frames
		 *          through crypto(), accumulates the outgoing CRYPTO data by the encryption
		 *          levels (they are extracted through data()) and derives the keys of the protection of the packets
		 *          of every level as the secrets arrive from the TLS stack.
		 *          The transport parameters are transmitted in the quic_transport_parameters extension
		 *          as opaque bytes — the serialization is done by the params module.
		 *          The assembly/parsing of the packets and of the frames, the losses and the timers are outside this class.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Handshake {
			public:
				/**
				 * \~russian
				 * @brief Состояния хендшейка
				 *
				 * \~english
				 * @brief States of the handshake
				 *
				 * \~
				 */
				enum class state_t : uint8_t {
					NONE      = 0x00, // Хендшейк не начат
					PROCESS   = 0x01, // Хендшейк выполняется
					COMPLETED = 0x02, // Хендшейк успешно завершён
					FAILED    = 0x03  // Хендшейк завершился ошибкой
				};
			public:
				/**
				 * \~russian
				 * @brief Количество уровней шифрования (RFC 9001 §4)
				 *
				 * \~english
				 * @brief Number of the encryption levels (RFC 9001 §4)
				 *
				 * \~
				 */
				static constexpr size_t LEVELS = 4;
			private:
				/**
				 * \~russian
				 * @brief Структура состояния одного уровня шифрования
				 *
				 * \~english
				 * @brief Structure of the state of a single encryption level
				 *
				 * \~
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
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
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
				 * \~russian
				 * @brief Метод продвижения TLS-хендшейка
				 *
				 * @return результат продвижения (OK - хендшейк продолжается или завершён)
				 *
				 * \~english
				 * @brief Method of advancing the TLS handshake
				 * @return result of the advancement (OK — the handshake continues or has been completed)
				 *
				 * \~
				 */
				status_t process() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения согласованного ALPN-протокола
				 *
				 * @note Согласование ALPN для QUIC обязательно (RFC 9001 §8.1).
				 *       Список поддерживаемых протоколов задаётся на контексте TLS,
				 *       здесь выдаётся результат согласования с идентификатором
				 *       протокола из этого списка
				 *
				 * @return согласованный ALPN-протокол (пустое название - согласование не выполнено)
				 *
				 * \~english
				 * @brief Method of extracting the negotiated ALPN protocol
				 * @note The negotiation of the ALPN is obligatory for QUIC (RFC 9001 §8.1).
				 *       The list of the supported protocols is given on the TLS context,
				 *       here the result of the negotiation with the identifier of the
				 *       protocol from that list is issued
				 * @return negotiated ALPN protocol (an empty name — the negotiation has not been performed)
				 *
				 * \~
				 */
				tls::coder_t::alpn_t alpn() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения возобновляемой сессии хендшейка (RFC 9001 §4.6)
				 *
				 * @note Сессия становится доступна после приёма билета возобновления,
				 *       который сервер присылает уже после завершения хендшейка.
				 *       Сохранив её, вызывающий код возобновляет соединение с тем же
				 *       сервером и отправляет ранние данные, не дожидаясь хендшейка
				 *
				 * @return сериализованная сессия (пусто - сессия недоступна)
				 *
				 * \~english
				 * @brief Method of extracting the resumable session of the handshake (RFC 9001 §4.6)
				 * @note The session becomes available after the reception of the resumption ticket,
				 *       which the server sends already after the completion of the handshake.
				 *       Having preserved it, the calling code resumes the connection with the same
				 *       server and sends the early data without waiting for the handshake
				 * @return serialized session (empty — the session is unavailable)
				 *
				 * \~
				 */
				string session() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки возобновляемой сессии хендшейка (RFC 9001 §4.6)
				 *
				 * @note Устанавливается до начала хендшейка и только на клиенте.
				 *       Сессия обязана относиться к тому же серверу: билет привязан
				 *       к его параметрам, и подмена приведёт к отказу возобновления
				 *
				 * @param session сериализованная сессия
				 * @return        результат установки
				 *
				 * \~english
				 * @brief Method of setting the resumable session of the handshake (RFC 9001 §4.6)
				 * @note It is set before the beginning of the handshake and only on the client.
				 *       The session is obliged to relate to the same server: the ticket is bound
				 *       to its parameters, and a substitution will lead to a refusal of the resumption
				 * @param session serialized session
				 * @return        result of the setting
				 *
				 * \~
				 */
				bool session(string_view session) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки принятия ранних данных удалённым узлом
				 *
				 * @note Отказ означает, что отправленные ранние данные потеряны и
				 *       подлежат повторной отправке на уровне приложения (RFC 9001 §4.6.2)
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking the acceptance of the early data by the remote node
				 * @note A refusal means that the sent early data has been lost and
				 *       is subject to a repeated sending at the level of the application (RFC 9001 §4.6.2)
				 * @return result of the check
				 *
				 * \~
				 */
				bool early() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки отказа удалённого узла в ранних данных (RFC 9001 §4.6.2)
				 *
				 * @note Отказ отказом хендшейка не является: он лишь означает, что
				 *       отправленные ранние данные потеряны и подлежат повторной
				 *       отправке после завершения хендшейка
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking a refusal of the early data by the remote node (RFC 9001 §4.6.2)
				 * @note A refusal is not a refusal of the handshake: it only means that
				 *       the sent early data has been lost and is subject to a repeated
				 *       sending after the completion of the handshake
				 * @return result of the check
				 *
				 * \~
				 */
				bool rejected() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки локальных транспортных параметров (RFC 9000 §7.4)
				 *
				 * @note Вызывается до начала хендшейка. Параметры сериализуются
				 *       и передаются в расширении quic_transport_parameters
				 *
				 * @param params локальные транспортные параметры
				 * @return       результат установки (false - ошибка сериализации)
				 *
				 * \~english
				 * @brief Method of setting the local transport parameters (RFC 9000 §7.4)
				 * @note Called before the beginning of the handshake. The parameters are serialized
				 *       and transmitted in the quic_transport_parameters extension
				 * @param params local transport parameters
				 * @return       result of the setting (false — an error of the serialization)
				 *
				 * \~
				 */
				bool params(const quic::params::params_t & params) noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения транспортных параметров удалённого узла (RFC 9000 §7.4)
				 *
				 * @note Параметры доступны после обработки ClientHello сервером
				 *       либо EncryptedExtensions клиентом
				 *
				 * @param params транспортные параметры удалённого узла
				 * @param error  код ошибки транспорта
				 * @return       результат извлечения (OK/INCOMPLETE/ERROR)
				 *
				 * \~english
				 * @brief Method of extracting the transport parameters of the remote node (RFC 9000 §7.4)
				 * @note The parameters are available after the processing of the ClientHello by the server
				 *       or of the EncryptedExtensions by the client
				 * @param params transport parameters of the remote node
				 * @param error  transport error code
				 * @return       result of the extraction (OK/INCOMPLETE/ERROR)
				 *
				 * \~
				 */
				status_t peer(quic::params::params_t & params, error_t & error) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод вывода ключей уровня Initial (RFC 9001 §5.2)
				 *
				 * @note Направления чтения/записи назначаются согласно роли эндпоинта:
				 *       клиент пишет клиентскими ключами и читает серверными, сервер - наоборот
				 *
				 * @param dcid идентификатор соединения получателя первого пакета Initial клиента
				 * @return     результат вывода (false - ошибка криптографической библиотеки)
				 *
				 * \~english
				 * @brief Method of the derivation of the keys of the Initial level (RFC 9001 §5.2)
				 * @note The directions of the reading/writing are assigned according to the role of the endpoint:
				 *       the client writes with the client keys and reads with the server ones, the server — the other way round
				 * @param dcid connection identifier of the recipient of the first Initial packet of the client
				 * @return     result of the derivation (false — an error of the cryptographic library)
				 *
				 * \~
				 */
				bool initial(const cid_t & dcid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод начала хендшейка
				 *
				 * @note Все настройки (ALPN, SNI, параметры, сертификат) должны быть
				 *       установлены до вызова. Клиент формирует ClientHello - исходящие
				 *       CRYPTO-данные появятся на уровне INITIAL. Сервер только
				 *       инициализирует TLS-стек и ожидает данных через crypto()
				 *
				 * @return результат начала хендшейка (OK/ERROR)
				 *
				 * \~english
				 * @brief Method of beginning the handshake
				 * @note All the settings (the ALPN, the SNI, the parameters, the certificate) must be
				 *       set before the call. The client forms the ClientHello — the outgoing
				 *       CRYPTO data will appear at the INITIAL level. The server only
				 *       initializes the TLS stack and waits for the data through crypto()
				 * @return result of the beginning of the handshake (OK/ERROR)
				 *
				 * \~
				 */
				status_t start() noexcept;
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Method of processing the incoming data of the CRYPTO frames
				 * @note The data must be passed in the order of the offsets of the CRYPTO stream of the level
				 *       (the assembly by the offsets is performed by the calling code). After the completion of the
				 *       handshake it processes the post-handshake messages (NewSessionTicket)
				 * @param level encryption level of the packet with the CRYPTO frame
				 * @param data  data of the CRYPTO frame
				 * @param size  size of the data of the CRYPTO frame
				 * @return      result of the processing (OK/ERROR)
				 *
				 * \~
				 */
				status_t crypto(const level_t level, const uint8_t * data, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки наличия исходящих CRYPTO-данных уровня
				 *
				 * @param level уровень шифрования
				 * @return      результат проверки (true - есть данные для отправки)
				 *
				 * \~english
				 * @brief Method of checking the presence of the outgoing CRYPTO data of a level
				 * @param level encryption level
				 * @return      result of the check (true — there is data to be sent)
				 *
				 * \~
				 */
				bool pending(const level_t level) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения исходящих CRYPTO-данных уровня
				 *
				 * @note Данные передаются вызывающему коду и удаляются из очереди уровня.
				 *       Вызывающий код упаковывает их в CRYPTO-фреймы, отслеживая смещения
				 *
				 * @param level уровень шифрования
				 * @return      исходящие CRYPTO-данные уровня
				 *
				 * \~english
				 * @brief Method of extracting the outgoing CRYPTO data of a level
				 * @note The data is passed to the calling code and is removed from the queue of the level.
				 *       The calling code packs it into the CRYPTO frames, tracking the offsets
				 * @param level encryption level
				 * @return      outgoing CRYPTO data of the level
				 *
				 * \~
				 */
				string data(const level_t level) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения ключей защиты исходящих пакетов уровня
				 *
				 * @param level уровень шифрования
				 * @return      ключи защиты пакетов либо nullptr если ключи ещё не выведены
				 *
				 * \~english
				 * @brief Method of extracting the keys of the protection of the outgoing packets of a level
				 * @param level encryption level
				 * @return      keys of the protection of the packets or nullptr if the keys have not been derived yet
				 *
				 * \~
				 */
				const crypto::keys_t * encryption(const level_t level) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения ключей снятия защиты входящих пакетов уровня
				 *
				 * @param level уровень шифрования
				 * @return      ключи защиты пакетов либо nullptr если ключи ещё не выведены
				 *
				 * \~english
				 * @brief Method of extracting the keys of the removal of the protection of the incoming packets of a level
				 * @param level encryption level
				 * @return      keys of the protection of the packets or nullptr if the keys have not been derived yet
				 *
				 * \~
				 */
				const crypto::keys_t * decryption(const level_t level) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод замены ключей уровня шифрования (RFC 9001 §6)
				 *
				 * @note Используется слоем соединения при обновлении ключей (key update):
				 *       ключи уровня APPLICATION заменяются ключами следующей фазы
				 *
				 * @param level уровень шифрования
				 * @param read  ключи снятия защиты входящих пакетов уровня
				 * @param write ключи защиты исходящих пакетов уровня
				 *
				 * \~english
				 * @brief Method of replacing the keys of an encryption level (RFC 9001 §6)
				 * @note Used by the layer of the connection at a key update:
				 *       the keys of the APPLICATION level are replaced by the keys of the next phase
				 * @param level encryption level
				 * @param read  keys of the removal of the protection of the incoming packets of the level
				 * @param write keys of the protection of the outgoing packets of the level
				 *
				 * \~
				 */
				void install(const level_t level, const crypto::keys_t & read, const crypto::keys_t & write) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод сброса ключей уровня шифрования (RFC 9001 §4.9)
				 *
				 * @note Ключи Initial сбрасываются после установки ключей Handshake,
				 *       ключи Handshake - после подтверждения хендшейка
				 *
				 * @param level уровень шифрования
				 *
				 * \~english
				 * @brief Method of discarding the keys of an encryption level (RFC 9001 §4.9)
				 * @note The Initial keys are discarded after the setting of the Handshake keys,
				 *       the Handshake keys — after the confirmation of the handshake
				 * @param level encryption level
				 *
				 * \~
				 */
				void discard(const level_t level) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения состояния хендшейка
				 *
				 * @return состояние хендшейка
				 *
				 * \~english
				 * @brief Method of getting the state of the handshake
				 * @return state of the handshake
				 *
				 * \~
				 */
				state_t state() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения кода ошибки транспорта (RFC 9001 §4.8)
				 *
				 * @note При фатальном TLS-алерте возвращает CRYPTO_ERROR + код алерта,
				 *       при иных ошибках хендшейка - INTERNAL_ERROR
				 *
				 * @return код ошибки транспорта (NO_ERROR - ошибки нет)
				 *
				 * \~english
				 * @brief Method of getting the transport error code (RFC 9001 §4.8)
				 * @note At a fatal TLS alert it returns CRYPTO_ERROR + the code of the alert,
				 *       at the other errors of the handshake — INTERNAL_ERROR
				 * @return transport error code (NO_ERROR — there is no error)
				 *
				 * \~
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
				 * \~russian
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
				 *
				 * \~english
				 * @brief Constructor
				 * @note The TLS context is obligatory: the whole configuration of the cryptography is performed
				 *       on it by the means of the transport security module, therefore
				 *       the handshake has no settings of its own. The context is obliged to
				 *       outlive the object of the handshake
				 * @param endpoint role of the local endpoint on the connection
				 * @param ctx      identifier of the template of the security context
				 * @param coder    object of the coder of the transport security
				 * @param log      object for working with logs
				 *
				 * \~
				 */
				explicit Handshake(const endpoint_t endpoint, const tls::coder_t::id_t ctx, const tls::coder_t & coder, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Handshake() noexcept;
		} handshake_t;
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/pop.hpp"

#endif // __AWH_PROTO_QUIC_HANDSHAKE__
