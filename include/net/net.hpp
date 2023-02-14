/**
 * @file: net.hpp
 * @date: 2023-02-14
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_NET__
#define __AWH_NET__

/**
 * Стандартная библиотека
 */
#include <array>
#include <regex>
#include <string>
#include <vector>
#include <iostream>

/**
 * Наши модули
 */
#include <sys/log.hpp>
#include <sys/fmk.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Net Класс для работы с сетями
	 */
	typedef class Net {
		public:
			/**
			 * Формат формирования IP адреса
			 */
			enum class format_t : uint8_t {
				NONE  = 0x00, // Формат IP адреса не указан
				LONG  = 0x01, // Полный формат IP адреса [0000:0000:0000:0000:0000:0000:ae21:ad12 / 192.168.000.001]
				MIDDL = 0x02, // Короткий формат IP адреса [0:0:0:0:0:0:ae21:ad12 / 192.168.0.1]
				SHORT = 0x03, // Короткий формат IP адреса [::ae21:ad12 / 192.168.0.1]
			};
			/**
			 * Флаги типа адреса
			 */
			enum class type_t : uint8_t {
				MAC  = 0x01, // Аппаратный адрес сетевого интерфейса
				NONE = 0x00, // Не определено
				IPV4 = 0x02, // Адрес подключения IPv4
				IPV6 = 0x03, // Адрес подключения IPv6
				DOMN = 0x04, // Доменное имя
				NETW = 0x05, // Адрес/Маска сети
				ADDR = 0x06, // Адрес в файловой системе
				HTTP = 0x07  // HTTP адрес
			};
		private:
			// Тип обрабатываемого адреса
			type_t _type;
		private:
			// Бинарный буфер данных
			vector <uint8_t> _buffer;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			/**
			 * zerro Метод заполнения недостающих элементов нулями
			 * @param num  число для заполнения нулями
			 * @param size максимальная длина строки
			 * @return     полученное число строки
			 */
			string & zerro(string && num, const uint8_t size = 3) const noexcept;
		public:
			/**
			 * clear Метод очистки данных IP адреса
			 */
			void clear() noexcept;
		public:
			/**
			 * type Метод извлечение типа IP адреса
			 * @return тип IP адреса
			 */
			type_t type() const noexcept;
			/**
			 * host Метод определения типа хоста
			 * @param host хост для определения
			 * @return     определённый тип хоста
			 */
			type_t host(const string & host) const noexcept;
		public:
			/**
			 * v4 Метод извлечения адреса IPv4 в чистом виде
			 * @return адрес IPv4 в чистом виде
			 */
			uint32_t v4() const noexcept;
			/**
			 * v4 Метод установки адреса IPv4 в чистом виде
			 * @param addr адрес IPv4 в чистом виде
			 */
			void v4(const uint32_t addr) noexcept;
		public:
			/**
			 * v6 Метод извлечения адреса IPv6 в чистом виде
			 * @return адрес IPv6 в чистом виде
			 */
			array <uint64_t, 2> v6() const noexcept;
			/**
			 * v6 Метод установки адреса IPv6 в чистом виде
			 * @param addr адрес IPv6 в чистом виде
			 */
			void v6(const array <uint64_t, 2> & addr) noexcept;
		public:
			/**
			 * parse Метод парсинга IP адреса
			 * @param ip адрес интернет подключения для парсинга
			 * @return   результат работы парсинга
			 */
			bool parse(const string & ip) noexcept;
		public:
			/**
			 * get Метод извлечения данных IP адреса
			 * @param format формат формирования IP адреса
			 * @return       сформированная строка IP адреса
			 */
			string get(const format_t format = format_t::SHORT) const noexcept;
		public:
			/**
			 * Оператор вывода IP адреса в качестве строки
			 * @return IP адрес в качестве строки
			 */
			operator std::string() const noexcept;
		public:
			/**
			 * Оператор [<] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator < (const Net & addr) noexcept;
			/**
			 * Оператор [>] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator > (const Net & addr) noexcept;
			/**
			 * Оператор [<=] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator <= (const Net & addr) noexcept;
			/**
			 * Оператор [>=] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator >= (const Net & addr) noexcept;
			/**
			 * Оператор [!=] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator != (const Net & addr) noexcept;
			/**
			 * Оператор [==] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator == (const Net & addr) noexcept;
		public:
			/**
			 * Оператор [=] присвоения IP адреса
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 */
			Net & operator = (const Net & addr) noexcept;
			/**
			 * Оператор [=] присвоения IP адреса
			 * @param ip адрес для присвоения
			 * @return   текущий объект
			 */
			Net & operator = (const string & ip) noexcept;
			/**
			 * Оператор [=] присвоения IP адреса
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 */
			Net & operator = (const uint32_t addr) noexcept;
			/**
			 * Оператор [=] присвоения IP адреса
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 */
			Net & operator = (const array <uint64_t, 2> & addr) noexcept;
		public:
			/**
			 * Net конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Net(const fmk_t * fmk, const log_t * log) noexcept : _type(type_t::NONE), _fmk(fmk), _log(log) {}
			/**
			 * ~Net деструктор
			 */
			~Net() noexcept {}
	} net_t;
	/**
	 * Оператор [>>] чтения из потока IP адреса
	 * @param is   поток для чтения
	 * @param addr адрес для присвоения
	 */
	istream & operator >> (istream & is, net_t & addr) noexcept;
	/**
	 * Оператор [<<] вывода в поток IP адреса
	 * @param os   поток куда нужно вывести данные
	 * @param addr адрес для присвоения
	 */
	ostream & operator << (ostream & os, const net_t & addr) noexcept;
};

#endif // __AWH_NET__
