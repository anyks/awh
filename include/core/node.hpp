/**
 * @file: node.hpp
 * @date: 2024-03-11
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

#ifndef __AWH_CORE_NODE__
#define __AWH_CORE_NODE__

/**
 * Стандартные модули
 */
#include <set>
#include <map>
#include <queue>
#include <mutex>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include "core.hpp"
#include "../net/uri.hpp"
#include "../net/dns.hpp"
#include "../net/engine.hpp"
#include "../sys/buffer.hpp"
#include "../scheme/core.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс рабочей ноды сетевого ядра
	 *
	 */
	typedef class AWHSHARED_EXPORT Node : public awh::core_t {
		public:
			/**
			 * Режим отправки сообщений
			 */
			enum class sending_t : uint8_t {
				DEFFER  = 0x01, // Режим отложенной отправки
				INSTANT = 0x02  // Режим мгновенной отправки
			};
		public:
			/**
			 * @brief Класс SSL-параметров
			 *
			 */
			typedef class AWHSHARED_EXPORT SSL {
				public:
					// Флаг выполнения валидации доменного имени
					bool verify;
				public:
					// Ключ SSL-сертификата
					string key;
					// SSL-сертификат
					string cert;
				public:
					// Сертификат центра сертификации (CA-файл)
					string ca;
					// Сертификат отозванных сертификатов (CRL-файл)
					string crl;
					// Каталог с сертификатами центра сертификации (CA-файлами)
					string capath;
				public:
					// Список алгоритмов шифрования
					vector <string> ciphers;
				public:
					/**
					 * @brief Оператор [=] перемещения SSL-параметров
					 *
					 * @param ssl объект SSL-параметров
					 * @return    объект текущий параметров
					 */
					SSL & operator = (SSL && ssl) noexcept;
					/**
					 * @brief Оператор [=] присванивания SSL-параметров
					 *
					 * @param ssl объект SSL-параметров
					 * @return    объект текущий параметров
					 */
					SSL & operator = (const SSL & ssl) noexcept;
				public:
					/**
					 * @brief Оператор сравнения
					 *
					 * @param ssl объект SSL-параметров
					 * @return    результат сравнения
					 */
					bool operator == (const SSL & ssl) noexcept;
				public:
					/**
					 * @brief Конструктор перемещения
					 *
					 * @param ssl объект SSL-параметров
					 */
					SSL(SSL && ssl) noexcept;
					/**
					 * @brief Конструктор копирования
					 *
					 * @param ssl объект SSL-параметров
					 */
					SSL(const SSL & ssl) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					SSL() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~SSL() noexcept {}
			} ssl_t;
		protected:
			/**
			 * @brief Объект основных мютексов
			 *
			 */
			typedef struct Mutex {
				// Для работы с параметрами модуля
				std::recursive_mutex main;
				// Для отправки сообщений
				std::recursive_mutex send;
			} mtx_t;
			/**
			 * @brief Структура текущих параметров сети
			 *
			 */
			typedef struct Settings {
				// Флаг работы в режиме только IPv6
				bool ipV6only;
				// Протокол активного подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
				engine_t::proto_t proto;
				// Тип сокета подключения (TCP / UDP)
				scheme_t::sonet_t sonet;
				// Тип протокола интернета (IPV4 / IPV6 / IPC)
				scheme_t::family_t family;
				// Адрес файла unix-сокета
				string sockname;
				// Адрес каталога для хранения unix-сокетов
				string sockpath;
				// Параметры для сети
				vector <string> network;
				/**
				 * @brief Конструктор
				 *
				 */
				Settings() noexcept :
				 ipV6only(false),
				 proto(engine_t::proto_t::RAW),
				 sonet(scheme_t::sonet_t::TCP),
				 family(scheme_t::family_t::IPV4),
				 sockname{""}, sockpath{"/tmp"},
				 network{"0.0.0.0","[::]"} {}
			} settings_t;
		protected:
			// Мютекс для блокировки потоков
			mtx_t _mtx;
		protected:
			// Объект работы с файловой системой
			fs_t _fs;
			// Объект IP-адресов
			net_t _net;
			// Объект работы с URI
			uri_t _uri;
			// Объект сетевого двигателя
			engine_t _engine;
			// Режим отправки сообщений
			sending_t _sending;
			// Объект сетевых параметров
			settings_t _settings;
		protected:
			// Размер буфера полезной нагрузки
			size_t _payloadSize;
		private:
			// Максимальный размер памяти для хранений полезной нагрузки всех брокеров
			size_t _memoryAvailableSize;
			// Максимальный размер хранимой полезной нагрузки для одного брокера
			size_t _brokerAvailableSize;
		protected:
			// Список занятых процессов брокера
			std::set <uint64_t> _busy;
			// Список свободной памяти хранения полезной нагрузки
			std::map <uint64_t, size_t> _available;
			// Список активных схем сети
			std::map <uint16_t, const scheme_t *> _schemes;
			// Список брокеров подключения
			std::map <uint64_t, const scheme_t::broker_t *> _brokers;
			// Буферы отправляемой полезной нагрузки
			std::map <uint64_t, std::unique_ptr <buffer_t>> _payloads;
		protected:
			// Объект DNS-резолвера
			const dns_t * _dns;
		protected:
			/**
			 * @brief Метод удаления всех схем сети
			 *
			 */
			void remove() noexcept;
			/**
			 * @brief Метод удаления схемы сети
			 *
			 * @param sid идентификатор схемы сети
			 */
			void remove(const uint16_t sid) noexcept;
			/**
			 * @brief Метод удаления брокера подключения
			 *
			 * @param bid идентификатор брокера
			 */
			void remove(const uint64_t bid) noexcept;
		protected:
			/**
			 * @brief Метод проверки существования схемы сети
			 *
			 * @param sid идентификатор схемы сети
			 * @return    результат проверки
			 */
			bool has(const uint16_t sid) const noexcept;
			/**
			 * @brief Метод проверки существования брокера подключения
			 *
			 * @param bid идентификатор брокера
			 * @return    результат проверки
			 */
			bool has(const uint64_t bid) const noexcept;
		public:
			/**
			 * @brief Метод извлечения идентификатора схемы сети
			 *
			 * @param bid идентификатор брокера
			 * @return    идентификатор схемы сети
			 */
			uint16_t sid(const uint64_t bid) const noexcept;
		protected:
			/**
			 * @brief Метод инициализации буфера полезной нагрузки
			 *
			 * @param bid идентификатор брокера
			 */
			void initBuffer(const uint64_t bid) noexcept;
			/**
			 * @brief Метод освобождение памяти занятой для хранение полезной нагрузки брокера
			 *
			 * @param bid  идентификатор брокера
			 * @param size размер байт удаляемых из буфера
			 */
			void erase(const uint64_t bid, const size_t size) noexcept;
		protected:
			/**
			 * @brief Метод извлечения брокера подключения
			 *
			 * @param bid идентификатор брокера
			 * @return    объект брокера подключения
			 */
			const scheme_t::broker_t * broker(const uint64_t bid) const noexcept;
		public:
			/**
			 * @brief Метод добавления схемы сети
			 *
			 * @param scheme схема рабочей сети
			 * @return       идентификатор схемы сети
			 */
			uint16_t scheme(const scheme_t * scheme) noexcept;
		public:
			/**
			 * @brief Метод установки SSL-параметров
			 *
			 * @param ssl параметры SSL для установки
			 */
			void ssl(const ssl_t & ssl) noexcept;
		public:
			/**
			 * @brief Метод установки объекта DNS-резолвера
			 *
			 * @param dns объект DNS-резолвер
			 */
			void resolver(const dns_t * dns) noexcept;
		public:
			/**
			 * @brief Метод установки названия unix-сокета
			 *
			 * @param name название unix-сокета
			 * @return     результат установки названия unix-сокета
			 */
			bool sockname(const string & name = "") noexcept;
			/**
			 * @brief Метод установки адреса каталога где хранится unix-сокет
			 *
			 * @param path адрес каталога в файловой системе где хранится unix-сокет
			 * @return     результат установки адреса каталога где хранится unix-сокет
			 */
			bool sockpath(const string & path = "") noexcept;
		public:
			/**
			 * @brief Метод извлечения поддерживаемого протокола подключения
			 *
			 * @return поддерживаемый протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			engine_t::proto_t proto() const noexcept;
			/**
			 * @brief Метод извлечения активного протокола подключения
			 *
			 * @param bid идентификатор брокера
			 * @return    активный протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			engine_t::proto_t proto(const uint64_t bid) const noexcept;
			/**
			 * @brief Метод установки поддерживаемого протокола подключения
			 *
			 * @param proto устанавливаемый протокол (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			void proto(const engine_t::proto_t proto) noexcept;
		public:
			/**
			 * @brief Метод извлечения типа сокета подключения
			 *
			 * @return тип сокета подключения (TCP / UDP / SCTP)
			 */
			scheme_t::sonet_t sonet() const noexcept;
			/**
			 * @brief Метод установки типа сокета подключения
			 *
			 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
			 */
			void sonet(const scheme_t::sonet_t sonet) noexcept;
		public:
			/**
			 * @brief Метод извлечения типа протокола интернета
			 *
			 * @return тип протокола интернета (IPV4 / IPV6 / IPC)
			 */
			scheme_t::family_t family() const noexcept;
			/**
			 * @brief Метод установки типа протокола интернета
			 *
			 * @param family тип протокола интернета (IPV4 / IPV6 / IPC)
			 */
			void family(const scheme_t::family_t family) noexcept;
		public:
			/**
			 * @brief Метод получения режима отправки сообщений
			 *
			 * @return установленный режим отправки сообщений
			 */
			sending_t sending() const noexcept;
			/**
			 * @brief Метод установки режима отправки сообщений
			 *
			 * @param sending режим отправки сообщений для установки
			 */
			void sending(const sending_t sending) noexcept;
		public:
			/**
			 * @brief Метод получения максимального рамзера памяти для хранения полезной нагрузки всех брокеров
			 *
			 * @return размер памяти для хранения полезной нагрузки всех брокеров
			 */
			size_t memoryAvailableSize() const noexcept;
			/**
			 * @brief Метод установки максимального рамзера памяти для хранения полезной нагрузки всех брокеров
			 *
			 * @param size размер памяти для хранения полезной нагрузки всех брокеров
			 */
			void memoryAvailableSize(const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения максимального размера хранимой полезной нагрузки для одного брокера
			 *
			 * @return размер хранимой полезной нагрузки для одного брокера
			 */
			size_t brokerAvailableSize() const noexcept;
			/**
			 * @brief Метод получения размера хранимой полезной нагрузки для текущего брокера
			 *
			 * @param bid идентификатор брокера
			 * @return    размер хранимой полезной нагрузки для текущего брокера
			 */
			size_t brokerAvailableSize(const uint64_t bid) const noexcept;
			/**
			 * @brief Метод установки максимального размера хранимой полезной нагрузки для одного брокера
			 *
			 * @param size размер хранимой полезной нагрузки для одного брокера
			 */
			void brokerAvailableSize(const size_t size) noexcept;
		public:
			/**
			 * @brief Метод отключения/включения алгоритма TCP/CORK
			 *
			 * @param bid  идентификатор брокера
			 * @param mode режим применимой операции
			 * @return     результат выполенния операции
			 */
			bool cork(const uint64_t bid, const engine_t::mode_t mode) noexcept;
			/**
			 * @brief Метод отключения/включения алгоритма Нейгла
			 *
			 * @param bid  идентификатор брокера
			 * @param mode режим применимой операции
			 * @return     результат выполенния операции
			 */
			bool nodelay(const uint64_t bid, const engine_t::mode_t mode) noexcept;
		public:
			/**
			 * @brief Метод асинхронной отправки буфера данных в сокет
			 *
			 * @param buffer буфер для записи данных
			 * @param size   размер записываемых данных
			 * @param bid    идентификатор брокера
			 * @return       результат отправки сообщения
			 */
			virtual bool send(const char * buffer, const size_t size, const uint64_t bid) noexcept;
		public:
			/**
			 * @brief Метод установки пропускной способности сети
			 *
			 * @param bid   идентификатор брокера
			 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
			 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
			 */
			virtual void bandwidth(const uint64_t bid, const string & read = "", const string & write = "") noexcept;
		public:
			/**
			 * @brief Метод активации/деактивации метода события сокета
			 *
			 * @param bid    идентификатор брокера
			 * @param mode   сигнал активации сокета
			 * @param method метод режима работы
			 */
			void events(const uint64_t bid, const awh::scheme_t::mode_t mode, const engine_t::method_t method) noexcept;
		public:
			/**
			 * @brief Метод установки параметров сети
			 *
			 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
			 * @param family тип протокола интернета (IPV4 / IPV6 / IPC)
			 * @param sonet  тип сокета подключения (TCP / UDP)
			 */
			void network(const vector <string> & ips = {}, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
		public:
			/**
			 * @brief Оператор извлечения поддерживаемого протокола подключения
			 *
			 * @return поддерживаемый протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			operator engine_t::proto_t() const noexcept;
			/**
			 * @brief Оператор извлечения типа сокета подключения
			 *
			 * @return тип сокета подключения (TCP / UDP / SCTP)
			 */
			operator scheme_t::sonet_t() const noexcept;
			/**
			 * @brief Оператор извлечения типа протокола интернета
			 *
			 * @return тип протокола интернета (IPV4 / IPV6 / IPC)
			 */
			operator scheme_t::family_t() const noexcept;
		public:
			/**
			 * @brief Оператор [=] установки SSL-параметров
			 *
			 * @param ssl параметры SSL для установки
			 * @return    текущий объект
			 */
			Node & operator = (const ssl_t & ssl) noexcept;
			/**
			 * @brief Оператор [=] установки объекта DNS-резолвера
			 *
			 * @param dns объект DNS-резолвер
			 * @return    текущий объект
			 */
			Node & operator = (const dns_t & dns) noexcept;
		public:
			/**
			 * @brief Оператор [=] установки поддерживаемого протокола подключения
			 *
			 * @param proto устанавливаемый протокол (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 * @return      текущий объект
			 */
			Node & operator = (const engine_t::proto_t proto) noexcept;
			/**
			 * @brief Оператор [=] установки типа сокета подключения
			 *
			 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
			 * @return      текущий объект
			 */
			Node & operator = (const scheme_t::sonet_t sonet) noexcept;
			/**
			 * @brief Оператор [=] установки типа протокола интернета
			 *
			 * @param family тип протокола интернета (IPV4 / IPV6 / IPC)
			 * @return       текущий объект
			 */
			Node & operator = (const scheme_t::family_t family) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Node(const fmk_t * fmk, const log_t * log) noexcept :
			 awh::core_t(fmk, log), _fs(fmk, log), _net(log), _uri(fmk, log),
			 _engine(fmk, log, &_uri), _sending(sending_t::INSTANT),
			 _payloadSize(0), _memoryAvailableSize(AWH_WINDOW_SIZE),
			 _brokerAvailableSize(AWH_PAYLOAD_SIZE), _dns(nullptr) {}
			/**
			 * @brief Конструктор
			 *
			 * @param dns объект DNS-резолвера
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Node(const dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
			 awh::core_t(fmk, log), _fs(fmk, log), _net(log), _uri(fmk, log),
			 _engine(fmk, log, &_uri), _sending(sending_t::INSTANT),
			 _payloadSize(0), _memoryAvailableSize(AWH_WINDOW_SIZE),
			 _brokerAvailableSize(AWH_PAYLOAD_SIZE), _dns(dns) {}
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Node() noexcept;
	} node_t;
};

#endif // __AWH_CORE_NODE__
