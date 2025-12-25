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
 * Если максимальный размер SSL буфера не определён
 */
#ifndef AWH_MAX_SSL_BUFFER_SIZE
	/**
	 * Устанавливаем максимальный размер SSL буфера в 16 КБ
	 */
	#define AWH_MAX_SSL_BUFFER_SIZE 0x4000
#endif

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * @brief Прототип класса участника уровня защищённых сокетов
	 *
	 */
	class Member;
	/**
	 * @brief Тип контейнера участников уровня защищённых сокетов
	 *
	 */
	using members_t = unordered_set <unique_ptr <Member>>;

	/**
	 * @brief Глобальный список алгоритмов шифрования
	 *
	 */
	string __awh_ssl_ciphers__;
	/**
	 * @brief Глобальный контейнер уровней защищённых сокетов
	 *
	 */
	members_t __awh_ssl_members__;
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
	/**
	 * @brief Мьютекс для синхронизации потоков
	 *
	 */
	awh::lock_state_t <mutex> __awh_ssl_mtx__;
	/**
	 * @brief Индексы для хранения состояний проверки куков
	 *
	 */
	int32_t __awh_ssl_index__[3] = {-1, -1, -1};
};

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

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
		explicit Cookie() noexcept :
		 buffer{0}, initialized(false) {}
	} cookie_t;

	/**
	 * @brief Структура хоста
	 *
	 */
	typedef struct Host {
		// Имя хоста
		string name;
		// Параметры однорангового узла
		unique_ptr <net::attr_net_t> peer;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Host() noexcept :
		 name{""}, peer(nullptr) {}
	} host_t;

	/**
	 * @brief Структура ALPN-протоколов
	 *
	 */
	typedef struct ALPN {
		// Идентификатор выбранного ALPN-протокола
		uint8_t id;
		// Список идентификаторов поддерживаемых ALPN-протоколов
		vector <uint8_t> ids;
		// Буфер ALPN-протоколов
		vector <uint8_t> buffer;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit ALPN() noexcept : id(0) {}
	} alpn_t;

	/**
	 * @brief Структура состояния
	 *
	 */
	typedef struct State {
		// Флаг выполнения рукопожатия SSL
		bool handshake;
		// Флаг выполнения проверки сертификата
		bool certificate;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit State() noexcept :
		 handshake(false), certificate(true) {}
	} __attribute__((packed)) state_t;

	/**
	 * @brief Структура обратных вызовов
	 *
	 */
	typedef struct Callback {
		// Функция обратного вызова чтения данных
		tls_t::read_callback_t read;
		// Функция обратного вызова записи данных
		tls_t::write_callback_t write;
		// Функция обратного вызова получения ошибок
		tls_t::error_callback_t error;
		// Функция обратного вызова завершения рукопожатия
		tls_t::handshake_callback_t handshake;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Callback() noexcept :
		 read(nullptr), write(nullptr),
		 error(nullptr), handshake(nullptr) {}
	} callback_t;

	/**
	 * @brief Класс участника обмена защищёнными данными
	 *
	 */
	typedef class Member {
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
			// Объект ALPN-протоколов
			alpn_t alpn;
			// Объект хоста
			host_t host;
			// Объект состояния
			state_t state;
			// Объект печенок SSL
			cookie_t cookie;
			// Объект обратных вызовов
			callback_t callback;
			// Тип узла события
			event::node_t node;
			// Тип протокола события
			event::protocol_t proto;
			// Итератор уровня защищённых сокетов
			members_t::iterator iterator;
			// Мьютекс для синхронизации потоков
			lock_state_t <recursive_mutex> mtx;
		public:
			/**
			 * @brief Метод удаления уровня защищённых сокетов
			 *
			 * @param members контейнер уровней защищённых сокетов
			 */
			void erase(members_t & members) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Member() noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Member() noexcept;
	} member_t;

	/**
	 * @brief Метод удаления уровня защищённых сокетов
	 *
	 * @param members контейнер уровней защищённых сокетов
	 */
	void Member::erase(members_t & members) noexcept {
		// Выполняем блокировку потоков
		const locker_t lock(::__awh_ssl_mtx__);
		// Удаляем уровень защищённых сокетов из контейнера
		members.erase(this->iterator);
	}
	/**
	 * @brief Конструктор
	 *
	 */
	Member::Member() noexcept :
	 ssl(nullptr),
	 rbio(nullptr), wbio(nullptr),
	 ctx(nullptr), crl(nullptr),
	 node(event::node_t::NONE),
	 proto(event::protocol_t::NONE) {}
	/**
	 * @brief Деструктор
	 *
	 */
	Member::~Member() noexcept {
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
};

/**
 * Инкапсулируем методы TLS в пространство имён
 */
namespace ssl {
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
		auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Получаем данные описание ошибки
			uint64_t error = ::ERR_get_error();
			// Получаем объект фреймворка
			awh::fmk_t * fmk = reinterpret_cast <awh::fmk_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[1]));
			// Если ошибка получена
			if(error != 0){
				// Буфер данных для получения сообщения об ошибке
				char buffer[256];
				// Получаем текст общего сообщения
				const string state = ::SSL_state_string(member->ssl);
				/**
				 * Выполняем извлечение остальных ошибок
				 */
				do {
					// Зануляем буфер данных
					::memset(buffer, 0, sizeof(buffer));
					// Получаем сообщение об ошибке
					::ERR_error_string_n(error, buffer, sizeof(buffer));
					// Если результат уже сформирован
					if(!result.empty())
						// Добавляем разделитель
						result.append("\n\n");
					// Если получено состояние SSL
					if(!state.empty())
						// Добавляем информацию об ошибке в результат
						result.append(fmk->format("%s: %s", state.c_str(), buffer));
					// Если получено дополнительное сообщение
					else if(!message.empty())
						// Добавляем информацию об ошибке в результат
						result.append(fmk->format("%s: %s", message.c_str(), buffer));
					// Если не получено ни состояние SSL, ни дополнительное сообщение
					else result.append(fmk->format("%s", buffer));
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
			awh::fmk_t * fmk = reinterpret_cast <awh::fmk_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[1]));
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
	static bool selectProto(uint8_t ** out, uint8_t * outSize, const uint8_t * in, const uint8_t inSize, const uint8_t * key, const uint8_t keySize) noexcept {
		// Результат работы функции
		bool result = false;
		// Выполняем перебор всех данных в входящем буфере
		for(uint8_t i = 0; (i + keySize) <= inSize; i += static_cast <uint8_t> (in[i] + 1)){
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
				auto member = reinterpret_cast <::member_t *> (ctx);
				// Выполняем установку буфера данных
				(* data) = member->alpn.buffer.data();
				// Выполняем установку размер буфера данных протокола
				(* len) = static_cast <uint32_t> (member->alpn.buffer.size());
				// Выводим результат
				return SSL_TLSEXT_ERR_OK;
			}
			// Выводим результат
			return SSL_TLSEXT_ERR_NOACK;
		}
		/**
		 * @brief Функция обратного вызова клиента для расширения ALPN TLS
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
				// Размер и индекс протокола
				uint8_t size = 0, index = 0;
				// Получаем объект контекста модуля
				auto member = reinterpret_cast <::member_t *> (ctx);
				// Выполняем перебор всех поддерживаемых протоколов
				for(uint8_t i = 0; i < member->alpn.buffer.size(); i++){
					// Получаем размер протокола
					size = member->alpn.buffer[i];
					// Выполняем выбор протокола из входящего буфера
					if(::ssl::selectProto(out, outSize, in, static_cast <uint8_t> (inSize), &member->alpn.buffer[i], size + 1)){
						// Выполняем переключение на выбранный протокол
						member->alpn.id = member->alpn.ids[index];
						// Выводим результат
						return SSL_TLSEXT_ERR_OK;
					}
					// Переходим к следующему протоколу
					i += size;
					// Увеличиваем индекс протокола
					index++;
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
		 * @brief Функция обратного вызова сервера для расширения ALPN TLS
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
				// Размер и индекс протокола
				uint8_t size = 0, index = 0;
				// Получаем объект контекста модуля
				auto member = reinterpret_cast <::member_t *> (ctx);
				// Выполняем перебор всех поддерживаемых протоколов
				for(uint8_t i = 0; i < member->alpn.buffer.size(); i++){
					// Получаем размер протокола
					size = member->alpn.buffer[i];
					// Выполняем выбор протокола из входящего буфера
					if(::ssl::selectProto(const_cast <uint8_t **> (out), outSize, in, static_cast <uint8_t> (inSize), &member->alpn.buffer[i], size + 1)){
						// Выполняем переключение на выбранный протокол
						member->alpn.id = member->alpn.ids[index];
						// Выводим результат
						return SSL_TLSEXT_ERR_OK;
					}
					// Переходим к следующему протоколу
					i += size;
					// Увеличиваем индекс протокола
					index++;
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
		 * @param store  магазин с сертификатами для работы
		 * @param name   название параметра сертификата
		 * @param member объект уровня защищённых сокетов
		 * @return       результат проверки
		 */
		static bool addCertToStore(X509_STORE * store, const char * name, const ::member_t * member) noexcept {
			// Результат работы функции
			bool result = false;
			// Если объекты переданы верно
			if((store != nullptr) && (name != nullptr)){
				// Получаем данные системного стора
				HCERTSTORE sys = ::CertOpenSystemStore(0, name);
				// Если системный стор не получен
				if(!(result = (sys != nullptr))){
					// Выполняем получение идентификатора контекста TLS
					const uint64_t id = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (member));
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Failed to open system certificate store");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, awh::tls_t::error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						// Получаем объект логирования
						awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[2]));
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
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
					X509 * x509 = X509_new();
					// Если сертификат создан удачно
					if((result = (x509 != nullptr))){
						// Получаем объект закодированного сертификата
						const BYTE * encoded = ctx->pbCertEncoded;
						// Добавляем сертификат в стор
						::X509_STORE_add_cert(store, ::d2i_X509(&x509, reinterpret_cast <const uint8_t **> (&encoded), ctx->cbCertEncoded));
						// Очищаем выделенную память
						::X509_free(x509);
					// Если сертификат не создан
					} else {
						// Выполняем получение идентификатора контекста TLS
						const uint64_t id = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (member));
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Create X509 is failed");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, awh::tls_t::error_t::CRITICAL, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							// Получаем объект логирования
							awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[2]));
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
							#endif
						}
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
 * Для операционной системы Linux или FreeBSD
 */
