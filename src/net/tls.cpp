/**
 * @file: tls.cpp
 * @date: 2025-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Если разделитель алгоритмов шифрования не определён
 */
#ifndef __AWH_TLS_CIPHER_SEPARATOR__
	/**
	 * Определяем разделитель алгоритмов шифрования
	 */
	#define __AWH_TLS_CIPHER_SEPARATOR__ ":"
#endif // __AWH_TLS_CIPHER_SEPARATOR__

/**
 * Стандартные модули
 */
#include <atomic>
#include <memory>
#include <csignal>
#include <unordered_set>

/**
 * Подключаем OpenSSL
 */
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>

/**
 * Подключаем заголовочный файл TLS
 */
#include <net/tls.hpp>

/**
 * Подключаем системные заголовочные файлы
 */
#include <sys/locker.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * Прототип структуры уровня защищённых сокетов
	 *
	 */
	class SecureSocketsLayer;
	/**
	 * @brief Тип контейнера уровней защищённых сокетов
	 *
	 */
	using layers_t = unordered_set <unique_ptr <SecureSocketsLayer>>;

	/**
	 * @brief Структура уровня защищённых сокетов
	 *
	 */
	typedef class SecureSocketsLayer {
		public:
			// Объект SSL
			SSL * ssl;
			// Объект буфера BIO для чтения
			BIO * rbio;
			// Объект буфера BIO для записи
			BIO * wbio;
			// Объект SSL контекста
			SSL_CTX * ctx;
			// Объект CRL-файла сертификата
			X509_CRL * crl;
			// Флаг выполнения рукопожатия SSL
			bool handshake;
			// Флаг проверки валидности хоста сервера
			bool validateHost;
			// Тип узла события
			event::node_t node;
			// Тип протокола события
			event::protocol_t proto;
			// Активный ALPN-протокол
			tls_t::alpn_t alpn;
			// Функция обратного вызова получения ошибок
			tls_t::error_callback_t error;
			// Список поддерживаемых ALPN-протоколов
			vector <uint8_t> support;
			// Мьютекс для синхронизации потоков
			lock_state_t <mutex> mtx;
			// Итератор уровня защищённых сокетов
			layers_t::iterator iterator;
		public:
			/**
			 * @brief Метод удаления уровня защищённых сокетов
			 *
			 * @param layers контейнер уровней защищённых сокетов
			 */
			void erase(layers_t & layers) noexcept {
				/**
				 * Выполняем перехват ошибок
				 */
				try {
					// Удаляем уровень защищённых сокетов из контейнера
					layers.erase(this->iterator);
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", error.what());
					#endif
				}
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			SecureSocketsLayer() noexcept :
			 ssl(nullptr),
			 rbio(nullptr),
			 wbio(nullptr),
			 ctx(nullptr),
			 crl(nullptr),
			 handshake(false),
			 validateHost(false),
			 node(event::node_t::NONE),
			 proto(event::protocol_t::NONE),
			 alpn(tls_t::alpn_t::NONE),
			 error(nullptr) {}
			/**
			 * @brief Деструктор
			 *
			 */
			~SecureSocketsLayer() noexcept {
				// Если CRL-файл сертификата уже создан
				if(this->crl != nullptr)
					// Выполняем освобождение памяти
					::X509_CRL_free(this->crl);
				// Если объект SSL существует
				if(this->ssl != nullptr)
					// Удаляем объект SSL
					::SSL_free(this->ssl);
				// Если объект SSL контекста существует
				if(this->ctx != nullptr)
					// Удаляем объект SSL контекста
					::SSL_CTX_free(this->ctx);
			}
	} ssl_t;

	/**
	 * @brief Глобальный список алгоритмов шифрования
	 *
	 */
	string __awh_ssl_ciphers__;
	/**
	 * @brief Глобальный контейнер уровней защищённых сокетов
	 *
	 */
	layers_t __awh_ssl_layers__;
	/**
	 * @brief Счётчик инициализации библиотеки OpenSSL
	 *
	 */
	uint16_t __awh_ssl_init_count__ = 0;
	/**
	 * @brief Флаг инициализации библиотеки OpenSSL
	 *
	 */
	bool __awh_ssl_initialized__ = false;
};

/**
 * Инкапсулируем ALPN-протоколы в пространство имён
 */
namespace alpn {
	/**
	 * Идентификатор протокола HTTP/2.0
	 */
	static constexpr char HTTP2[] = "\x2h2";
	/**
	 * Идентификатор протокола HTTP/3.0
	 */
	static constexpr char HTTP3[] = "\x2h3";
	/**
	 * Идентификатор протокола SPDY
	 */
	static constexpr char SPDY[] = "\x6spdy/1";
	/**
	 * Идентификатор протокола HTTP/1.0
	 */
	static constexpr char HTTP[] = "\x6http/1";
	/*
	 * Идентификатор протокола HTTP/1.1
	 */
	static constexpr char HTTP1_1[] = "\x8http/1.1";
};

/**	
 * Инкапсулируем методы TLS в пространство имён
 */
