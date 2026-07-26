/**
 * @file: handshake.cpp
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
 * Заголовочные файлы BoringSSL
 */
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/quic/handshake.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутреннее пространство имён вспомогательных функций
 *
 */
namespace {
	/**
	 * @brief Функция преобразования уровня шифрования BoringSSL во внутренний уровень
	 *
	 * @param level уровень шифрования BoringSSL
	 * @return      внутренний уровень шифрования
	 */
	static awh::quic::level_t levelFromSSL(const enum ssl_encryption_level_t level) noexcept {
		/**
		 * Определяем уровень шифрования BoringSSL
		 */
		switch(level){
			// Уровень Initial
			case ssl_encryption_initial:
				// Выводим внутренний уровень шифрования
				return awh::quic::level_t::INITIAL;
			// Уровень ранних данных 0-RTT
			case ssl_encryption_early_data:
				// Выводим внутренний уровень шифрования
				return awh::quic::level_t::EARLY_DATA;
			// Уровень хендшейка
			case ssl_encryption_handshake:
				// Выводим внутренний уровень шифрования
				return awh::quic::level_t::HANDSHAKE;
			// Уровень приложения 1-RTT
			case ssl_encryption_application:
				// Выводим внутренний уровень шифрования
				return awh::quic::level_t::APPLICATION;
		}
		// Выводим уровень Initial по умолчанию
		return awh::quic::level_t::INITIAL;
	}
	/**
	 * @brief Функция преобразования внутреннего уровня шифрования в уровень BoringSSL
	 *
	 * @param level внутренний уровень шифрования
	 * @return      уровень шифрования BoringSSL
	 */
	static enum ssl_encryption_level_t levelToSSL(const awh::quic::level_t level) noexcept {
		/**
		 * Определяем внутренний уровень шифрования
		 */
		switch(level){
			// Уровень Initial
			case awh::quic::level_t::INITIAL:
				// Выводим уровень шифрования BoringSSL
				return ssl_encryption_initial;
			// Уровень ранних данных 0-RTT
			case awh::quic::level_t::EARLY_DATA:
				// Выводим уровень шифрования BoringSSL
				return ssl_encryption_early_data;
			// Уровень хендшейка
			case awh::quic::level_t::HANDSHAKE:
				// Выводим уровень шифрования BoringSSL
				return ssl_encryption_handshake;
			// Уровень приложения 1-RTT
			case awh::quic::level_t::APPLICATION:
				// Выводим уровень шифрования BoringSSL
				return ssl_encryption_application;
		}
		// Выводим уровень Initial по умолчанию
		return ssl_encryption_initial;
	}
	/**
	 * @brief Функция определения криптографического набора по шифру TLS (RFC 9001 §5.1)
	 *
	 * @param cipher шифр согласованный TLS-стеком
	 * @param suite  определённый криптографический набор
	 * @return       результат определения (false - шифр не поддерживается QUIC)
	 */
	static bool suiteFromCipher(const SSL_CIPHER * cipher, awh::quic::crypto::suite_t & suite) noexcept {
		/**
		 * Определяем двухбайтовый идентификатор шифра IANA
		 */
		switch(::SSL_CIPHER_get_protocol_id(cipher)){
			// Шифр TLS_AES_128_GCM_SHA256
			case 0x1301: {
				// Устанавливаем криптографический набор
				suite = awh::quic::crypto::suite_t::AES_128_GCM_SHA256;
				// Выводим положительный результат
				return true;
			}
			// Шифр TLS_AES_256_GCM_SHA384
			case 0x1302: {
				// Устанавливаем криптографический набор
				suite = awh::quic::crypto::suite_t::AES_256_GCM_SHA384;
				// Выводим положительный результат
				return true;
			}
			// Шифр TLS_CHACHA20_POLY1305_SHA256
			case 0x1303: {
				// Устанавливаем криптографический набор
				suite = awh::quic::crypto::suite_t::CHACHA20_POLY1305_SHA256;
				// Выводим положительный результат
				return true;
			}
		}
		// Выводим отрицательный результат (TLS 1.3 для QUIC допускает только AEAD-шифры выше)
		return false;
	}
};

/**
 * @brief Пространство имён обратных вызовов BoringSSL QUIC API
 *
 */
