/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_SOCKS5__
#define __AWH_SOCKS5__

/**
 * Стандартная библиотека
 */
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>

/**
 * Наши модули
 */
#include <fmk.hpp>
#include <log.hpp>
#include <uri.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Socks5 Класс работы с socks5 прокси-сервером
	 */
	typedef class Socks5 {
		private:
			/**
			 * Список сообщений системы
			 */
			map <uint8_t, string> messages = {
				{0x00, "successful"},
				{0x01, "SOCKS server error"},
				{0x02, "connection is prohibited by a set of rules"},
				{0x03, "network unavailable"},
				{0x04, "host unreachable"},
				{0x05, "connection denied"},
				{0x06, "TTL expiration"},
				{0x07, "command not supported"},
				{0x08, "address type not supported"},
				{0x09, "until X'FF' are undefined"},
				{0x10, "login or password is not set"},
				{0x11, "unsupported protocol version"},
				{0x12, "login or password is not correct"},
				{0x13, "agreement version not supported"}
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
			 * Коды ответа socks5 сервера
			 */
			enum class rep_t : uint8_t {
				SUCCESS   = 0x00, // Подключение успешное
				ERROR     = 0x01, // Ошибка SOCKS-сервера
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
			/**
			 * Стейты работы модуля
			 */
			enum class state_t : uint8_t {
				AUTH,     // Режим ожидания прохождения аутентификации
				METHOD,   // Режим ожидания получения метода
				BROKEN,   // Режим бракованных данных
				RESPONSE, // Режим ожидания получения ответа
				HANDSHAKE // Режим выполненного рукопожатия
			};
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
				ResMethod() : ver(0x0), method(0x0) {}
			} __attribute__((packed)) resMet_t;
			/**
			 * ResAuth Структура ответа на авторизацию
			 */
			typedef struct ResAuth {
				uint8_t ver;    // Версия прокси-протокола
				uint8_t status; // Статус авторизации на сервере
				/**
				 * ResAuth Конструктор
				 */
				ResAuth() : ver(0x0), status(0x0) {}
			} __attribute__((packed)) resAuth_t;
			/**
			 * Res Структура ответа
			 */
			typedef struct Resp {
				uint8_t ver;  // Версия прокси-протокола
				uint8_t rep;  // Код ответа прокси-сервера
				uint8_t rsv;  // Зарезервированный октет
				uint8_t atyp; // Тип подключения
				/**
				 * Resp Конструктор
				 */
				Resp() : ver(0x0), rep(0x0), rsv(0x0), atyp(0x0) {}
			} __attribute__((packed)) res_t;
			/**
			 * IP Структура ip адреса сервера
			 */
			typedef struct IP {
				uint16_t host; // Хост сервера
				uint16_t port; // Порт сервера
				/**
				 * IP Конструктор
				 */
				IP() : host(0x0), port(0x0) {}
			} __attribute__((packed)) ip_t;
		protected:
			// URL параметры REST запроса
			uri_t::url_t url;
		protected:
			// Код сообщения
			uint8_t code = 0x00;
			// Стейт текущего запроса
			state_t state = state_t::METHOD;
		protected:
			// Буфер бинарных данных
			mutable vector <char> buffer;
		protected:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект работы с URI ссылками
			const uri_t * uri = nullptr;
		protected:
			/**
			 * ipToHex Метод конвертации IP адреса в бинарный буфер
			 * @param ip     индернет адрес в виде строки
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       бинарный буфер IP адреса
			 */
			vector <char> ipToHex(const string & ip, const int family = AF_INET) const noexcept;
			/**
			 * hexToIp Метод конвертации бинарного буфера в IP адрес
			 * @param buffer бинарный буфер для конвертации
			 * @param size   размер бинарного буфера
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       IP адрес в виде строки
			 */
			string hexToIp(const char * buffer, const size_t size, const int family = AF_INET) const noexcept;
		protected:
			/**
			 * setText Метод установки в буфер текстовых данных
			 * @param text текст для установки
			 * @return     текущее значение смещения
			 */
			u_short setText(const string & text) const noexcept;
			/**
			 * getText Метод извлечения текстовых данных из буфера
			 * @param buffer буфер данных для извлечения текста
			 * @param size   размер буфера данных
			 * @return       текст содержащийся в буфере данных
			 */
			const string getText(const char * buffer, const size_t size) const noexcept;
		protected:
			/**
			 * setOctet Метод установки октета
			 * @param octet  октет для установки
			 * @param offset размер смещения в буфере
			 * @return       текущее значение смещения
			 */
			u_short setOctet(const uint8_t octet, const u_short offset = 0) const noexcept;
		public:
			/**
			 * isEnd Метод проверки завершения обработки
			 * @return результат проверки
			 */
			bool isEnd() const noexcept;
			/**
			 * isHandshake Метод получения флага рукопожатия
			 * @return флаг получения рукопожатия
			 */
			bool isHandshake() const noexcept;
		public:
			/**
			 * getCode Метод получения кода сообщения
			 * @return код сообщения
			 */
			uint8_t getCode() const noexcept;
			/**
			 * getMessage Метод получения сообщения
			 * @param code код сообщения
			 * @return     текстовое значение кода
			 */
			const string & getMessage(const uint8_t code) const noexcept;
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
			 * setUrl Метод установки URL параметров REST запроса
			 * @param url параметры REST запроса
			 */
			void setUrl(const uri_t::url_t & url) noexcept;
		public:
			/**
			 * Socks5 Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект для работы с URI
			 */
			Socks5(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : fmk(fmk), log(log), uri(uri) {}
			/**
			 * ~Socks5 Деструктор
			 */
			~Socks5() noexcept {}
	} socks5_t;
};

#endif // __AWH_SOCKS5__