#if __linux__ || __FreeBSD__
	/**
	 * Инкапсулируем методы SCTP в пространство имён
	 */
	namespace sctp {
		/**
		 * @brief Функция обработки нотификации SCTP
		 *
		 * @param bio    объект подключения BIO
		 * @param ctx    промежуточный передаваемый контекст
		 * @param buffer буфер передаваемых данных
		 */
		static void notifications(BIO * bio, void * ctx, void * buffer) noexcept {
			// Если данные переданы
			if((bio != nullptr) && (ctx != nullptr) && (buffer != nullptr)){
				// Получаем объект контекста модуля
				auto member = reinterpret_cast <::member_t *> (ctx);
				// Создаём объект событий SCTP
				union sctp_notification * snp = reinterpret_cast <union sctp_notification *> (buffer);
				// Получаем объект логирования
				awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[2]));
				/**
				 * Определяем тип события
				 */
				switch(snp->sn_header.sn_type){
					// Если произошло событие изменения ассоциации
					case SCTP_ASSOC_CHANGE: {
						// Получаем ассоциацию
						struct sctp_assoc_change * sac = &snp->sn_assoc_change;
						// Выводим в лог информационное сообщение
						log->print("Assoc_change: STATE=%hu, ERROR=%hu, INPUT=%hu, OUTPUT=%hu", awh::log_t::flag_t::INFO, sac->sac_state, sac->sac_error, sac->sac_inbound_streams, sac->sac_outbound_streams);
					} break;
					// Если изменился адрес подключения клиента
					case SCTP_PEER_ADDR_CHANGE: {
						// Получаем данные изменившегося адреса
						struct sctp_paddr_change * spc = &snp->sn_paddr_change;
						// Получаем объект хоста IPv4-адреса
						net::attr_net_t * address = awh_cast <net::attr_net_t *> (member->host.peer.get());
						/**
						 * Определяем тип адреса
						 */
						switch(address->ip->size){
							// Если адрес является IPv4
							case 4: {
								// Объект данных подключения
								char buffer[INET_ADDRSTRLEN];
								// Выполняем зануление буфера данных
								::memset(buffer, 0, sizeof(buffer));
								// Выводим в лог информационное сообщение
								log->print("Intf change: IP=%s, STATE=%d, ERROR=%d", awh::log_t::flag_t::INFO, ::inet_ntop(AF_INET, &awh_cast <net::addr_net_ipv4_t *> (address->ip.get())->address, buffer, sizeof(buffer)), spc->spc_state, spc->spc_error);
							} break;
							// Если адрес является IPv6
							case 16: {
								// Объект данных подключения
								char buffer[INET6_ADDRSTRLEN];
								// Выполняем зануление буфера данных
								::memset(buffer, 0, sizeof(buffer));
								// Выводим в лог информационное сообщение
								log->print("Intf change: IP=%s, STATE=%d, ERROR=%d", awh::log_t::flag_t::INFO, ::inet_ntop(AF_INET6, &awh_cast <net::addr_net_ipv6_t *> (address->ip.get())->address[0], buffer, sizeof(buffer)), spc->spc_state, spc->spc_error);
							} break;
						}
					} break;
					// Если произошла ошибка удалённого подключения
					case SCTP_REMOTE_ERROR: {
						// Получаем данные ошибки удалённого подключения
						struct sctp_remote_error * sre = &snp->sn_remote_error;
						// Выводим в лог информационное сообщение
						log->print("Remote error: ERROR=%hu, LENGTH=%hu", awh::log_t::flag_t::INFO, ntohs(sre->sre_error), ntohs(sre->sre_length));
					} break;
					// Если произошло событие неудачной отправки
					case SCTP_SEND_FAILED: {
						// Получаем объект ошибки
						struct sctp_send_failed * ssf = &snp->sn_send_failed;
						// Выводим в лог информационное сообщение
						log->print("Sendfailed: ERROR=%d, LENGTH=%u", awh::log_t::flag_t::INFO, ssf->ssf_error, ssf->ssf_length);
					} break;
					// Если произошло событие отключения подключения
					case SCTP_SHUTDOWN_EVENT:
						// Выводим в лог информационное сообщение
						log->print("Shutdown event", awh::log_t::flag_t::INFO);
					break;
					// Если произошло событие адаптации
					case SCTP_ADAPTATION_INDICATION:
						// Выводим в лог информационное сообщение
						log->print("Adaptation event", awh::log_t::flag_t::INFO);
					break;
					// Если произошло сообщение частичной передачи данных
					case SCTP_PARTIAL_DELIVERY_EVENT:
						// Выводим в лог информационное сообщение
						log->print("Partial delivery", awh::log_t::flag_t::INFO);
					break;
					/**
					 * Если требуется аутентификация
					 */
					#ifdef SCTP_AUTHENTICATION_EVENT
						// Если произошло событие аутентификации
						case SCTP_AUTHENTICATION_EVENT:
							// Выводим в лог информационное сообщение
							log->print("Authentication event", awh::log_t::flag_t::INFO);
						break;
					#endif
					/**
					 * Если требуется отображение сухих событий
					 */
					#ifdef SCTP_SENDER_DRY_EVENT
						// Отправитель прислал сухое событие
						case SCTP_SENDER_DRY_EVENT:
							// Выводим в лог информационное сообщение
							log->print("Sender dry event", awh::log_t::flag_t::INFO);
						break;
					#endif
					// Если произошло неизвестное событие
					default:
						// Выводим в лог информационное сообщение
						log->print("Unknown type: %hu", awh::log_t::flag_t::INFO, snp->sn_header.sn_type);
				}
			}
		}
	};
#endif

/**
 * Инкапсулируем методы работы с cookie в пространство имён
 */
namespace cookie {
	/**
	 * @brief Функция обратного вызова для генерации куков
	 *
	 * @param ssl    объект SSL
	 * @param cookie данные куков
	 * @param size   количество символов
	 * @return       результат проверки
	 */
	static int32_t generate(SSL * ssl, uint8_t * cookie, uint32_t * size) noexcept {
		// Получаем объект уровня защищённых сокетов
		::member_t * member = reinterpret_cast <::member_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
		// Если печенки еще не проинициализированны
		if(!member->cookie.initialized){
			// Выполняем произвольно генерацию байт в буфере печенок
			if(!(member->cookie.initialized = ::RAND_bytes(member->cookie.buffer, sizeof(member->cookie.buffer)))){
				// Выполняем получение идентификатора контекста TLS
				const uint64_t id = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (member));
				// Получаем текст ошибки
				const string error = ::ssl::error(id, "Setting random cookie secret");
				// Если функция обратного вызова ошибки установлена
				if(member->callback.error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(id, awh::tls_t::error_t::CRITICAL, error);
				// Если функция обратного вызова ошибки не установлена
				else {
					// Получаем объект логирования
					awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::CRITICAL, error.c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						log->print("%s", awh::log_t::flag_t::CRITICAL, error.c_str());
					#endif
				}
				// Выходим и сообщаем, что генерация куков не удалась
				return 0;
			}
		}
		// Получаем объект хоста IPv4-адреса
		net::attr_net_t * address = awh_cast <net::attr_net_t *> (member->host.peer.get());
		// Размер буфера и длина сгенерированных печенок
		uint32_t bytes = (address->ip->size + 2), length = 0;
		// Выполняем выделение память для буфера данных
		uint8_t * buffer = reinterpret_cast <uint8_t *> (::OPENSSL_malloc(bytes));
		// Если память для буфера данных не выделена
		if(buffer == nullptr){
			// Выполняем получение идентификатора контекста TLS
			const uint64_t id = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (member));
			// Получаем текст ошибки
			const string error = ::ssl::error(id, "Out of memory cookie");
			// Если функция обратного вызова ошибки установлена
			if(member->callback.error != nullptr)
				// Вызываем функцию обратного вызова ошибки
				member->callback.error(id, awh::tls_t::error_t::CRITICAL, error);
			// Если функция обратного вызова ошибки не установлена
			else {
				// Получаем объект логирования
				awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::CRITICAL, error.c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					log->print("%s", awh::log_t::flag_t::CRITICAL, error.c_str());
				#endif
			}
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
			case 16:
				// Выполняем чтение в буфер данных данные структуры подключения
				::memcpy(buffer + 2, &awh_cast <net::addr_net_ipv6_t *> (address->ip.get())->address[0], 16);
			break;
			// Если производится работа с другими протоколами, выходим
			default: OPENSSL_assert(0);
		}
		// Буфер под генерацию печенок
		uint8_t result[EVP_MAX_MD_SIZE];
		// Выполняем расчёт HMAC в буфере, с использованием секретного ключа
		::HMAC(::EVP_sha1(), reinterpret_cast <void *> (member->cookie.buffer), sizeof(member->cookie.buffer), buffer, bytes, result, &length);
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
	static int32_t generateStateless(SSL * ssl, uint8_t * cookie, size_t * size) noexcept {
		// Размер буфера с печенками
		uint32_t length = 0;
		// Выполняем генерацию печенок
		const int32_t result = ::cookie::generate(ssl, cookie, &length);
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
	static int32_t verify(SSL * ssl, const uint8_t * cookie, uint32_t size) noexcept {
		// Получаем объект уровня защищённых сокетов
		::member_t * member = reinterpret_cast <::member_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[0]));
		// Если печенки не проинициализированы, значит куки не валидные
		if(!member->cookie.initialized)
			// Выходим из функции
			return 0;
		// Получаем объект хоста IPv4-адреса
		net::attr_net_t * address = awh_cast <net::attr_net_t *> (member->host.peer.get());
		// Размер буфера и длина сгенерированных печенок
		uint32_t bytes = (address->ip->size + 2), length = 0;
		// Выполняем выделение память для буфера данных
		uint8_t * buffer = reinterpret_cast <uint8_t *> (::OPENSSL_malloc(bytes));
		// Если память для буфера данных не выделена
		if(buffer == nullptr){
			// Выполняем получение идентификатора контекста TLS
			const uint64_t id = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (member));
			// Получаем текст ошибки
			const string error = ::ssl::error(id, "Out of memory cookie");
			// Если функция обратного вызова ошибки установлена
			if(member->callback.error != nullptr)
				// Вызываем функцию обратного вызова ошибки
				member->callback.error(id, awh::tls_t::error_t::CRITICAL, error);
			// Если функция обратного вызова ошибки не установлена
			else {
				// Получаем объект логирования
				awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(ssl, ::__awh_ssl_index__[2]));
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::CRITICAL, error.c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					log->print("%s", awh::log_t::flag_t::CRITICAL, error.c_str());
				#endif
			}
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
		::HMAC(::EVP_sha1(), reinterpret_cast <void *> (member->cookie.buffer), sizeof(member->cookie.buffer), buffer, bytes, result, &length);
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
	static int32_t verifyStateless(SSL * ssl, const uint8_t * cookie, size_t size) noexcept {
		// Выполняем проверку печенок
		return ::cookie::verify(ssl, cookie, static_cast <uint32_t> (size));
	}
};