namespace {
	/**
	 * @brief Функция установки секрета чтения уровня шифрования
	 *
	 * @param ssl    объект TLS-соединения
	 * @param level  уровень шифрования BoringSSL
	 * @param cipher шифр согласованный TLS-стеком
	 * @param secret секрет направления чтения
	 * @param size   размер секрета
	 * @return       результат установки (0 - завершить хендшейк с ошибкой)
	 */
	static int32_t setReadSecret(SSL * ssl, enum ssl_encryption_level_t level, const SSL_CIPHER * cipher, const uint8_t * secret, size_t size) noexcept;
	/**
	 * @brief Функция установки секрета записи уровня шифрования
	 *
	 * @param ssl    объект TLS-соединения
	 * @param level  уровень шифрования BoringSSL
	 * @param cipher шифр согласованный TLS-стеком
	 * @param secret секрет направления записи
	 * @param size   размер секрета
	 * @return       результат установки (0 - завершить хендшейк с ошибкой)
	 */
	static int32_t setWriteSecret(SSL * ssl, enum ssl_encryption_level_t level, const SSL_CIPHER * cipher, const uint8_t * secret, size_t size) noexcept;
	/**
	 * @brief Функция добавления исходящих данных хендшейка на уровне шифрования
	 *
	 * @param ssl   объект TLS-соединения
	 * @param level уровень шифрования BoringSSL
	 * @param data  данные хендшейка
	 * @param size  размер данных хендшейка
	 * @return      результат добавления (0 - завершить хендшейк с ошибкой)
	 */
	static int32_t addHandshakeData(SSL * ssl, enum ssl_encryption_level_t level, const uint8_t * data, size_t size) noexcept;
	/**
	 * @brief Функция завершения формирования полётной порции данных хендшейка
	 *
	 * @param ssl объект TLS-соединения
	 * @return    результат завершения (0 - завершить хендшейк с ошибкой)
	 */
	static int32_t flushFlight(SSL * ssl) noexcept;
	/**
	 * @brief Функция отправки фатального TLS-алерта (RFC 9001 §4.8)
	 *
	 * @param ssl   объект TLS-соединения
	 * @param level уровень шифрования BoringSSL
	 * @param alert код TLS-алерта
	 * @return      результат отправки (0 - завершить хендшейк с ошибкой)
	 */
	static int32_t sendAlert(SSL * ssl, enum ssl_encryption_level_t level, uint8_t alert) noexcept;
	/**
	 * Таблица обратных вызовов BoringSSL QUIC API (SSL_QUIC_METHOD)
	 */
	static const SSL_QUIC_METHOD quicMethod = {
		&setReadSecret,
		&setWriteSecret,
		&addHandshakeData,
		&flushFlight,
		&sendAlert
	};
};

/**
 * @brief Класс доступа обратных вызовов BoringSSL к приватному состоянию хендшейка
 *
 */
class awh::quic::HandshakeHook {
	public:
		/**
		 * @brief Метод сохранения билета возобновления сессии
		 *
		 * @param handshake объект хендшейка
		 * @param session   сессия возобновления
		 */
		static void ticket(handshake_t * handshake, SSL_SESSION * session) noexcept {
			// Данные сериализованной сессии
			uint8_t * data = nullptr;
			// Размер сериализованной сессии
			size_t size = 0;
			// Если сериализация сессии возобновления не выполнена
			if(::SSL_SESSION_to_bytes(session, &data, &size) != 1)
				// Выходим из метода
				return;
			// Сохраняем сериализованный билет возобновления
			handshake->_ticket.assign(reinterpret_cast <const char *> (data), size);
			// Освобождаем буфер сериализованной сессии
			::OPENSSL_free(data);
		}
	public:
		/**
		 * @brief Метод установки секрета уровня шифрования
		 *
		 * @param handshake объект хендшейка
		 * @param level     уровень шифрования
		 * @param write     флаг направления записи (false - направление чтения)
		 * @param cipher    шифр согласованный TLS-стеком
		 * @param secret    секрет направления
		 * @param size      размер секрета
		 * @return          результат установки
		 */
		static bool secret(handshake_t * handshake, const level_t level, const bool write, const SSL_CIPHER * cipher, const uint8_t * secret, const size_t size) noexcept {
			// Криптографический набор защиты пакетов
			crypto::suite_t suite = crypto::suite_t::AES_128_GCM_SHA256;
			// Если шифр не поддерживается QUIC, выводим отрицательный результат
			if(!::suiteFromCipher(cipher, suite))
				// Выводим отрицательный результат
				return false;
			// Получаем состояние уровня шифрования
			auto & item = handshake->_levels[static_cast <size_t> (level)];
			// Определяем ключи направления
			auto & keys = (write ? item.write : item.read);
			// Устанавливаем криптографический набор
			keys.suite = suite;
			// Устанавливаем секрет направления
			keys.secret.assign(reinterpret_cast <const char *> (secret), size);
			// Выводим ключи защиты пакетов из секрета
			if(!crypto::derive(keys))
				// Выводим отрицательный результат
				return false;
			// Если установлены ключи направления записи
			if(write)
				// Устанавливаем флаг наличия ключей записи
				item.hasWrite = true;
			// Если установлены ключи направления чтения
			else item.hasRead = true;
			// Выводим положительный результат
			return true;
		}
		/**
		 * @brief Метод добавления исходящих CRYPTO-данных уровня шифрования
		 *
		 * @param handshake объект хендшейка
		 * @param level     уровень шифрования
		 * @param data      данные хендшейка
		 * @param size      размер данных хендшейка
		 */
		static void data(handshake_t * handshake, const level_t level, const uint8_t * data, const size_t size) noexcept {
			// Добавляем данные в очередь исходящих CRYPTO-данных уровня
			handshake->_levels[static_cast <size_t> (level)].data.append(reinterpret_cast <const char *> (data), size);
		}
		/**
		 * @brief Метод регистрации фатального TLS-алерта
		 *
		 * @param handshake объект хендшейка
		 * @param alert     код TLS-алерта
		 */
		static void alert(handshake_t * handshake, const uint8_t alert) noexcept {
			// Устанавливаем код фатального TLS-алерта
			handshake->_alert = alert;
			// Устанавливаем флаг получения фатального TLS-алерта
			handshake->_hasAlert = true;
		}
};

