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
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_NET__
#define __AWH_NET__

/**
 * Стандартные модули
 */
#include <array>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <iostream>
#include <unordered_map>

/**
 * Наши модуля
 */
#include "../sys/os.hpp"
#include "../sys/log.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс для работы с сетями
	 *
	 */
	typedef class AWH_SHARED_EXPORT Net {
		public:
			/**
			 * Режим дислокации IP-адреса
			 */
			enum class mode_t : uint8_t {
				NONE = 0x00, // Адрес не установлен
				LAN  = 0x01, // Адрес является локальным
				WAN  = 0x02, // Адрес является глобальным
				SYS  = 0x03  // Адрес является зарезервированным
			};
			/**
			 * Составная часть IP-адреса
			 */
			enum class addr_t : uint8_t {
				NONE    = 0x00, // Адрес не установлен
				HOST    = 0x01, // Адрес хоста
				NETWORK = 0x02  // Адрес сети
			};
			/**
			 * Порядок следования байт
			 */
			enum class endian_t : uint8_t {
				NONE   = 0x00, // Порядок следования байт не установлен
				BIG    = 0x01, // Порядок байт от старшего к младшему
				LITTLE = 0x02  // Порядок байт от младшего к старшему
			};
			/**
			 * Размер формата IP-адреса
			 */
			enum class format_size_t : uint8_t {
				NONE   = 0x00, // Размер формата не установлен
				LONG   = 0x01, // Полный формат IP-адреса [0000:0000:0000:0000:0000:0000:ae21:ad12 / 192.168.000.001]
				SHORT  = 0x02, // Короткий формат IP-адреса [::ae21:ad12 / 192.168.0.1]
				MIDDLE = 0x03  // Средний формат IP-адреса [0:0:0:0:0:0:ae21:ad12 / 192.168.0.1]
			};
			/**
			 * Флаги форматирования IP-адреса
			 */
			enum class format_flag_t : uint8_t {
				NONE      = 0x00, // Флаг не установлен
				HEX       = 0x01, // Шестнадцатеричный формат
				OCTAL     = 0x02, // Восьмеричный формат
				DECIMAL   = 0x03, // Десятичный формат
				HEX_IPV4  = 0x04, // Шестнадцатеричный формат IPv4
				HEX_IPV6  = 0x05  // Шестнадцатеричный формат IPv6
			};
			/**
			 * Идентификаторы разновидностей адресов
			 */
			enum class type_t : uint8_t {
				NONE  = 0x00, // Не определено
				FS    = 0x01, // Адрес в файловой системе
				MAC   = 0x02, // Аппаратный адрес сетевого интерфейса
				URL   = 0x03, // URL-адрес
				IPV4  = 0x04, // Адрес подключения IPv4
				IPV6  = 0x05, // Адрес подключения IPv6
				FQDN  = 0x06, // Доменная зона
				NETV4 = 0x07, // Адрес/Маска сети
				NETV6 = 0x08  // Адрес/Маска сети
			};
		private:
			/**
			 * @brief Структура локального адреса
			 *
			 */
			typedef struct LocalNet {
				bool reserved;               // Адрес является зарезервированным
				uint8_t prefix;              // Префикс сети
				std::unique_ptr <Net> end;   // Конечный диапазон адреса
				std::unique_ptr <Net> begin; // Начальный IP-адрес
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				LocalNet(const fmk_t * fmk, const log_t * log) noexcept :
				 reserved(false), prefix(0),
				 end(std::make_unique <Net> (fmk, log)),
				 begin(std::make_unique <Net> (fmk, log)) {}
			} localNet_t;
		private:
			// Тип обрабатываемого адреса
			type_t _type;
		private:
			// Зона IPv6 адреса
			string _zone;
		private:
			// Бинарный буфер данных
			vector <uint8_t> _buffer;
		private:
			// Список локальных адресов
			std::unordered_multimap <type_t, localNet_t> _localsNet;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект для работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Метод инициализации списка локальных адресов
			 *
			 */
			void initLocalNet() noexcept;
		private:
			/**
			 * @brief Метод заполнения недостающих элементов нулями
			 *
			 * @param num  число для заполнения нулями
			 * @param size максимальная длина строки
			 * @return     полученное число строки
			 */
			string && zerro(string && num, const uint8_t size = 3) const noexcept;
		public:
			/**
			 * @brief Метод очистки данных IP-адреса
			 *
			 */
			void clear() noexcept;
		public:
			/**
			 * @brief Метод проверки соответствия адреса зеркалу IPv6 => IPv4
			 *
			 * @return результат проверки
			 */
			bool broadcastIPv6ToIPv4() const noexcept;
		public:
			/**
			 * @brief Метод извлечения зоны IPv6 адреса
			 *
			 * @return зона IPv6 адреса
			 */
			const string & zone() const noexcept;
			/**
			 * @brief Метод установки зоны IPv6 адреса
			 *
			 * @param zone зона IPv6 адреса для установки
			 */
			void zone(const string & zone) noexcept;
		public:
			/**
			 * @brief Метод извлечения типа IP-адреса
			 *
			 * @return тип IP-адреса
			 */
			type_t type() const noexcept;
			/**
			 * @brief Метод установки типа IP-адреса
			 *
			 * @param type тип IP-адреса для установки
			 */
			void type(const type_t type) noexcept;
		public:
			/**
			 * @brief Метод определения типа хоста
			 *
			 * @param host хост для определения
			 * @return     определённый тип хоста
			 */
			type_t host(const string & host) const noexcept;
		public:
			/**
			 * @brief Метод извлечения аппаратного адреса в чистом виде
			 *
			 * @return аппаратный адрес в чистом виде
			 */
			std::array <uint8_t, 6> mac() const noexcept;
			/**
			 * @brief Метод установки аппаратного адреса в чистом виде
			 *
			 * @param addr аппаратный адрес в чистом виде
			 */
			void mac(const std::array <uint8_t, 6> & addr) noexcept;
		public:
			/**
			 * @brief Метод извлечения адреса IPv4 в чистом виде
			 *
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 * @return       адрес IPv4 в чистом виде
			 */
			uint32_t v4(const endian_t endian = endian_t::LITTLE) const noexcept;
			/**
			 * @brief Метод установки адреса IPv4 в чистом виде
			 *
			 * @param addr   адрес IPv4 в чистом виде
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 */
			void v4(const uint32_t addr, const endian_t endian = endian_t::LITTLE) noexcept;
		public:
			/**
			 * @brief Метод извлечения адреса IPv6 в чистом виде
			 *
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 * @return       адрес IPv6 в чистом виде
			 */
			std::array <uint8_t, 16> v6(const endian_t endian = endian_t::LITTLE) const noexcept;
			/**
			 * @brief Метод установки адреса IPv6 в чистом виде
			 *
			 * @param addr   адрес IPv6 в чистом виде
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 */
			void v6(const std::array <uint8_t, 16> & addr, const endian_t endian = endian_t::LITTLE) noexcept;
		public:
			/**
			 * @brief Метод проверки валидности IP-адреса
			 *
			 * @param addr адрес аппаратный или интернет подключения для проверки
			 * @param type тип адреса аппаратного или интернет подключения для проверки
			 * @return     результат проверки
			 */
			bool check(const string_view addr, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод наложения маски сети
			 *
			 * @param mask маска сети для наложения
			 * @param addr тип получаемого адреса
			 */
			void impose(const string & mask, const addr_t addr) noexcept;
			/**
			 * @brief Метод наложения маски сети
			 *
			 * @param mask маска сети для наложения
			 * @param addr тип получаемого адреса
			 * @param type тип адреса аппаратного или интернет подключения
			 */
			void impose(const string & mask, const addr_t addr, const type_t type) noexcept;
		public:
			/**
			 * @brief Метод наложения префикса
			 *
			 * @param prefix префикс для наложения
			 * @param addr   тип получаемого адреса
			 */
			void impose(const uint8_t prefix, const addr_t addr) noexcept;
			/**
			 * @brief Метод наложения префикса
			 *
			 * @param prefix префикс для наложения
			 * @param addr   тип получаемого адреса
			 * @param type   тип адреса аппаратного или интернет подключения
			 */
			void impose(const uint8_t prefix, const addr_t addr, const type_t type) noexcept;
		public:
			/**
			 * @brief Метод перевода маски сети в префикс адреса
			 *
			 * @param mask маска сети для перевода
			 * @return     полученный префикс адреса
			 */
			uint8_t mask2Prefix(const string & mask) const noexcept;
			/**
			 * @brief Метод перевода маски сети в префикс адреса
			 *
			 * @param mask маска сети для перевода
			 * @param type тип адреса аппаратного или интернет подключения
			 * @return     полученный префикс адреса
			 */
			uint8_t mask2Prefix(const string & mask, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод преобразования префикса адреса в маску сети
			 *
			 * @param prefix префикс адреса для преобразования
			 * @return       полученная маска сети
			 */
			string prefix2Mask(const uint8_t prefix) const noexcept;
			/**
			 * @brief Метод преобразования префикса адреса в маску сети
			 *
			 * @param prefix префикс адреса для преобразования
			 * @param type   тип адреса аппаратного или интернет подключения
			 * @return       полученная маска сети
			 */
			string prefix2Mask(const uint8_t prefix, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @return      результат првоерки
			 */
			bool range(const Net & begin, const Net & end, const string & mask) const noexcept;
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @param type  тип адреса аппаратного или интернет подключения
			 * @return      результат првоерки
			 */
			bool range(const Net & begin, const Net & end, const string & mask, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @return       результат првоерки
			 */
			bool range(const Net & begin, const Net & end, const uint8_t prefix) const noexcept;
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @param type   тип адреса аппаратного или интернет подключения
			 * @return       результат првоерки
			 */
			bool range(const Net & begin, const Net & end, const uint8_t prefix, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @return      результат првоерки
			 */
			bool range(const string & begin, const string & end, const string & mask) const noexcept;
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @param type  тип адреса аппаратного или интернет подключения
			 * @return      результат првоерки
			 */
			bool range(const string & begin, const string & end, const string & mask, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @return       результат првоерки
			 */
			bool range(const string & begin, const string & end, const uint8_t prefix) const noexcept;
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @param type   тип адреса аппаратного или интернет подключения
			 * @return       результат првоерки
			 */
			bool range(const string & begin, const string & end, const uint8_t prefix, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @return        результат проверки
			 */
			bool mapping(const string & network) const noexcept;
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param type    тип адреса аппаратного или интернет подключения
			 * @return        результат проверки
			 */
			bool mapping(const string & network, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param mask    маска сети для наложения
			 * @param addr    тип получаемого адреса
			 * @return        результат проверки
			 */
			bool mapping(const string & network, const string & mask, const addr_t addr) const noexcept;
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param mask    маска сети для наложения
			 * @param addr    тип получаемого адреса
			 * @param type    тип адреса аппаратного или интернет подключения
			 * @return        результат проверки
			 */
			bool mapping(const string & network, const string & mask, const addr_t addr, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param prefix  префикс для наложения
			 * @param addr    тип получаемого адреса
			 * @return        результат проверки
			 */
			bool mapping(const string & network, const uint8_t prefix, const addr_t addr) const noexcept;
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param prefix  префикс для наложения
			 * @param addr    тип получаемого адреса
			 * @param type    тип адреса аппаратного или интернет подключения
			 * @return        результат проверки
			 */
			bool mapping(const string & network, const uint8_t prefix, const addr_t addr, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод определения режима дислокации IP-адреса
			 *
			 * @return режим дислокации
			 */
			mode_t mode() const noexcept;
		public:
			/**
			 * @brief Получение записи в формате ARPA
			 *
			 * @return запись в формате ARPA
			 */
			string arpa() const noexcept;
			/**
			 * @brief Метод установки записи в формате ARPA
			 *
			 * @param addr адрес в формате ARPA (1.0.168.192.in-addr.arpa)
			 * @return     результат установки записи
			 */
			bool arpa(const string & addr) noexcept;
		public:
			/**
			 * @brief Метод парсинга адреса
			 *
			 * @param addr адрес аппаратный или интернет подключения для парсинга
			 * @return     результат работы парсинга
			 */
			bool parse(const string & addr) noexcept;
			/**
			 * @brief Метод парсинга адреса
			 *
			 * @param addr адрес аппаратный или интернет подключения для парсинга
			 * @param type тип адреса аппаратного или интернет подключения для парсинга
			 * @return     результат работы парсинга
			 */
			bool parse(const string & addr, const type_t type) noexcept;
		public:
			/**
			 * @brief Метод извлечения данных IP-адреса
			 *
			 * @param size  размер формата формирования IP-адреса
			 * @param flag  флаг форматирования IP-адреса
			 * @param delim разделитель формата формирования IP-адреса
			 * @return      сформированная строка IP-адреса
			 */
			string print(const format_size_t size = format_size_t::NONE, const format_flag_t flag = format_flag_t::NONE, const char delim = -1) const noexcept;
		public:
			/**
			 * @brief Оператор вывода IP-адреса в качестве строки
			 *
			 * @return IP-адрес в качестве строки
			 */
			operator string() const noexcept;
		public:
			/**
			 * @brief Оператор [<] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator < (const Net & addr) const noexcept;
			/**
			 * @brief Оператор [>] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator > (const Net & addr) const noexcept;
			/**
			 * @brief Оператор [<=] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator <= (const Net & addr) const noexcept;
			/**
			 * @brief Оператор [>=] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator >= (const Net & addr) const noexcept;
			/**
			 * @brief Оператор [!=] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator != (const Net & addr) const noexcept;
			/**
			 * @brief Оператор [==] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator == (const Net & addr) const noexcept;
		public:
			/**
			 * @brief Оператор [=] присвоения IP-адреса
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 */
			Net & operator = (const Net & addr) noexcept;
			/**
			 * @brief Оператор [=] присвоения IP-адреса
			 *
			 * @param ip адрес для присвоения
			 * @return   текущий объект
			 */
			Net & operator = (const string & ip) noexcept;
			/**
			 * @brief Оператор [=] установки типа IP-адреса
			 *
			 * @param type тип IP-адреса для установки
			 * @return     текущий объект
			 */
			Net & operator = (const type_t type) noexcept;
			/**
			 * @brief Оператор [=] присвоения IP-адреса
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 */
			Net & operator = (const uint32_t addr) noexcept;
			/**
			 * @brief Оператор [=] присвоения MAC-адреса
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 */
			Net & operator = (const std::array <uint8_t, 6> & addr) noexcept;
			/**
			 * @brief Оператор [=] присвоения IP-адреса
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 */
			Net & operator = (const std::array <uint8_t, 16> & addr) noexcept;
		public:
			/**
			 * @brief конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit Net(const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * @brief деструктор
			 *
			 */
			~Net() noexcept {}
	} net_t;
	/**
	 * @brief Оператор [>>] чтения из потока IP-адреса
	 *
	 * @param is   поток для чтения
	 * @param addr адрес для присвоения
	 */
	AWH_SHARED_EXPORT istream & operator >> (istream & is, net_t & addr) noexcept;
	/**
	 * @brief Оператор [<<] вывода в поток IP-адреса
	 *
	 * @param os   поток куда нужно вывести данные
	 * @param addr адрес для присвоения
	 */
	AWH_SHARED_EXPORT ostream & operator << (ostream & os, const net_t & addr) noexcept;
};

#endif // __AWH_NET__
