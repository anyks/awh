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
 * @copyright: Copyright © 2025
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
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"
#include "../sys/buffer.hpp"
#include "../sys/callback.hpp"
#include "../net/engine.hpp"
#include "../events/evbase.hpp"

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
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
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
				IPC  = 0x03  // Протокол IPC (unix-сокет)
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
				uint16_t wait;    // Таймаут ожидания получения данных
				uint16_t read;    // Таймаут на чтение в секундах
				uint16_t write;   // Таймаут на запись в секундах
				uint16_t connect; // Таймаут на подключение в секундах
				/**
				 * Timeouts Конструктор
				 */
				Timeouts() noexcept :
				 wait(0), read(READ_TIMEOUT),
				 write(WRITE_TIMEOUT),
				 connect(CONNECT_TIMEOUT) {}
			} __attribute__((packed)) timeouts_t;
			/**
			 * Buffer Структура буфера полезной нагрузки
			 */
			typedef struct Buffer {
				size_t size;                    // Размер буфера
				std::unique_ptr <char []> data; // Данные буфера
				/**
				 * Buffer Конструктор
				 */
				Buffer() noexcept : size(0), data(nullptr) {}
				/**
				 * ~Buffer Деструктор
				 */
				~Buffer() noexcept {}
			} buffer_t;
		private:
			/**
			 * Broker Класс брокера подключения
			 */
			typedef class AWHSHARED_EXPORT Broker {
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
				public:
					// Буфер полезной нагрузки
					buffer_t buffer;
					// Объект таймаутов
					timeouts_t timeouts;
				private:
					// Хранилище функций обратного вызова
					callback_t _callback;
				public:
					// Контекст двигателя для работы с передачей данных
					engine_t::ctx_t ectx;
					// Объект подключения клиента
					engine_t::addr_t addr;
				private:
					// Мютекс для блокировки потока
					std::recursive_mutex _mtx;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
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
					 * callback Метод установки функций обратного вызова
					 * @param callback функции обратного вызова
					 */
					void callback(const callback_t & callback) noexcept;
				public:
					/**
					 * @tparam Шаблон метода подключения финкции обратного вызова
					 * @param T    тип функции обратного вызова
					 * @param Args аргументы функции обратного вызова
					 */
					template <typename T, class... Args>
					/**
					 * on Метод подключения финкции обратного вызова
					 * @param name  идентификатор функкции обратного вызова
					 * @param args аргументы функции обратного вызова
					 * @return     идентификатор добавленной функции обратного вызова
					 */
					auto on(const char * name, Args... args) noexcept -> uint64_t {
						// Если мы получили название функции обратного вызова
						if(name != nullptr)
							// Выполняем установку функции обратного вызова
							return this->_callback.on <T> (name, args...);
						// Выводим результат по умолчанию
						return 0;
					}
					/**
					 * @tparam Шаблон метода подключения финкции обратного вызова
					 * @param T    тип функции обратного вызова
					 * @param Args аргументы функции обратного вызова
					 */
					template <typename T, class... Args>
					/**
					 * on Метод подключения финкции обратного вызова
					 * @param name  идентификатор функкции обратного вызова
					 * @param args аргументы функции обратного вызова
					 * @return     идентификатор добавленной функции обратного вызова
					 */
					auto on(const string & name, Args... args) noexcept -> uint64_t {
						// Если мы получили название функции обратного вызова
						if(!name.empty())
							// Выполняем установку функции обратного вызова
							return this->_callback.on <T> (name, args...);
						// Выводим результат по умолчанию
						return 0;
					}
					/**
					 * @tparam Шаблон метода подключения финкции обратного вызова
					 * @param T    тип функции обратного вызова
					 * @param Args аргументы функции обратного вызова
					 */
					template <typename T, class... Args>
					/**
					 * on Метод подключения финкции обратного вызова
					 * @param fid  идентификатор функкции обратного вызова
					 * @param args аргументы функции обратного вызова
					 * @return     идентификатор добавленной функции обратного вызова
					 */
					auto on(const uint64_t fid, Args... args) noexcept -> uint64_t {
						// Если мы получили название функции обратного вызова
						if(fid > 0)
							// Выполняем установку функции обратного вызова
							return this->_callback.on <T> (fid, args...);
						// Выводим результат по умолчанию
						return 0;
					}
					/**
					 * @tparam Шаблон метода подключения финкции обратного вызова
					 * @param A    тип идентификатора функции
					 * @param B    тип функции обратного вызова
					 * @param Args аргументы функции обратного вызова
					 */
					template <typename A, typename B, class... Args>
					/**
					 * on Метод подключения финкции обратного вызова
					 * @param fid  идентификатор функкции обратного вызова
					 * @param args аргументы функции обратного вызова
					 * @return     идентификатор добавленной функции обратного вызова
					 */
					auto on(const A fid, Args... args) noexcept -> uint64_t {
						// Если мы получили на вход число
						if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
							// Выполняем установку функции обратного вызова
							return this->_callback.on <B> (static_cast <uint64_t> (fid), args...);
						// Выводим результат по умолчанию
						return 0;
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
					 * stop Метод остановки работы
					 */
					void stop() noexcept;
					/**
					 * start Метод запуска работы
					 */
					void start() noexcept;
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
					void timeout(const uint16_t seconds, const engine_t::method_t method) noexcept;
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
			std::map <uint64_t, std::unique_ptr <broker_t>> _brokers;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
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