/**
 * @brief Реализация обратных вызовов BoringSSL QUIC API
 *
 */
namespace {
	/**
	 * @brief Функция приёма билета возобновления сессии (RFC 9001 §4.6)
	 *
	 * @note Билет присылается сервером уже после завершения хендшейка отдельным
	 *       сообщением, поэтому доступным он становится не сразу и выдаётся
	 *       криптографической библиотекой этим обратным вызовом
	 *
	 * @param ssl     объект TLS-соединения
	 * @param session сессия возобновления
	 * @return        признак принятия владения сессией
	 */
	static int32_t newSession(SSL * ssl, SSL_SESSION * session) noexcept {
		// Извлекаем объект хендшейка из обратного указателя TLS-соединения
		auto handshake = reinterpret_cast <awh::quic::handshake_t *> (SSL_get_app_data(ssl));
		// Если объект хендшейка либо сессия не получены
		if((handshake == nullptr) || (session == nullptr))
			// Выводим отказ от владения сессией
			return 0;
		// Сохраняем билет возобновления в объекте хендшейка
		awh::quic::HandshakeHook::ticket(handshake, session);
		// Выводим отказ от владения сессией - её копия сохранена
		return 0;
	}
	/**
	 * @brief Функция установки секрета чтения уровня шифрования
	 *
	 * @param ssl    объект TLS-соединения
	 * @param level  уровень шифрования BoringSSL
	 * @param cipher шифр согласованный TLS-стеком
	 * @param secret секрет направления чтения
	 * @param size   размер секрета
	 * @return       результат установки (0 - завершить хендшейк с ошибкой)
	 */
	static int32_t setReadSecret(SSL * ssl, enum ssl_encryption_level_t level, const SSL_CIPHER * cipher, const uint8_t * secret, size_t size) noexcept {
		// Получаем объект хендшейка
		awh::quic::handshake_t * handshake = reinterpret_cast <awh::quic::handshake_t *> (SSL_get_app_data(ssl));
		// Устанавливаем секрет направления чтения и выводим результат
		return (awh::quic::HandshakeHook::secret(handshake, ::levelFromSSL(level), false, cipher, secret, size) ? 1 : 0);
	}
	/**
	 * @brief Функция установки секрета записи уровня шифрования
	 *
	 * @param ssl    объект TLS-соединения
	 * @param level  уровень шифрования BoringSSL
	 * @param cipher шифр согласованный TLS-стеком
	 * @param secret секрет направления записи
	 * @param size   размер секрета
	 * @return       результат установки (0 - завершить хендшейк с ошибкой)
	 */
	static int32_t setWriteSecret(SSL * ssl, enum ssl_encryption_level_t level, const SSL_CIPHER * cipher, const uint8_t * secret, size_t size) noexcept {
		// Получаем объект хендшейка
		awh::quic::handshake_t * handshake = reinterpret_cast <awh::quic::handshake_t *> (SSL_get_app_data(ssl));
		// Устанавливаем секрет направления записи и выводим результат
		return (awh::quic::HandshakeHook::secret(handshake, ::levelFromSSL(level), true, cipher, secret, size) ? 1 : 0);
	}
	/**
	 * @brief Функция добавления исходящих данных хендшейка на уровне шифрования
	 *
	 * @param ssl   объект TLS-соединения
	 * @param level уровень шифрования BoringSSL
	 * @param data  данные хендшейка
	 * @param size  размер данных хендшейка
	 * @return      результат добавления (0 - завершить хендшейк с ошибкой)
	 */
	static int32_t addHandshakeData(SSL * ssl, enum ssl_encryption_level_t level, const uint8_t * data, size_t size) noexcept {
		// Получаем объект хендшейка
		awh::quic::handshake_t * handshake = reinterpret_cast <awh::quic::handshake_t *> (SSL_get_app_data(ssl));
		// Добавляем данные в очередь исходящих CRYPTO-данных уровня
		awh::quic::HandshakeHook::data(handshake, ::levelFromSSL(level), data, size);
		// Выводим положительный результат
		return 1;
	}
	/**
	 * @brief Функция завершения формирования полётной порции данных хендшейка
	 *
	 * @param ssl объект TLS-соединения
	 * @return    результат завершения (0 - завершить хендшейк с ошибкой)
	 */
	static int32_t flushFlight([[maybe_unused]] SSL * ssl) noexcept {
		// Данные уже накоплены по уровням - упаковку в пакеты выполняет вызывающий код
		return 1;
	}
	/**
	 * @brief Функция отправки фатального TLS-алерта (RFC 9001 §4.8)
	 *
	 * @param ssl   объект TLS-соединения
	 * @param level уровень шифрования BoringSSL
	 * @param alert код TLS-алерта
	 * @return      результат отправки (0 - завершить хендшейк с ошибкой)
	 */
	static int32_t sendAlert(SSL * ssl, [[maybe_unused]] enum ssl_encryption_level_t level, uint8_t alert) noexcept {
		// Получаем объект хендшейка
		awh::quic::handshake_t * handshake = reinterpret_cast <awh::quic::handshake_t *> (SSL_get_app_data(ssl));
		// Регистрируем фатальный TLS-алерт
		awh::quic::HandshakeHook::alert(handshake, alert);
		// Выводим положительный результат
		return 1;
	}
};

