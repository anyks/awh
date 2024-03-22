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
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_CORE_NODE__
#define __AWH_CORE_NODE__

/**
 * Стандартные модули
 */
#include <set>
#include <map>
#include <mutex>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include <net/uri.hpp>
#include <net/dns.hpp>
#include <net/engine.hpp>
#include <core/core.hpp>
#include <scheme/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Node Класс рабочей ноды сетевого ядра
	 */
	typedef class Node : public awh::core_t {
		public:
			/**
			 * SSL Структура SSL-параметров
			 */
			typedef struct SSL {
				// Флаг выполнения валидации доменного имени
				bool verify;
				// Адрес CA-файла
				string ca;
				// Адрес ключа сертификата
				string key;
				// Адрес файла сертификата
				string cert;
				// Путь где хранится CA-файл
				string capath;
				// Список алгоритмов шифрования для установки
				vector <string> ciphers;
				/**
				 * SSL Конструктор
				 */
				SSL() noexcept : verify(true), ca{""}, key{""}, cert{""}, capath{""} {}
			} ssl_t;
		protected:
			/**
			 * Settings Структура текущих параметров сети
			 */
			typedef struct Settings {
				// Флаг работы в режиме только IPv6
				bool ipV6only;
				// Протокол активного подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
				engine_t::proto_t proto;
				// Тип сокета подключения (TCP / UDP)
				scheme_t::sonet_t sonet;
				// Тип протокола интернета (IPV4 / IPV6 / NIX)
				scheme_t::family_t family;
				// Адрес файла unix-сокета
				string sockname;
				// Параметры для сети
				vector <string> network;
				/**
				 * Settings Конструктор
				 */
				Settings() noexcept :
				 ipV6only(false),
				 proto(engine_t::proto_t::RAW),
				 sonet(scheme_t::sonet_t::TCP),
				 family(scheme_t::family_t::IPV4),
				 sockname{""}, network{"0.0.0.0","[::]"} {}
			} settings_t;
		private:
			// Мютекс для блокировки потоков
			mutex _mtx;
		protected:
			// Объект работы с файловой системой
			fs_t _fs;
			// Объект работы с IP-адресами
			net_t _net;
			// Объект работы с URI-адресами
			uri_t _uri;
			// Объект для работы с сетевым двигателем
			engine_t _engine;
			// Объект сетевых параметров
			settings_t _settings;
		protected:
			// Список занятых процессов брокера
			set <uint64_t> _busy;
			// Список брокеров подключения
			map <uint64_t, uint16_t> _brokers;
			// Список активных схем сети
			map <uint16_t, const scheme_t *> _schemes;
		protected:
			// Создаём объект DNS-резолвера
			const dns_t * _dns;
		protected:
			/**
			 * remove Метод удаления всех схем сети
			 */
			void remove() noexcept;
			/**
			 * remove Метод удаления схемы сети
			 * @param sid идентификатор схемы сети
			 */
			void remove(const uint16_t sid) noexcept;
			/**
			 * remove Метод удаления брокера подключения
			 * @param bid идентификатор брокера
			 */
			void remove(const uint64_t bid) noexcept;
		protected:
			/**
			 * has Метод проверки существования схемы сети
			 * @param sid идентификатор схемы сети
			 * @return    результат проверки
			 */
			bool has(const uint16_t sid) const noexcept;
			/**
			 * has Метод проверки существования брокера подключения
			 * @param bid идентификатор брокера
			 * @return    результат проверки
			 */
			bool has(const uint64_t bid) const noexcept;
		public:
			/**
			 * sid Метод извлечения идентификатора схемы сети
			 * @param bid идентификатор брокера
			 * @return    идентификатор схемы сети
			 */
			uint16_t sid(const uint64_t bid) const noexcept;
		protected:
			/**
			 * broker Метод извлечения брокера подключения
			 * @param bid идентификатор брокера
			 * @return    объект брокера подключения
			 */
			const scheme_t::broker_t * broker(const uint64_t bid) const noexcept;
		public:
			/**
			 * scheme Метод добавления схемы сети
			 * @param scheme схема рабочей сети
			 * @return       идентификатор схемы сети
			 */
			uint16_t scheme(const scheme_t * scheme) noexcept;
		public:
			/**
			 * ssl Метод установки SSL-параметров
			 * @param ssl параметры SSL для установки
			 */
			void ssl(const ssl_t & ssl) noexcept;
		public:
			/**
			 * resolver Метод установки объекта DNS-резолвера
			 * @param dns объект DNS-резолвер
			 */
			void resolver(const dns_t * dns) noexcept;
		public:
			/**
			 * sockname Метод установки адреса файла unix-сокета
			 * @param name адрес файла unix-сокета
			 * @return     результат установки unix-сокета
			 */
			bool sockname(const string & name = "") noexcept;
		public:
			/**
			 * proto Метод извлечения поддерживаемого протокола подключения
			 * @return поддерживаемый протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			engine_t::proto_t proto() const noexcept;
			/**
			 * proto Метод извлечения активного протокола подключения
			 * @param bid идентификатор брокера
			 * @return    активный протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			engine_t::proto_t proto(const uint64_t bid) const noexcept;
			/**
			 * proto Метод установки поддерживаемого протокола подключения
			 * @param proto устанавливаемый протокол (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			void proto(const engine_t::proto_t proto) noexcept;
		public:
			/**
			 * sonet Метод извлечения типа сокета подключения
			 * @return тип сокета подключения (TCP / UDP / SCTP)
			 */
			scheme_t::sonet_t sonet() const noexcept;
			/**
			 * sonet Метод установки типа сокета подключения
			 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
			 */
			void sonet(const scheme_t::sonet_t sonet) noexcept;
		public:
			/**
			 * family Метод извлечения типа протокола интернета
			 * @return тип протокола интернета (IPV4 / IPV6 / NIX)
			 */
			scheme_t::family_t family() const noexcept;
			/**
			 * family Метод установки типа протокола интернета
			 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
			 */
			void family(const scheme_t::family_t family) noexcept;
		public:
			/**
			 * bandwidth Метод установки пропускной способности сети
			 * @param bid   идентификатор брокера
			 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
			 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
			 */
			virtual void bandwidth(const uint64_t bid, const string & read, const string & write) noexcept;
		public:
			/**
			 * events Метод активации/деактивации метода события сокета
			 * @param bid    идентификатор брокера
			 * @param mode   сигнал активации сокета
			 * @param method метод режима работы
			 */
			void events(const uint64_t bid, const awh::scheme_t::mode_t mode, const engine_t::method_t method) noexcept;
		public:
			/**
			 * network Метод установки параметров сети
			 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
			 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
			 * @param sonet  тип сокета подключения (TCP / UDP)
			 */
			void network(const vector <string> & ips = {}, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
		public:
			/**
			 * operator Оператор извлечения поддерживаемого протокола подключения
			 * @return поддерживаемый протокол подключения (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 */
			operator engine_t::proto_t() const noexcept;
			/**
			 * operator Оператор извлечения типа сокета подключения
			 * @return тип сокета подключения (TCP / UDP / SCTP)
			 */
			operator scheme_t::sonet_t() const noexcept;
			/**
			 * operator Оператор извлечения типа протокола интернета
			 * @return тип протокола интернета (IPV4 / IPV6 / NIX)
			 */
			operator scheme_t::family_t() const noexcept;
		public:
			/**
			 * Оператор [=] установки SSL-параметров
			 * @param ssl параметры SSL для установки
			 * @return    текущий объект
			 */
			Node & operator = (const ssl_t & ssl) noexcept;
			/**
			 * Оператор [=] установки объекта DNS-резолвера
			 * @param dns объект DNS-резолвер
			 * @return    текущий объект
			 */
			Node & operator = (const dns_t & dns) noexcept;
		public:
			/**
			 * Оператор [=] установки поддерживаемого протокола подключения
			 * @param proto устанавливаемый протокол (RAW, HTTP1, HTTP1_1, HTTP2, HTTP3)
			 * @return      текущий объект
			 */
			Node & operator = (const engine_t::proto_t proto) noexcept;
			/**
			 * Оператор [=] установки типа сокета подключения
			 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
			 * @return      текущий объект
			 */
			Node & operator = (const scheme_t::sonet_t sonet) noexcept;
			/**
			 * Оператор [=] установки типа протокола интернета
			 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
			 * @return       текущий объект
			 */
			Node & operator = (const scheme_t::family_t family) noexcept;
		public:
			/**
			 * Core Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Node(const fmk_t * fmk, const log_t * log) noexcept :
			 awh::core_t(fmk, log), _fs(fmk, log), _uri(fmk), _engine(fmk, log, &_uri), _dns(nullptr) {}
			/**
			 * Core Конструктор
			 * @param dns объект DNS-резолвера
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Node(const dns_t * dns, const fmk_t * fmk, const log_t * log) noexcept :
			 awh::core_t(fmk, log), _fs(fmk, log), _uri(fmk), _engine(fmk, log, &_uri), _dns(dns) {}
			/**
			 * ~Node Деструктор
			 */
			virtual ~Node() noexcept;
	} node_t;
};

#endif // __AWH_CORE_NODE__
