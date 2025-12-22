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
 * Стандартные модули
 */
#include <ctime>
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
 * Подключаем системные заголовки
 */
#include <netinet/in.h>

/**
 * Подключаем заголовочный файл TLS
 */
#include <net/net.hpp>
#include <net/tls.hpp>

/**
 * Подключаем системные заголовочные файлы
 */
#include <sys/os.hpp>
#include <sys/locker.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

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
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * Прототип класса уровня защищённых сокетов
	 *
	 */
	class SecureSocketsLayer;
	/**
	 * @brief Тип контейнера уровней защищённых сокетов
	 *
	 */
	using layers_t = unordered_set <unique_ptr <SecureSocketsLayer>>;

	/**
	 * @brief Структура печенок SSL
	 *
	 */
	typedef struct Cookie {
		// Буфер секретного слова печенок
		uint8_t buffer[16];
		// Флаг инициализации печенок SSL
		atomic_bool initialized;
		/**
		 * @brief Конструктор
		 *
		 */
		Cookie() noexcept : buffer{0}, initialized(false) {}
	} cookie_t;

	/**
	 * @brief Структура сервера назначения
	 *
	 */
	typedef struct Destination {
		// Имя хоста события
		string hostname;
		// Адрес сервера
		unique_ptr <net::attr_net_t> address;
		/**
		 * @brief Конструктор
		 *
		 */
		Destination() noexcept : hostname{""}, address(nullptr) {}
	} dest_t;

	/**
	 * @brief Класс уровня защищённых сокетов
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
			// Объект печенок SSL
			cookie_t cookie;
			// Объект сервера назначения
			dest_t destination;
			// Тип узла события
			event::node_t node;
			// Тип протокола события
			event::protocol_t proto;
			// Активный ALPN-протокол
			tls_t::alpn_t alpn;
			// Функция обратного вызова получения ошибок
			tls_t::error_callback_t error;
			// Мьютекс для синхронизации потоков
			lock_state_t <mutex> mtx;
			// Список поддерживаемых ALPN-протоколов
			vector <uint8_t> support;
			// Итератор уровня защищённых сокетов
			layers_t::iterator iterator;
		public:
			/**
			 * @brief Метод удаления уровня защищённых сокетов
			 *
			 * @param layers контейнер уровней защищённых сокетов
			 */
			void erase(layers_t & layers) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			SecureSocketsLayer() noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~SecureSocketsLayer() noexcept;
	} ssl_t;

	/**
	 * @brief Метод удаления уровня защищённых сокетов
	 *
	 * @param layers контейнер уровней защищённых сокетов
	 */
	void SecureSocketsLayer::erase(layers_t & layers) noexcept {
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

	/**
	 * @brief Конструктор
	 *
	 */
	SecureSocketsLayer::SecureSocketsLayer() noexcept :
	 ssl(nullptr), rbio(nullptr), wbio(nullptr),
	 ctx(nullptr), crl(nullptr), handshake(false),
	 node(event::node_t::NONE), proto(event::protocol_t::NONE),
	 alpn(tls_t::alpn_t::NONE), error(nullptr) {}

	/**
	 * @brief Деструктор
	 *
	 */
	SecureSocketsLayer::~SecureSocketsLayer() noexcept {
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
	/**
	 * @brief Глобальный набор идентификаторов контекстов TLS
	 *
	 */
	unordered_set <uint64_t> __awh_ssl_ids__;
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
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * @brief Функция проверки параметров сертификата
		 *
		 * @param store стор с сертификатами для работы
		 * @param name  название параметра сертификата
		 * @param log   объект для работы с логами
		 * @return      результат проверки
		 */
		static bool addCertToStore(X509_STORE * store, const char * name, const awh::log_t * log) noexcept {
			// Результат работы функции
			bool result = false;
			// Если объекты переданы верно
			if((store != nullptr) && (name != nullptr)){
				// Получаем данные системного стора
				HCERTSTORE sys = ::CertOpenSystemStore(0, name);
				// Если системный стор не получен
				if(!(result = (sys != nullptr))){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						log->debug("Failed to open system certificate store", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("Failed to open system certificate store", log_t::flag_t::CRITICAL);
					#endif
					// Выходим
					return result;
				}
				// Контекст сертификата
				PCCERT_CONTEXT ctx = nullptr;
				/**
				 * Перебираем все сертификаты в системном сторе
				 */
				while((ctx = ::CertEnumCertificatesInStore(sys, ctx))){
					// Выполняем создание сертификата
					X509 * cert = X509_new();
					// Если сертификат создан удачно
					if((result = (cert != nullptr))){
						// Получаем объект закодированного сертификата
						const BYTE * encoded = ctx->pbCertEncoded;
						// Добавляем сертификат в стор
						::X509_STORE_add_cert(store, ::d2i_X509(&cert, reinterpret_cast <const uint8_t **> (&encoded), ctx->cbCertEncoded));
						// Очищаем выделенную память
						::X509_free(cert);
					// Если сертификат не создан
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							log->debug("Create X509 is failed", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							log->print("Create X509 is failed", log_t::flag_t::CRITICAL);
						#endif
						// Выходим из цикла
						break;
					}
				}
				// Закрываем системный стор
				::CertCloseStore(sys, 0);
			}
			// Выводим сформированный результат
			return result;
		}
	#endif
};

/**
 * Инкапсулируем методы работы с cookie в пространство имён
 */
namespace cookie {
	/**
	 * Индексы для хранения состояний проверки куков
	 */
	static int32_t index[3] = {-1};

	/**
	 * @brief Функция обратного вызова для генерации куков
	 *
	 * @param ssl    объект SSL
	 * @param cookie данные куков
	 * @param size   количество символов
	 * @return       результат проверки
	 */
	static int32_t generateCookie(SSL * ssl, uint8_t * cookie, uint32_t * size) noexcept {
		// Получаем объект уровня защищённых сокетов
		::ssl_t * layer = reinterpret_cast <::ssl_t *> (::SSL_get_ex_data(ssl, ::cookie::index[0]));
		// Если печенки еще не проинициализированны
		if(!layer->cookie.initialized){
			// Выполняем произвольно генерацию байт в буфере печенок
			if(!(layer->cookie.initialized = ::RAND_bytes(layer->cookie.buffer, sizeof(layer->cookie.buffer)))){
				// Получаем объект логирования
				awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::cookie::index[2]));
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					log->debug("Setting random cookie secret", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::CRITICAL);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					log->print("Setting random cookie secret", awh::log_t::flag_t::CRITICAL);
				#endif
				// Выходим и сообщаем, что генерация куков не удалась
				return 0;
			}
		}
		// Получаем объект хоста IPv4-адреса
		net::attr_net_t * address = awh_cast <net::attr_net_t *> (layer->destination.address.get());
		// Размер буфера и длина сгенерированных печенок
		uint32_t bytes = (address->ip->size + 2), length = 0;
		// Выполняем выделение память для буфера данных
		uint8_t * buffer = reinterpret_cast <uint8_t *> (::OPENSSL_malloc(bytes));
		// Если память для буфера данных не выделена
		if(buffer == nullptr){
			// Получаем объект логирования
			awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::cookie::index[2]));
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				log->debug("Out of memory cookie", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::CRITICAL);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				log->print("Out of memory cookie", awh::log_t::flag_t::CRITICAL);
			#endif
			// Выходим и сообщаем, что генерация куков не удалась
			return 0;
		}
		// Выполняем чтение в буфер данных данные порта
		::memcpy(buffer, &address->port, 2);
		/**
		 * Определяем тип адреса
		 */
		switch(address->ip->size){
			// Если адрес является IPv4
			case 4:
				// Выполняем чтение в буфер данных данные структуры подключения
				::memcpy(buffer + 2, &awh_cast <net::addr_net_ipv4_t *> (address->ip.get())->address, 4);
			break;
			// Если адрес является IPv6
			case 16: {
				// Выполняем чтение в буфер данных данные структуры подключения
				::memcpy(buffer + 2, &awh_cast <net::addr_net_ipv6_t *> (address->ip.get())->address[0], 16);
			} break;
			// Если производится работа с другими протоколами, выходим
			default: OPENSSL_assert(0);
		}
		// Буфер под генерацию печенок
		uint8_t result[EVP_MAX_MD_SIZE];
		// Выполняем расчёт HMAC в буфере, с использованием секретного ключа
		::HMAC(::EVP_sha1(), reinterpret_cast <void *> (layer->cookie.buffer), sizeof(layer->cookie.buffer), buffer, bytes, result, &length);
		// Очищаем ранее выделенную память
		OPENSSL_free(buffer);
		// Выполняем копирование полученного результата в буфер печенок
		::memcpy(cookie, result, length);
		// Устанавливаем размер буфера печенок
		(* size) = length;
		// Выводим положительный ответ
		return 1;
	}
	/**
	 * @brief Функция обратного вызова для генерации куков
	 *
	 * @param ssl    объект SSL
	 * @param cookie данные куков
	 * @param size   количество символов
	 * @return       результат проверки
	 */
	static int32_t generateStatelessCookie(SSL * ssl, uint8_t * cookie, size_t * size) noexcept {
		// Размер буфера с печенками
		uint32_t length = 0;
		// Выполняем генерацию печенок
		const int32_t result = ::cookie::generateCookie(ssl, cookie, &length);
		// Получаем размер буфера с печенками
		(* size) = length;
		// Выводим результат работы функции
		return result;
	}
	/**
	 * @brief Функция обратного вызова для проверки куков
	 *
	 * @param ssl    объект SSL
	 * @param cookie данные куков
	 * @param size   количество символов
	 * @return       результат проверки
	 */
	static int32_t verifyCookie(SSL * ssl, const uint8_t * cookie, uint32_t size) noexcept {
		// Получаем объект уровня защищённых сокетов
		::ssl_t * layer = reinterpret_cast <::ssl_t *> (::SSL_get_ex_data(ssl, ::cookie::index[0]));
		// Если печенки не проинициализированы, значит куки не валидные
		if(!layer->cookie.initialized)
			// Выходим из функции
			return 0;
		// Получаем объект хоста IPv4-адреса
		net::attr_net_t * address = awh_cast <net::attr_net_t *> (layer->destination.address.get());
		// Размер буфера и длина сгенерированных печенок
		uint32_t bytes = (address->ip->size + 2), length = 0;
		// Выполняем выделение память для буфера данных
		uint8_t * buffer = reinterpret_cast <uint8_t *> (::OPENSSL_malloc(bytes));
		// Если память для буфера данных не выделена
		if(buffer == nullptr){
			// Получаем объект логирования
			awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::cookie::index[2]));
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				log->debug("Out of memory cookie", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::CRITICAL);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				log->print("Out of memory cookie", awh::log_t::flag_t::CRITICAL);
			#endif
			// Выходим и сообщаем, что генерация куков не удалась
			return 0;
		}
		// Выполняем чтение в буфер данных данные порта
		::memcpy(buffer, &address->port, 2);
		/**
		 * Определяем тип адреса
		 */
		switch(address->ip->size){
			// Если адрес является IPv4
			case 4:
				// Выполняем чтение в буфер данных данные структуры подключения
				::memcpy(buffer + 2, &awh_cast <net::addr_net_ipv4_t *> (address->ip.get())->address, 4);
			break;
			// Если адрес является IPv6
			case 16: {
				// Выполняем чтение в буфер данных данные структуры подключения
				::memcpy(buffer + 2, &awh_cast <net::addr_net_ipv6_t *> (address->ip.get())->address[0], 16);
			} break;
			// Если производится работа с другими протоколами, выходим
			default: OPENSSL_assert(0);
		}
		// Буфер под генерацию печенок
		uint8_t result[EVP_MAX_MD_SIZE];
		// Выполняем расчёт HMAC в буфере, с использованием секретного ключа
		::HMAC(::EVP_sha1(), reinterpret_cast <void *> (layer->cookie.buffer), sizeof(layer->cookie.buffer), buffer, bytes, result, &length);
		// Очищаем ранее выделенную память
		::OPENSSL_free(buffer);
		// Выполняем проверку печенок, если печенки совпадают, значит всё хорошо
		if((size == length) && (::memcmp(result, cookie, length) == 0))
			// Выходим из функции с удачей
			return 1;
		// Выходим из функции с неудачей
		return 0;
	}
	/**
	 * @brief Функция обратного вызова для проверки куков
	 *
	 * @param ssl    объект SSL
	 * @param cookie данные куков
	 * @param size   количество символов
	 * @return       результат проверки
	 */
	static int32_t verifyStatelessCookie(SSL * ssl, const uint8_t * cookie, size_t size) noexcept {
		// Выполняем проверку печенок
		return ::cookie::verifyCookie(ssl, cookie, static_cast <uint32_t> (size));
	}
};