/**
 * @brief Конструктор состояния уровня шифрования
 *
 */
awh::quic::Handshake::Level::Level() noexcept : hasRead(false), hasWrite(false), data{""} {}

/**
 * @brief Метод продвижения TLS-хендшейка
 *
 * @return результат продвижения (OK - хендшейк продолжается или завершён)
 */
awh::quic::status_t awh::quic::Handshake::process() noexcept {
	// Выполняем шаг TLS-хендшейка
	const int32_t result = ::SSL_do_handshake(this->_ssl);
	// Если шаг хендшейка выполнен успешно
	if(result == 1){
		/**
		 * Если хендшейк приостановлен для отправки ранних данных: криптографическая
		 * библиотека возвращает успех, не завершив хендшейк, чтобы вызывающий код
		 * успел отправить ранние данные выданными ключами. Завершённым такой
		 * хендшейк считать нельзя - он продолжится с приходом ответа сервера
		 */
		if(::SSL_in_early_data(this->_ssl) == 1)
			// Выводим положительный результат - хендшейк продолжается
			return status_t::OK;
		// Устанавливаем состояние успешного завершения хендшейка
		this->_state = state_t::COMPLETED;
		// Выводим положительный результат
		return status_t::OK;
	}
	// Получаем код ошибки шага хендшейка
	int32_t code = ::SSL_get_error(this->_ssl, result);
	/**
	 * Если удалённый узел отказал в ранних данных: отказ отказом хендшейка не
	 * является - отправленные ранние данные потеряны и подлежат повторной
	 * отправке, а сам хендшейк продолжается обычным порядком (RFC 9001 §4.6.2)
	 */
	if(code == SSL_ERROR_EARLY_DATA_REJECTED){
		// Устанавливаем флаг отказа удалённого узла в ранних данных
		this->_rejected = true;
		// Сбрасываем состояние ранних данных для продолжения хендшейка
		::SSL_reset_early_data_reject(this->_ssl);
		// Выполняем повторный шаг TLS-хендшейка
		const int32_t repeat = ::SSL_do_handshake(this->_ssl);
		// Если повторный шаг хендшейка выполнен успешно
		if(repeat == 1){
			// Если хендшейк приостановлен для отправки ранних данных
			if(::SSL_in_early_data(this->_ssl) == 1)
				// Выводим положительный результат - хендшейк продолжается
				return status_t::OK;
			// Устанавливаем состояние успешного завершения хендшейка
			this->_state = state_t::COMPLETED;
			// Выводим положительный результат
			return status_t::OK;
		}
		// Получаем код ошибки повторного шага хендшейка
		code = ::SSL_get_error(this->_ssl, repeat);
	}
	/**
	 * Если хендшейк ожидает данных удалённого узла либо завершения асинхронной
	 * операции (подпись закрытым ключом, проверка сертификата, поиск X509): это не
	 * отказ, а приостановка - шаг повторяется по готовности. Асинхронные коды
	 * BoringSSL берём под условной компиляцией для совместимости со сборками, где
	 * они не объявлены (RFC 9001 §4.1.1)
	 */
	if((code == SSL_ERROR_WANT_READ) || (code == SSL_ERROR_WANT_X509_LOOKUP)
#ifdef SSL_ERROR_WANT_PRIVATE_KEY_OPERATION
	   || (code == SSL_ERROR_WANT_PRIVATE_KEY_OPERATION)
#endif
#ifdef SSL_ERROR_WANT_CERTIFICATE_VERIFY
	   || (code == SSL_ERROR_WANT_CERTIFICATE_VERIFY)
#endif
	)
		// Выводим положительный результат - хендшейк продолжается
		return status_t::OK;
	// Устанавливаем состояние ошибки хендшейка
	this->_state = state_t::FAILED;
	/**
	 * Извлекаем причину отказа из очереди ошибок криптографической библиотеки:
	 * без неё отказ хендшейка неотличим от любого другого и неразбираем
	 */
	const uint32_t reason = ::ERR_peek_last_error();
	// Буфер текста ошибки криптографической библиотеки
	char buffer[256];
	// Обнуляем буфер текста ошибки
	buffer[0] = '\0';
	// Если ошибка криптографической библиотеки зарегистрирована
	if(reason != 0)
		// Извлекаем текст ошибки криптографической библиотеки
		::ERR_error_string_n(reason, buffer, sizeof(buffer));
	// Записываем ошибку в лог
	this->_log->print(
		"QUIC TLS handshake failed: code=%d%s%s", log_t::flag_t::CRITICAL,
		code, (buffer[0] != '\0' ? ", reason: " : ""), buffer
	);
	// Выводим отрицательный результат
	return status_t::ERROR;
}
/**
 * @brief Метод извлечения согласованного ALPN-протокола
 *
 * @return согласованный ALPN-протокол (пустое название - согласование не выполнено)
 */
