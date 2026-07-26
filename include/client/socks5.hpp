/**
 * @file: socks5.hpp
 * @date: 2026-05-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл клиента SOCKS5-прокси — публичный API класса client::Socks5,
 *        выполняющего согласование методов авторизации, установку туннеля через прокси-сервер (CONNECT, BIND,
 *        UDP ASSOCIATE) и проксирование прикладного трафика поверх базового клиента
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовка
 */
#ifndef __AWH_CLIENT_SOCKS5__
#define __AWH_CLIENT_SOCKS5__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "client.hpp"
#include "../proto/socks5/client.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён клиента
	 *
	 */
	namespace client {
		/**
		 * @brief Класс клиента SOCKS5-прокси
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Socks5 : public client_t {
			private:
				/**
				 * @brief Структура конечной точки клиента, работающего через прокси
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Endpoint {
					/**
					 * @brief Идентификатор события UDP-клиента
					 *
					 */
					struct {
						// Идентификатор события клиента
						event::id_t eid = 0;
						// Объект контекста заголовка UDP пакета
						proto::socks5_t::udp_head_t ctx{};
					} udp;
					// Атрибуты сети для конечной точки
					unique_ptr <net::attr_t> attr;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Endpoint() noexcept;
				} endpoint_t;
			private:
				// Конечная точка клиента, работающего через прокси
				endpoint_t _endpoint;
			private:
				// Контекст для хранения параметров сообщений
				proto::socks5_t::ctx_t _ctx;
			private:
				// Объект для работы с протоколом SOCKS5
				proto::client_socks5_t _socks5;
			private:
				// Буфер накопления входящих SOCKS5-кадров по TCP
				vector <uint8_t> _rx;
			private:
				/**
				 * @brief Метод изменения статуса клиента
				 *
				 * @param index  индекс очереди запускаемого события
				 * @param status новый статус клиента
				 *
				 */
				void status(const uint8_t index, const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод обработки событий подключения клиента к удалённому серверу
				 *
				 * @param eid идентификатор клиента
				 * @param ok  результат подключения
				 *
				 */
				void connect(const event::id_t eid, const bool ok) noexcept;
			private:
				/**
				 * @brief Метод обработки событий записи данных клиентом
				 *
				 * @param      идентификатор клиента
				 * @param size размер данных для записи
				 *
				 */
				void write(const event::id_t, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий изменения состояния клиента
				 *
				 * @param eid    идентификатор клиента
				 * @param status новый статус клиента
				 *
				 */
				void state(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки событий получения данных клиентом
				 *
				 * @param eid    идентификатор клиента
				 * @param buffer буфер данных клиента
				 * @param size   размер данных клиента
				 *
				 */
				void read(const event::id_t eid, const uint8_t * buffer, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод разрешения доменного имени удалённого хоста в сетевой адрес
				 *
				 * @param        идентификатор DNS-запроса
				 * @param family семейство адресов (IPv4/IPv6)
				 * @param domain доменное имя для разрешения
				 * @param addr   указатель на структуру для хранения результата разрешения
				 *
				 */
				void resolve(const unit::dns_t::id_t, const event::family_t family, const string & domain, const net::addr_t * addr) noexcept;
			private:
				/**
				 * @brief Метод получения состояния TLS
				 *
				 * @param       идентификатор TLS
				 * @param state состояние TLS
				 *
				 */
				void stateTLS(const tls::coder_t::id_t, const tls::coder_t::state_t state) noexcept;
				/**
				 * @brief Метод получения событий шифрования/дешифрования данных TLS
				 *
				 * @param        идентификатор TLS
				 * @param event  тип события TLS
				 * @param buffer буфер данных для события шифрования/дешифрования TLS
				 * @param size   размер данных для события шифрования/дешифрования TLS
				 *
				 */
				void processTLS(const tls::coder_t::id_t, const tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод приостановки работы клиента
				 *
				 * @return результат выполнения приостановки работы
				 *
				 */
				bool pause() noexcept;
				/**
				 * @brief Метод возобновления работы клиента
				 *
				 * @return результат выполнения возобновления работы
				 *
				 */
				bool resume() noexcept;
			public:
				/**
				 * @brief Метод мультиподключения клиентов к удалённым хостам (заглушка для клиента SOCKS5)
				 *
				 * @return результат выполнения подключения
				 *
				 */
				bool connect() noexcept;
				/**
				 * @brief Метод отключения клиента от удалённого сервера (заглушка для клиента SOCKS5)
				 *
				 * @return результат выполнения отключения
				 *
				 */
				bool disconnect() noexcept;
			public:
				/**
				 * @brief Метод получения данных от сервера
				 *
				 * @return результат получения данных
				 *
				 */
				bool recv() noexcept;
				/**
				 * @brief Метод отправки данных серверу
				 *
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных серверу
				 *
				 */
				size_t send(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод объединения данных между клиентом и другим событием (заглушка для клиента SOCKS5)
				 *
				 * @return результат выполнения объединения
				 *
				 */
				bool splice(const event::id_t, const event::direct_t) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности клиента
				 *
				 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
				 * @param bandwidth пропускная способность клиента для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 *
				 */
				bool bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для клиента SOCKS5)
				 *
				 * @return результат выполнения установки
				 *
				 */
				bool membership(const event::mode_t, string_view, string_view, const uint16_t) noexcept;
				/**
				 * @brief Метод активации/деактивации мультикаст группы (заглушка для клиента SOCKS5)
				 *
				 * @return результат выполнения установки
				 *
				 */
				bool membership(const event::mode_t, const net::addr_t *, const net::addr_t *, const uint16_t) noexcept;
			public:
				/**
				 * @brief Метод установки параметров авторизации
				 *
				 * @param username имя пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 *
				 */
				void setUser(const string & username, const string & password) noexcept;
			public:
				/**
				 * @brief Метод установки исходящего адреса для UDP-клиента
				 *
				 * @param addr исходящий адрес для UDP-клиента
				 * @return 	   результат выполнения установки исходящего адреса для UDP-клиента
				 *
				 */
				bool udp(const net::attr_net_t * addr) noexcept;
				/**
				 * @brief Метод установки исходящего адреса для UDP-клиента
				 *
				 * @param addr исходящий адрес для UDP-клиента
				 * @param port исходящий порт для UDP-клиента
				 * @return     результат выполнения установки исходящего адреса для UDP-клиента
				 *
				 */
				bool udp(string_view addr, const uint16_t port = 0) noexcept;
			public:
				/**
				 * @brief Метод установки конечной точки клиента
				 *
				 * @param attr параметры подключения для установки конечной точки
				 * @return     результат выполнения установки конечной точки
				 *
				 */
				bool endpoint(const net::attr_t * attr) noexcept;
				/**
				 * @brief Метод установки конечной точки клиента
				 *
				 * @param addr адрес хоста для установки
				 * @param port порт хоста для установки
				 * @return     результат выполнения установки конечной точки
				 *
				 */
				bool endpoint(string_view addr, const uint16_t port) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещён)
				 *
				 */
				Socks5(const Socks5 &) = delete;
				/**
				 * @brief Оператор копирования (запрещён)
				 *
				 * @return текущее значение объекта
				 *
				 */
				Socks5 & operator = (const Socks5 &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Socks5(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param dns объект DNS-резолвера
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Socks5(unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param ctl   идентификатор контекста безопасности
				 * @param coder объект транспортного уровня безопасности
				 * @param fmk   объект фреймворка
				 * @param log   объект для работы с логами
				 *
				 */
				explicit Socks5(const tls::coder_t::id_t ctl, tls::coder_t * coder, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param ctl   идентификатор контекста безопасности
				 * @param coder объект транспортного уровня безопасности
				 * @param dns   объект DNS-резолвера
				 * @param fmk   объект фреймворка
				 * @param log   объект для работы с логами
				 *
				 */
				explicit Socks5(const tls::coder_t::id_t ctl, tls::coder_t * coder, unit::dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Деструктор
				 *
				 */
				~Socks5() noexcept;
		} socks5_t;
	};
};

#endif // __AWH_CLIENT_SOCKS5__