namespace ssl {
	/**
	 * @brief Функция выполнения выбора протокола
	 *
	 * @param out     буфер назначения
	 * @param outSize размер буфера назначения
	 * @param in      буфер входящих данных
	 * @param inSize  размер буфера входящих данных
	 * @param key     ключ копирования
	 * @param keySize размер ключа для копирования
	 * @return        результат переключения протокола
	 */
	static bool selectProto(uint8_t ** out, uint8_t * outSize, const uint8_t * in, uint32_t inSize, const char * key, uint32_t keySize) noexcept {
		// Результат работы функции
		bool result = false;
		// Выполняем перебор всех данных в входящем буфере
		for(uint32_t i = 0; (i + keySize) <= inSize; i += (uint32_t) (in[i] + 1)){
			// Если данные ключа скопированны удачно
			if((result = (::memcmp(&in[i], key, keySize) == 0))){
				// Выполняем установку размеров исходящего буфера
				(* outSize) = in[i];
				// Выполняем установку полученных данных в исходящий буфер
				(* out) = const_cast <uint8_t *> (&in[i + 1]);
				// Выходим из функции
				break;
			}
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief собран без следующих переговорщиков по протоколам
	 *
	 */
	#ifndef OPENSSL_NO_NEXTPROTONEG
		/**
		 * @brief Функция обратного вызова сервера для переключения на следующий протокол
		 *
		 * @param ssl  объект SSL
		 * @param data данные буфера данных протокола
		 * @param len  размер буфера данных протокола
		 * @param ctx  передаваемый контекст
		 * @return     результат переключения протокола
		 */
		static int32_t nextProto([[maybe_unused]] SSL * ssl, const uint8_t ** data, uint32_t * len, void * ctx) noexcept {
			// Если объекты переданы верно
			if((ssl != nullptr) && (ctx != nullptr)){
				// Получаем объект контекста модуля
				ssl_t * ctx = reinterpret_cast <ssl_t *> (ctx);
				// Выполняем установку буфера данных
				(* data) = ctx->support.data();
				// Выполняем установку размер буфера данных протокола
				(* len) = static_cast <uint32_t> (ctx->support.size());
				// Выводим результат
				return SSL_TLSEXT_ERR_OK;
			}
			// Выводим результат
			return SSL_TLSEXT_ERR_NOACK;
		}
		/**
		 * @brief Функция обратного вызова клиента для расширения NPN TLS. Выполняется проверка, что сервер объявил протокол HTTP/2, который поддерживает библиотека nghttp2.
		 *
		 * @param ssl     объект SSL
		 * @param out     буфер исходящего протокола
		 * @param outSize размер буфера исходящего протокола
		 * @param in      буфер входящего протокола
		 * @param inSize  размер буфера входящего протокола
		 * @param ctx     передаваемый контекст
		 * @return        результат выбора протокола
		 */
		static int32_t clientNextProtoSelect([[maybe_unused]] SSL * ssl, uint8_t ** out, uint8_t * outSize, const uint8_t * in, uint32_t inSize, void * ctx) noexcept {
			// Если объекты переданы верно
			if((ssl != nullptr) && (ctx != nullptr)){
				// Получаем объект контекста модуля
				ssl_t * ctx = reinterpret_cast <ssl_t *> (ctx);
				// Если протокол переключить получилось на HTTP/2
				if(::ssl::selectProto(out, outSize, in, inSize, ::alpn::HTTP3, static_cast <uint32_t> (::alpn::HTTP3[0])))
					// Выводим результат
					return SSL_TLSEXT_ERR_OK;
				// Если протокол переключить получилось на HTTP/2
				else if(::ssl::selectProto(out, outSize, in, inSize, ::alpn::HTTP2, static_cast <uint32_t> (::alpn::HTTP2[0])))
					// Выводим результат
					return SSL_TLSEXT_ERR_OK;
				// Если протокол переключить не получилось
				else {
					// Выполняем переключение протокола обратно на HTTP/1.1
					::ssl::selectProto(out, outSize, in, inSize, ::alpn::HTTP1_1, static_cast <uint32_t> (::alpn::HTTP1_1[0]));
					// Выполняем переключение протокола на HTTP/1.1
					ctx->alpn = tls_t::alpn_t::HTTP;
				}
			}
			// Выводим результат
			return SSL_TLSEXT_ERR_NOACK;
		}
	#endif // !OPENSSL_NO_NEXTPROTONEG
	/**
	 * Если версия OpenSSL соответствует или выше версии 1.0.2
	 */
	#if OPENSSL_VERSION_NUMBER >= 0x10002000L
		/**
		 * @brief Функция обратного вызова сервера для расширения NPN TLS. Выполняется проверка, что сервер объявил протокол HTTP/2, который поддерживает библиотека nghttp2.
		 *
		 * @param ssl     объект SSL
		 * @param out     буфер исходящего протокола
		 * @param outSize размер буфера исходящего протокола
		 * @param in      буфер входящего протокола
		 * @param inSize  размер буфера входящего протокола
		 * @param ctx     передаваемый контекст
		 * @return        результат выбора протокола
		 */
		static int32_t serverNextProtoSelect([[maybe_unused]] SSL * ssl, const uint8_t ** out, uint8_t * outSize, const uint8_t * in, uint32_t inSize, void * ctx) noexcept {
			// Если объекты переданы верно
			if((ssl != nullptr) && (ctx != nullptr)){
				// Получаем объект контекста модуля
				ssl_t * ctx = reinterpret_cast <ssl_t *> (ctx);
				// Если протокол переключить получилось на HTTP/2
				if(::ssl::selectProto(const_cast <uint8_t **> (out), outSize, in, inSize, ::alpn::HTTP3, static_cast <uint32_t> (::alpn::HTTP3[0])))
					// Выводим результат
					return SSL_TLSEXT_ERR_OK;
				// Если протокол переключить получилось на HTTP/2
				else if(::ssl::selectProto(const_cast <uint8_t **> (out), outSize, in, inSize, ::alpn::HTTP2, static_cast <uint32_t> (::alpn::HTTP2[0])))
					// Выводим результат
					return SSL_TLSEXT_ERR_OK;
				// Если протокол переключить не получилось
				else {
					// Выполняем переключение протокола обратно на HTTP/1.1
					::ssl::selectProto(const_cast <uint8_t **> (out), outSize, in, inSize, ::alpn::HTTP1_1, static_cast <uint32_t> (::alpn::HTTP1_1[0]));
					// Выполняем переключение протокола на HTTP/1.1
					ctx->alpn = tls_t::alpn_t::HTTP;
				}
			}
			// Выводим результат
			return SSL_TLSEXT_ERR_NOACK;
		}
	#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
};

/**
 * @brief Метод получения версии протокола TLS
 *
 * @param id идентификатор события
 * @return   версия протокола TLS
 */
string awh::TransportLayerSecurity::version(const id_t id) const noexcept {

	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Метод извлечения активного протокола
 *
 * @param id идентификатор события
 * @return   метод активного протокола
 */
awh::TransportLayerSecurity::alpn_t awh::TransportLayerSecurity::alpn(const id_t id) const noexcept {

	// Возвращаем значение по умолчанию
	return alpn_t::NONE;
}
/**
 * @brief Метод получения информации о шифре
 *
 * @param id идентификатор события
 * @return   информация о шифре
 */
string awh::TransportLayerSecurity::cipherInfo(const id_t id) const noexcept {

	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Метод получения информации о сертификате
 *
 * @param id идентификатор события
 * @return   информация о сертификате
 */
string awh::TransportLayerSecurity::certificateInfo(const id_t id) const noexcept {

	// Возвращаем пустую строку
	return "";
}
/**
 * @brief Метод проверки валидности сертификата
 *
 * @param id идентификатор события
 * @return   результат проверки валидности сертификата
 */
bool awh::TransportLayerSecurity::validateCertificate(const id_t id) const noexcept {

	// Возвращаем отрицательный результат
	return false;
}
/**
 * @brief Метод установки проверки хоста сервера
 *
 * @param id   идентификатор события
 * @param mode режим проверки хоста сервера
 */
void awh::TransportLayerSecurity::validateHostname(const id_t id, const bool mode) noexcept {

}
/**
 * @brief Метод установки имени хоста сервера
 *
 * @param id       идентификатор события
 * @param hostname имя хоста сервера
 */
void awh::TransportLayerSecurity::setHostname(const id_t id, const string & hostname) noexcept {

}
/**
 * @brief Метод выполнения TLS рукопожатия
 *
 * @param id  идентификатор события
 * @param out буфер для записи зашифрованных данных
 * @return    результат выполнения рукопожатия
 */
bool awh::TransportLayerSecurity::handshake(const id_t id, buffer_t out) noexcept {

	// Возвращаем отрицательный результат
	return false;
}
/**
 * @brief Метод проверки завершённости TLS рукопожатия
 *
 * @param id идентификатор события
 * @return   результат проверки завершённости рукопожатия
 */
bool awh::TransportLayerSecurity::isHandshakeComplete(const id_t id) const noexcept {

	// Возвращаем отрицательный результат
	return false;
}
/**
 * @brief Метод шифрования данных
 *
 * @param id  идентификатор события
 * @param in  буфер данных для шифрования
 * @param out буфер для записи зашифрованных данных
 * @return    результат выполнения шифрования
 */
bool awh::TransportLayerSecurity::encrypt(const id_t id, const buffer_t in, buffer_t out) noexcept {

	// Возвращаем отрицательный результат
	return false;
}
/**
 * @brief Метод расшифровки данных
 *
 * @param id  идентификатор события
 * @param in  буфер данных для расшифровки
 * @param out буфер для записи расшифрованных данных
 * @return    результат выполнения расшифровки
 */
bool awh::TransportLayerSecurity::decrypt(const id_t id, const buffer_t in, buffer_t out) noexcept {

	// Возвращаем отрицательный результат
	return false;
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param id   идентификатор события
 * @param mode режим безопасности потоков
 */
void awh::TransportLayerSecurity::threadSafety(const id_t id, const event::mode_t mode) noexcept {

}
/**
 * @brief Метод установки алгоритмов шифрования
 *
 * @param id      идентификатор события
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::TransportLayerSecurity::ciphers(const id_t id, const vector <string> & ciphers) noexcept {

}
/**
 * @brief Метод установки списка отзыва сертификатов
 *
 * @param id   идентификатор события
 * @param path адрес файла списка отзыва сертификатов
 */
void awh::TransportLayerSecurity::crl(const id_t id, const string & path) noexcept {

}
/**
 * @brief Метод установки приватного ключа клиента
 *
 * @param id   идентификатор события
 * @param path адрес файла приватного ключа клиента
 */
void awh::TransportLayerSecurity::privateKey(const id_t id, const string & path) noexcept {

}
/**
 * @brief Метод установки клиентского сертификата
 *
 * @param id   идентификатор события
 * @param path адрес файла клиентского сертификата
 */
void awh::TransportLayerSecurity::certificate(const id_t id, const string & path) noexcept {

}
/**
 * @brief Метод установки сертификатов доверенных центров сертификации
 *
 * @param id   идентификатор события
 * @param path адрес файла сертификата доверенных центров сертификации
 */
void awh::TransportLayerSecurity::ca(const id_t id, const string & path) noexcept {

}
/**
 * @brief Метод установки сертификатов доверенных центров сертификации
 *
 * @param id   идентификатор события
 * @param dir  адрес директории с сертификатами доверенных центров сертификации
 * @param file адрес файла сертификата доверенного центра сертификации
 */
void awh::TransportLayerSecurity::ca(const id_t id, const string & dir, const string & file) noexcept {

}
/**
 * @brief Метод установки функции обратного вывода получения ошибок
 *
 * @param id       идентификатор события
 * @param callback объект функции обратного вызова
 * @return        результат установки функции обратного вызова
 */
bool awh::TransportLayerSecurity::error(const id_t id, error_callback_t callback) noexcept {

	// Возвращаем отрицательный результат
	return false;
}
/**
 * @brief Метод удаления контекста TLS
 *
 * @param id идентификатор контекста TLS
 * @return   результат выполнения удаления
 */
bool awh::TransportLayerSecurity::destroy(const id_t id) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если идентификатор контекста TLS передан
		if((result = (id > 0)))
			// Удаляем контекст TLS из контейнера уровней защищённых сокетов
			reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id))->erase(::__awh_ssl_layers__);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем отрицательный результат
	return result;
}
/**
 * @brief Метод создания контекста TLS
 *
 * @param node  тип узла события
 * @param proto тип протокола события
 * @param alpn  поддерживаемый ALPN-протокол
 * @return      идентификатор контекста TLS
 */
awh::TransportLayerSecurity::id_t awh::TransportLayerSecurity::create(const event::node_t node, const event::protocol_t proto, const alpn_t alpn) noexcept {
	// Результат работы функции
	id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Создаём новый уровень защищённых сокетов и добавляем его в контейнер
		auto ret = ::__awh_ssl_layers__.emplace(::make_unique <SecureSocketsLayer> ());
		// Устанавливаем поддерживаемый ALPN-протокол
		(* ret.first)->alpn = alpn;
		// Устанавливаем тип узла события
		(* ret.first)->node = node;
		// Устанавливаем тип протокола события
		(* ret.first)->proto = proto;
		// Сохраняем итератор уровня защищённых сокетов
		(* ret.first)->iterator = ret.first;
		// Выполняем получение идентификатора контекста TLS
		result = static_cast <uint64_t> (reinterpret_cast <uintptr_t> ((* ret.first).get()));
		/**
		 * Определяем узел события к которому относится контекст TLS
		 */
		switch(static_cast <uint8_t> (node)){
			// Если узел является клиентом
			case static_cast <uint8_t> (event::node_t::CLIENT): {
				/**
				 * Для операционной системы Linux или FreeBSD
				 */
				#if __linux__ || __FreeBSD__
					/**
					 * Определяем тип протокола подключения
					 */
					switch(static_cast <uint8_t> (proto)){
						// Если протокол подключения UDP
						case static_cast <uint8_t> (event::protocol_t::UDP):
						// Если протокол подключения SCTP
						case static_cast <uint8_t> (event::protocol_t::SCTP):
							// Устанавливаем режим клиента для контекста TLS
							(* ret.first)->ctx = ::SSL_CTX_new(::DTLS_client_method());
						break;
						// Если протокол подключения TCP
						case static_cast <uint8_t> (event::protocol_t::TCP):
							// Устанавливаем режим клиента для контекста TLS
							(* ret.first)->ctx = ::SSL_CTX_new(::TLS_client_method());
						break;
					}
				/**
				 * Для операционной системы Linux
				 */
				#else
					// Устанавливаем режим клиента для контекста TLS
					(* ret.first)->ctx = ::SSL_CTX_new(::TLS_client_method());
				#endif
				// Если контекст не создан
				if((* ret.first)->ctx == nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Context SSL is not initialization: %s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto),
								static_cast <uint16_t> (alpn)
							), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Context SSL is not initialization: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_layers__);
					// Выходим
					return 0;
				}
				// Устанавливаем опции запроса
				::SSL_CTX_set_options((* ret.first)->ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION);
				/**
				 * Если версия OpenSSL соответствует или выше версии 3.0.0
				 */
				#if OPENSSL_VERSION_NUMBER >= 0x30000000L
					// Выполняем установку кривых P-256, P-384 и P-521
					if(::SSL_CTX_set1_curves_list((* ret.first)->ctx, "P-521:P-384:P-256") != 1){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"Set SSL CURVEs list failed: %s", __PRETTY_FUNCTION__,
								std::make_tuple(
									static_cast <uint16_t> (node),
									static_cast <uint16_t> (proto),
									static_cast <uint16_t> (alpn)
								), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
							);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Set SSL CURVEs list failed: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
						#endif
						// Удаляем контекст TLS из контейнера уровней защищённых сокетов
						(* ret.first)->erase(::__awh_ssl_layers__);
						// Выходим
						return 0;
					}
				/**
				 * Если версия OpenSSL ниже версии 3.0.0
				 */
				#else
					// Выполняем создание объекта кривой P-256, доступны также (P-384 и P-521) или NID_secp256k1
					EC_KEY * ecdh = ::EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
					// Если кривые не получилось установить
					if(ecdh == nullptr){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"Set new SSL CURVE name failed: %s", __PRETTY_FUNCTION__,
								std::make_tuple(
									static_cast <uint16_t> (node),
									static_cast <uint16_t> (proto),
									static_cast <uint16_t> (alpn)
								), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
							);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Set new SSL CURVE name failed: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
						#endif
						// Удаляем контекст TLS из контейнера уровней защищённых сокетов
						(* ret.first)->erase(::__awh_ssl_layers__);
						// Выходим
						return 0;
					}
					// Выполняем установку кривых P-256
					::SSL_CTX_set_tmp_ecdh((* ret.first)->ctx, ecdh);
					// Выполняем очистку объекта кривой
					::EC_KEY_free(ecdh);
				#endif
				/**
				 * Определяем поддерживаемый ALPN-протокол
				 */
				switch(static_cast <uint8_t> (alpn)){
					// Если протокол соответствует SPDY
					case static_cast <uint8_t> (alpn_t::SPDY): {
						// Устанавливаем идентификатор протокола SPDY/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::SPDY, ::alpn::SPDY + (static_cast <uint16_t> (::alpn::SPDY[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP, ::alpn::HTTP + (static_cast <uint16_t> (::alpn::HTTP[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1.1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP1_1, ::alpn::HTTP1_1 + (static_cast <uint16_t> (::alpn::HTTP1_1[0]) + 1));
					} break;
					// Если протокол соответствует HTTP/1.1
					case static_cast <uint8_t> (alpn_t::HTTP): {
						// Устанавливаем идентификатор протокола HTTP/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP, ::alpn::HTTP + (static_cast <uint16_t> (::alpn::HTTP[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1.1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP1_1, ::alpn::HTTP1_1 + (static_cast <uint16_t> (::alpn::HTTP1_1[0]) + 1));
					} break;
					// Если протокол соответствует HTTP/2.0
					case static_cast <uint8_t> (alpn_t::HTTP2): {
						// Устанавливаем идентификатор протокола HTTP/2
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP2, ::alpn::HTTP2 + (static_cast <uint16_t> (::alpn::HTTP2[0]) + 1));
						// Устанавливаем идентификатор протокола SPDY/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::SPDY, ::alpn::SPDY + (static_cast <uint16_t> (::alpn::SPDY[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP, ::alpn::HTTP + (static_cast <uint16_t> (::alpn::HTTP[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1.1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP1_1, ::alpn::HTTP1_1 + (static_cast <uint16_t> (::alpn::HTTP1_1[0]) + 1));
					} break;
					// Если протокол соответствует HTTP/3.0
					case static_cast <uint8_t> (alpn_t::HTTP3): {
						// Устанавливаем идентификатор протокола HTTP/3
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP3, ::alpn::HTTP3 + (static_cast <uint16_t> (::alpn::HTTP3[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/2
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP2, ::alpn::HTTP2 + (static_cast <uint16_t> (::alpn::HTTP2[0]) + 1));
						// Устанавливаем идентификатор протокола SPDY/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::SPDY, ::alpn::SPDY + (static_cast <uint16_t> (::alpn::SPDY[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP, ::alpn::HTTP + (static_cast <uint16_t> (::alpn::HTTP[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1.1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP1_1, ::alpn::HTTP1_1 + (static_cast <uint16_t> (::alpn::HTTP1_1[0]) + 1));
					} break;
				}
				/**
				 * @brief собран без следующих переговорщиков по протоколам
				 *
				 */
				#ifndef OPENSSL_NO_NEXTPROTONEG
					// Устанавливаем функцию обратного вызова для переключения протокола на HTTP
					::SSL_CTX_set_next_proto_select_cb((* ret.first)->ctx, &::ssl::clientNextProtoSelect, (* ret.first).get());
				#endif // !OPENSSL_NO_NEXTPROTONEG
				/**
				 * Если версия OpenSSL соответствует или выше версии 1.0.2
				 */
				#if OPENSSL_VERSION_NUMBER >= 0x10002000L
					// Выполняем установку доступных протоколов передачи данных
					::SSL_CTX_set_alpn_protos((* ret.first)->ctx, (* ret.first)->support.data(), static_cast <uint32_t> ((* ret.first)->support.size()));
				#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
				// Устанавливаем все основные алгоритмы шифрования
				if(::SSL_CTX_set_cipher_list((* ret.first)->ctx, ::__awh_ssl_ciphers__.c_str()) < 1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Set SSL ciphers: %s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto),
								static_cast <uint16_t> (alpn)
							), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Set SSL ciphers: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_layers__);
					// Выходим
					return 0;
				}
				// Устанавливаем флаг очистки буферов на чтение и запись когда они не требуются
				::SSL_CTX_set_mode((* ret.first)->ctx, SSL_MODE_RELEASE_BUFFERS);
				// Устанавливаем проверку сертификата сервера
				::SSL_CTX_set_verify((* ret.first)->ctx, SSL_VERIFY_PEER, nullptr);
				// Устанавливаем пути по умолчанию для проверки сертификатов
				::SSL_CTX_set_default_verify_paths((* ret.first)->ctx);
				// Отключаем проверку сертификата сервера
				::SSL_CTX_set_verify((* ret.first)->ctx, SSL_VERIFY_NONE, nullptr);
				// Устанавливаем, что мы должны читать как можно больше входных байтов
				::SSL_CTX_set_read_ahead((* ret.first)->ctx, 1);
				// Создаем SSL объект
				(* ret.first)->ssl = ::SSL_new((* ret.first)->ctx);
				// Если объект не создан
				if((* ret.first)->ssl == nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Could not create TLS session object: %s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto),
								static_cast <uint16_t> (alpn)
							), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Could not create TLS session object: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_layers__);
					// Выходим
					return 0;
				}
				// Создаём объект BIO для чтения
				(* ret.first)->rbio = ::BIO_new(::BIO_s_mem());
				// Создаём объект BIO для записи
				(* ret.first)->wbio = ::BIO_new(::BIO_s_mem());
				// Если один из объектов BIO не создан
				if(((* ret.first)->rbio == nullptr) || ((* ret.first)->wbio == nullptr)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Create BIO is failed: %s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto),
								static_cast <uint16_t> (alpn)
							), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Create BIO is failed: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
					#endif
					// Если объект BIO для чтения создан
					if((* ret.first)->rbio != nullptr)
						// Освобождаем объект BIO для чтения
						::BIO_free((* ret.first)->rbio);
					// Если объект BIO для записи создан
					if((* ret.first)->wbio != nullptr)
						// Освобождаем объект BIO для записи
						::BIO_free((* ret.first)->wbio);
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_layers__);
					// Выходим
					return 0;
				}
				// Привязываем объекты BIO к SSL объекту
				::SSL_set_bio((* ret.first)->ssl, (* ret.first)->rbio, (* ret.first)->wbio);
				// Устанавливаем режим клиента для SSL объекта
				::SSL_set_connect_state((* ret.first)->ssl);
			} break;
			// Если узел является сервером
			case static_cast <uint8_t> (event::node_t::SERVER): {
				/**
				 * Для операционной системы Linux или FreeBSD
				 */
				#if __linux__ || __FreeBSD__
					/**
					 * Определяем тип протокола подключения
					 */
					switch(static_cast <uint8_t> (proto)){
						// Если протокол подключения UDP
						case static_cast <uint8_t> (event::protocol_t::UDP):
						// Если протокол подключения SCTP
						case static_cast <uint8_t> (event::protocol_t::SCTP):
							// Устанавливаем режим клиента для контекста TLS
							(* ret.first)->ctx = ::SSL_CTX_new(::DTLS_server_method());
						break;
						// Если протокол подключения TCP
						case static_cast <uint8_t> (event::protocol_t::TCP):
							// Устанавливаем режим клиента для контекста TLS
							(* ret.first)->ctx = ::SSL_CTX_new(::TLS_server_method());
						break;
					}
				/**
				 * Для операционной системы Linux
				 */
				#else
					// Устанавливаем режим сервера для контекста TLS
					(* ret.first)->ctx = ::SSL_CTX_new(::TLS_server_method());
				#endif
				// Если контекст не создан
				if((* ret.first)->ctx == nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Context SSL is not initialization: %s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto),
								static_cast <uint16_t> (alpn)
							), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Context SSL is not initialization: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_layers__);
					// Выходим
					return 0;
				}
				// Устанавливаем опции запроса
				::SSL_CTX_set_options((* ret.first)->ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
				// Устанавливаем минимально-возможную версию TLS
				::SSL_CTX_set_min_proto_version((* ret.first)->ctx, TLS1_VERSION);
				// Устанавливаем максимально-возможную версию TLS
				::SSL_CTX_set_max_proto_version((* ret.first)->ctx, TLS1_3_VERSION);
				// Устанавливаем все основные алгоритмы шифрования
				if(::SSL_CTX_set_cipher_list((* ret.first)->ctx, ::__awh_ssl_ciphers__.c_str()) < 1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Set SSL ciphers: %s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto),
								static_cast <uint16_t> (alpn)
							), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Set SSL ciphers: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_layers__);
					// Выходим
					return 0;
				}
				// Заставляем серверные алгоритмы шифрования использовать в приоритете
				::SSL_CTX_set_options((* ret.first)->ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION | SSL_OP_CIPHER_SERVER_PREFERENCE | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
				/**
				 * Если версия OpenSSL соответствует или выше версии 3.0.0
				 */
				#if OPENSSL_VERSION_NUMBER >= 0x30000000L
					// Выполняем установку кривых P-256, P-384 и P-521
					if(::SSL_CTX_set1_curves_list((* ret.first)->ctx, "P-521:P-384:P-256") != 1){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"Set SSL CURVEs list failed: %s", __PRETTY_FUNCTION__,
								std::make_tuple(
									static_cast <uint16_t> (node),
									static_cast <uint16_t> (proto),
									static_cast <uint16_t> (alpn)
								), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
							);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Set SSL CURVEs list failed: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
						#endif
						// Удаляем контекст TLS из контейнера уровней защищённых сокетов
						(* ret.first)->erase(::__awh_ssl_layers__);
						// Выходим
						return 0;
					}
				/**
				 * Если версия OpenSSL ниже версии 3.0.0
				 */
				#else
					// Выполняем создание объекта кривой P-256, доступны также (P-384 и P-521) или NID_secp256k1
					EC_KEY * ecdh = ::EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
					// Если кривые не получилось установить
					if(ecdh == nullptr){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"Set new SSL CURVE name failed: %s", __PRETTY_FUNCTION__,
								std::make_tuple(
									static_cast <uint16_t> (node),
									static_cast <uint16_t> (proto),
									static_cast <uint16_t> (alpn)
								), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
							);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Set new SSL CURVE name failed: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
						#endif
						// Удаляем контекст TLS из контейнера уровней защищённых сокетов
						(* ret.first)->erase(::__awh_ssl_layers__);
						// Выходим
						return 0;
					}
					// Выполняем установку кривых P-256
					::SSL_CTX_set_tmp_ecdh((* ret.first)->ctx, ecdh);
					// Выполняем очистку объекта кривой
					::EC_KEY_free(ecdh);
				#endif
				/**
				 * Определяем поддерживаемый ALPN-протокол
				 */
				switch(static_cast <uint8_t> (alpn)){
					// Если протокол соответствует SPDY
					case static_cast <uint8_t> (alpn_t::SPDY): {
						// Устанавливаем идентификатор протокола SPDY/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::SPDY, ::alpn::SPDY + (static_cast <uint16_t> (::alpn::SPDY[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP, ::alpn::HTTP + (static_cast <uint16_t> (::alpn::HTTP[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1.1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP1_1, ::alpn::HTTP1_1 + (static_cast <uint16_t> (::alpn::HTTP1_1[0]) + 1));
					} break;
					// Если протокол соответствует HTTP/1.1
					case static_cast <uint8_t> (alpn_t::HTTP): {
						// Устанавливаем идентификатор протокола HTTP/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP, ::alpn::HTTP + (static_cast <uint16_t> (::alpn::HTTP[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1.1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP1_1, ::alpn::HTTP1_1 + (static_cast <uint16_t> (::alpn::HTTP1_1[0]) + 1));
					} break;
					// Если протокол соответствует HTTP/2.0
					case static_cast <uint8_t> (alpn_t::HTTP2): {
						// Устанавливаем идентификатор протокола HTTP/2
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP2, ::alpn::HTTP2 + (static_cast <uint16_t> (::alpn::HTTP2[0]) + 1));
						// Устанавливаем идентификатор протокола SPDY/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::SPDY, ::alpn::SPDY + (static_cast <uint16_t> (::alpn::SPDY[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP, ::alpn::HTTP + (static_cast <uint16_t> (::alpn::HTTP[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1.1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP1_1, ::alpn::HTTP1_1 + (static_cast <uint16_t> (::alpn::HTTP1_1[0]) + 1));
					} break;
					// Если протокол соответствует HTTP/3.0
					case static_cast <uint8_t> (alpn_t::HTTP3): {
						// Устанавливаем идентификатор протокола HTTP/3
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP3, ::alpn::HTTP3 + (static_cast <uint16_t> (::alpn::HTTP3[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/2
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP2, ::alpn::HTTP2 + (static_cast <uint16_t> (::alpn::HTTP2[0]) + 1));
						// Устанавливаем идентификатор протокола SPDY/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::SPDY, ::alpn::SPDY + (static_cast <uint16_t> (::alpn::SPDY[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP, ::alpn::HTTP + (static_cast <uint16_t> (::alpn::HTTP[0]) + 1));
						// Устанавливаем идентификатор протокола HTTP/1.1
						(* ret.first)->support.insert((* ret.first)->support.end(), ::alpn::HTTP1_1, ::alpn::HTTP1_1 + (static_cast <uint16_t> (::alpn::HTTP1_1[0]) + 1));
					} break;
				}
				/**
				 * @brief собран без следующих переговорщиков по протоколам
				 *
				 */
				#ifndef OPENSSL_NO_NEXTPROTONEG
					// Выполняем установку функцию обратного вызова при выборе следующего протокола
					::SSL_CTX_set_next_protos_advertised_cb((* ret.first)->ctx, &::ssl::nextProto, (* ret.first).get());
				#endif // !OPENSSL_NO_NEXTPROTONEG
				/**
				 * Если версия OpenSSL соответствует или выше версии 1.0.2
				 */
				#if OPENSSL_VERSION_NUMBER >= 0x10002000L
					// Устанавливаем функцию обратного вызова для переключения протокола на HTTP/2
					::SSL_CTX_set_alpn_select_cb((* ret.first)->ctx, &::ssl::serverNextProtoSelect, (* ret.first).get());
				#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
				// Выполняем установку идентификатора сессии
				if(::SSL_CTX_set_session_id_context((* ret.first)->ctx, reinterpret_cast <const uint8_t *> (&result), sizeof(result)) < 1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Failed to set session ID: %s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto),
								static_cast <uint16_t> (alpn)
							), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Failed to set session ID: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_layers__);
					// Выходим
					return 0;
				}
				// Устанавливаем поддерживаемые кривые
				if(SSL_CTX_set_ecdh_auto((* ret.first)->ctx, 1) < 1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Set SSL ECDH: %s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto),
								static_cast <uint16_t> (alpn)
							), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Set SSL ECDH: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_layers__);
					// Выходим
					return 0;
				}
				// Устанавливаем флаг quiet shutdown
				// ::SSL_CTX_set_quiet_shutdown((* ret.first)->ctx, 1);
				// Устанавливаем флаг очистки буферов на чтение и запись когда они не требуются
				::SSL_CTX_set_mode((* ret.first)->ctx, SSL_MODE_RELEASE_BUFFERS);
				// Запускаем кэширование
				::SSL_CTX_set_session_cache_mode((* ret.first)->ctx, SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_NO_INTERNAL);
				// Устанавливаем проверку сертификата сервера
				::SSL_CTX_set_verify((* ret.first)->ctx, SSL_VERIFY_PEER, nullptr);
				// Устанавливаем пути по умолчанию для проверки сертификатов
				::SSL_CTX_set_default_verify_paths((* ret.first)->ctx);
				// Отключаем проверку сертификата сервера
				::SSL_CTX_set_verify((* ret.first)->ctx, SSL_VERIFY_NONE, nullptr);
				// Устанавливаем, что мы должны читать как можно больше входных байтов
				::SSL_CTX_set_read_ahead((* ret.first)->ctx, 1);
				// Создаем SSL объект
				(* ret.first)->ssl = ::SSL_new((* ret.first)->ctx);
				// Если объект не создан
				if((* ret.first)->ssl == nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Could not create TLS session object: %s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto),
								static_cast <uint16_t> (alpn)
							), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Could not create TLS session object: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_layers__);
					// Выходим
					return 0;
				}
				// Создаём объект BIO для чтения
				(* ret.first)->rbio = ::BIO_new(::BIO_s_mem());
				// Создаём объект BIO для записи
				(* ret.first)->wbio = ::BIO_new(::BIO_s_mem());
				// Если один из объектов BIO не создан
				if(((* ret.first)->rbio == nullptr) || ((* ret.first)->wbio == nullptr)){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"Create BIO is failed: %s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto),
								static_cast <uint16_t> (alpn)
							), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr)
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Create BIO is failed: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
					#endif
					// Если объект BIO для чтения создан
					if((* ret.first)->rbio != nullptr)
						// Освобождаем объект BIO для чтения
						::BIO_free((* ret.first)->rbio);
					// Если объект BIO для записи создан
					if((* ret.first)->wbio != nullptr)
						// Освобождаем объект BIO для записи
						::BIO_free((* ret.first)->wbio);
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_layers__);
					// Выходим
					return 0;
				}
				// Привязываем объекты BIO к SSL объекту
				::SSL_set_bio((* ret.first)->ssl, (* ret.first)->rbio, (* ret.first)->wbio);
				// Устанавливаем режим сервера для SSL объекта
				::SSL_set_accept_state((* ret.first)->ssl);
			} break;
			// Во всех остальных случаях
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug(
						"Invalid event node type", __PRETTY_FUNCTION__,
						std::make_tuple(
							static_cast <uint16_t> (node),
							static_cast <uint16_t> (proto),
							static_cast <uint16_t> (alpn)
						), log_t::flag_t::WARNING
					);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Invalid event node type", log_t::flag_t::WARNING);
				#endif
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				std::make_tuple(
					static_cast <uint16_t> (node),
					static_cast <uint16_t> (proto),
					static_cast <uint16_t> (alpn)
				), log_t::flag_t::CRITICAL, error.what()
			);
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат работы функции
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::TransportLayerSecurity::TransportLayerSecurity(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {
	// Увеличиваем счётчик инициализации библиотеки OpenSSL
	::__awh_ssl_init_count__++;
	// Если библиотека OpenSSL ещё не инициализирована
	if(!::__awh_ssl_initialized__){
		// Устанавливаем флаг инициализации библиотеки OpenSSL
		::__awh_ssl_initialized__ = !::__awh_ssl_initialized__;
		// Выполняем игнорирование сигналов SIGPIPE
		if(::signal(SIGPIPE, SIG_IGN) == SIG_ERR){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Failed to ignoring signal SIGPIPE", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Failed to ignoring signal SIGPIPE", log_t::flag_t::CRITICAL);
			#endif
		}
		// Выполняем установку алгоритмов шифрования
		::__awh_ssl_ciphers__ = ""
			"ECDHE+AESGCM"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE+CHACHA20"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-RSA-AES128-GCM-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-ECDSA-AES128-GCM-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-RSA-AES256-GCM-SHA384"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-ECDSA-AES256-GCM-SHA384"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DHE-RSA-AES128-GCM-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DHE-DSS-AES128-GCM-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"kEDH+AESGCM"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-RSA-AES128-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-ECDSA-AES128-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-RSA-AES128-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-ECDSA-AES128-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-RSA-AES256-SHA384"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-ECDSA-AES256-SHA384"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-RSA-AES256-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"ECDHE-ECDSA-AES256-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DHE-RSA-AES128-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DHE-RSA-AES128-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DHE-DSS-AES128-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DHE-RSA-AES256-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DHE-DSS-AES256-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DHE-RSA-AES256-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DHE+AESGCM"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DHE+CHACHA20"
			__AWH_TLS_CIPHER_SEPARATOR__
			"AES128-GCM-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"AES256-GCM-SHA384"
			__AWH_TLS_CIPHER_SEPARATOR__
			"AES128-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"AES256-SHA256"
			__AWH_TLS_CIPHER_SEPARATOR__
			"AES128-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"AES256-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"AES"
			__AWH_TLS_CIPHER_SEPARATOR__
			"CAMELLIA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"DES-CBC3-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!aNULL"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!eNULL"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!EXPORT"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!DES"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!RC4"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!MD5"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!PSK"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!aECDH"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!EDH-DSS-DES-CBC3-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!EDH-RSA-DES-CBC3-SHA"
			__AWH_TLS_CIPHER_SEPARATOR__
			"!KRB5-DES-CBC3-SHA";
		/**
		 * Если версия OPENSSL ниже версии 1.1.0
		 */
		#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (LIBRESSL_VERSION_NUMBER && (LIBRESSL_VERSION_NUMBER < 0x20700000L))
			// Выполняем конфигурацию OpenSSL
			::OPENSSL_config(nullptr);
			// Выполняем инициализацию OpenSSL
			::SSL_library_init();
		/**
		 * Для более свежей версии
		 */
		#else
			// Выполняем инициализацию OpenSSL
			::OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, nullptr);
		#endif
		// Выполняем загрузки описаний ошибок шифрования
		::ERR_load_crypto_strings();
		// Выполняем загрузки описаний ошибок OpenSSL
		::SSL_load_error_strings();
		// Добавляем все алгоритмы шифрования
		::OpenSSL_add_all_algorithms();
		// Активируем рандомный генератор
		if(::RAND_poll() < 1){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Rand poll is not allow", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Rand poll is not allow", log_t::flag_t::CRITICAL);
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
}
/**
 * @brief Деструктор
 *
 */
