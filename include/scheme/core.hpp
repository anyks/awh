/**
 * @file: core.hpp
 * @date: 2024-07-08
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_SCHEME__
#define __AWH_SCHEME__

/**
 * Стандартные модули
 */
#include <map>
#include <mutex>
#include <string>

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/events.hpp>
#include <net/engine.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Core Прототип класса ядра биндинга TCP/IP
		 */
		class Core;
	};
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Core Прототип класса ядра биндинга TCP/IP
		 */
		class Core;
	};
}

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Scheme Структура схемы сети
	 */
	typedef struct AWHSHARED_EXPORT Scheme {
		private:
			/**
			 * Node Core Устанавливаем дружбу с нодой сетевого ядра
			 */
			friend class Node;
			/**
			 * Client Core Устанавливаем дружбу с клиентским классом ядра
			 */
			friend class client::Core;
			/**
			 * Server Core Устанавливаем дружбу с серверным классом ядра
			 */
			friend class server::Core;
		public:
			/**
			 * Флаги активации/деактивации
			 */
			enum class mode_t : uint8_t {
				ENABLED  = 0x01, // Включено
				DISABLED = 0x00  // Отключено
			};
			/**
			 * Семейство протоколов интернета
			 */
			enum class family_t : uint8_t {
				IPV4 = 0x01, // Протокол IPv4
				IPV6 = 0x02, // Протокол IPv6
				NIX  = 0x03  // Протокол unix-сокет
			};
			/**
			 * Тип сокета подключения
			 */
			enum class sonet_t : uint8_t {
				TCP  = 0x01, // Нешифрованное подключение TCP
				UDP  = 0x02, // Нешифрованное подключение UDP
				TLS  = 0x03, // Шифрованное подключение TCP
				DTLS = 0x04, // Шифрованное подключение UDP
				SCTP = 0x05  // Шифрованное подключение SCTP
			};
		public:
			/**
			 * Timeouts Структура таймаутов
			 */
			typedef struct Timeouts {
				time_t read;    // Таймаут на чтение в секундах
				time_t write;   // Таймаут на запись в секундах
				time_t connect; // Таймаут на подключение в секундах
				/**
				 * Timeouts Конструктор
				 */
				Timeouts() noexcept :
				 read(READ_TIMEOUT),
				 write(WRITE_TIMEOUT),
				 connect(CONNECT_TIMEOUT) {}
			} timeouts_t;
			/**
			 * Payload Структура полезной нагрузки
			 */
			typedef struct Payload {
				size_t size;               // Размер буфера
				unique_ptr <char []> data; // Данные буфера
				/**
				 * Payload Конструктор
				 */
				Payload() noexcept : size(0), data(nullptr) {}
			} payload_t;
		private:
			/**
			 * Broker Класс брокера подключения
			 */
			typedef class AWHSHARED_EXPORT Broker {
				private:
					// Мютекс для блокировки потока
					mutex _mtx;
				private:
					// Идентификатор брокера
					uint64_t _id;
				private:
					// Идентификатор схемы сети
					uint16_t _sid;
				private:
					// Адрес интернет-подключения клиента
					string _ip;
					// Мак адрес подключившегося клиента
					string _mac;
					// Порт интернет-подключения клиента
					uint32_t _port;
				private:
					// Тип сокета подключения
					sonet_t _sonet;
				private:
					// Объект основных событий
					event_t _event;
				private:
					// Хранилище функций обратного вызова
					fn_t _callbacks;
				public:
					// Буфер полезной нагрузки
					payload_t _payload;
				public:
					// Объект таймаутов
					timeouts_t _timeouts;
				public:
					// Контекст двигателя для работы с передачей данных
					engine_t::ctx_t _ectx;
					// Создаём объект подключения клиента
					engine_t::addr_t _addr;
				private:
					// Создаём объект фреймворка
					const fmk_t * _fmk;
					// Создаём объект работы с логами
					const log_t * _log;
				private:
					// База данных событий
					base_t * _base;
				public:
					/**
					 * id Метод извлечения идентификатора брокера
					 * @return идентификатор брокера
					 */
					uint64_t id() const noexcept;
				public:
					/**
					 * sid Метод извлечения идентификатора схемы сети
					 * @return идентификатор схемы сети
					 */
					uint16_t sid() const noexcept;
				public:
					/**
					 * port Метод извлечения порта подключения
					 * @return установленный порт подключения
					 */
					uint32_t port() const noexcept;
					/**
					 * port Метод установки порта подключения
					 * @param порт подключения для установки
					 */
					void port(const uint32_t port) noexcept;
				public:
					/**
					 * ip Метод извлечения IP-адреса
					 * @return установленный IP-адрес
					 */
					const string & ip() const noexcept;
					/**
					 * ip Метод установки IP-адреса
					 * @param ip адрес для установки
					 */
					void ip(const string & ip) noexcept;
				public:
					/**
					 * mac Метод извлечения MAC-адреса
					 * @return установленный MAC-адрес
					 */
					const string & mac() const noexcept;
					/**
					 * mac Метод установки MAC-адреса
					 * @param mac адрес для установки
					 */
					void mac(const string & mac) noexcept;
				public:
					/**
					 * callbacks Метод установки функций обратного вызова
					 * @param callbacks функции обратного вызова
					 */
					void callbacks(const fn_t & callbacks) noexcept;
				public:
					/**
					 * callback Шаблон метода установки финкции обратного вызова
					 * @tparam A тип функции обратного вызова
					 */
					template <typename A>
					/**
					 * callback Метод установки функции обратного вызова
					 * @param idw идентификатор функции обратного вызова
					 * @param fn  функция обратного вызова для установки
					 */
					void callback(const uint64_t idw, function <A> fn) noexcept {
						// Если функция обратного вызова передана
						if((idw > 0) && (fn != nullptr)){
							// Выполняем блокировку потока
							const lock_guard <mutex> lock(this->_mtx);
							// Выполняем установку функции обратного вызова
							this->_callbacks.set <A> (idw, fn);
						}
					}
					/**
					 * callback Шаблон метода установки финкции обратного вызова
					 * @tparam A тип функции обратного вызова
					 */
					template <typename A>
					/**
					 * callback Метод установки функции обратного вызова
					 * @param name название функции обратного вызова
					 * @param fn   функция обратного вызова для установки
					 */
					void callback(const string & name, function <A> fn) noexcept {
						// Если функция обратного вызова передана
						if(!name.empty() && (fn != nullptr)){
							// Выполняем блокировку потока
							const lock_guard <mutex> lock(this->_mtx);
							// Выполняем установку функции обратного вызова
							this->_callbacks.set <A> (name, fn);
						}
					}
				private:
					/**
					 * sonet Метод установки типа сокета подключения
					 * @param sonet тип сокета подключения
					 */
					void sonet(const sonet_t sonet) noexcept;
				private:
					/**
					 * callback Метод вызова при получении события сокета
					 * @param fd    файловый дескриптор (сокет)
					 * @param event произошедшее событие
					 */
					void callback(const SOCKET fd, const base_t::event_type_t event) noexcept;
				public:
					/**
					 * events Метод активации/деактивации метода события сокета
					 * @param mode   сигнал активации сокета
					 * @param method метод режима работы
					 */
					void events(const mode_t mode, const engine_t::method_t method) noexcept;
					/**
					 * timeout Метод установки таймаута ожидания появления данных
					 * @param seconds время ожидания в секундах
					 * @param method  метод режима работы
					 */
					void timeout(const time_t seconds, const engine_t::method_t method) noexcept;
				public:
					/**
					 * base Метод установки базы событий
					 * @param base база событий для установки
					 */
					void base(base_t * base) noexcept;
				public:
					/**
					 * Оператор [=] установки базы событий
					 * @param base база событий для установки
					 * @return     текущий объект
					 */
					Broker & operator = (base_t * base) noexcept;
				public:
					/**
					 * Broker Конструктор
					 * @param sid идентификатор схемы сети
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Broker(const uint16_t sid, const fmk_t * fmk, const log_t * log) noexcept;
					/**
					 * ~Broker Деструктор
					 */
					~Broker() noexcept;
			} broker_t;
		public:
			// Идентификатор родительской схемы
			uint16_t id;
		public:
			// Флаг автоматического поддержания подключения
			bool alive;
		public:
			// Объект таймаутов
			timeouts_t timeouts;
		public:
			// Время жизни подключения
			engine_t::alive_t keepAlive;
		protected:
			// Список подключённых брокеров
			map <uint64_t, unique_ptr <broker_t>> _brokers;
		protected:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		public:
			/**
			 * clear Метод очистки
			 */
			virtual void clear() noexcept;
		public:
			/**
			 * socket Метод извлечения сокета брокера
			 * @param bid идентификатор брокера
			 * @return    активный сокет брокера
			 */
			SOCKET socket(const uint64_t bid) const noexcept;
		public:
			/**
			 * port Метод получения порта подключения брокера
			 * @param bid идентификатор брокера
			 * @return   порт подключения брокера
			 */
			uint32_t port(const uint64_t bid) const noexcept;
			/**
			 * ip Метод получения IP-адреса брокера
			 * @param bid идентификатор брокера
			 * @return    адрес интернет подключения брокера
			 */
			const string & ip(const uint64_t bid) const noexcept;
			/**
			 * mac Метод получения MAC-адреса брокера
			 * @param bid идентификатор брокера
			 * @return    адрес устройства брокера
			 */
			const string & mac(const uint64_t bid) const noexcept;
		public:
			/**
			 * Scheme Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Scheme(const fmk_t * fmk, const log_t * log) noexcept : id(0), alive(false), _fmk(fmk), _log(log) {}
			/**
			 * ~Scheme Деструктор
			 */
			virtual ~Scheme() noexcept {}
	} scheme_t;
};

#endif // __AWH_SCHEME__
