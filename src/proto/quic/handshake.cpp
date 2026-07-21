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
	/**
	 * @brief Функция выбора ALPN-протокола на сервере (RFC 7301 §3.2)
	 *
	 * @param ssl    объект TLS-соединения
	 * @param output выбранный ALPN-протокол
	 * @param size   размер выбранного ALPN-протокола
	 * @param data   список ALPN-протоколов предложенных клиентом
	 * @param count  размер списка ALPN-протоколов клиента
	 * @param ctx    контекст обратного вызова (не используется)
	 * @return       результат выбора ALPN-протокола
	 */
	static int32_t selectALPN(SSL * ssl, const uint8_t ** output, uint8_t * size, const uint8_t * data, uint32_t count, void * ctx) noexcept;
};

/**
 * @brief Класс доступа обратных вызовов BoringSSL к приватному состоянию хендшейка
 *
 */
class awh::quic::HandshakeHook {
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
		/**
		 * @brief Метод извлечения списка поддерживаемых ALPN-протоколов
		 *
		 * @param handshake объект хендшейка
		 * @return          список поддерживаемых ALPN-протоколов
		 */
		static const vector <string> & protocols(const handshake_t * handshake) noexcept {
			// Выводим список поддерживаемых ALPN-протоколов
			return handshake->_protocols;
		}
};

/**
 * @brief Реализация обратных вызовов BoringSSL QUIC API
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
	/**
	 * @brief Функция выбора ALPN-протокола на сервере (RFC 7301 §3.2)
	 *
	 * @param ssl    объект TLS-соединения
	 * @param output выбранный ALPN-протокол
	 * @param size   размер выбранного ALPN-протокола
	 * @param data   список ALPN-протоколов предложенных клиентом
	 * @param count  размер списка ALPN-протоколов клиента
	 * @param ctx    контекст обратного вызова (не используется)
	 * @return       результат выбора ALPN-протокола
	 */
	static int32_t selectALPN(SSL * ssl, const uint8_t ** output, uint8_t * size, const uint8_t * data, uint32_t count, [[maybe_unused]] void * ctx) noexcept {
		// Получаем объект хендшейка
		const awh::quic::handshake_t * handshake = reinterpret_cast <const awh::quic::handshake_t *> (SSL_get_app_data(ssl));
		// Получаем список поддерживаемых ALPN-протоколов
		const vector <string> & protocols = awh::quic::HandshakeHook::protocols(handshake);
		/**
		 * Перебираем список поддерживаемых ALPN-протоколов в порядке предпочтения сервера
		 */
		for(auto & protocol : protocols){
			// Смещение в списке ALPN-протоколов клиента
			size_t offset = 0;
			/**
			 *  Перебираем список ALPN-протоколов клиента (формат: длина + протокол)
			 */
			while(offset < static_cast <size_t> (count)){
				// Получаем длину ALPN-протокола клиента
				const size_t length = static_cast <size_t> (data[offset]);
				// Если запись выходит за пределы списка, прекращаем разбор
				if((offset + 1 + length) > static_cast <size_t> (count))
					// Выходим из цикла разбора
					break;
				// Если ALPN-протокол клиента совпадает с поддерживаемым
				if((length == protocol.size()) && (::memcmp(data + offset + 1, protocol.data(), length) == 0)){
					// Устанавливаем выбранный ALPN-протокол
					(* output) = (data + offset + 1);
					// Устанавливаем размер выбранного ALPN-протокола
					(* size) = static_cast <uint8_t> (length);
					// Выводим результат успешного выбора
					return SSL_TLSEXT_ERR_OK;
				}
				// Переходим к следующей записи списка
				offset += (1 + length);
			}
		}
		// Общих протоколов нет - завершаем хендшейк алертом no_application_protocol (RFC 9001 §8.1)
		return SSL_TLSEXT_ERR_ALERT_FATAL;
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
	// Если хендшейк успешно завершён
	if(result == 1){
		// Устанавливаем состояние успешного завершения хендшейка
		this->_state = state_t::COMPLETED;
		// Выводим положительный результат
		return status_t::OK;
	}
	// Если хендшейк ожидает данных от удалённого узла
	if(::SSL_get_error(this->_ssl, result) == SSL_ERROR_WANT_READ)
		// Выводим положительный результат - хендшейк продолжается
		return status_t::OK;
	// Устанавливаем состояние ошибки хендшейка
	this->_state = state_t::FAILED;
	// Выводим отрицательный результат
	return status_t::ERROR;
}
/**
 * @brief Метод установки списка поддерживаемых ALPN-протоколов
 *
 * @param protocols список поддерживаемых ALPN-протоколов (например "h3")
 */