awh::TransportLayerSecurity::~TransportLayerSecurity() noexcept {
	// Уменьшаем счётчик инициализации библиотеки OpenSSL
	::__awh_ssl_init_count__--;
	// Если счётчик инициализации библиотеки OpenSSL равен нулю
	if(::__awh_ssl_init_count__ == 0){
		// Сбрасываем флаг инициализации библиотеки OpenSSL
		::__awh_ssl_initialized__ = !::__awh_ssl_initialized__;
		/**
		 * Если версия OPENSSL ниже версии 1.1.0
		 */
		#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (LIBRESSL_VERSION_NUMBER && LIBRESSL_VERSION_NUMBER < 0x20700000L)
			// Выполняем освобождение памяти
			::EVP_cleanup();
			::ERR_free_strings();
			/**
			 * Если версия OPENSSL ниже версии 1.0.0
			 */
			#if OPENSSL_VERSION_NUMBER < 0x10000000L
				// Освобождаем стейт
				::ERR_remove_state(0);
			/**
			 * Если версия OpenSSL более новая
			 */
			#else
				// Освобождаем стейт для потока
				::ERR_remove_thread_state(nullptr);
			#endif
			// Освобождаем оставшиеся данные
			::CRYPTO_cleanup_all_ex_data();
			// Выполняем освобождение памяти для методов компрессии
			::sk_SSL_COMP_free(::SSL_COMP_get_compression_methods());
		#endif
	}
}