awh::tls::coder_t::alpn_t awh::quic::Handshake::alpn() const noexcept {
	// Согласованный ALPN-протокол
	tls::coder_t::alpn_t result;
	// Если TLS-соединение не создано
	if(this->_ssl == nullptr)
		// Выводим пустой результат
		return result;
	// Данные согласованного ALPN-протокола
	const uint8_t * data = nullptr;
	// Размер согласованного ALPN-протокола
	uint32_t size = 0;
	// Извлекаем согласованный ALPN-протокол
	::SSL_get0_alpn_selected(this->_ssl, &data, &size);
	// Если ALPN-протокол не согласован
	if((data == nullptr) || (size == 0))
		// Выводим пустой результат
		return result;
	// Устанавливаем название согласованного протокола
	result.protocol.assign(reinterpret_cast <const char *> (data), static_cast <size_t> (size));
	/**
	 * Восстанавливаем идентификатор протокола по списку контекста: согласование
	 * возвращает только название, а идентификатор назначается вызывающим кодом
	 * при настройке списка поддерживаемых протоколов
	 */
	for(auto & protocol : this->_protocols){
		// Если название протокола совпадает с согласованным
		if(protocol.protocol == result.protocol){
			// Устанавливаем идентификатор согласованного протокола
			result.id = protocol.id;
			// Прекращаем поиск
			break;
		}
	}
	// Выводим согласованный ALPN-протокол
	return result;
}
/**
 * @brief Метод извлечения возобновляемой сессии хендшейка (RFC 9001 §4.6)
 *
 * @return сериализованная сессия (пусто - сессия недоступна)
 */
string awh::quic::Handshake::session() const noexcept {
	// Выводим принятый от удалённого узла билет возобновления
	return this->_ticket;
}
/**
 * @brief Метод установки возобновляемой сессии хендшейка (RFC 9001 §4.6)
 *
 * @param session сериализованная сессия
 * @return        результат установки
 */