/**
 * Инкапсулируем методы проверки сертификата в пространство имён
 */
namespace verify {
	/**
	 * Типы ошибок валидации
	 */
	enum class validate_t : uint8_t {
		NONE                 = 0x00, // Не установлено
		Error                = 0x01, // Ошибка валидации
		MatchFound           = 0x02, // Валидация пройдена
		NoSANPresent         = 0x03, // Сеть не распознана
		MatchNotFound        = 0x04, // Валидация не пройдена
		MalformedCertificate = 0x05  // Неверный сертификат
	};

	/**
	 * @brief Функция проверки на эквивалентность доменных имен
	 *
	 * @param first  первое доменное имя
	 * @param second второе доменное имя
	 * @return       результат проверки
	 */
	static bool equal(string_view first, string_view second) noexcept {
		// Результат работы функции
		bool result = false;
		// Если данные переданы
		if(!first.empty() && !second.empty())
			// Проверяем совпадение строки
			result = (first.compare(second) == 0);
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция проверки на эквивалентность доменных имен с пропуском начальных символов
	 *
	 * @param first  первое доменное имя
	 * @param second второе доменное имя
	 * @param max    количество начальных символов для проверки
	 * @return       результат проверки
	 */
	static bool noqual(string_view first, string_view second, size_t max) noexcept {
		// Результат работы функции
		bool result = false;
		// Если данные переданы
		if(!first.empty() && !second.empty())
			// Проверяем совпадение строки
			result = (first.substr(max, first.length() - max).compare(second.substr(max, second.length() - max)) == 0);
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция проверки эквивалентности доменного имени с учетом шаблона
	 *
	 * @param host доменное имя
	 * @param fqdn шаблон доменного имени
	 * @return     результат проверки
	 */
	static bool hostmatch(string_view host, string_view fqdn) noexcept {
		// Результат работы функции
		bool result = true;
		// Если данные переданы
		if(!host.empty() && !fqdn.empty()){
			// Позиция звездочки в шаблоне
			const size_t pos1 = fqdn.find('*');
			// Ищем звездочку в шаблоне не найдена
			if(pos1 == string::npos)
				// Выполняем проверку эквивалентности доменных имён
				return ::verify::equal(fqdn, host);
			// Определяем конец шаблона
			const size_t pos2 = fqdn.find('.');
			// Если это конец тогда запрещаем активацию шаблона
			if((pos2 == string::npos) || (pos1 > pos2) || ::verify::noqual(fqdn, "xn--", 4))
				// Выполняем проверку эквивалентности доменных имён
				return ::verify::equal(fqdn, host);
			// Выполняем поиск точки в название хоста
			const size_t pos3 = host.find('.');
			// Если хост не найден
			if((pos2 != string::npos) && (pos3 != string::npos)){
				// Выполняем сравнение
				if(!::verify::equal(fqdn.substr(0, pos2), host.substr(0, pos3)))
					// Выходим из функции
					return false;
			// Выходим из функции
			} else return false;
			// Если диапазоны точки в шаблоне и хосте отличаются тогда выходим
			if(pos3 < pos2)
				// Выходим из функции
				return false;
			// Вычисляем длину обрезаемой строки
			const size_t length = (pos2 - (pos1 + 1));
			// Проверяем эквивалент результата
			return (
				::verify::noqual(fqdn, host, pos1) &&
				::verify::noqual(fqdn.substr(pos1 + 1, length), host.substr(pos3 - length, length), length)
			);
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция проверки доменного имени по шаблону
	 *
	 * @param host доменное имя
	 * @param fqdn шаблон доменного имени
	 * @return     результат проверки
	 */
	static bool certHostcheck(string_view host, string_view fqdn) noexcept {
		// Результат работы функции
		bool result = false;
		// Если данные переданы
		if(!host.empty() && !fqdn.empty())
			// Проверяем эквивалентны ли домен и шаблон
			result = (::verify::equal(host, fqdn) || ::verify::hostmatch(host, fqdn));
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция проверки доменного имени по списку доменных имен из сертификата
	 *
	 * @param host доменное имя
	 * @param x509 сертификат
	 * @return     результат проверки
	 */
	static validate_t matchSubjectName(const string & host, const X509 * x509) noexcept {
		// Результат работы функции
		validate_t result = validate_t::MatchNotFound;
		// Если данные переданы
		if(!host.empty() && (x509 != nullptr)){
			// Извлекаем SAN из сертификата
			STACK_OF(GENERAL_NAME) * san = reinterpret_cast <STACK_OF(GENERAL_NAME) *> (::X509_get_ext_d2i(const_cast <X509 *> (x509), NID_subject_alt_name, nullptr, nullptr));
			// Если SAN присутствует
			if(san != nullptr){
				// Полученное доменное имя
				string fqdn = "";
				// Проверяем каждый элемент SAN
				for(int32_t i = 0; i < sk_GENERAL_NAME_num(san); i++){
					// Извлекаем элемент SAN
					const GENERAL_NAME * cn = sk_GENERAL_NAME_value(san, i);
					// Проверяем тип имени
					if(cn->type == GEN_DNS){
						// Формируем строковое представление доменного имени
						fqdn.assign(reinterpret_cast <char *> (const_cast <uint8_t *> (::ASN1_STRING_get0_data(cn->d.dNSName))), ::ASN1_STRING_length(cn->d.dNSName));
						// Если размер имени и dns имя совпадает
						if(::verify::certHostcheck(host, fqdn)){
							// Запоминаем результат что домен найден
							result = validate_t::MatchFound;
							// Выходим из цикла
							break;
						}
					}
				}
				// Очищаем список имен
				sk_GENERAL_NAME_pop_free(san, GENERAL_NAME_free);
			// Если SAN отсутствует или имя не совпало
			} else {
				// Буфер данных для получения данных
				char buffer[256];
				// Fallback на Common Name (устаревшее, но иногда нужно)
				X509_NAME * subject = ::X509_get_subject_name(x509);
				// Если удалось получить Common Name
				if(::X509_NAME_get_text_by_NID(subject, NID_commonName, buffer, sizeof(buffer)) == 1)
					// Если размер имени и dns имя совпадает
					if(::verify::certHostcheck(host, buffer))
						// Запоминаем результат что домен найден
						result = validate_t::MatchFound;
			}
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция проверки доменного имени по данным из сертификата
	 *
	 * @param host доменное имя
	 * @param x509 сертификат
	 * @return     результат проверки
	 */
	validate_t matchesCommonName(const string & host, const X509 * x509) noexcept {
		// Результат работы функции
		validate_t result = validate_t::MatchNotFound;
		// Если данные переданы
		if(!host.empty() && (x509 != nullptr)){
			// Получаем индекс имени по "NID"
			const int32_t cnl = ::X509_NAME_get_index_by_NID(X509_get_subject_name(const_cast <X509 *> (x509)), NID_commonName, -1);
			// Если индекс не получен тогда выходим
			if(cnl < 0)
				// Выводим сформированную ошибку
				return validate_t::Error;
			// Извлекаем поле "CN"
			X509_NAME_ENTRY * cne = ::X509_NAME_get_entry(X509_get_subject_name(const_cast <X509 *> (x509)), cnl);
			// Если поле не получено тогда выходим
			if(cne == nullptr)
				// Выводим сформированную ошибку
				return validate_t::Error;
			// Конвертируем "CN" поле в "C" строку
			ASN1_STRING * cna = ::X509_NAME_ENTRY_get_data(cne);
			// Если строка не сконвертирована тогда выходим
			if(cna == nullptr)
				// Выводим сформированную ошибку
				return validate_t::Error;
			// Извлекаем название в виде строки
			const string cn(reinterpret_cast <char *> (const_cast <uint8_t *> (::ASN1_STRING_get0_data(cna))), ::ASN1_STRING_length(cna));
			// Выполняем рукопожатие
			if(::verify::certHostcheck(host, cn))
				// Выводим сформированную ошибку
				return validate_t::MatchFound;
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция проверки доменного имени
	 *
	 * @param host доменное имя
	 * @param x509 сертификат
	 * @return     результат проверки
	 */
	static validate_t validateHostname(const string & host, const X509 * x509) noexcept {
		// Результат работы функции
		validate_t result = validate_t::Error;
		// Если данные переданы
		if(!host.empty() && (x509 != nullptr)){
			// Выполняем проверку имени хоста по списку доменов у сертификата
			result = ::verify::matchSubjectName(host, x509);
			// Если у сертификата только один домен
			if(result == validate_t::NoSANPresent)
				// Выполняем проверку имени хоста по общему имени у сертификата
				result = ::verify::matchesCommonName(host, x509);
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция обратного вызова для проверки валидности хоста
	 *
	 * @param x509 данные сертификата
	 * @param ctx  передаваемый контекст
	 * @return     результат проверки
	 */
	static int32_t hostname(X509_STORE_CTX * x509, void * ctx) noexcept {
		// Если объекты переданы верно
		if((x509 != nullptr) && (ctx != nullptr)){
			// Буфер данных сертификатов из хранилища
			char buffer[256];
			// Заполняем структуру нулями
			::memset(buffer, 0, sizeof(buffer));
			// Ошибка проверки сертификата
			string status = "X509VerifyCertFailed";
			// Выполняем проверку сертификата
			const int32_t ok = ::X509_verify_cert(x509);
			// Запрашиваем данные сертификата
			X509 * cert = ::X509_STORE_CTX_get_current_cert(x509);
			// Результат проверки домена
			validate_t validate = validate_t::Error;
			// Получаем объект SSL
			SSL * ssl = reinterpret_cast <SSL *> (ctx);
			// Получаем объект уровня защищённых сокетов
			::ssl_t * layer = reinterpret_cast <::ssl_t *> (::SSL_get_ex_data(ssl, ::cookie::index[0]));
			// Если проверка сертификата прошла удачно
			if(ok){
				// Выполняем проверку на соответствие хоста с данными хостов у сертификата
				validate = ::verify::validateHostname(layer->destination.hostname, cert);
				/**
				 * Определяем полученную ошибку
				 */
				switch(static_cast <uint8_t> (validate)){
					// Если домен найден в записях сертификата
					case static_cast <uint8_t> (validate_t::MatchFound):
						// Устанавливаем статус проверки
						status = "MatchFound";
					break;
					// Если домен не найден в записях сертификата
					case static_cast <uint8_t> (validate_t::MatchNotFound):
						// Устанавливаем статус проверки
						status = "MatchNotFound";
					break;
					// Если в сертификате отсутствует SAN
					case static_cast <uint8_t> (validate_t::NoSANPresent):
						// Устанавливаем статус проверки
						status = "NoSANPresent";
					break;
					// Если сертификат имеет неверный формат
					case static_cast <uint8_t> (validate_t::MalformedCertificate):
						// Устанавливаем статус проверки
						status = "MalformedCertificate";
					break;
					// Если произошла ошибка при проверке
					case static_cast <uint8_t> (validate_t::Error):
						// Устанавливаем статус проверки
						status = "Error";
					break;
					// В иных случаях
					default: status = "WTF!";
				}
			}
			// Запрашиваем имя домена
			::X509_NAME_oneline(::X509_get_subject_name(cert), buffer, sizeof(buffer));
			// Очищаем выделенную память
			// ::X509_free(cert);
			// Если домен найден в записях сертификата (т.е. сертификат соответствует данному домену)
			if(validate == validate_t::MatchFound){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Получаем объект логирования
					awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::cookie::index[2]));
					// Выводим в лог сообщение
					log->print("HTTPS server [%s] has this certificate, which looks good to me: %s", awh::log_t::flag_t::INFO, layer->destination.hostname.c_str(), buffer);
				#endif
				// Выводим сообщение, что проверка пройдена
				return 1;
			// Если ресурс не найден тогда выводим сообщение об ошибке
			} else {
				// Получаем объект логирования
				awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::cookie::index[2]));
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					log->debug("%s for hostname '%s' [%s]", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, status.c_str(), layer->destination.hostname.c_str(), buffer);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					log->print("%s for hostname '%s' [%s]", awh::log_t::flag_t::WARNING, status.c_str(), layer->destination.hostname.c_str(), buffer);
				#endif
			}
		}
		// Выводим сообщение, что проверка не пройдена
		return 0;
	}
	/**
	 * @brief Функция обратного вызова для проверки валидности сертификата
	 *
	 * @param ok   результат получения сертификата
	 * @param x509 данные сертификата
	 * @return     результат проверки
	 */
	static int32_t certificate(const int32_t ok, X509_STORE_CTX * x509) noexcept {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Если проверка не выполнена
			if(!ok){
				// Получаем данные ошибки
				int32_t error = ::X509_STORE_CTX_get_error(x509);
				// Получаем глубину ошибки
				int32_t depth = ::X509_STORE_CTX_get_error_depth(x509);
				// Выполняем извлечение сертификата
				X509 * cert = ::X509_STORE_CTX_get_current_cert(x509);
				// Буфер данных для получения данных
				char buffer[256];
				// Выводим начальный разделитель
				printf("------------------------------------------------------------\n\n");
				// Выводим заголовок
				printf("Current certificate verification:\n");
				// Получаем название сертификата
				::X509_NAME_oneline(::X509_get_subject_name(cert), buffer, sizeof(buffer));
				// Выводим название сертификата
				printf("Subject: %s\n", buffer);
				// Получаем эмитента выпустившего сертификат
				::X509_NAME_oneline(::X509_get_issuer_name(cert), buffer, sizeof(buffer));
				// Выводим эмитента сертификата
				printf("Issuer: %s\n", buffer);
				// Выводим информацию о ошибке
				printf("Error: %s\n", ::X509_verify_cert_error_string(error));
				// Выводим конечный разделитель
				printf("\n------------------------------------------------------------\n\n");
				// Очищаем объект сертификата
				// ::X509_free(cert);
			}
		#endif
		// Выводим результат
		return ok;
	}
	/**
	 * @brief Функция формирования сообщения об ошибке
	 *
	 * @param id      идентификатор события
	 * @param message дополнительное сообщение
	 * @return        сформированное сообщение об ошибке
	 */
	static string error(const tls_t::id_t id, const string & message = "") noexcept {
		// Результат работы функции
		string result = "";
		// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
		auto layer = reinterpret_cast <::ssl_t *> (static_cast <uintptr_t> (id));
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Получаем данные описание ошибки
			uint64_t error = ::ERR_get_error();
			// Получаем объект фреймворка
			awh::fmk_t * fmk = reinterpret_cast <awh::fmk_t *> (::SSL_get_ex_data(layer->ssl, ::cookie::index[1]));
			// Если ошибка получена
			if(error != 0){
				// Получаем текст общего сообщения
				const string state = ::SSL_state_string(layer->ssl);
				/**
				 * Выполняем извлечение остальных ошибок
				 */
				do {
					// Если результат уже сформирован
					if(!result.empty())
						// Добавляем разделитель
						result.append("\n\n");
					// Если получено состояние SSL
					if(!state.empty())
						// Добавляем информацию об ошибке в результат
						result.append(fmk->format("%s: %s", state.c_str(), ::ERR_error_string(error, nullptr)));
					// Если получено дополнительное сообщение
					else if(!message.empty())
						// Добавляем информацию об ошибке в результат
						result.append(fmk->format("%s: %s", message.c_str(), ::ERR_error_string(error, nullptr)));
					// Если не получено ни состояние SSL, ни дополнительное сообщение
					else result.append(fmk->format("%s", ::ERR_error_string(error, nullptr)));
				/**
				 * Если ещё есть ошибки
				 */
				} while((error = ::ERR_get_error()));
			// Если получено дополнительное сообщение
			} else if(!message.empty())
				// Добавляем информацию об ошибке в результат
				result.append(fmk->format("%s", message.c_str()));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Получаем объект фреймворка
			awh::fmk_t * fmk = reinterpret_cast <awh::fmk_t *> (::SSL_get_ex_data(layer->ssl, ::cookie::index[1]));
			// Если получено дополнительное сообщение
			if(!message.empty())
				// Добавляем информацию об ошибке в результат
				result.append(fmk->format("%s: %s", message.c_str(), error.what()));
			// Если не получено ни состояние SSL, ни дополнительное сообщение
			else result.append(fmk->format("%s", error.what()));
		}
		// Выводим сформированное сообщение об ошибке
		return result;
	}
};

/**
 * @brief Метод получения версии протокола TLS
 *
 * @return версия протокола TLS
 */
string awh::TransportLayerSecurity::version() const noexcept {
	// Возвращаем версию OpenSSL
	return ::OpenSSL_version(OPENSSL_VERSION);
}
/**
 * @brief Метод извлечения активного протокола
 *
 * @param id идентификатор события
 * @return   метод активного протокола
 */
awh::TransportLayerSecurity::alpn_t awh::TransportLayerSecurity::alpn(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем извлечение активного протокола
		return reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id))->alpn;
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
	// Возвращаем значение по умолчанию
	return alpn_t::NONE;
}
/**
 * @brief Метод получения общей информации о TLS соединении
 *
 * @param id идентификатор события
 * @return   общая информация о TLS соединении
 */
string awh::TransportLayerSecurity::info(const id_t id) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if(i != ::__awh_ssl_ids__.end()){
			// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
			auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
			// Если версия OpenSSL не соответствует указанной при сборке
			if(::OpenSSL_version_num() != OPENSSL_VERSION_NUMBER){
				// Если функция обратного вызова ошибки установлена
				if(layer->error != nullptr){
					// Вызываем функцию обратного вызова ошибки
					layer->error(
						id, error_t::WARNING,
						this->_fmk->format(
							"OpenSSL version mismatch!\n"
							"Compiled against %s\n"
							"Linked against   %s",
							OPENSSL_VERSION_TEXT,
							::OpenSSL_version(OPENSSL_VERSION)
						)
					);
					// Если мажорная и минорная версия OpenSSL не совпадают
					if((::OpenSSL_version_num() >> 20) != (OPENSSL_VERSION_NUMBER >> 20))
						// Вызываем функцию обратного вызова ошибки
						layer->error(id, error_t::CRITICAL, "Major and minor version numbers must match, exiting");
				// Если функция обратного вызова ошибки не установлена
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"OpenSSL version mismatch!\n"
							"Compiled against %s\n"
							"Linked against   %s",
							__PRETTY_FUNCTION__,
							std::make_tuple(id),
							log_t::flag_t::WARNING,
							OPENSSL_VERSION_TEXT,
							::OpenSSL_version(OPENSSL_VERSION)
						);
						// Если мажорная и минорная версия OpenSSL не совпадают
						if((::OpenSSL_version_num() >> 20) != (OPENSSL_VERSION_NUMBER >> 20))
							// Выводим в лог сообщение
							this->_log->debug("Major and minor version numbers must match, exiting", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим в лог сообщение
						this->_log->print(
							"OpenSSL version mismatch!\r\n"
							"Compiled against %s\r\n"
							"Linked against   %s",
							log_t::flag_t::WARNING,
							OPENSSL_VERSION_TEXT,
							::OpenSSL_version(OPENSSL_VERSION)
						);
						// Если мажорная и минорная версия OpenSSL не совпадают
						if((::OpenSSL_version_num() >> 20) != (OPENSSL_VERSION_NUMBER >> 20))
							// Выводим в лог сообщение
							this->_log->print("Major and minor version numbers must match, exiting", log_t::flag_t::CRITICAL);
					#endif
				}
			// Если всё хорошо, формируем версию OpenSSL
			} else result.append(this->_fmk->format("Using %s\n\n", ::OpenSSL_version(OPENSSL_VERSION)));
			// Если версия OpenSSL ниже версии 1.1.1b
			if(OPENSSL_VERSION_NUMBER < 0x1010102fL){
				// Если функция обратного вызова ошибки установлена
				if(layer->error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					layer->error(
						id, error_t::CRITICAL,
						this->_fmk->format("%s is unsupported, use OpenSSL Version 1.1.1a or higher", ::OpenSSL_version(OPENSSL_VERSION))
					);
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим в лог сообщение
						this->_log->debug("%s is unsupported, use OpenSSL Version 1.1.1a or higher", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, ::OpenSSL_version(OPENSSL_VERSION));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим в лог сообщение
						this->_log->print("%s is unsupported, use OpenSSL Version 1.1.1a or higher", log_t::flag_t::CRITICAL, ::OpenSSL_version(OPENSSL_VERSION));
					#endif
				}
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			}
			// Если объект подключения создан и сертификат передан
			if(layer->ssl != nullptr){
				// Выполняем получение сертификата сервера
				X509 * x509 = ::SSL_get_peer_certificate(layer->ssl);
				// Если сертификат сервера получен
				if(x509 != nullptr){
					// Буфер данных для получения данных
					char buffer[256];
					// Получаем название сертификата
					::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
					// Формируем результат
					result = ::move(this->_fmk->format("%sPeer certificates:\nSubject: %s\n", result.c_str(), buffer));
					// Получаем эмитента выпустившего сертификат
					::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
					// Формируем результат
					result = ::move(this->_fmk->format("%sIssuer: %s\n", result.c_str(), buffer));
					// Выводим параметры шифрования
					result = ::move(this->_fmk->format("%sCipher: %s\n", result.c_str(), ::SSL_CIPHER_get_name(::SSL_get_current_cipher(layer->ssl))));
				}
			}
			// Если объект CRL-файла сертификата создан
			if(layer->crl != nullptr){
				// Создаём memory BIO
				BIO * bio = ::BIO_new(::BIO_s_mem());
				// Если BIO не создан
				if(bio == nullptr){
					// Если функция обратного вызова ошибки установлена
					if(layer->error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						layer->error(id, error_t::CRITICAL, ::verify::error(id, "Engine store CRL"));
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, ::verify::error(id, "Engine store CRL").c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::verify::error(id, "Engine store CRL").c_str());
						#endif
					}
					// Выходим из функции
					return result;
				}
				// Печатаем CRL в BIO
				if(::X509_CRL_print(bio, layer->crl) == 0){
					// Если функция обратного вызова ошибки установлена
					if(layer->error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						layer->error(id, error_t::CRITICAL, ::verify::error(id, "Engine store CRL"));
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, ::verify::error(id, "Engine store CRL").c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::verify::error(id, "Engine store CRL").c_str());
						#endif
					}
					// Выполняем очистку BIO
					::BIO_free(bio);
					// Выводим результат
					return result;
				}
				// Получаем размер данных
				char * data = nullptr;
				// Выполняем извлечение данных из BIO
				const size_t length = static_cast <size_t> (::BIO_get_mem_data(bio, &data));
				// Если информация получена
				if(length > 0)
					// Выводим параметры шифрования
					result = ::move(this->_fmk->format("%sCertificate Revocation List: %s\n", result.c_str(), string(data, length).c_str()));
				// Выполняем очистку BIO
				::BIO_free(bio);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения информации о списке отзыва сертификатов
 *
 * @param id идентификатор события
 * @return   информация о списке отзыва сертификатов
 */
string awh::TransportLayerSecurity::crlInfo(const id_t id) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if(i != ::__awh_ssl_ids__.end()){
			// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
			auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
			// Если объект CRL-файла сертификата создан
			if(layer->crl != nullptr){
				// Создаём memory BIO
				BIO * bio = ::BIO_new(::BIO_s_mem());
				// Если BIO не создан
				if(bio == nullptr){
					// Если функция обратного вызова ошибки установлена
					if(layer->error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						layer->error(id, error_t::CRITICAL, ::verify::error(id, "Engine store CRL"));
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, ::verify::error(id, "Engine store CRL").c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::verify::error(id, "Engine store CRL").c_str());
						#endif
					}
					// Выходим из функции
					return result;
				}
				// Печатаем CRL в BIO
				if(::X509_CRL_print(bio, layer->crl) == 0){
					// Если функция обратного вызова ошибки установлена
					if(layer->error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						layer->error(id, error_t::CRITICAL, ::verify::error(id, "Engine store CRL"));
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, ::verify::error(id, "Engine store CRL").c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::verify::error(id, "Engine store CRL").c_str());
						#endif
					}
					// Выполняем очистку BIO
					::BIO_free(bio);
					// Выводим результат
					return result;
				}
				// Получаем размер данных
				char * data = nullptr;
				// Выполняем извлечение данных из BIO
				const size_t length = static_cast <size_t> (::BIO_get_mem_data(bio, &data));
				// Если информация получена
				if(length > 0)
					// Выводим параметры шифрования
					result.assign(data, length);
				// Выполняем очистку BIO
				::BIO_free(bio);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения информации о шифре
 *
 * @param id идентификатор события
 * @return   информация о шифре
 */
string awh::TransportLayerSecurity::cipherInfo(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if(i != ::__awh_ssl_ids__.end())
			// Выполняем извлечение информации о шифре
			return ::SSL_CIPHER_get_name(::SSL_get_current_cipher(reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id))->ssl));
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
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if(i != ::__awh_ssl_ids__.end()){
			// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
			auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
			// Получить сертификат клиента (на сервере) или сервера (на клиенте)
			X509 * x509 = ::SSL_get_peer_certificate(layer->ssl);
			// Если сертификат получен
			if(x509 != nullptr){
				// Буфер данных для получения данных
				char buffer[256];
				// Получаем название сертификата
				::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
				// Формируем результат
				result = ::move(this->_fmk->format("Peer certificates:\nSubject: %s\n", buffer));
				// Получаем эмитента выпустившего сертификат
				::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
				// Формируем результат
				result = ::move(this->_fmk->format("%sIssuer: %s\n", result.c_str(), buffer));
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки валидности сертификата
 *
 * @param id идентификатор события
 * @return   результат проверки валидности сертификата
 */
bool awh::TransportLayerSecurity::validateCertificate(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if(i != ::__awh_ssl_ids__.end()){
			// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
			auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
			// Шаг 1: Получить сертификат
			X509 * x509 = ::SSL_get_peer_certificate(layer->ssl);
			// Если сертификат не получен
			if(x509 == nullptr)
				// Нет сертификата
				return false;
			// Получаем текущую дату и время
			time_t date = ::time(nullptr);
			// Если срок действия сертификата истёк
			if(!((::X509_cmp_time(::X509_get0_notBefore(x509), &date) <= 0) && (::X509_cmp_time(::X509_get0_notAfter(x509), &date) >= 0))){
				// Если функция обратного вызова ошибки установлена
				if(layer->error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					layer->error(id, error_t::WARNING, "Сertificate is not yet valid or has expired");
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Сertificate is not yet valid or has expired", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Сertificate is not yet valid or has expired", log_t::flag_t::WARNING);
					#endif
				}
				// Выводим сообщение, что сертификат ещё не действителен или просрочен
				return false;
			}
			// Получить хранилище CA из SSL_CTX
			X509_STORE * store = ::SSL_CTX_get_cert_store(::SSL_get_SSL_CTX(layer->ssl));
			// Создать контекст проверки
			X509_STORE_CTX * ctx = ::X509_STORE_CTX_new();
			// Если контекст не создан
			if(ctx == nullptr){
				// Если функция обратного вызова ошибки установлена
				if(layer->error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					layer->error(id, error_t::CRITICAL, ::verify::error(id));
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, ::verify::error(id).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::verify::error(id).c_str());
					#endif
				}
				// Возвращаем отрицательный результат
				return false;
			}
			// Инициализировать (x509 — сертификат пира, untrusted — промежуточные, если есть)
			if(::X509_STORE_CTX_init(ctx, store, x509, ::SSL_get_peer_cert_chain(layer->ssl)) == 0){
				// Выполняем очистку контекста
				::X509_STORE_CTX_free(ctx);
				// Если функция обратного вызова ошибки установлена
				if(layer->error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					layer->error(id, error_t::CRITICAL, ::verify::error(id));
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, ::verify::error(id).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::verify::error(id).c_str());
					#endif
				}
				// Возвращаем отрицательный результат
				return false;
			}
			// Запустить проверку
			const int32_t result = ::X509_verify_cert(ctx);
			// Проверить результат
			if(result <= 0){
				// Получаем код ошибки
				const int32_t error = ::X509_STORE_CTX_get_error(ctx);
				// Если функция обратного вызова ошибки установлена
				if(layer->error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					layer->error(id, error_t::CRITICAL, ::X509_verify_cert_error_string(error));
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, ::X509_verify_cert_error_string(error));
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::X509_verify_cert_error_string(error));
					#endif
				}
				// Выполняем очистку контекста
				::X509_STORE_CTX_free(ctx);
				// Возвращаем отрицательный результат
				return false;
			}
			// Выполняем очистку контекста
			::X509_STORE_CTX_free(ctx);
			// Проверка по Subject Alternative Name (SAN) или Common Name (CN)
			bool ok = false;
			// Извлекаем SAN из сертификата
			GENERAL_NAMES * san = reinterpret_cast <GENERAL_NAMES *> (::X509_get_ext_d2i(x509, NID_subject_alt_name, nullptr, nullptr));
			// Если SAN присутствует
			if(san != nullptr){
				// Полученное доменное имя
				string fqdn = "";
				// Проверяем каждый элемент SAN
				for(int32_t i = 0; i < sk_GENERAL_NAME_num(san); i++){
					// Извлекаем элемент SAN
					const GENERAL_NAME * cn = sk_GENERAL_NAME_value(san, i);
					// Проверяем тип имени
					if(cn->type == GEN_DNS){
						// Формируем строковое представление доменного имени
						fqdn.assign(reinterpret_cast <char *> (const_cast <uint8_t *> (::ASN1_STRING_get0_data(cn->d.dNSName))), ::ASN1_STRING_length(cn->d.dNSName));
						// Если размер имени и dns имя совпадает
						if((ok = ::verify::certHostcheck(layer->destination.hostname, fqdn)))
							// Выходим из цикла
							break;
					}
				}
				// Выполняем очистку SAN
				::GENERAL_NAMES_free(san);
			// Если SAN отсутствует или имя не совпало
			} else {
				// Буфер данных для получения данных
				char buffer[256];
				// Fallback на Common Name (устаревшее, но иногда нужно)
				X509_NAME * subject = ::X509_get_subject_name(x509);
				// Если удалось получить Common Name
				if(::X509_NAME_get_text_by_NID(subject, NID_commonName, buffer, sizeof(buffer)) == 1)
					// Если размер имени и dns имя совпадает
					ok = ::verify::certHostcheck(layer->destination.hostname, buffer);
			}
			// Возвращаем результат проверки
			return ok;
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
	return false;
}
/**
 * @brief Метод установки проверки хоста сервера
 *
 * @param id   идентификатор события
 * @param mode режим проверки хоста сервера
 */
void awh::TransportLayerSecurity::validateHostname(const id_t id, const bool mode) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if(i != ::__awh_ssl_ids__.end()){
			// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
			auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
			/**
			 * Определяем узел события к которому относится контекст TLS
			 */
			switch(static_cast <uint8_t> (layer->node)){
				// Если узел является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT): {
					// Если нужно произвести проверку
					if(mode){
						// Выполняем проверку сертификата
						::SSL_CTX_set_verify(layer->ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, nullptr);
						// Выполняем проверку всех дочерних сертификатов
						::SSL_CTX_set_cert_verify_callback(layer->ctx, &::verify::hostname, layer->ssl);
						// Устанавливаем глубину проверки
						::SSL_CTX_set_verify_depth(layer->ctx, 4);
					// Запрещаем выполнять првоерку доменного имени
					} else {
						// Устанавливаем проверку сертификата сервера
						::SSL_CTX_set_verify(layer->ctx, SSL_VERIFY_PEER, nullptr);
						// Устанавливаем пути по умолчанию для проверки сертификатов
						::SSL_CTX_set_default_verify_paths(layer->ctx);
						// Отключаем проверку сертификата сервера
						::SSL_CTX_set_verify(layer->ctx, SSL_VERIFY_NONE, nullptr);
					}
				} break;
				// Если узел является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Если нужно произвести проверку
					if(mode){
						// Устанавливаем глубину проверки
						::SSL_CTX_set_verify_depth(layer->ctx, 2);
						// Выполняем проверку сертификата клиента
						::SSL_CTX_set_verify(layer->ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, &::verify::certificate);
					// Запрещаем выполнять првоерку доменного имени
					} else {
						// Устанавливаем проверку сертификата сервера
						::SSL_CTX_set_verify(layer->ctx, SSL_VERIFY_PEER, nullptr);
						// Отключаем проверку сертификата сервера
						::SSL_CTX_set_verify(layer->ctx, SSL_VERIFY_NONE, nullptr);
					}
				} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, mode), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки имени хоста сервера
 *
 * @param id       идентификатор события
 * @param hostname имя хоста сервера
 */