void awh::quic::Handshake::alpn(const vector <string> & protocols) noexcept {
	// Устанавливаем список поддерживаемых ALPN-протоколов
	this->_protocols = protocols;
}
/**
 * @brief Метод извлечения согласованного ALPN-протокола
 *
 * @return согласованный ALPN-протокол (пусто - согласование не выполнено)
 */
string awh::quic::Handshake::alpn() const noexcept {
	// Если TLS-соединение создано
	if(this->_ssl != nullptr){
		// Данные согласованного ALPN-протокола
		const uint8_t * data = nullptr;
		// Размер согласованного ALPN-протокола
		uint32_t size = 0;
		// Извлекаем согласованный ALPN-протокол
		::SSL_get0_alpn_selected(this->_ssl, &data, &size);
		// Если ALPN-протокол согласован
		if((data != nullptr) && (size > 0))
			// Выводим согласованный ALPN-протокол
			return string(reinterpret_cast <const char *> (data), static_cast <size_t> (size));
	}
	// Выводим пустой результат
	return "";
}
/**
 * @brief Метод установки доменного имени удалённого сервера (SNI)
 *
 * @param sni доменное имя удалённого сервера
 */
void awh::quic::Handshake::serverNameIndication(string_view sni) noexcept {
	// Устанавливаем доменное имя удалённого сервера
	this->_sni.assign(sni);
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
	// Выполняем сериализацию транспортных параметров и выводим результат
	return quic::params::serialize::encode(this->_params, params, this->_endpoint);
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
 * @brief Метод установки сертификата и приватного ключа локального узла
 *
 * @param certificate сертификат в формате PEM
 * @param privateKey  приватный ключ в формате PEM
 */
void awh::quic::Handshake::certificate(string_view certificate, string_view privateKey) noexcept {
	// Устанавливаем сертификат локального узла
	this->_certificate.assign(certificate);
	// Устанавливаем приватный ключ локального узла
	this->_privateKey.assign(privateKey);
}
/**
 * @brief Метод установки проверки сертификата удалённого узла
 *
 * @param mode режим проверки сертификата удалённого узла
 */
void awh::quic::Handshake::verify(const bool mode) noexcept {
	// Устанавливаем флаг проверки сертификата удалённого узла
	this->_verify = mode;
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
	if((this->_state != state_t::NONE) || this->_params.empty())
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Создаём контекст TLS
	this->_ctx = ::SSL_CTX_new(::TLS_method());
	// Если контекст TLS не создан
	if(this->_ctx == nullptr){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Устанавливаем минимальную версию протокола TLS 1.3 (RFC 9001 §4.2)
	::SSL_CTX_set_min_proto_version(this->_ctx, TLS1_3_VERSION);
	// Устанавливаем максимальную версию протокола TLS 1.3
	::SSL_CTX_set_max_proto_version(this->_ctx, TLS1_3_VERSION);
	// Если локальный эндпоинт является сервером и установлен список ALPN-протоколов
	if((this->_endpoint == endpoint_t::SERVER) && !this->_protocols.empty())
		// Устанавливаем функцию выбора ALPN-протокола на сервере
		::SSL_CTX_set_alpn_select_cb(this->_ctx, &::selectALPN, nullptr);
	// Создаём TLS-соединение
	this->_ssl = ::SSL_new(this->_ctx);
	// Если TLS-соединение не создано
	if(this->_ssl == nullptr){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Устанавливаем обратный указатель на объект хендшейка
	SSL_set_app_data(this->_ssl, this);
	// Устанавливаем таблицу обратных вызовов BoringSSL QUIC API
	if(::SSL_set_quic_method(this->_ssl, &::quicMethod) != 1){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Устанавливаем локальные транспортные параметры
	if(::SSL_set_quic_transport_params(this->_ssl, reinterpret_cast <const uint8_t *> (this->_params.data()), this->_params.size()) != 1){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Устанавливаем режим проверки сертификата удалённого узла
	::SSL_set_verify(this->_ssl, (this->_verify ? SSL_VERIFY_PEER : SSL_VERIFY_NONE), nullptr);
	// Если установлены сертификат и приватный ключ локального узла
	if(!this->_certificate.empty() && !this->_privateKey.empty()){
		// Флаг успешной установки сертификата и приватного ключа
		bool result = false;
		// Создаём буфер BIO для чтения сертификата из памяти
		BIO * cbio = ::BIO_new_mem_buf(this->_certificate.data(), static_cast <int32_t> (this->_certificate.size()));
		// Создаём буфер BIO для чтения приватного ключа из памяти
		BIO * kbio = ::BIO_new_mem_buf(this->_privateKey.data(), static_cast <int32_t> (this->_privateKey.size()));
		// Если буферы BIO созданы
		if((cbio != nullptr) && (kbio != nullptr)){
			// Читаем сертификат из буфера BIO
			X509 * x509 = ::PEM_read_bio_X509(cbio, nullptr, nullptr, nullptr);
			// Читаем приватный ключ из буфера BIO
			EVP_PKEY * pkey = ::PEM_read_bio_PrivateKey(kbio, nullptr, nullptr, nullptr);
			// Если сертификат и приватный ключ прочитаны
			if((x509 != nullptr) && (pkey != nullptr))
				// Устанавливаем сертификат и приватный ключ на TLS-соединение
				result = ((::SSL_use_certificate(this->_ssl, x509) == 1) && (::SSL_use_PrivateKey(this->_ssl, pkey) == 1));
			// Если сертификат прочитан, освобождаем его
			if(x509 != nullptr)
				// Освобождаем объект сертификата
				::X509_free(x509);
			// Если приватный ключ прочитан, освобождаем его
			if(pkey != nullptr)
				// Освобождаем объект приватного ключа
				::EVP_PKEY_free(pkey);
		}
		// Если буфер BIO сертификата создан, освобождаем его
		if(cbio != nullptr)
			// Освобождаем буфер BIO сертификата
			::BIO_free(cbio);
		// Если буфер BIO приватного ключа создан, освобождаем его
		if(kbio != nullptr)
			// Освобождаем буфер BIO приватного ключа
			::BIO_free(kbio);
		// Если сертификат либо приватный ключ не установлены
		if(!result){
			// Устанавливаем состояние ошибки хендшейка
			this->_state = state_t::FAILED;
			// Выводим отрицательный результат
			return status_t::ERROR;
		}
	}
	// Если локальный эндпоинт является клиентом
	if(this->_endpoint == endpoint_t::CLIENT){
		// Устанавливаем клиентское состояние TLS-соединения
		::SSL_set_connect_state(this->_ssl);
		// Если установлено доменное имя удалённого сервера
		if(!this->_sni.empty())
			// Устанавливаем доменное имя удалённого сервера (SNI)
			::SSL_set_tlsext_host_name(this->_ssl, this->_sni.c_str());
		// Если установлен список поддерживаемых ALPN-протоколов
		if(!this->_protocols.empty()){
			// Список ALPN-протоколов в проводном формате (длина + протокол)
			string wire = "";
			/**
			 * Перебираем список поддерживаемых ALPN-протоколов
			 */
			for(auto & protocol : this->_protocols){
				// Добавляем длину ALPN-протокола
				wire.push_back(static_cast <char> (protocol.size()));
				// Добавляем название ALPN-протокола
				wire.append(protocol);
			}
			// Устанавливаем список ALPN-протоколов клиента
			if(::SSL_set_alpn_protos(this->_ssl, reinterpret_cast <const uint8_t *> (wire.data()), static_cast <uint32_t> (wire.size())) != 0){
				// Устанавливаем состояние ошибки хендшейка
				this->_state = state_t::FAILED;
				// Выводим отрицательный результат
				return status_t::ERROR;
			}
		}
	// Если локальный эндпоинт является сервером, устанавливаем серверное состояние TLS-соединения
	} else ::SSL_set_accept_state(this->_ssl);
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
 */
awh::quic::Handshake::Handshake(const endpoint_t endpoint) noexcept :
 _endpoint(endpoint), _state(state_t::NONE), _alert(0), _hasAlert(false),
 _verify(false), _ctx(nullptr), _ssl(nullptr), _sni{""}, _params{""},
 _certificate{""}, _privateKey{""} {}
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
