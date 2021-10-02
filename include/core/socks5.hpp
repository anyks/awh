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
		protected:
			/**
			 * Типы адресации socks5
			 */
			enum class atyp_t : u_short {
				IPv4   = 0x01, // Поддерживается IPv4 IP адрес
				DMNAME = 0x03, // Поддерживается доменное имя
				IPv6   = 0x04  // Поддерживается IPv6 IP адрес
			};
			/**
			 * Комманды запроса socks5 клиента
			 */
			enum class cmd_t : u_short {
				CONNECT = 0x01, // Метод подключения
				BIND    = 0x02, // Метод обратного подключения (сервера к клиенту)
				UDP     = 0x03  // Работа с UDP протоколом
			};
			/**
			 * Коды ответа socks5 сервера
			 */
			enum class rep_t : u_short {
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
			enum class method_t : u_short {
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
			enum class state_t : u_short {
				AUTH,     // Режим ожидания прохождения аутентификации
				GOOD,     // Режим завершения сбора данных
				METHOD,   // Режим ожидания получения метода
				BROKEN,   // Режим бракованных данных
				RESPONSE, // Режим ожидания получения ответа
				HANDSHAKE // Режим выполненного рукопожатия
			};
			// Устанавливаем уровень сжатия
			static constexpr u_short VER = 0x05;
		protected:
			// URL параметры REST запроса
			uri_t::url_t url;
		protected:
			// Стейт текущего запроса
			state_t state = state_t::METHOD;
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
			 * @param family тип протокола интернета AF_INET или AF_INET6
			 * @return       IP адрес в виде строки
			 */
			string hexToIp(const vector <char> & buffer, const int family = AF_INET) const noexcept;
		protected:
			/**
			 * setOctet Метод установки октета
			 * @param buffer сформированный бинарный буфер
			 * @param octet  октет для установки
			 * @param offset размер смещения в буфере
			 * @return       текущее значение смещения
			 */
			u_short setOctet(vector <char> & buffer, const uint8_t octet, const u_short offset = 0) const noexcept;
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
			 * parse Метод парсинга входящих данных
			 * @param buffer бинарный буфер входящих данных
			 * @param size   размер бинарного буфера входящих данных
			 */
			virtual void parse(const char * buffer, const size_t size) noexcept = 0;
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
			 * @param uri объект для работы с URI
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Socks5(const uri_t * uri, const fmk_t * fmk, const log_t * log) noexcept : uri(uri), fmk(fmk), log(log) {}
			/**
			 * ~Socks5 Деструктор
			 */
			~Socks5() noexcept {}
	} socks5_t;
};

#endif // __AWH_SOCKS5__