/**
 * Инкапсулируем методы проверки сертификата в пространство имён
 */
namespace verify {
	/**
	 * Статусы проверки сертификата
	 */
	enum class status_t : uint8_t {
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
	static status_t matchSubjectName(const string & host, const X509 * x509) noexcept {
		// Результат работы функции
		status_t result = status_t::MatchNotFound;
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
							result = status_t::MatchFound;
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
						result = status_t::MatchFound;
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
	static status_t matchesCommonName(const string & host, const X509 * x509) noexcept {
		// Результат работы функции
		status_t result = status_t::MatchNotFound;
		// Если данные переданы
		if(!host.empty() && (x509 != nullptr)){
			// Получаем индекс имени по "NID"
			const int32_t cnl = ::X509_NAME_get_index_by_NID(X509_get_subject_name(const_cast <X509 *> (x509)), NID_commonName, -1);
			// Если индекс не получен тогда выходим
			if(cnl < 0)
				// Выводим сформированную ошибку
				return status_t::Error;
			// Извлекаем поле "CN"
			X509_NAME_ENTRY * cne = ::X509_NAME_get_entry(X509_get_subject_name(const_cast <X509 *> (x509)), cnl);
			// Если поле не получено тогда выходим
			if(cne == nullptr)
				// Выводим сформированную ошибку
				return status_t::Error;
			// Конвертируем "CN" поле в "C" строку
			ASN1_STRING * cna = ::X509_NAME_ENTRY_get_data(cne);
			// Если строка не сконвертирована тогда выходим
			if(cna == nullptr)
				// Выводим сформированную ошибку
				return status_t::Error;
			// Выполняем рукопожатие
			if(::verify::certHostcheck(host, string(reinterpret_cast <char *> (const_cast <uint8_t *> (::ASN1_STRING_get0_data(cna))), ::ASN1_STRING_length(cna))))
				// Выводим сформированную ошибку
				return status_t::MatchFound;
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
	static status_t validateHostname(const string & host, const X509 * x509) noexcept {
		// Результат работы функции
		status_t result = status_t::Error;
		// Если данные переданы
		if(!host.empty() && (x509 != nullptr)){
			// Выполняем проверку имени хоста по списку доменов у сертификата
			result = ::verify::matchSubjectName(host, x509);
			// Если у сертификата только один домен
			if(result == status_t::NoSANPresent)
				// Выполняем проверку имени хоста по общему имени у сертификата
				result = ::verify::matchesCommonName(host, x509);
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция обратного вызова для проверки валидности сертификата
	 *
	 * @param ok    результат получения сертификата
	 * @param store хранилище сертификатов
	 * @return      результат проверки
	 */
	static int32_t certificate(const int32_t ok, X509_STORE_CTX * store) noexcept {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Если хранилище сертификатов передано верное
			if(store != nullptr){
				// Выполняем извлечение сертификата
				X509 * x509 = ::X509_STORE_CTX_get_current_cert(store);
				// Если сертификат получен
				if(x509 != nullptr){
					// Буфер данных для получения данных
					char buffer[256];
					// Выводим начальный разделитель
					printf("------------------------------------------------------------\n\n");
					// Выводим заголовок
					printf("Current certificate verification:\n");
					// Получаем название сертификата
					::X509_NAME_oneline(::X509_get_subject_name(x509), buffer, sizeof(buffer));
					// Выводим название сертификата
					printf("Subject: %s\n", buffer);
					// Получаем эмитента выпустившего сертификат
					::X509_NAME_oneline(::X509_get_issuer_name(x509), buffer, sizeof(buffer));
					// Выводим эмитента сертификата
					printf("Issuer: %s\n", buffer);
					// Выводим информацию о ошибке
					printf("Error: %s\n", ::X509_verify_cert_error_string(::X509_STORE_CTX_get_error(store)));
					// Выводим информацию об успешной проверке
					printf("Status: Certificate verified successfully at depth %d\n", ::X509_STORE_CTX_get_error_depth(store));
					// Выводим конечный разделитель
					printf("\n------------------------------------------------------------\n\n");
				}
			}
		#endif
		// Выводим результат
		return ok;
	}
	/**
	 * @brief Функция обратного вызова для проверки валидности хоста
	 *
	 * @param store хранилище сертификатов
	 * @param ctx   передаваемый контекст
	 * @return      результат проверки
	 */
	static int32_t hostname(X509_STORE_CTX * store, void * ctx) noexcept {
		// Результат проверки домена
		int32_t result = 0;
		// Если объекты переданы верно
		if((store != nullptr) && (ctx != nullptr)){
			// Получаем объект контекста модуля
			auto member = reinterpret_cast <::member_t *> (ctx);
			// Если проверка сертификата не требуется
			if(!member->state.certificate)
				// Выводим сообщение, что проверка пройдена
				return ::verify::certificate(1, store);
			// Если проверка сертификата прошла удачно
			if((result = ::X509_verify_cert(store)) != 1){
				// Если произошла ошибка несоответствия имени хоста
				if(::X509_STORE_CTX_get_error(store) == X509_V_ERR_HOSTNAME_MISMATCH){
					// Запрашиваем данные сертификата
					X509 * x509 = ::X509_STORE_CTX_get_current_cert(store);
					// Если данные сертификата не получены
					if(x509 == nullptr){
						// Выполняем получение идентификатора контекста TLS
						const uint64_t id = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (member));
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Certificate is not found in store");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, awh::tls_t::error_t::WARNING, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							// Получаем объект логирования
							awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[2]));
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, error.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								log->print("%s", awh::log_t::flag_t::WARNING, error.c_str());
							#endif
						}
					// Если данные сертификата получены
					} else {
						// Получаем имя эмитента выпустившего сертификат
						X509_NAME * name = ::X509_get_issuer_name(x509);
						// Если имя эмитента не получено
						if(name == nullptr){
							// Выполняем получение идентификатора контекста TLS
							const uint64_t id = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (member));
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Certificate issuer name is not found");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, awh::tls_t::error_t::CRITICAL, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								// Получаем объект логирования
								awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[2]));
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, error.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									log->print("%s", awh::log_t::flag_t::WARNING, error.c_str());
								#endif
							}
						// Если имя эмитента получено
						} else {
							// Буфер доменного имени
							char fqdn[256];
							// Заполняем буфер нулями
							::memset(fqdn, 0, sizeof(fqdn));
							// Запрашиваем имя домена
							::X509_NAME_oneline(name, fqdn, sizeof(fqdn));
							// Выполняем проверку на соответствие хоста с данными хостов у сертификата
							const status_t status = ::verify::validateHostname(member->host.name, x509);
							// Если домен найден в записях сертификата (т.е. сертификат соответствует данному домену)
							if((result = static_cast <int32_t> (status == status_t::MatchFound))){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Получаем объект логирования
									awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[2]));
									// Выводим в лог сообщение
									log->print("HTTPS server [%s] has this certificate, which looks good to me: %s", awh::log_t::flag_t::INFO, member->host.name.c_str(), fqdn);
								#endif
							// Если ресурс не найден тогда выводим сообщение об ошибке
							} else {
								// Буфер под результат
								char result[31];
								// Устанавливаем результат ошибки по умолчанию
								::snprintf(result, 31, "%s", "X509 Verify certificate failed");
								/**
								 * Определяем полученную ошибку
								 */
								switch(static_cast <uint8_t> (status)){
									// Если домен найден в записях сертификата
									case static_cast <uint8_t> (status_t::MatchFound):
										// Устанавливаем статус проверки
										::snprintf(result, 14, "%s", "Found a match");
									break;
									// Если домен не найден в записях сертификата
									case static_cast <uint8_t> (status_t::MatchNotFound):
										// Устанавливаем статус проверки
										::snprintf(result, 15, "%s", "No match found");
									break;
									// Если в сертификате отсутствует SAN
									case static_cast <uint8_t> (status_t::NoSANPresent):
										// Устанавливаем статус проверки
										::snprintf(result, 18, "%s", "Present is no SAN");
									break;
									// Если сертификат имеет неверный формат
									case static_cast <uint8_t> (status_t::MalformedCertificate):
										// Устанавливаем статус проверки
										::snprintf(result, 22, "%s", "Malformed certificate");
									break;
									// Если произошла ошибка при проверке
									case static_cast <uint8_t> (status_t::Error):
										// Устанавливаем статус проверки
										::snprintf(result, 6, "%s", "Error");
									break;
									// В иных случаях
									default: ::snprintf(result, 4, "%s", "WTF");
								}
								// Выполняем получение идентификатора контекста TLS
								const uint64_t id = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (member));
								// Получаем объект фреймворка
								awh::fmk_t * fmk = reinterpret_cast <awh::fmk_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[1]));
								// Получаем текст ошибки
								const string error = ::ssl::error(id, fmk->format("%s for hostname '%s' [%s]", result, member->host.name.c_str(), fqdn));
								// Если функция обратного вызова ошибки установлена
								if(member->callback.error != nullptr)
									// Вызываем функцию обратного вызова ошибки
									member->callback.error(id, awh::tls_t::error_t::WARNING, error);
								// Если функция обратного вызова ошибки не установлена
								else {
									// Получаем объект логирования
									awh::log_t * log = reinterpret_cast <awh::log_t *> (::SSL_get_ex_data(member->ssl, ::__awh_ssl_index__[2]));
									/**
									 * Если включён режим отладки
									 */
									#if DEBUG_MODE
										// Выводим сообщение об ошибке
										log->debug("%s", __PRETTY_FUNCTION__, {}, awh::log_t::flag_t::WARNING, error.c_str());
									/**
									* Если режим отладки не включён
									*/
									#else
										// Выводим сообщение об ошибке
										log->print("%s", awh::log_t::flag_t::WARNING, error.c_str());
									#endif
								}
							}
						}
					}
				}
			}
		}
		// Выводим результат
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
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Если версия OpenSSL не соответствует указанной при сборке
			if(::OpenSSL_version_num() != OPENSSL_VERSION_NUMBER){
				// Если функция обратного вызова ошибки установлена
				if(member->callback.error != nullptr){
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(
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
						member->callback.error(id, error_t::CRITICAL, "Major and minor version numbers must match, exiting");
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
				if(member->callback.error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(
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
			if(member->ssl != nullptr){
				// Выполняем получение сертификата сервера
				X509 * x509 = ::SSL_get_peer_certificate(member->ssl);
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
					result = ::move(this->_fmk->format("%sCipher: %s\n", result.c_str(), ::SSL_CIPHER_get_name(::SSL_get_current_cipher(member->ssl))));
				}
			}
			// Если объект CRL-файла сертификата создан
			if(member->crl != nullptr){
				// Создаём memory BIO
				BIO * bio = ::BIO_new(::BIO_s_mem());
				// Если BIO не создан
				if(bio == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Engine store CRL");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
					// Выходим из функции
					return result;
				}
				// Печатаем CRL в BIO
				if(::X509_CRL_print(bio, member->crl) == 0){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Engine store CRL");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
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
			return ::SSL_CIPHER_get_name(::SSL_get_current_cipher(reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->ssl));
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
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Получить сертификат клиента (на сервере) или сервера (на клиенте)
			X509 * x509 = ::SSL_get_peer_certificate(member->ssl);
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
 * @brief Метод получения информации о списке отзыва сертификатов
 *
 * @param id идентификатор события
 * @return   информация о списке отзыва сертификатов
 */
string awh::TransportLayerSecurity::certificateRevocationListInfo(const id_t id) const noexcept {
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
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Если объект CRL-файла сертификата создан
			if(member->crl != nullptr){
				// Создаём memory BIO
				BIO * bio = ::BIO_new(::BIO_s_mem());
				// Если BIO не создан
				if(bio == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Engine store CRL");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
					// Выходим из функции
					return result;
				}
				// Печатаем CRL в BIO
				if(::X509_CRL_print(bio, member->crl) == 0){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Engine store CRL");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
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
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Шаг 1: Получить сертификат
			X509 * x509 = ::SSL_get_peer_certificate(member->ssl);
			// Если сертификат не получен
			if(x509 == nullptr)
				// Нет сертификата
				return false;
			// Получаем текущую дату и время
			time_t date = ::time(nullptr);
			// Если срок действия сертификата истёк
			if(!((::X509_cmp_time(::X509_get0_notBefore(x509), &date) <= 0) && (::X509_cmp_time(::X509_get0_notAfter(x509), &date) >= 0))){
				// Получаем текст ошибки
				const string error = ::ssl::error(id, "Сertificate is not yet valid or has expired");
				// Если функция обратного вызова ошибки установлена
				if(member->callback.error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(id, error_t::WARNING, error);
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::WARNING, error.c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				}
				// Выводим сообщение, что сертификат ещё не действителен или просрочен
				return false;
			}
			// Получить хранилище CA из SSL_CTX
			X509_STORE * store = ::SSL_CTX_get_cert_store(member->ctx);
			// Создать контекст проверки
			X509_STORE_CTX * ctx = ::X509_STORE_CTX_new();
			// Если контекст не создан
			if(ctx == nullptr){
				// Получаем текст ошибки
				const string error = ::ssl::error(id, "X509 Store context init");
				// Если функция обратного вызова ошибки установлена
				if(member->callback.error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(id, error_t::CRITICAL, error);
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
				}
				// Возвращаем отрицательный результат
				return false;
			}
			// Инициализировать (x509 — сертификат пира, untrusted — промежуточные, если есть)
			if(::X509_STORE_CTX_init(ctx, store, x509, ::SSL_get_peer_cert_chain(member->ssl)) == 0){
				// Выполняем очистку контекста
				::X509_STORE_CTX_free(ctx);
				// Получаем текст ошибки
				const string error = ::ssl::error(id, "X509 Store context init");
				// Если функция обратного вызова ошибки установлена
				if(member->callback.error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(id, error_t::CRITICAL, error);
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
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
				if(member->callback.error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(id, error_t::CRITICAL, ::X509_verify_cert_error_string(error));
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
						if((ok = ::verify::certHostcheck(member->host.name, fqdn)))
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
					ok = ::verify::certHostcheck(member->host.name, buffer);
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
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Выполняем блокировку потоков
			const locker_t lock(member->mtx);
			// Устанавливаем режим проверки сертификата
			member->state.certificate = mode;
			/**
			 * Определяем узел события к которому относится контекст TLS
			 */
			switch(static_cast <uint8_t> (member->node)){
				// Если узел является клиентом
				case static_cast <uint8_t> (event::node_t::CLIENT): {
					// Если нужно произвести проверку
					if(member->state.certificate)
						// Активируем проверку сертификата сервера
						::SSL_set_verify(member->ssl, SSL_VERIFY_PEER, &::verify::certificate);
					// Деактивируем проверку сертификата сервера
					else ::SSL_set_verify(member->ssl, SSL_VERIFY_NONE, nullptr);
				} break;
				// Если узел является сервером
				case static_cast <uint8_t> (event::node_t::SERVER): {
					// Если нужно произвести проверку
					if(member->state.certificate)
						// Выполняем проверку сертификата клиента
						::SSL_set_verify(member->ssl, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, &::verify::certificate);
					// Запрещаем выполнять првоерку доменного имени
					else ::SSL_set_verify(member->ssl, SSL_VERIFY_NONE, nullptr);
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
				auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
				// Выполняем блокировку потоков
				const locker_t lock(member->mtx);
				// Устанавливаем хост для уровня защищённых сокетов
				member->host.name = hostname;
				// Устанавливаем имя хоста для SNI расширения
				::SSL_set_tlsext_host_name(member->ssl, member->host.name.c_str());
				/**
				 * Если версия OpenSSL соответствует или выше версии 1.1.1
				 */
				#if OPENSSL_VERSION_NUMBER >= 0x10101000L
					// Устанавливаем имя хоста для проверки
					::SSL_set1_host(member->ssl, member->host.name.c_str());
				#endif
				// Активируем верификацию доменного имени
				if(::X509_VERIFY_PARAM_set1_host(::SSL_get0_param(member->ssl), member->host.name.c_str(), 0) != 1){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Host SSL verification failed");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, hostname), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
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
 * @brief Метод установки адреса и порта отдалённого узла
 *
 * @param id   идентификатор события
 * @param ip   IP-адрес отдалённого узла
 * @param port порт отдалённого узла
 * @return     результат выполнения установки
 */
bool awh::TransportLayerSecurity::peer(const id_t id, const string & ip, const uint16_t port) noexcept {
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
				// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
				auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
				// Выполняем блокировку потоков
				const locker_t lock(member->mtx);
				// Выполняем парсинг I-адреса
				if(this->_addr.parse(ip)){
					// Выполняем инициализацию объекта хоста IPv4-адреса
					member->host.peer = make_unique <net::attr_net_t> ();
					// Получаем объект хоста IPv4-адреса
					net::attr_net_t * address = awh_cast <net::attr_net_t *> (member->host.peer.get());
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
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Unsupported IP-address type");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRITICAL, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, ip, port), log_t::flag_t::CRITICAL, error.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
							// Возвращаем отрицательный результат
							return false;
						}
					}
				// Если парсинг IP-адреса не выполнен
				} else {
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Failed to parse IP-address");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, ip, port), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
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
 * @param id идентификатор события
 * @return   результат выполнения рукопожатия
 */
bool awh::TransportLayerSecurity::handshake(const id_t id) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
		auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
		// Если рукопожатие ещё не выполнено
		if(!(result = member->state.handshake)){
			// Выполняем блокировку потоков
			const locker_t lock(member->mtx);
			// Выполняем TLS рукопожатие
			const int32_t handshake = ::SSL_do_handshake(member->ssl);
			// Если рукопожатие не выполнено
			if(!(result = member->state.handshake = (handshake == 1))){
				// Получаем код ошибки
				const int32_t error = ::SSL_get_error(member->ssl, handshake);
				// Если ошибка не связана с необходимостью повторного чтения или записи
				if(!(result = ((error == SSL_ERROR_WANT_READ) || (error == SSL_ERROR_WANT_WRITE)))){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "SSL handshake failed");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				// Если ошибка связана с необходимостью повторного чтения или записи
				} else {
					// Количество прочитанных данных
					int32_t bytes = 0;
					// Количество ожидающих данных для чтения
					size_t pending = 0;
					// Буфер данных для чтения
					uint8_t buffer[AWH_MAX_SSL_BUFFER_SIZE];
					/**
					 * Читаем все ожидающие данные из BIO буфера записи
					 */
					while((pending = ::BIO_ctrl_pending(member->wbio)) > 0){
						// Читаем данные из BIO буфера записи
						bytes = ::BIO_read(member->wbio, buffer, static_cast <size_t> (::min(pending, static_cast <size_t> (AWH_MAX_SSL_BUFFER_SIZE))));
						// Если данные не прочитаны
						if(bytes <= 0)
							// Выходим из цикла
							break;
						// Если функция обратного вызова чтения данных установлена
						else if(member->callback.read != nullptr)
							// Вызываем функцию обратного вызова чтения данных
							member->callback.read(id, event_t::ENCRYPTION, buffer, static_cast <size_t> (bytes));
					}
				}
				// Выводим результат
				return result;
			}
		}
		// Если узел является клиентом
		if(member->node == event::node_t::CLIENT){
			// Длина извлекаемого протокола
			uint32_t length = 0;
			// Название извлекаемого протокола
			const uint8_t * proto = nullptr;
			// Выполняем извлечение активного протокола
			::SSL_get0_alpn_selected(member->ssl, &proto, &length);
			// Размер и индекс протокола
			uint8_t size = 0, index = 0;
			// Выполняем перебор всего буфера поддерживаемых ALPN-протоколов
			for(uint8_t i = 0; i < member->alpn.buffer.size(); i++){
				// Извлекаем размер протокола
				size = member->alpn.buffer[i];
				// Если размер протокола совпадает с длиной извлекаемого протокола
				if(size == static_cast <uint8_t> (length)){
					// Если протокол совпадает с извлекаемым протоколом
					if(::memcmp(&member->alpn.buffer[i + 1], proto, length) == 0){
						// Устанавливаем активный протокол
						member->alpn.id = member->alpn.ids[index];
						// Выход из цикла
						break;
					}
				}
				// Смещаем индекс на размер протокола
				i += size;
				// Увеличиваем индекс протокола
				index++;
			}
		}
		// Если рукопожатие выполнено успешно
		if(member->callback.handshake != nullptr)
			// Вызываем функцию обратного вызова успешного выполнения рукопожатия
			member->callback.handshake(id);
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
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод шифрования данных
 *
 * @param id     идентификатор события
 * @param buffer буфер данных для шифрования
 * @param size   размер буфера данных для шифрования
 * @return       результат выполнения шифрования
 */
bool awh::TransportLayerSecurity::encrypt(const id_t id, const void * buffer, const size_t size) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если данные для шифрования переданы корректно
		if((buffer != nullptr) && (size > 0)){
			// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Если рукопожатие выполнено успешно
			if(member->state.handshake){
				// Выполняем блокировку потоков
				const locker_t lock(member->mtx);
				/**
				 * Для операционной системы Linux или FreeBSD
				 */
				#if __linux__ || __FreeBSD__
					// Если протокол интернета установлен как SCTP
					if(member->proto == event::protocol_t::SCTP){
						// Создаём объект получения информационных событий
						struct bio_dgram_sctp_sndinfo info;
						// Выполняем зануление объекта информационного события
						::memset(&info, 0, sizeof(info));
						// Выполняем установку события
						::BIO_ctrl(member->wbio, BIO_CTRL_DGRAM_SCTP_SET_SNDINFO, sizeof(info), &info);
					}
				#endif
				// Выполняем запись данных в защищённый сокет
				int32_t bytes = ::SSL_write(member->ssl, buffer, static_cast <int32_t> (size));
				// Если данные не записаны
				if(!(result = (bytes > 0))){
					// Получаем код ошибки
					const int32_t error = ::SSL_get_error(member->ssl, bytes);
					// Если ошибка не связана с необходимостью повторного чтения или записи
					if((error != SSL_ERROR_WANT_READ) && (error != SSL_ERROR_WANT_WRITE)){
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "SSL write failed");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::CRITICAL, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, size), log_t::flag_t::CRITICAL, error.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
							#endif
						}
					}
				// Если данные записаны удачно
				} else {
					// Если функция обратного вызова записи данных установлена
					if(member->callback.write != nullptr)
						// Вызываем функцию обратного вызова записи данных
						member->callback.write(id, event_t::ENCRYPTION, static_cast <size_t> (bytes));
					/**
					 * Если операционной системой является Linux или FreeBSD и включён режим отладки
					 */
					#if (__linux__ || __FreeBSD__) && DEBUG_MODE
						// Если протокол интернета установлен как SCTP
						if((member->proto == event::protocol_t::SCTP) && (::SSL_get_error(member->ssl, bytes) == SSL_ERROR_NONE)){
							/**
							 * Определяем узел события к которому относится контекст TLS
							 */
							switch(static_cast <uint8_t> (member->node)){
								// Если узел является клиентом
								case static_cast <uint8_t> (event::node_t::CLIENT): {
									// Создаём объект получения информационных событий
									struct bio_dgram_sctp_sndinfo info;
									// Выполняем зануление объекта информационного события
									::memset(&info, 0, sizeof(info));
									// Выполняем извлечение события
									::BIO_ctrl(member->wbio, BIO_CTRL_DGRAM_SCTP_GET_SNDINFO, sizeof(info), &info);
									// Выводим в лог информационное сообщение
									this->_log->print("Wrote %d bytes, stream: %u, ppid: %u", log_t::flag_t::INFO, bytes, info.snd_sid, info.snd_ppid);
								} break;
								// Если узел является сервером
								case static_cast <uint8_t> (event::node_t::SERVER): {
									// Создаём объект получения информационных событий
									struct bio_dgram_sctp_rcvinfo info;
									// Выполняем зануление объекта информационного события
									::memset(&info, 0, sizeof(info));
									// Выполняем извлечение события
									::BIO_ctrl(member->wbio, BIO_CTRL_DGRAM_SCTP_GET_SNDINFO, sizeof(info), &info);
									// Выводим в лог информационное сообщение
									this->_log->print("Wrote %d bytes, stream: %u, ssn: %u, ppid: %u, tsn: %u", log_t::flag_t::INFO, bytes, info.rcv_sid, info.rcv_ssn, info.rcv_ppid, info.rcv_tsn);
								} break;
							}
						}
					#endif
					// Количество ожидающих данных для чтения
					size_t pending = 0;
					// Буфер данных для чтения
					uint8_t buffer[AWH_MAX_SSL_BUFFER_SIZE];
					/**
					 * Читаем все ожидающие данные из BIO буфера записи
					 */
					while((pending = ::BIO_ctrl_pending(member->wbio)) > 0){
						// Читаем данные из BIO буфера записи
						bytes = ::BIO_read(member->wbio, buffer, static_cast <size_t> (::min(pending, static_cast <size_t> (AWH_MAX_SSL_BUFFER_SIZE))));
						// Если данные не прочитаны
						if(bytes <= 0)
							// Выходим из цикла
							break;
						// Если функция обратного вызова чтения данных установлена
						else if(member->callback.read != nullptr)
							// Вызываем функцию обратного вызова чтения данных
							member->callback.read(id, event_t::ENCRYPTION, buffer, static_cast <size_t> (bytes));
					}
				}
			// Если рукопожатие не выполнено
			} else {
				// Получаем текст ошибки
				const string error = ::ssl::error(id, "Handshake has not yet been completed");
				// Если функция обратного вызова ошибки установлена
				if(member->callback.error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(id, error_t::WARNING, error);
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, size), log_t::flag_t::WARNING, error.c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, size), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод расшифровки данных
 *
 * @param id     идентификатор события
 * @param buffer буфер данных для расшифровки
 * @param size   размер буфера данных для расшифровки
 * @return       результат выполнения расшифровки
 */
bool awh::TransportLayerSecurity::decrypt(const id_t id, const void * buffer, const size_t size) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если данные для шифрования переданы корректно
		if((buffer != nullptr) && (size > 0)){
			// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Выполняем блокировку потоков
			const locker_t lock(member->mtx);
			// Выполняем запись данных в BIO буфер чтения
			int32_t bytes = ::BIO_write(member->rbio, buffer, static_cast <int32_t> (size));
			// Если данные записаны успешно и размер записанных данных совпадает с размером входного буфера
			if((result = (bytes == static_cast <int32_t> (size)))){
				// Если функция обратного вызова записи данных установлена
				if(member->callback.write != nullptr)
					// Вызываем функцию обратного вызова записи данных
					member->callback.write(id, event_t::DECRYPTION, static_cast <size_t> (bytes));
				/**
				 * Если операционной системой является Linux или FreeBSD и включён режим отладки
				 */
				#if (__linux__ || __FreeBSD__) && DEBUG_MODE
					// Если протокол интернета установлен как SCTP
					if((member->proto == event::protocol_t::SCTP) && (::SSL_get_error(member->ssl, bytes) == SSL_ERROR_NONE)){
						// Создаём объект получения информационных событий
						struct bio_dgram_sctp_rcvinfo info;
						// Выполняем зануление объекта информационного события
						::memset(&info, 0, sizeof(info));
						// Выполняем извлечение события
						::BIO_ctrl(member->rbio, BIO_CTRL_DGRAM_SCTP_GET_RCVINFO, sizeof(info), &info);
						// Выводим в лог информационное сообщение
						this->_log->print("Read %d bytes, stream: %u, ssn: %u, ppid: %u, tsn: %u", log_t::flag_t::INFO, bytes, info.rcv_sid, info.rcv_ssn, info.rcv_ppid, info.rcv_tsn);
					}
				#endif
				// Если рукопожатие ещё не выполнено
				if(!member->state.handshake)
					// Выполняем рукопожатие TLS
					result = this->handshake(id);
				// Если у нас есть подготовленные данные для чтения
				if((::BIO_ctrl_pending(member->rbio) > 0) || ::SSL_has_pending(member->ssl)){
					// Буфер данных для чтения
					uint8_t buffer[AWH_MAX_SSL_BUFFER_SIZE];
					/**
					 * Читаем все доступные данные из защищённого сокета
					 */
					while((::BIO_ctrl_pending(member->rbio) > 0) || ::SSL_has_pending(member->ssl)){
						// Читаем данные из защищённого сокета
						bytes = ::SSL_read(member->ssl, buffer, AWH_MAX_SSL_BUFFER_SIZE);
						// Если данные не прочитаны
						if(bytes <= 0)
							// Выходим из цикла
							break;
						// Если функция обратного вызова чтения данных установлена
						else if(member->callback.read != nullptr)
							// Вызываем функцию обратного вызова чтения данных
							member->callback.read(id, event_t::DECRYPTION, buffer, static_cast <size_t> (bytes));
					}
				}
			// Если данные не записаны полностью
			} else {
				// Получаем текст ошибки
				const string error = ::ssl::error(id, "SSL BIO write failed");
				// Если функция обратного вызова ошибки установлена
				if(member->callback.error != nullptr)
					// Вызываем функцию обратного вызова ошибки
					member->callback.error(id, error_t::CRITICAL, error);
				// Если функция обратного вызова ошибки не установлена
				else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, size), log_t::flag_t::CRITICAL, error.c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, size), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
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
		if(i != ::__awh_ssl_ids__.end()){
			// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Устанавливаем режим безопасности потоков
			member->mtx.enabled = (mode == event::mode_t::ENABLED);
			// Устанавливаем глобальный режим безопасности потоков
			::__awh_ssl_mtx__.enabled = (mode == event::mode_t::ENABLED);
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
					auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
					// Выполняем блокировку потоков
					const locker_t lock(member->mtx);
					// Устанавливаем все основные алгоритмы шифрования
					if(::SSL_set_cipher_list(member->ssl, result.c_str()) != 1){
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "Set SSL ciphers");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::CRITICAL, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, ciphers.size()), log_t::flag_t::CRITICAL, error.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
							#endif
						}
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
				auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
				// Выполняем блокировку потоков
				const locker_t lock(member->mtx);
				// Если приватный ключ не может быть установлен
				if(::SSL_use_PrivateKey_file(member->ssl, filename.c_str(), SSL_FILETYPE_PEM) != 1){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Private key cannot be set");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
					// Выходим
					return;
				}
				// Если приватный ключ недействителен
				if(::SSL_check_private_key(member->ssl) != 1){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Private key is not valid");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
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
				auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
				// Выполняем блокировку потоков
				const locker_t lock(member->mtx);
				/**
				 * Определяем узел события к которому относится контекст TLS
				 */
				switch(static_cast <uint8_t> (member->node)){
					// Если узел является клиентом
					case static_cast <uint8_t> (event::node_t::CLIENT): {
						// Если сертификат не устанавливается
						if(::SSL_use_certificate_file(member->ssl, filename.c_str(), SSL_FILETYPE_PEM) != 1){
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Certificate cannot be set");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRITICAL, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
						}
					} break;
					// Если узел является сервером
					case static_cast <uint8_t> (event::node_t::SERVER): {
						// Если сертификат не устанавливается
						if(::SSL_use_certificate_chain_file(member->ssl, filename.c_str()) != 1){
							// Получаем текст ошибки
							const string error = ::ssl::error(id, "Certificate cannot be set");
							// Если функция обратного вызова ошибки установлена
							if(member->callback.error != nullptr)
								// Вызываем функцию обратного вызова ошибки
								member->callback.error(id, error_t::CRITICAL, error);
							// Если функция обратного вызова ошибки не установлена
							else {
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
								#endif
							}
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
 * @brief Метод установки списка отзыва сертификатов
 *
 * @param id       идентификатор события
 * @param filename адрес файла списка отзыва сертификатов
 */
void awh::TransportLayerSecurity::certificateRevocationList(const id_t id, const string & filename) noexcept {
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
				auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
				// Выполняем блокировку потоков
				const locker_t lock(member->mtx);
				// Если CRL-файл сертификата уже создан
				if(member->crl != nullptr)
					// Выполняем освобождение памяти
					::X509_CRL_free(member->crl);
				// Создаём объект BIO для загрузки файла
				BIO * bio = ::BIO_new(::BIO_s_file());
				// Если BIO не создан
				if(bio == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Engine store CRL");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
					// Выходим из функции
					return;
				}
				// Выполняем чтение CRL-файла сертификата
				if(BIO_read_filename(bio, filename.c_str()) <= 0){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "CRL-file is corrupted or unreadable");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
					// Выполняем очистку памяти BIO
					::BIO_free(bio);
					// Выходим из функции
					return;
				}
				// Выполняем создание объекта CRL-файла сертификата
				member->crl = ::PEM_read_bio_X509_CRL(bio, nullptr, nullptr, nullptr);
				// Если CRL-файл сертификата не создан
				if(member->crl == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "CRL-file cannot be set");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
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
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if(i != ::__awh_ssl_ids__.end()){
			// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Выполняем блокировку потоков
			const locker_t lock(member->mtx);
			// Если адрес файла центра сертификации не пустой
			if(!filename.empty()){
				// Создаём новое хранилище
				X509_STORE * store = ::SSL_CTX_get_cert_store(member->ctx);
				// Если хранилище не создано
				if(store == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Get x509 store is not found");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
					// Выходим из функции
					return;
				}
				// Загружаем местоположение центра сертификации
				if(::X509_STORE_load_locations(store, filename.c_str(), nullptr) != 1){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "CA-file is not loaded");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, filename), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
					// handle error
					return;
				}
				// Если узел является сервером
				if(member->node == event::node_t::SERVER)
					// Выполняем установку CRL-файла сертификата
					::SSL_set_client_CA_list(member->ssl, ::SSL_load_client_CA_file(filename.c_str()));
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
			auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
			// Выполняем блокировку потоков
			const locker_t lock(member->mtx);
			// Если название файла центра сертификации не пустое
			if(!file.empty()){
				// Создаём новое хранилище
				X509_STORE * store = ::SSL_CTX_get_cert_store(member->ctx);
				// Если хранилище не создано
				if(store == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(id, "Get x509 store is not found");
					// Если функция обратного вызова ошибки установлена
					if(member->callback.error != nullptr)
						// Вызываем функцию обратного вызова ошибки
						member->callback.error(id, error_t::CRITICAL, error);
					// Если функция обратного вызова ошибки не установлена
					else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
					// Выходим из функции
					return;
				}
				// Если каталог сертификатов передан
				if(!dir.empty()){
					// Полный адрес файла центра сертификации
					string filename = "";
					// Если последний символ каталога является разделителем
					if(dir.back() == AWH_FS_SEPARATOR[0])
						// Формируем полный адрес файла центра сертификации
						filename = ::move(this->_fmk->format("%s%s", dir.c_str(), file.c_str()));
					// Формируем полный адрес файла центра сертификации
					else filename = ::move(this->_fmk->format("%s%s%s", dir.c_str(), AWH_FS_SEPARATOR, file.c_str()));
					// Загружаем местоположение центра сертификации
					if(::X509_STORE_load_locations(store, filename.c_str(), nullptr) != 1){
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "CA-file is not loaded");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::CRITICAL, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
							#endif
						}
						// handle error
						return;
					}
					// Если узел является сервером
					if(member->node == event::node_t::SERVER)
						// Выполняем установку CRL-файла сертификата
						::SSL_set_client_CA_list(member->ssl, ::SSL_load_client_CA_file(filename.c_str()));
				// Если каталог сертификатов не передан
				} else {
					// Загружаем местоположение центра сертификации
					if(::X509_STORE_load_locations(store, file.c_str(), nullptr) != 1){
						// Получаем текст ошибки
						const string error = ::ssl::error(id, "CA-file is not loaded");
						// Если функция обратного вызова ошибки установлена
						if(member->callback.error != nullptr)
							// Вызываем функцию обратного вызова ошибки
							member->callback.error(id, error_t::CRITICAL, error);
						// Если функция обратного вызова ошибки не установлена
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Выводим сообщение об ошибке
								this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, dir, file), log_t::flag_t::CRITICAL, error.c_str());
							/**
							* Если режим отладки не включён
							*/
							#else
								// Выводим сообщение об ошибке
								this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
							#endif
						}
						// handle error
						return;
					}
					// Если узел является сервером
					if(member->node == event::node_t::SERVER)
						// Выполняем установку CRL-файла сертификата
						::SSL_set_client_CA_list(member->ssl, ::SSL_load_client_CA_file(file.c_str()));
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
 * @brief Метод извлечения активного протокола
 *
 * @param id идентификатор события
 * @return   метод активного протокола
 */
uint8_t awh::TransportLayerSecurity::alpn(const id_t id) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
		auto i = ::__awh_ssl_ids__.find(id);
		// Если идентификатор контекста TLS найден
		if(i != ::__awh_ssl_ids__.end())
			// Выполняем извлечение активного протокола
			return reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->alpn.id;
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
	return -1;
}
/**
 * @brief Метод установки поддерживаемых ALPN-протоколов
 *
 * @param id   идентификатор события
 * @param alpn список поддерживаемых ALPN-протоколов
 */
void awh::TransportLayerSecurity::alpn(const id_t id, const vector <alpn_t> & alpn) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если список поддерживаемых ALPN-протоколов не пустой
		if(!alpn.empty()){
			// Выполняем поиск идентификатора контекста TLS в глобальном наборе идентификаторов контекстов TLS
			auto i = ::__awh_ssl_ids__.find(id);
			// Если идентификатор контекста TLS найден
			if(i != ::__awh_ssl_ids__.end()){
				// Выполняем извлечение уровня защищённых сокетов из глобального контейнера уровней защищённых сокетов
				auto member = reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id));
				// Выполняем сброс списка идентификаторов поддерживаемых ALPN-протоколов
				member->alpn.ids.clear();
				// Выполняем сброс буфера поддерживаемых ALPN-протоколов
				member->alpn.buffer.clear();
				/**
				 * Выполняем перебор всего списка поддерживаемых ALPN-протоколов
				 */
				for(const auto & item : alpn){
					// Добавляем идентификатор протокола в список поддерживаемых протоколов
					member->alpn.ids.push_back(item.id);
					// Добавляем в буфер длину названия протокола
					member->alpn.buffer.push_back(static_cast <uint8_t> (item.protocol.size()));
					// Добавляем в буфер название протокола
					member->alpn.buffer.insert(member->alpn.buffer.end(), item.protocol.begin(), item.protocol.end());
				}
				// Если идентификатор выбранного ALPN-протокола не передан
				member->alpn.id = member->alpn.ids.front();
				// Если узел является клиентом
				if(member->node == event::node_t::CLIENT){
					/**
					 * Если версия OpenSSL соответствует или выше версии 1.0.2
					 */
					#if OPENSSL_VERSION_NUMBER >= 0x10002000L
						// Выполняем установку доступных протоколов передачи данных
						::SSL_set_alpn_protos(member->ssl, member->alpn.buffer.data(), static_cast <uint32_t> (member->alpn.buffer.size()));
					#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, alpn.size()), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки функции обратного вывода получения данных
 *
 * @param id       идентификатор события
 * @param callback объект функции обратного вызова
 * @return         результат установки функции обратного вызова
 */
bool awh::TransportLayerSecurity::on(const id_t id, read_callback_t callback) noexcept {
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
			// Устанавливаем функцию обратного вызова получения данных
			reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->callback.read = ::move(callback);
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
 * @brief Метод установки функции обратного вывода передачи данных
 *
 * @param id       идентификатор события
 * @param callback объект функции обратного вызова
 * @return         результат установки функции обратного вызова
 */
bool awh::TransportLayerSecurity::on(const id_t id, write_callback_t callback) noexcept {
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
			// Устанавливаем функцию обратного вызова передачи данных
			reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->callback.write = ::move(callback);
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
 * @brief Метод установки функции обратного вывода получения ошибок
 *
 * @param id       идентификатор события
 * @param callback объект функции обратного вызова
 * @return         результат установки функции обратного вызова
 */
bool awh::TransportLayerSecurity::on(const id_t id, error_callback_t callback) noexcept {
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
			reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->callback.error = ::move(callback);
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
 * @brief Метод установки функции обратного вывода выполнения рукопожатия
 *
 * @param id       идентификатор события
 * @param callback объект функции обратного вызова
 * @return         результат установки функции обратного вызова
 */
bool awh::TransportLayerSecurity::on(const id_t id, handshake_callback_t callback) noexcept {
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
			// Устанавливаем функцию обратного вызова выполнения рукопожатия
			reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->callback.handshake = ::move(callback);
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
			reinterpret_cast <::member_t *> (static_cast <uintptr_t> (id))->erase(::__awh_ssl_members__);
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
 * @return      идентификатор контекста TLS
 */
awh::TransportLayerSecurity::id_t awh::TransportLayerSecurity::create(const event::node_t node, const event::protocol_t proto) noexcept {
	// Результат работы функции
	id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потоков
		const locker_t lock(::__awh_ssl_mtx__);
		// Создаём новый уровень защищённых сокетов и добавляем его в контейнер
		auto ret = ::__awh_ssl_members__.emplace(::make_unique <::member_t> ());
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
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Context SSL is not initialization");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Устанавливаем опции запроса
				::SSL_CTX_set_options(
					(* ret.first)->ctx,
					/**
					 * 1. Совместимость и безопасность по умолчанию
					 */
					SSL_OP_ALL |
					/**
					 * 2. Отключить устаревшие и небезопасные протоколы
					 */
					SSL_OP_NO_SSLv2 |
					SSL_OP_NO_SSLv3 |
					SSL_OP_NO_TLSv1 |
					SSL_OP_NO_TLSv1_1 |
					/**
					 * 3. Защита от атак
					 */
					SSL_OP_NO_COMPRESSION | // CRIME / BREACH
					/**
					 * 5. Дополнительные меры (опционально, но рекомендованы)
					 */
					SSL_OP_NO_RENEGOTIATION | // Отключить ренеготиацию вообще (OpenSSL 1.1.1+)
					SSL_OP_SINGLE_DH_USE |    // Свежие DH-ключи (для DHE)
					SSL_OP_SINGLE_ECDH_USE    // Свежие ECDH-ключи (для ECDHE)
				);
				// Устанавливаем минимально-возможную версию TLS
				::SSL_CTX_set_min_proto_version((* ret.first)->ctx, TLS1_2_VERSION);
				// Устанавливаем максимально-возможную версию TLS
				::SSL_CTX_set_max_proto_version((* ret.first)->ctx, TLS1_3_VERSION);
				/**
				 * Если версия OpenSSL соответствует или выше версии 3.0.0
				 */
				#if OPENSSL_VERSION_NUMBER >= 0x30000000L
					// Выполняем установку кривых P-256, P-384 и P-521
					if(::SSL_CTX_set1_curves_list((* ret.first)->ctx, "P-521:P-384:P-256") != 1){
						// Получаем текст ошибки
						const string error = ::ssl::error(result, "Set SSL CURVEs list failed");
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									static_cast <uint16_t> (node),
									static_cast <uint16_t> (proto)
								), log_t::flag_t::CRITICAL, error.c_str()
							);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
						// Удаляем контекст TLS из контейнера уровней защищённых сокетов
						(* ret.first)->erase(::__awh_ssl_members__);
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
						// Получаем текст ошибки
						const string error = ::ssl::error(result, "Set new SSL CURVE name failed");
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									static_cast <uint16_t> (node),
									static_cast <uint16_t> (proto)
								), log_t::flag_t::CRITICAL, error.c_str()
							);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
						// Удаляем контекст TLS из контейнера уровней защищённых сокетов
						(* ret.first)->erase(::__awh_ssl_members__);
						// Выходим
						return 0;
					}
					// Выполняем установку кривых P-256
					::SSL_CTX_set_tmp_ecdh((* ret.first)->ctx, ecdh);
					// Выполняем очистку объекта кривой
					::EC_KEY_free(ecdh);
				#endif
				// Устанавливаем все основные алгоритмы шифрования
				if(::SSL_CTX_set_cipher_list((* ret.first)->ctx, ::__awh_ssl_ciphers__.c_str()) != 1){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Set SSL ciphers failed");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Получаем данные стора
				X509_STORE * store = ::SSL_CTX_get_cert_store((* ret.first)->ctx);
				// Если стор не получен
				if(store == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Get x509 store is failed");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
							static_cast <uint16_t> (node),
							static_cast <uint16_t> (proto)
						), log_t::flag_t::WARNING, error.c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				// Если стор получен
				} else {
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Проверяем существует ли путь
						if(!::ssl::addCertToStore(store, "CA", (* ret.first).get()) ||
						   !::ssl::addCertToStore(store, "ROOT", (* ret.first).get()) ||
						   !::ssl::addCertToStore(store, "AuthRoot", (* ret.first).get()))
							// Выходим из функции
							return;
					#endif
					// Если стор не устанавливается, тогда выводим ошибку
					if(::X509_STORE_set_default_paths(store) == 0){
						// Получаем текст ошибки
						const string error = ::ssl::error(result, "Set default paths for x509 store is not allowed");
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				}
				// Устанавливаем, что мы должны читать как можно больше входных байтов
				::SSL_CTX_set_read_ahead((* ret.first)->ctx, 1);
				// Устанавливаем флаг очистки буферов на чтение и запись когда они не требуются
				::SSL_CTX_set_mode((* ret.first)->ctx, SSL_MODE_RELEASE_BUFFERS);
				// Устанавливаем пути по умолчанию для проверки сертификатов
				::SSL_CTX_set_default_verify_paths((* ret.first)->ctx);
				// Устанавливаем проверку сертификата сервера
				::SSL_CTX_set_verify((* ret.first)->ctx, SSL_VERIFY_PEER, &::verify::certificate);
				// Выполняем проверку всех дочерних сертификатов
				::SSL_CTX_set_cert_verify_callback((* ret.first)->ctx, &::verify::hostname, (* ret.first).get());
				/**
				 * @brief собран без следующих переговорщиков по протоколам
				 *
				 */
				#ifndef OPENSSL_NO_NEXTPROTONEG
					// Устанавливаем функцию обратного вызова для переключения протокола на HTTP
					::SSL_CTX_set_next_proto_select_cb((* ret.first)->ctx, &::ssl::clientNextProtoSelect, (* ret.first).get());
				#endif // !OPENSSL_NO_NEXTPROTONEG
				// Создаем SSL объект
				(* ret.first)->ssl = ::SSL_new((* ret.first)->ctx);
				// Если объект не создан
				if((* ret.first)->ssl == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Could not create TLS session object");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Привязываем текущий объект TLS к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::__awh_ssl_index__[0], (* ret.first).get());
				// Привязываем текущий объект фреймворка к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::__awh_ssl_index__[1], const_cast <fmk_t *> (this->_fmk));
				// Привязываем текущий объект лога к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::__awh_ssl_index__[2], const_cast <log_t *> (this->_log));
				// Создаём объект BIO для чтения
				(* ret.first)->rbio = ::BIO_new(::BIO_s_mem());
				// Создаём объект BIO для записи
				(* ret.first)->wbio = ::BIO_new(::BIO_s_mem());
				// Если один из объектов BIO не создан
				if(((* ret.first)->rbio == nullptr) || ((* ret.first)->wbio == nullptr)){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Create BIO is failed");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
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
					(* ret.first)->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Привязываем объекты BIO к SSL объекту
				::SSL_set_bio((* ret.first)->ssl, (* ret.first)->rbio, (* ret.first)->wbio);
				// Устанавливаем режим клиента для SSL объекта
				::SSL_set_connect_state((* ret.first)->ssl);
				/**
				 * Если операционной системой является Linux или FreeBSD и включён режим отладки
				 */
				#if (__linux__ || __FreeBSD__) && DEBUG_MODE
					// Если протокол интернета установлен как SCTP
					if(proto == event::protocol_t::SCTP)
						// Устанавливаем функцию нотификации
						::BIO_dgram_sctp_notification_cb((* ret.first)->rbio, &::sctp::notifications, (* ret.first).get());
				#endif
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
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Context SSL is not initialization");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Устанавливаем опции запроса
				::SSL_CTX_set_options(
					(* ret.first)->ctx,
					/**
					 * 1. Совместимость и безопасность по умолчанию
					 */
					SSL_OP_ALL |
					/**
					 * 2. Отключить устаревшие и небезопасные протоколы
					 */
					SSL_OP_NO_SSLv2 |
					SSL_OP_NO_SSLv3 |
					SSL_OP_NO_TLSv1 |
					SSL_OP_NO_TLSv1_1 |
					/**
					 * 3. Защита от атак
					 */
					SSL_OP_NO_COMPRESSION |                         // CRIME / BREACH
					SSL_OP_CIPHER_SERVER_PREFERENCE |               // Сервер выбирает шифр
					SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION | // Безопасная ренеготиация
					/**
					 * Защита от DoS (если используете cookie)
					 */
					SSL_OP_COOKIE_EXCHANGE | // Только для сервера!
					/**
					 * 5. Дополнительные меры (опционально, но рекомендованы)
					 */
					SSL_OP_NO_RENEGOTIATION | // Отключить ренеготиацию вообще (OpenSSL 1.1.1+)
					SSL_OP_SINGLE_DH_USE |    // Свежие DH-ключи (для DHE)
					SSL_OP_SINGLE_ECDH_USE    // Свежие ECDH-ключи (для ECDHE)
				);
				// Устанавливаем минимально-возможную версию TLS
				::SSL_CTX_set_min_proto_version((* ret.first)->ctx, TLS1_2_VERSION);
				// Устанавливаем максимально-возможную версию TLS
				::SSL_CTX_set_max_proto_version((* ret.first)->ctx, TLS1_3_VERSION);
				// Устанавливаем все основные алгоритмы шифрования
				if(::SSL_CTX_set_cipher_list((* ret.first)->ctx, ::__awh_ssl_ciphers__.c_str()) != 1){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Set SSL ciphers failed");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				/**
				 * Если версия OpenSSL соответствует или выше версии 3.0.0
				 */
				#if OPENSSL_VERSION_NUMBER >= 0x30000000L
					// Выполняем установку кривых P-256, P-384 и P-521
					if(::SSL_CTX_set1_curves_list((* ret.first)->ctx, "P-521:P-384:P-256") != 1){
						// Получаем текст ошибки
						const string error = ::ssl::error(result, "Set SSL CURVEs list failed");
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									static_cast <uint16_t> (node),
									static_cast <uint16_t> (proto)
								), log_t::flag_t::CRITICAL, error.c_str()
							);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
						// Удаляем контекст TLS из контейнера уровней защищённых сокетов
						(* ret.first)->erase(::__awh_ssl_members__);
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
						// Получаем текст ошибки
						const string error = ::ssl::error(result, "Set new SSL CURVE name failed");
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug(
								"%s", __PRETTY_FUNCTION__,
								std::make_tuple(
									static_cast <uint16_t> (node),
									static_cast <uint16_t> (proto)
								), log_t::flag_t::CRITICAL, error.c_str()
							);
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
						// Удаляем контекст TLS из контейнера уровней защищённых сокетов
						(* ret.first)->erase(::__awh_ssl_members__);
						// Выходим
						return 0;
					}
					// Выполняем установку кривых P-256
					::SSL_CTX_set_tmp_ecdh((* ret.first)->ctx, ecdh);
					// Выполняем очистку объекта кривой
					::EC_KEY_free(ecdh);
				#endif
				// Выполняем установку идентификатора сессии
				if(::SSL_CTX_set_session_id_context((* ret.first)->ctx, reinterpret_cast <const uint8_t *> (&result), sizeof(result)) != 1){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Failed to set session ID");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Устанавливаем поддерживаемые кривые
				if(SSL_CTX_set_ecdh_auto((* ret.first)->ctx, 1) != 1){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Set SSL ECDH failed");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Получаем данные стора
				X509_STORE * store = ::SSL_CTX_get_cert_store((* ret.first)->ctx);
				// Если стор не получен
				if(store == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Get x509 store is failed");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
							static_cast <uint16_t> (node),
							static_cast <uint16_t> (proto)
						), log_t::flag_t::WARNING, error.c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
					#endif
				// Если стор получен
				} else {
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Проверяем существует ли путь
						if(!::ssl::addCertToStore(store, "CA", (* ret.first).get()) ||
						   !::ssl::addCertToStore(store, "ROOT", (* ret.first).get()) ||
						   !::ssl::addCertToStore(store, "AuthRoot", (* ret.first).get()))
							// Выходим из функции
							return;
					#endif
					// Если стор не устанавливается, тогда выводим ошибку
					if(::X509_STORE_set_default_paths(store) == 0){
						// Получаем текст ошибки
						const string error = ::ssl::error(result, "Set default paths for x509 store is not allowed");
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
						#endif
					}
				}
				// Устанавливаем, что мы должны читать как можно больше входных байтов
				::SSL_CTX_set_read_ahead((* ret.first)->ctx, 1);
				// Устанавливаем флаг quiet shutdown
				// ::SSL_CTX_set_quiet_shutdown((* ret.first)->ctx, 1);
				// Устанавливаем флаг очистки буферов на чтение и запись когда они не требуются
				::SSL_CTX_set_mode((* ret.first)->ctx, SSL_MODE_RELEASE_BUFFERS);
				// Выполняем отключение SSL кеша
				// ::SSL_CTX_set_session_cache_mode((* ret.first)->ctx, SSL_SESS_CACHE_OFF);
				// Запускаем кэширование
				::SSL_CTX_set_session_cache_mode((* ret.first)->ctx, SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_NO_INTERNAL);
				// Устанавливаем пути по умолчанию для проверки сертификатов
				::SSL_CTX_set_default_verify_paths((* ret.first)->ctx);
				// Устанавливаем проверку сертификата сервера
				::SSL_CTX_set_verify((* ret.first)->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, &::verify::certificate);
				/**
				 * Определяем тип протокола подключения
				 */
				switch(static_cast <uint8_t> (proto)){
					// Если протокол подключения UDP
					case static_cast <uint8_t> (event::protocol_t::UDP): {
						// Выполняем проверку файлов печенок
						::SSL_CTX_set_cookie_verify_cb((* ret.first)->ctx, &::cookie::verify);
						// Выполняем генерацию файлов печенок
						::SSL_CTX_set_cookie_generate_cb((* ret.first)->ctx, &::cookie::generate);
					} break;
					// Если протокол подключения TCP
					case static_cast <uint8_t> (event::protocol_t::TCP):
					// Если протокол подключения SCTP
					case static_cast <uint8_t> (event::protocol_t::SCTP): {
						// Выполняем проверку файлов печенок
						::SSL_CTX_set_stateless_cookie_verify_cb((* ret.first)->ctx, &::cookie::verifyStateless);
						// Выполняем генерацию файлов печенок
						::SSL_CTX_set_stateless_cookie_generate_cb((* ret.first)->ctx, &::cookie::generateStateless);
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
					// Устанавливаем функцию обратного вызова для переключения протокола на другой
					::SSL_CTX_set_alpn_select_cb((* ret.first)->ctx, &::ssl::serverNextProtoSelect, (* ret.first).get());
				#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
				// Создаем SSL объект
				(* ret.first)->ssl = ::SSL_new((* ret.first)->ctx);
				// Если объект не создан
				if((* ret.first)->ssl == nullptr){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Could not create TLS session object");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
					#endif
					// Удаляем контекст TLS из контейнера уровней защищённых сокетов
					(* ret.first)->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Привязываем текущий объект TLS к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::__awh_ssl_index__[0], (* ret.first).get());
				// Привязываем текущий объект фреймворка к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::__awh_ssl_index__[1], const_cast <fmk_t *> (this->_fmk));
				// Привязываем текущий объект лога к SSL объекту
				::SSL_set_ex_data((* ret.first)->ssl, ::__awh_ssl_index__[2], const_cast <log_t *> (this->_log));
				// Создаём объект BIO для чтения
				(* ret.first)->rbio = ::BIO_new(::BIO_s_mem());
				// Создаём объект BIO для записи
				(* ret.first)->wbio = ::BIO_new(::BIO_s_mem());
				// Если один из объектов BIO не создан
				if(((* ret.first)->rbio == nullptr) || ((* ret.first)->wbio == nullptr)){
					// Получаем текст ошибки
					const string error = ::ssl::error(result, "Create BIO is failed");
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"%s", __PRETTY_FUNCTION__,
							std::make_tuple(
								static_cast <uint16_t> (node),
								static_cast <uint16_t> (proto)
							), log_t::flag_t::CRITICAL, error.c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
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
					(* ret.first)->erase(::__awh_ssl_members__);
					// Выходим
					return 0;
				}
				// Привязываем объекты BIO к SSL объекту
				::SSL_set_bio((* ret.first)->ssl, (* ret.first)->rbio, (* ret.first)->wbio);
				// Устанавливаем режим сервера для SSL объекта
				::SSL_set_accept_state((* ret.first)->ssl);
				/**
				 * Если операционной системой является Linux или FreeBSD и включён режим отладки
				 */
				#if (__linux__ || __FreeBSD__) && DEBUG_MODE
					// Если протокол интернета установлен как SCTP
					if(proto == event::protocol_t::SCTP)
						// Устанавливаем функцию нотификации
						::BIO_dgram_sctp_notification_cb((* ret.first)->rbio, &::sctp::notifications, (* ret.first).get());
				#endif
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
							static_cast <uint16_t> (proto)
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
					static_cast <uint16_t> (proto)
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
			// Выполняем загрузки описаний ошибок TLS/SSL
			::ERR_load_SSL_strings();
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
		::__awh_ssl_index__[0] = ::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
		// Регистрируем новый индекс для хранения объекта фреймворка AWH в структуре SSL
		::__awh_ssl_index__[1] = ::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
		// Регистрируем новый индекс для хранения объекта логирования AWH в структуре SSL
		::__awh_ssl_index__[2] = ::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
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
