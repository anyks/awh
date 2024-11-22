/**
 * @file: core.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_SOCKS5__
#define __AWH_SOCKS5__

/**
 * Стандартные модули
 */
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <ws2tcpip.h>
// Если - это Unix
#else
	#include <arpa/inet.h>
#endif

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/uri.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Socks5 Класс работы с socks5 прокси-сервером
	 */
	typedef class AWHSHARED_EXPORT Socks5 {
		private:
			// Список ответов сервера
			std::map <uint8_t, string> _responses;
		public:
			/**
			 * Коды ответа socks5 сервера
			 */
			enum class rep_t : uint8_t {
				SUCCESS   = 0x00, // Подключение успешное
				SOCKSERR  = 0x01, // Ошибка SOCKS-сервера
				FORBIDDEN = 0x02, // Соединение запрещено набором правил
				UNAVNET   = 0x03, // Сеть недоступна
				UNAVHOST  = 0x04, // Хост недоступен
				DENIED    = 0x05, // Отказ в соединении
				TIMETTL   = 0x06, // Истечение TTL
				NOCOMMAND = 0x07, // Команда не поддерживается
				NOADDR    = 0x08, // Тип адреса не поддерживается
				NOSUPPORT = 0x09  // До X'FF' не определены
			};
			/**
			 * Стейты работы модуля
			 */
			enum class state_t : uint8_t {
				END       = 0x00, // Режим завершения сбора данных
				AUTH      = 0x01, // Режим ожидания прохождения аутентификации
				METHOD    = 0x02, // Режим ожидания получения метода
				BROKEN    = 0x03, // Режим бракованных данных
				CONNECT   = 0x04, // Режим выполнения подключения
				REQUEST   = 0x05, // Режим ожидания получения запроса
				RESPONSE  = 0x06, // Режим ожидания получения ответа
				HANDSHAKE = 0x07  // Режим выполненного рукопожатия
			};
		protected:
			/**
			 * Типы адресации socks5
			 */
			enum class atyp_t : uint8_t {
				IPv4   = 0x01, // Поддерживается IPv4 IP адрес
				DMNAME = 0x03, // Поддерживается доменное имя
				IPv6   = 0x04  // Поддерживается IPv6 IP адрес
			};
			/**
			 * Комманды запроса socks5 клиента
			 */
			enum class cmd_t : uint8_t {
				CONNECT = 0x01, // Метод подключения
				BIND    = 0x02, // Метод обратного подключения (сервера к клиенту)
				UDP     = 0x03  // Работа с UDP протоколом
			};
			/**
			 * Основные методы socks5
			 */
			enum class method_t : uint8_t {
				NOAUTH   = 0x00, // Аутентификация не требуется
				GSSAPI   = 0x01, // Аутентификация по GSSAPI
				PASSWD   = 0x02, // Аутентификация по USERNAME/PASSWORD
				IANA     = 0x03, // До X'7F' зарезервировано IANA
				RESERVE  = 0x80, // До X'FE' преднозначено для частных методов
				NOMETHOD = 0xFF  // Нет применимых методов
			};
		protected:
			// Устанавливаем версию прокси-протокола
			static constexpr uint8_t VER = 0x05;
			// Устанавливаем версию соглашения авторизации
			static constexpr uint8_t AVER = 0x01;
		protected:
			/**
			 * ResMethod Структура ответа с выбранным методом
			 */
			typedef struct ResMethod {
				uint8_t ver;    // Версия прокси-протокола
				uint8_t method; // Выбранный метод сервера
				/**
				 * ResMethod Конструктор
				 */
				ResMethod() noexcept : ver(0x0), method(0x0) {}
			} __attribute__((packed)) res_method_t;
			/**
			 * Auth Структура ответа на авторизацию
			 */
			typedef struct Auth {
				uint8_t ver;    // Версия прокси-протокола
				uint8_t status; // Статус авторизации на сервере
				/**
				 * Auth Конструктор
				 */
				Auth() noexcept : ver(0x0), status(0x0) {}
			} __attribute__((packed)) auth_t;
			/**
			 * Req Структура запроса
			 */
			typedef struct Req {
				uint8_t ver;  // Версия прокси-протокола
				uint8_t cmd;  // Код запроса у прокси-сервера
				uint8_t rsv;  // Зарезервированный октет
				uint8_t atyp; // Тип подключения
				/**
				 * Req Конструктор
				 */
				Req() noexcept : ver(0x0), cmd(0x0), rsv(0x0), atyp(0x0) {}
			} __attribute__((packed)) req_t;
			/**
			 * Res Структура ответа
			 */
			typedef struct Res {
				uint8_t ver;  // Версия прокси-протокола
				uint8_t rep;  // Код ответа прокси-сервера
				uint8_t rsv;  // Зарезервированный октет
				uint8_t atyp; // Тип подключения
				/**
				 * Resp Конструктор
				 */
				Res() noexcept : ver(0x0), rep(0x0), rsv(0x0), atyp(0x0) {}
			} __attribute__((packed)) res_t;
			/**
			 * IP Структура ip адреса сервера
			 */
			typedef struct IP {
				uint32_t host; // Хост сервера
				uint16_t port; // Порт сервера
				/**
				 * IP Конструктор
				 */
				IP() noexcept : host(0x0), port(0x0) {}
			} __attribute__((packed)) ip_t;
		protected:
			// URL параметры REST запроса
			uri_t::url_t _url;
		protected:
			// Код сообщения
			uint8_t _code;
			// Стейт текущего запроса
			state_t _state;
		protected:
			// Буфер бинарных данных
			mutable vector <char> _buffer;
		protected:
			// Создаём объект работы с логами
			const log_t * _log;
		protected:
			/**
			 * ipToHex Метод конвертации IP адреса в бинарный буфер
			 * @param ip     индернет адрес в виде строки
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       бинарный буфер IP адреса
			 */
			vector <char> ipToHex(const string & ip, const int32_t family = AF_INET) const noexcept;
			/**
			 * hexToIp Метод конвертации бинарного буфера в IP адрес
			 * @param buffer бинарный буфер для конвертации
			 * @param size   размер бинарного буфера
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       IP адрес в виде строки
			 */
			string hexToIp(const char * buffer, const size_t size, const int32_t family = AF_INET) const noexcept;
		protected:
			/**
			 * text Метод установки в буфер текстовых данных
			 * @param text текст для установки
			 * @return     текущее значение смещения
			 */
			uint16_t text(const string & text) const noexcept;
			/**
			 * text Метод извлечения текстовых данных из буфера
			 * @param buffer буфер данных для извлечения текста
			 * @param size   размер буфера данных
			 * @return       текст содержащийся в буфере данных
			 */
			const string text(const char * buffer, const size_t size) const noexcept;
		protected:
			/**
			 * octet Метод установки октета
			 * @param octet  октет для установки
			 * @param offset размер смещения в буфере
			 * @return       текущее значение смещения
			 */
			uint16_t octet(const uint8_t octet, const uint16_t offset = 0) const noexcept;
		public:
			/**
			 * is Метод проверки активного состояния
			 * @param state состояние которое необходимо проверить
			 */
			bool is(const state_t state) const noexcept;
		public:
			/**
			 * code Метод получения кода сообщения
			 * @return код сообщения
			 */
			uint8_t code() const noexcept;
			/**
			 * message Метод получения сообщения
			 * @param code код сообщения
			 * @return     текстовое значение кода
			 */
			const string & message(const uint8_t code) const noexcept;
		public:
			/**
			 * get Метод извлечения буфера запроса/ответа
			 * @return бинарный буфер
			 */
			const vector <char> & get() const noexcept;
		public:
			/**
			 * parse Метод парсинга входящих данных
			 * @param buffer бинарный буфер входящих данных
			 * @param size   размер бинарного буфера входящих данных
			 */
			virtual void parse(const char * buffer = nullptr, const size_t size = 0) noexcept = 0;
		public:
			/**
			 * reset Метод сброса собранных данных
			 */
			virtual void reset() noexcept = 0;
		public:
			/**
			 * url Метод установки URL параметров REST запроса
			 * @param url параметры REST запроса
			 */
			void url(const uri_t::url_t & url) noexcept;
		public:
			/**
			 * Socks5 Конструктор
			 * @param log объект для работы с логами
			 */
			Socks5(const log_t * log) noexcept;
			/**
			 * ~Socks5 Деструктор
			 */
			virtual ~Socks5() noexcept {}
	} socks5_t;
};

#endif // __AWH_SOCKS5__