void awh::TransportLayerSecurity::setHostname(const id_t id, const string & hostname) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если имя хоста сервера не пустое
		if(!hostname.empty()){
			// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
			auto i = ::__awh_ssl_ids__.find(id);
			// Если идентификатор контекста TLS найден
			if(i != ::__awh_ssl_ids__.end()){
				// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
				auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
				// Устанавливаем хост для уровня защищённых сокетов
				layer->destination.hostname = hostname;
				// Устанавливаем имя хоста для SNI расширения
				::SSL_set_tlsext_host_name(layer->ssl, layer->destination.hostname.c_str());
				/**
				 * Если версия OpenSSL соответствует или выше версии 1.1.1
				 */
				#if OPENSSL_VERSION_NUMBER >= 0x10101000L
					// Устанавливаем имя хоста для проверки
					::SSL_set1_host(layer->ssl, layer->destination.hostname.c_str());
				#endif
				// Активируем верификацию доменного имени
				if(::X509_VERIFY_PARAM_set1_host(::SSL_get0_param(layer->ssl), layer->destination.hostname.c_str(), 0) != 1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Host SSL verification failed", __PRETTY_FUNCTION__, std::make_tuple(id, hostname), log_t::flag_t::CRITICAL);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Host SSL verification failed", log_t::flag_t::CRITICAL);
					#endif
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, hostname), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки адреса и порта сервера назначения
 *
 * @param id   идентификатор события
 * @param ip   IP-адрес сервера
 * @param port порт сервера
 * @return     результат выполнения установки
 */
bool awh::TransportLayerSecurity::destination(const id_t id, const string & ip, const uint16_t port) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если IP-адрес сервера не пустой и порт сервера задан верно
		if((!ip.empty()) && (port > 0)){
			// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
			auto i = ::__awh_ssl_ids__.find(id);
			// Если идентификатор контекста TLS найден
			if(i != ::__awh_ssl_ids__.end()){
				// Выполняем парсинг I-адреса
				if(this->_addr.parse(ip)){
					// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
					auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
					// Выполняем инициализацию объекта хоста IPv4-адреса
					layer->destination.address = make_unique <net::attr_net_t> ();
					// Получаем объект хоста IPv4-адреса
					net::attr_net_t * address = awh_cast <net::attr_net_t *> (layer->destination.address.get());
					/**
					 * Выполняем определение типа IP-адреса
					 */
					switch(static_cast <uint8_t> (this->_addr.type())){
						// Для IPv4-адреса
						case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
							// Выполняем инициализацию объекта IP-адреса
							address->ip = make_unique <net::addr_net_ipv4_t> ();
							// Устанавливаем порт
							address->port = port;
							// Устанавливаем IP-адрес
							awh_cast <net::addr_net_ipv4_t *> (address->ip.get())->address = this->_addr.v4(net_addr_t::endian_t::LITTLE);
						} break;
						// Для IPv6-адреса
						case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
							// Выполняем инициализацию объекта IP-адреса
							address->ip = make_unique <net::addr_net_ipv6_t> ();
							// Устанавливаем порт
							address->port = port;
							// Устанавливаем полученный IP-адрес
							awh_cast <net::addr_net_ipv6_t *> (address->ip.get())->address = ::move(this->_addr.v6(net_addr_t::endian_t::LITTLE));
						} break;
						// Для других типов адресов
						default: {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Unsupported IP-address type", __PRETTY_FUNCTION__, std::make_tuple(id, ip, port), log_t::flag_t::CRITICAL);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Unsupported IP-address type", log_t::flag_t::CRITICAL);
							#endif
							// Возвращаем отрицательный результат
							return false;
						}
					}
				// Если парсинг IP-адреса не выполнен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Failed to parse IP-address", __PRETTY_FUNCTION__, std::make_tuple(id, ip, port), log_t::flag_t::CRITICAL);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Failed to parse IP-address", log_t::flag_t::CRITICAL);
					#endif
					// Возвращаем отрицательный результат
					return false;
				}
				// Выводим положительный результат
				return true;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, ip, port), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем отрицательный результат
	return false;
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
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if(i != ::__awh_ssl_ids__.end())
			// Устанавливаем режим безопасности потоков
			reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id))->mtx.enabled = (mode == event::mode_t::ENABLED);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки алгоритмов шифрования
 *
 * @param id      идентификатор события
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::TransportLayerSecurity::ciphers(const id_t id, const vector <string> & ciphers) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список алгоритмов шифрования не пустой
		if(!ciphers.empty()){
			// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
			auto i = ::__awh_ssl_ids__.find(id);
			// Если идентификатор контекста TLS найден
			if(i != ::__awh_ssl_ids__.end()){
				// Результирующая строка алгоритмов шифрования
				string result = "";
				// Формируем строку алгоритмов шифрования
				for(const auto & cipher : ciphers){
					// Если строка алгоритмов шифрования не пустая
					if(!result.empty())
						// Добавляем разделитель алгоритмов шифрования
						result.append(__AWH_TLS_CIPHER_SEPARATOR__);
					// Добавляем алгоритм шифрования в строку алгоритмов шифрования
					result.append(cipher);
				}
				// Если строка алгоритмов шифрования собрана
				if(!result.empty()){
					// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
					auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
					// Устанавливаем все основные алгоритмы шифрования
					if(::SSL_CTX_set_cipher_list(layer->ctx, result.c_str()) != 1){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("Set SSL ciphers: %s", __PRETTY_FUNCTION__, std::make_tuple(id, ciphers.size()), log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("Set SSL ciphers: %s", log_t::flag_t::CRITICAL, ::ERR_error_string(::ERR_get_error(), nullptr));
						#endif
					}
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, ciphers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки списка отзыва сертификатов
 *
 * @param id       идентификатор события
 * @param filename адрес файла списка отзыва сертификатов
 */
void awh::TransportLayerSecurity::crl(const id_t id, const string & filename) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес файла сертификата не пустой
		if(!filename.empty()){
			// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
			auto i = ::__awh_ssl_ids__.find(id);
			// Если идентификатор контекста TLS найден
			if(i != ::__awh_ssl_ids__.end()){
				// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
				auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
				// Если CRL-файл сертификата уже создан
				if(layer->crl != nullptr)
					// Выполняем освобождение памяти
					::X509_CRL_free(layer->crl);
				// Создаём объект BIO для загрузки файла
				BIO * bio = ::BIO_new(::BIO_s_file());
				// Если BIO не создан
				if(bio == nullptr){
					// Если функция обратного вызова ошибки установлена
					if(layer->error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						layer->error(id, error_t::CRITICAL, ::verify::error(id, "Engine store CRL"));
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, ::verify::error(id, "Engine store CRL").c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::verify::error(id, "Engine store CRL").c_str());
						#endif
					}
					// Выходим из функции
					return;
				}
				// Выполняем чтение CRL-файла сертификата
				if(BIO_read_filename(bio, filename.c_str()) <= 0){
					// Если функция обратного вызова ошибки установлена
					if(layer->error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						layer->error(id, error_t::CRITICAL, ::verify::error(id, "CRL-file is corrupted or unreadable"));
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, ::verify::error(id, "CRL-file is corrupted or unreadable").c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, ::verify::error(id, "CRL-file is corrupted or unreadable").c_str());
						#endif
					}
					// Выполняем очистку памяти BIO
					::BIO_free(bio);
					// Выходим из функции
					return;
				}
				// Выполняем создание объекта CRL-файла сертификата
				layer->crl = ::PEM_read_bio_X509_CRL(bio, nullptr, nullptr, nullptr);
				// Если CRL-файл сертификата не создан
				if(layer->crl == nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("CRL file cannot be set", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("CRL file cannot be set", log_t::flag_t::CRITICAL);
					#endif
				}
				// Выполняем очистку памяти BIO
				::BIO_free(bio);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки приватного ключа клиента
 *
 * @param id       идентификатор события
 * @param filename адрес файла приватного ключа клиента
 */
void awh::TransportLayerSecurity::privateKey(const id_t id, const string & filename) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес файла сертификата не пустой
		if(!filename.empty()){
			// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
			auto i = ::__awh_ssl_ids__.find(id);
			// Если идентификатор контекста TLS найден
			if(i != ::__awh_ssl_ids__.end()){
				// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
				auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
				// Если приватный ключ не может быть установлен
				if(::SSL_CTX_use_PrivateKey_file(layer->ctx, filename.c_str(), SSL_FILETYPE_PEM) != 1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Private key cannot be set", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Private key cannot be set", log_t::flag_t::CRITICAL);
					#endif
					// Выходим
					return;
				}
				// Если приватный ключ недействителен
				if(::SSL_CTX_check_private_key(layer->ctx) != 1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Private key is not valid", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Private key is not valid", log_t::flag_t::CRITICAL);
					#endif
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки клиентского сертификата
 *
 * @param id       идентификатор события
 * @param filename адрес файла клиентского сертификата
 */
void awh::TransportLayerSecurity::certificate(const id_t id, const string & filename) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес файла сертификата не пустой
		if(!filename.empty()){
			// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
			auto i = ::__awh_ssl_ids__.find(id);
			// Если идентификатор контекста TLS найден
			if(i != ::__awh_ssl_ids__.end()){
				// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
				auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
				/**
				 * Определяем узел события к которому относится контекст TLS
				 */
				switch(static_cast <uint8_t> (layer->node)){
					// Если узел является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Если сертификат не устанавливается
						if(::SSL_CTX_use_certificate_file(layer->ctx, filename.c_str(), SSL_FILETYPE_PEM) != 1){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Certificate cannot be set", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Certificate cannot be set", log_t::flag_t::CRITICAL);
							#endif
						}
					} break;
					// Если узел является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Если сертификат не устанавливается
						if(::SSL_CTX_use_certificate_chain_file(layer->ctx, filename.c_str()) != 1){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("Certificate cannot be set", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL);
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("Certificate cannot be set", log_t::flag_t::CRITICAL);
							#endif
						}
					} break;
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки сертификатов доверенных центров сертификации
 *
 * @param id       идентификатор события
 * @param filename адрес файла сертификата доверенных центров сертификации
 */
void awh::TransportLayerSecurity::ca(const id_t id, const string & filename) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес файла центра сертификации не пустой
		if(!filename.empty()){
			// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
			auto i = ::__awh_ssl_ids__.find(id);
			// Если идентификатор контекста TLS найден
			if(i != ::__awh_ssl_ids__.end()){
				// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
				auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
				// Выполняем проверку
				if(::SSL_CTX_load_verify_locations(layer->ctx, filename.c_str(), nullptr) != 1){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("SSL verify locations is not allow", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("SSL verify locations is not allow", log_t::flag_t::CRITICAL);
					#endif
					// Выходим из функции
					return;
				}
				// Выполняем установку CRL-файла сертификата
				::SSL_CTX_set_client_CA_list(layer->ctx, ::SSL_load_client_CA_file(filename.c_str()));
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки сертификатов доверенных центров сертификации
 *
 * @param id   идентификатор события
 * @param dir  адрес директории с сертификатами доверенных центров сертификации
 * @param file адрес файла сертификата доверенного центра сертификации
 */
void awh::TransportLayerSecurity::ca(const id_t id, const string & dir, const string & file) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if(i != ::__awh_ssl_ids__.end()){
			// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
			auto layer = reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id));
			// Если название файла центра сертификации не пустое
			if(!file.empty()){
				// Если каталог сертификатов передан
				if(!dir.empty()){
					// Выполняем проверку
					if(::SSL_CTX_load_verify_locations(layer->ctx, file.c_str(), dir.c_str()) != 1){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("SSL verify locations is not allow", __PRETTY_FUNCTION__, std::make_tuple(id, dir, file), log_t::flag_t::CRITICAL);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("SSL verify locations is not allow", log_t::flag_t::CRITICAL);
						#endif
						// Выходим из функции
						return;
					}
					// Полный адрес файла центра сертификации
					string filename = "";
					// Если последний символ каталога является разделителем
					if(dir.back() == AWH_FS_SEPARATOR[0])
						// Формируем полный адрес файла центра сертификации
						filename = ::move(this->_fmk->format("%s%s", dir.c_str(), file.c_str()));
					// Формируем полный адрес файла центра сертификации
					else filename = ::move(this->_fmk->format("%s%s%s", dir.c_str(), AWH_FS_SEPARATOR, file.c_str()));
					// Выполняем установку CRL-файла сертификата
					::SSL_CTX_set_client_CA_list(layer->ctx, ::SSL_load_client_CA_file(filename.c_str()));
				// Если каталог сертификатов не передан
				} else {
					// Выполняем проверку
					if(::SSL_CTX_load_verify_locations(layer->ctx, file.c_str(), nullptr) != 1){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("SSL verify locations is not allow", __PRETTY_FUNCTION__, std::make_tuple(id, file), log_t::flag_t::CRITICAL);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("SSL verify locations is not allow", log_t::flag_t::CRITICAL);
						#endif
						// Выходим из функции
						return;
					}
					// Выполняем установку CRL-файла сертификата
					::SSL_CTX_set_client_CA_list(layer->ctx, ::SSL_load_client_CA_file(file.c_str()));
				}
			// Если адрес файла центра сертификации не передан
			} else {
				// Получаем данные стора
				X509_STORE * store = ::SSL_CTX_get_cert_store(layer->ctx);
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Проверяем существует ли путь
					if(!::ssl::addCertToStore(store, "CA", this->_log) ||
					   !::ssl::addCertToStore(store, "ROOT", this->_log) ||
					   !::ssl::addCertToStore(store, "AuthRoot", this->_log))
						// Выходим из функции
						return;
				#endif
				// Если стор не устанавливается, тогда выводим ошибку
				if(::X509_STORE_set_default_paths(store) == 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("Set default paths for x509 store is not allow", __PRETTY_FUNCTION__, std::make_tuple(id, dir, file), log_t::flag_t::CRITICAL);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("Set default paths for x509 store is not allow", log_t::flag_t::CRITICAL);
					#endif
				}
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки функции обратного вывода получения ошибок
 *
 * @param id       идентификатор события
 * @param callback объект функции обратного вызова
 * @return        результат установки функции обратного вызова
 */
bool awh::TransportLayerSecurity::error(const id_t id, error_callback_t callback) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if((result = (i != ::__awh_ssl_ids__.end())))
			// Устанавливаем функцию обратного вызова получения ошибок
			reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id))->error = ::move(callback);
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
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if((result = (i != ::__awh_ssl_ids__.end()))){
			// Удаляем идентификатор контекста TLS из глобального набора идентификаторов контекстов TLS
			::__awh_ssl_ids__.erase(i);
			// Удаляем контекст TLS из контейнера уровней защищённых сокетов
			reinterpret_cast <SecureSocketsLayer *> (static_cast <uintptr_t> (id))->erase(::__awh_ssl_layers__);
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
		// Отключаем режим безопасности потоков по умолчанию
		(* ret.first)->mtx.enabled = false;
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
				if(::SSL_CTX_set_cipher_list((* ret.first)->ctx, ::__awh_ssl_ciphers__.c_str()) != 1){
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
				// Привязываем текущий объект TLS к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::cookie::index[0], (* ret.first).get());
				// Привязываем текущий объект фреймворка к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::cookie::index[1], const_cast <fmk_t *> (this->_fmk));
				// Привязываем текущий объект лога к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::cookie::index[2], const_cast <log_t *> (this->_log));
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
				// Сохраняем идентификатор контекста TLS в глобальном наборе идентификаторов контекстов TLS
				::__awh_ssl_ids__.emplace(result);
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
				if(::SSL_CTX_set_cipher_list((* ret.first)->ctx, ::__awh_ssl_ciphers__.c_str()) != 1){
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
				if(::SSL_CTX_set_session_id_context((* ret.first)->ctx, reinterpret_cast <const uint8_t *> (&result), sizeof(result)) != 1){
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
				if(SSL_CTX_set_ecdh_auto((* ret.first)->ctx, 1) != 1){
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
				// Выполняем отключение SSL кеша
				// ::SSL_CTX_set_session_cache_mode((* ret.first)->ctx, SSL_SESS_CACHE_OFF);
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
				/**
				 * Определяем тип протокола подключения
				 */
				switch(static_cast <uint8_t> (proto)){
					// Если протокол подключения UDP
					case static_cast <uint8_t> (event::protocol_t::UDP): {
						// Выполняем проверку файлов печенок
						::SSL_CTX_set_cookie_verify_cb((* ret.first)->ctx, &::cookie::verifyCookie);
						// Выполняем генерацию файлов печенок
						::SSL_CTX_set_cookie_generate_cb((* ret.first)->ctx, &::cookie::generateCookie);
					} break;
					// Если протокол подключения TCP
					case static_cast <uint8_t> (event::protocol_t::TCP):
					// Если протокол подключения SCTP
					case static_cast <uint8_t> (event::protocol_t::SCTP): {
						// Выполняем проверку файлов печенок
						::SSL_CTX_set_stateless_cookie_verify_cb((* ret.first)->ctx, &::cookie::verifyStatelessCookie);
						// Выполняем генерацию файлов печенок
						::SSL_CTX_set_stateless_cookie_generate_cb((* ret.first)->ctx, &::cookie::generateStatelessCookie);
					} break;
				}
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
				// Включаем обмен куками
				::SSL_set_options((* ret.first)->ssl, SSL_OP_COOKIE_EXCHANGE);
				// Привязываем текущий объект TLS к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::cookie::index[0], (* ret.first).get());
				// Привязываем текущий объект фреймворка к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::cookie::index[1], const_cast <fmk_t *> (this->_fmk));
				// Привязываем текущий объект лога к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::cookie::index[2], const_cast <log_t *> (this->_log));
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
				// Сохраняем идентификатор контекста TLS в глобальном наборе идентификаторов контекстов TLS
				::__awh_ssl_ids__.emplace(result);
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
awh::TransportLayerSecurity::TransportLayerSecurity(const fmk_t * fmk, const log_t * log) noexcept : _addr(fmk, log), _fmk(fmk), _log(log) {
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
		if(::RAND_poll() != 1){
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
		// Регистрируем новый индекс для хранения пользовательских данных в структуре SSL
		::cookie::index[0] = ::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
		// Регистрируем новый индекс для хранения объекта фреймворка AWH в структуре SSL
		::cookie::index[1] = ::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
		// Регистрируем новый индекс для хранения объекта логирования AWH в структуре SSL
		::cookie::index[2] = ::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
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