bool awh::quic::Handshake::session(string_view session) noexcept {
	// Если хендшейк уже начат либо эндпоинт не является клиентом
	if((this->_state != state_t::NONE) || (this->_endpoint != endpoint_t::CLIENT))
		// Выводим отрицательный результат
		return false;
	// Устанавливаем сериализованную сессию возобновления
	this->_session.assign(session.begin(), session.end());
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод проверки принятия ранних данных удалённым узлом
 *
 * @return результат проверки
 */
bool awh::quic::Handshake::early() const noexcept {
	// Если TLS-соединение не создано либо в ранних данных отказано
	if((this->_ssl == nullptr) || this->_rejected)
		// Выводим отрицательный результат
		return false;
	// Выводим результат принятия ранних данных удалённым узлом
	return (::SSL_early_data_accepted(this->_ssl) == 1);
}
/**
 * @brief Метод проверки отказа удалённого узла в ранних данных (RFC 9001 §4.6.2)
 *
 * @return результат проверки
 */
bool awh::quic::Handshake::rejected() const noexcept {
	// Выводим флаг отказа удалённого узла в ранних данных
	return this->_rejected;
}
/**
 * @brief Метод установки локальных транспортных параметров (RFC 9000 §7.4)
 *
 * @param params локальные транспортные параметры
 * @return       результат установки (false - ошибка сериализации)
 */
bool awh::quic::Handshake::params(const quic::params::params_t & params) noexcept {
	// Очищаем сериализованные локальные транспортные параметры
	this->_params.clear();
	// Очищаем контекст ранних данных
	this->_early.clear();
	// Если сериализация транспортных параметров не выполнена
	if(!quic::params::serialize::encode(this->_params, params, this->_endpoint))
		// Выводим отрицательный результат
		return false;
	// Выполняем сборку контекста ранних данных и выводим результат
	return quic::params::serialize::early(this->_early, params);
}
/**
 * @brief Метод извлечения транспортных параметров удалённого узла (RFC 9000 §7.4)
 *
 * @param params транспортные параметры удалённого узла
 * @param error  код ошибки транспорта
 * @return       результат извлечения (OK/INCOMPLETE/ERROR)
 */
awh::quic::status_t awh::quic::Handshake::peer(quic::params::params_t & params, error_t & error) const noexcept {
	// Если TLS-соединение не создано, выводим результат отсутствия данных
	if(this->_ssl == nullptr)
		// Выводим результат отсутствия данных
		return status_t::INCOMPLETE;
	// Данные транспортных параметров удалённого узла
	const uint8_t * data = nullptr;
	// Размер транспортных параметров удалённого узла
	size_t size = 0;
	// Извлекаем транспортные параметры удалённого узла
	::SSL_get_peer_quic_transport_params(this->_ssl, &data, &size);
	// Если транспортные параметры ещё не получены
	if(size == 0)
		// Выводим результат отсутствия данных
		return status_t::INCOMPLETE;
	// Определяем роль удалённого узла (противоположна локальной)
	const endpoint_t sender = ((this->_endpoint == endpoint_t::CLIENT) ? endpoint_t::SERVER : endpoint_t::CLIENT);
	// Выполняем разбор транспортных параметров удалённого узла и выводим результат
	return quic::params::parser::decode(data, size, sender, params, error);
}
/**
 * @brief Метод вывода ключей уровня Initial (RFC 9001 §5.2)
 *
 * @param dcid идентификатор соединения получателя первого пакета Initial клиента
 * @return     результат вывода (false - ошибка криптографической библиотеки)
 */
bool awh::quic::Handshake::initial(const cid_t & dcid) noexcept {
	// Ключи защиты пакетов клиента
	crypto::keys_t client;
	// Ключи защиты пакетов сервера
	crypto::keys_t server;
	// Выполняем вывод ключей Initial обоих направлений
	if(!crypto::initial(dcid, client, server))
		// Выводим отрицательный результат
		return false;
	// Получаем состояние уровня шифрования Initial
	auto & item = this->_levels[static_cast <size_t> (level_t::INITIAL)];
	// Если локальный эндпоинт является клиентом
	if(this->_endpoint == endpoint_t::CLIENT){
		// Устанавливаем клиентские ключи направлением записи
		item.write = ::move(client);
		// Устанавливаем серверные ключи направлением чтения
		item.read = ::move(server);
	// Если локальный эндпоинт является сервером
	} else {
		// Устанавливаем серверные ключи направлением записи
		item.write = ::move(server);
		// Устанавливаем клиентские ключи направлением чтения
		item.read = ::move(client);
	}
	// Устанавливаем флаг наличия ключей чтения
	item.hasRead = true;
	// Устанавливаем флаг наличия ключей записи
	item.hasWrite = true;
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод начала хендшейка
 *
 * @return результат начала хендшейка (OK/ERROR)
 */
awh::quic::status_t awh::quic::Handshake::start() noexcept {
	// Если хендшейк уже начат либо не установлены транспортные параметры
	if((this->_state != state_t::NONE) || this->_params.empty()){
		// Записываем ошибку в лог
		this->_log->print("QUIC TLS start rejected: %s", log_t::flag_t::CRITICAL,
		 (this->_state != state_t::NONE ? "handshake is already started" : "transport parameters are not set"));
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Если контекст TLS не задан
	if(this->_ctx == nullptr){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Записываем ошибку в лог
		this->_log->print("QUIC TLS context is not set", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Создаём TLS-соединение
	this->_ssl = ::SSL_new(this->_ctx);
	// Если TLS-соединение не создано
	if(this->_ssl == nullptr){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Записываем ошибку в лог
		this->_log->print("QUIC TLS connection is not created", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Устанавливаем обратный указатель на объект хендшейка
	SSL_set_app_data(this->_ssl, this);
	// Устанавливаем таблицу обратных вызовов BoringSSL QUIC API
	if(::SSL_set_quic_method(this->_ssl, &::quicMethod) != 1){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Записываем ошибку в лог
		this->_log->print("QUIC TLS method is not applied", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Устанавливаем локальные транспортные параметры
	if(::SSL_set_quic_transport_params(this->_ssl, reinterpret_cast <const uint8_t *> (this->_params.data()), this->_params.size()) != 1){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Записываем ошибку в лог
		this->_log->print("QUIC transport parameters are not applied", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	/**
	 * Доменное имя удалённого узла и список ALPN-протоколов настроены на контексте
	 * кодера и наследуются созданным из него TLS-соединением: повторная настройка
	 * на уровне соединения не требуется
	 */
	if(this->_endpoint == endpoint_t::CLIENT)
		::SSL_set_connect_state(this->_ssl);
	// Если локальный эндпоинт является сервером, устанавливаем серверное состояние TLS-соединения
	else ::SSL_set_accept_state(this->_ssl);
	/**
	 * Привязываем ранние данные к локальным транспортным параметрам: билет
	 * возобновления действителен только при совпадении параметров, иначе клиент
	 * отправил бы ранние данные под лимиты, которых больше нет (RFC 9001 §4.6.1)
	 */
	if(::SSL_set_quic_early_data_context(this->_ssl, reinterpret_cast <const uint8_t *> (this->_early.data()), this->_early.size()) != 1){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Записываем ошибку в лог
		this->_log->print("QUIC early data context is not applied", log_t::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Разрешаем ранние данные соединению
	::SSL_set_early_data_enabled(this->_ssl, 1);
	/**
	 * Подключаем приём билетов возобновления: билет присылается сервером после
	 * завершения хендшейка и выдаётся обратным вызовом уровня контекста, поэтому
	 * он и регистрируется здесь. Внутреннее хранилище сессий не используется -
	 * владение билетом остаётся за вызывающим кодом. Режим кэша и обратный вызов
	 * приёма билета - чисто клиентская механика (SSL_SESS_CACHE_CLIENT), поэтому на
	 * сервере они не настраиваются: серверу они не нужны, а установка их на общий с
	 * прочими соединениями контекст кодера лишь мутировала бы его при каждом старте
	 */
	if(this->_endpoint == endpoint_t::CLIENT){
		// Включаем клиентский режим кэша сессий без внутреннего хранилища
		::SSL_CTX_set_session_cache_mode(this->_ctx, SSL_SESS_CACHE_CLIENT | SSL_SESS_CACHE_NO_INTERNAL_STORE);
		// Устанавливаем функцию обратного вызова приёма билета возобновления
		::SSL_CTX_sess_set_new_cb(this->_ctx, &::newSession);
	}
	// Если локальный эндпоинт является клиентом и сессия возобновления установлена
	if((this->_endpoint == endpoint_t::CLIENT) && !this->_session.empty()){
		// Восстанавливаем сессию возобновления из сериализованного представления
		SSL_SESSION * session = ::SSL_SESSION_from_bytes(reinterpret_cast <const uint8_t *> (this->_session.data()), this->_session.size(), this->_ctx);
		// Если сессия возобновления восстановлена
		if(session != nullptr){
			// Устанавливаем сессию возобновления соединению и проверяем результат установки
			if(::SSL_set_session(this->_ssl, session) != 1)
				// Записываем предупреждение в лог - сессия отклонена при установке, возобновление не состоится
				this->_log->print("QUIC session is not applied", log_t::flag_t::WARNING);
			// Освобождаем восстановленную сессию возобновления
			::SSL_SESSION_free(session);
		// Записываем предупреждение в лог - возобновление не состоится
		} else this->_log->print("QUIC session is not restored", log_t::flag_t::WARNING);
	}
	// Устанавливаем состояние выполнения хендшейка
	this->_state = state_t::PROCESS;
	// Если локальный эндпоинт является клиентом
	if(this->_endpoint == endpoint_t::CLIENT)
		// Выполняем шаг хендшейка для формирования ClientHello и выводим результат
		return this->process();
	// Выводим положительный результат - сервер ожидает данных клиента
	return status_t::OK;
}
/**
 * @brief Метод обработки входящих данных CRYPTO-фреймов
 *
 * @param level уровень шифрования пакета с CRYPTO-фреймом
 * @param data  данные CRYPTO-фрейма
 * @param size  размер данных CRYPTO-фрейма
 * @return      результат обработки (OK/ERROR)
 */
awh::quic::status_t awh::quic::Handshake::crypto(const level_t level, const uint8_t * data, const size_t size) noexcept {
	// Если хендшейк не начат либо завершился ошибкой
	if((this->_state == state_t::NONE) || (this->_state == state_t::FAILED))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Передаём данные CRYPTO-фрейма TLS-стеку
	if(::SSL_provide_quic_data(this->_ssl, ::levelToSSL(level), data, size) != 1){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Если хендшейк уже завершён
	if(this->_state == state_t::COMPLETED){
		// Обрабатываем post-handshake сообщения (NewSessionTicket и другие)
		if(::SSL_process_quic_post_handshake(this->_ssl) != 1){
			// Устанавливаем состояние ошибки хендшейка
			this->_state = state_t::FAILED;
			// Выводим отрицательный результат
			return status_t::ERROR;
		}
		// Выводим положительный результат
		return status_t::OK;
	}
	// Выполняем шаг хендшейка и выводим результат
	return this->process();
}
/**
 * @brief Метод проверки наличия исходящих CRYPTO-данных уровня
 *
 * @param level уровень шифрования
 * @return      результат проверки (true - есть данные для отправки)
 */
bool awh::quic::Handshake::pending(const level_t level) const noexcept {
	// Выводим результат проверки наличия исходящих CRYPTO-данных уровня
	return !this->_levels[static_cast <size_t> (level)].data.empty();
}
/**
 * @brief Метод извлечения исходящих CRYPTO-данных уровня
 *
 * @param level уровень шифрования
 * @return      исходящие CRYPTO-данные уровня
 */
string awh::quic::Handshake::data(const level_t level) noexcept {
	// Результат работы функции
	string result = "";
	// Извлекаем исходящие CRYPTO-данные уровня с очисткой очереди
	result.swap(this->_levels[static_cast <size_t> (level)].data);
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения ключей защиты исходящих пакетов уровня
 *
 * @param level уровень шифрования
 * @return      ключи защиты пакетов либо nullptr если ключи ещё не выведены
 */
const awh::quic::crypto::keys_t * awh::quic::Handshake::encryption(const level_t level) const noexcept {
	// Получаем состояние уровня шифрования
	const auto & item = this->_levels[static_cast <size_t> (level)];
	// Выводим ключи защиты исходящих пакетов уровня
	return (item.hasWrite ? &item.write : nullptr);
}
/**
 * @brief Метод извлечения ключей снятия защиты входящих пакетов уровня
 *
 * @param level уровень шифрования
 * @return      ключи защиты пакетов либо nullptr если ключи ещё не выведены
 */
const awh::quic::crypto::keys_t * awh::quic::Handshake::decryption(const level_t level) const noexcept {
	// Получаем состояние уровня шифрования
	const auto & item = this->_levels[static_cast <size_t> (level)];
	// Выводим ключи снятия защиты входящих пакетов уровня
	return (item.hasRead ? &item.read : nullptr);
}
/**
 * @brief Метод замены ключей уровня шифрования (RFC 9001 §6)
 *
 * @param level уровень шифрования
 * @param read  ключи снятия защиты входящих пакетов уровня
 * @param write ключи защиты исходящих пакетов уровня
 */
void awh::quic::Handshake::install(const level_t level, const crypto::keys_t & read, const crypto::keys_t & write) noexcept {
	// Получаем состояние уровня шифрования
	auto & item = this->_levels[static_cast <size_t> (level)];
	// Устанавливаем ключи снятия защиты входящих пакетов уровня
	item.read = read;
	// Устанавливаем ключи защиты исходящих пакетов уровня
	item.write = write;
	// Устанавливаем флаг наличия ключей чтения
	item.hasRead = true;
	// Устанавливаем флаг наличия ключей записи
	item.hasWrite = true;
}
/**
 * @brief Метод сброса ключей уровня шифрования (RFC 9001 §4.9)
 *
 * @param level уровень шифрования
 */
void awh::quic::Handshake::discard(const level_t level) noexcept {
	// Получаем состояние уровня шифрования
	auto & item = this->_levels[static_cast <size_t> (level)];
	// Сбрасываем флаг наличия ключей чтения
	item.hasRead = false;
	// Сбрасываем флаг наличия ключей записи
	item.hasWrite = false;
	// Очищаем ключи снятия защиты входящих пакетов
	item.read = crypto::keys_t();
	// Очищаем ключи защиты исходящих пакетов
	item.write = crypto::keys_t();
	// Очищаем очередь исходящих CRYPTO-данных уровня
	item.data.clear();
}
/**
 * @brief Метод получения состояния хендшейка
 *
 * @return состояние хендшейка
 */
awh::quic::Handshake::state_t awh::quic::Handshake::state() const noexcept {
	// Выводим состояние хендшейка
	return this->_state;
}
/**
 * @brief Метод получения кода ошибки транспорта (RFC 9001 §4.8)
 *
 * @return код ошибки транспорта (NO_ERROR - ошибки нет)
 */
awh::quic::error_t awh::quic::Handshake::error() const noexcept {
	// Если получен фатальный TLS-алерт
	if(this->_hasAlert)
		// Выводим код ошибки транспорта диапазона CRYPTO_ERROR (0x0100 + код алерта)
		return static_cast <error_t> (static_cast <uint64_t> (error_t::CRYPTO_ERROR) + static_cast <uint64_t> (this->_alert));
	// Если хендшейк завершился ошибкой
	if(this->_state == state_t::FAILED)
		// Выводим код внутренней ошибки реализации
		return error_t::INTERNAL_ERROR;
	// Выводим код отсутствия ошибки
	return error_t::NO_ERROR;
}
/**
 * @brief Конструктор
 *
 * @param endpoint роль локального эндпоинта на соединении
 * @param ctx      идентификатор шаблона контекста безопасности
 * @param coder    объект кодера транспортной безопасности
 * @param log      объект для работы с логами
 */
awh::quic::Handshake::Handshake(const endpoint_t endpoint, const tls::coder_t::id_t ctx, const tls::coder_t & coder, const log_t * log) noexcept :
 _endpoint(endpoint), _state(state_t::NONE), _alert(0), _hasAlert(false),
 _ctx(coder.native(ctx)), _ssl(nullptr), _params{""}, _early{""}, _session{""}, _ticket{""}, _rejected(false), _protocols(coder.protocols(ctx)), _log(log) {
	/**
	 * Удерживаем ссылку на контекст на всё время жизни объекта хендшейка:
	 * владение остаётся за кодером, а ссылка защищает от освобождения контекста
	 * раньше созданных из него объектов TLS. Ссылка берётся здесь, а не в
	 * методе начала хендшейка, поскольку деструктор освобождает её безусловно
	 */
	if(this->_ctx != nullptr)
		// Удерживаем ссылку на контекст TLS
		::SSL_CTX_up_ref(this->_ctx);
}
/**
 * @brief Деструктор
 *
 */
awh::quic::Handshake::~Handshake() noexcept {
	// Если TLS-соединение создано
	if(this->_ssl != nullptr)
		// Освобождаем объект TLS-соединения
		::SSL_free(this->_ssl);
	// Если контекст TLS создан
	if(this->_ctx != nullptr)
		// Освобождаем объект контекста TLS
		::SSL_CTX_free(this->_ctx);
}
