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
 * Стандартные модули
 */
#include <map>
#include <array>
#include <cmath>
#include <memory>
#include <bitset>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <sys/types.h>

/**
 * Наши модуля
 */
#include <sys/os.hpp>
#include <sys/reg.hpp>


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
			 * Форматирование IP-адреса
			 */
			enum class format_t : uint8_t {
				NONE        = 0x00, // Формат IP-адреса не указан
				LONG        = 0x01, // Полный формат IP-адреса [0000:0000:0000:0000:0000:0000:ae21:ad12 / 192.168.000.001]
				SHORT       = 0x02, // Короткий формат IP-адреса [::ae21:ad12 / 192.168.0.1]
				MIDDLE      = 0x03, // Средний формат IP-адреса [0:0:0:0:0:0:ae21:ad12 / 192.168.0.1]
				LONG_IPV4   = 0x04, // Полный формат IP-адреса для IPv4
				LONG_IPV6   = 0x05, // Полный формат IP-адреса для IPv6
				SHORT_IPV4  = 0x06, // Короткий формат IP-адреса для IPv4
				SHORT_IPV6  = 0x07, // Короткий формат IP-адреса для IPv6
				MIDDLE_IPV4 = 0x08, // Средний формат IP-адреса для IPv4
				MIDDLE_IPV6 = 0x09  // Средний формат IP-адреса для IPv6
			};
			/**
			 * Идентификаторы разновидностей адресов
			 */
			enum class type_t : uint8_t {
				NONE    = 0x00, // Не определено
				FS      = 0x01, // Адрес в файловой системе
				MAC     = 0x02, // Аппаратный адрес сетевого интерфейса
				URL     = 0x03, // URL-адрес
				IPV4    = 0x04, // Адрес подключения IPv4
				IPV6    = 0x05, // Адрес подключения IPv6
				FQDN    = 0x06, // Доменная зона
				NETWORK = 0x07  // Адрес/Маска сети
			};
		private:
			/**
			 * LocalNet Структура локального адреса
			 */
			typedef struct LocalNet {
				bool reserved;          // Адрес является зарезервированным
				uint8_t prefix;         // Префикс сети
				unique_ptr <Net> end;   // Конечный диапазон адреса
				unique_ptr <Net> begin; // Начальный IP-адрес
				/**
				 * LocalNet Конструктор
				 */
				LocalNet() noexcept : reserved(false), prefix(0), end(new Net), begin(new Net) {}
			} localNet_t;
		private:
			// Тип обрабатываемого адреса
			type_t _type;
		private:
			// Объект регулярного выражения
			regexp_t _regexp;
		private:
			// Регулярное выражение для проверки адреса
			regexp_t::exp_t _exp;
		private:
			// Бинарный буфер данных
			vector <uint8_t> _buffer;
		private:
			// Список локальных адресов
			multimap <type_t, localNet_t> _localsNet;
		private:
			/**
			 * initLocalNet Метод инициализации списка локальных адресов
			 */
			void initLocalNet() noexcept;
		private:
			/**
			 * atoi Метод конвертации строковых чисел в десятичную систему счисления
			 * @param value число для конвертации
			 * @return      полученная строка в системе счисления
			 */
			int64_t atoi(const string & value) const noexcept;
			/**
			 * itoa Метод конвертации чисел в указанную систему счисления
			 * @param value число для конвертации
			 * @param radix система счисления
			 * @return      полученная строка в системе счисления
			 */
			string itoa(const int64_t value, const uint8_t radix) const noexcept;
		private:
			/**
			 * zerro Метод заполнения недостающих элементов нулями
			 * @param num  число для заполнения нулями
			 * @param size максимальная длина строки
			 * @return     полученное число строки
			 */
			string & zerro(string && num, const uint8_t size = 3) const noexcept;
		private:
			/**
			 * split Метод разделения строк на составляющие
			 * @param str    строка для поиска
			 * @param delim  разделитель
			 * @param result результирующий вектор
			 * @return       результирующий вектор
			 */
			vector <string> & split(const string & str, const string & delim, vector <string> & result) const noexcept;
		public:
			/**
			 * clear Метод очистки данных IP-адреса
			 */
			void clear() noexcept;
		public:
			/**
			 * type Метод извлечения типа IP-адреса
			 * @return тип IP-адреса
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
			 * mac Метод извлечения аппаратного адреса в чистом виде
			 * @return аппаратный адрес в чистом виде
			 */
			uint64_t mac() const noexcept;
			/**
			 * mac Метод установки аппаратного адреса в чистом виде
			 * @param addr аппаратный адрес в чистом виде
			 */
			void mac(const uint64_t addr) noexcept;
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
			 * @tparam Шаблон извлечения дампа данных
			 */
			template <typename T>
			/**
			 * dump Метод получения дампа данных
			 * @return сформированный дамп данных
			 */
			T data() noexcept {
				// Результат работы функции
				T result;
				// Если бинарный буфер данных существует и мы работаем с контейнером
				if(!this->_buffer.empty() && is_class <T>::value){
					// Выполняем выделение памяти
					result.resize(this->_buffer.size() / sizeof(result[0]), 0);
					// Выполняем копирование IP-адреса
					memcpy(result.data(), this->_buffer.data(), this->_buffer.size());
				}
				// Выводим результат
				return result;
			}
		public:
			/**
			 * impose Метод наложения маски сети
			 * @param mask маска сети для наложения
			 * @param addr тип получаемого адреса
			 */
			void impose(const string & mask, const addr_t addr) noexcept;
			/**
			 * impose Метод наложения префикса
			 * @param prefix префикс для наложения
			 * @param addr тип получаемого адреса
			 */
			void impose(const uint8_t prefix, const addr_t addr) noexcept;
		public:
			/**
			 * mask2Prefix Метод перевода маски сети в префикс адреса
			 * @param mask маска сети для перевода
			 * @return     полученный префикс адреса
			 */
			uint8_t mask2Prefix(const string & mask) const noexcept;
			/**
			 * prefix2Mask Метод преобразования префикса адреса в маску сети
			 * @param prefix префикс адреса для преобразования
			 * @return       полученная маска сети
			 */
			string prefix2Mask(const uint8_t prefix) const noexcept;
		public:
			/**
			 * broadcastIPv6ToIPv4 Метод проверки соответствия адреса зеркалу IPv6 => IPv4
			 * @return результат проверки
			 */
			bool broadcastIPv6ToIPv4() const noexcept;
		public:
			/**
			 * range Метод проверки вхождения IP-адреса в диапазон адресов
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @return      результат првоерки
			 */
			bool range(const Net & begin, const Net & end, const string & mask) const noexcept;
			/**
			 * range Метод проверки вхождения IP-адреса в диапазон адресов
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @return       результат првоерки
			 */
			bool range(const Net & begin, const Net & end, const uint8_t prefix) const noexcept;
			/**
			 * range Метод проверки вхождения IP-адреса в диапазон адресов
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @return      результат првоерки
			 */
			bool range(const string & begin, const string & end, const string & mask) const noexcept;
			/**
			 * range Метод проверки вхождения IP-адреса в диапазон адресов
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @return       результат првоерки
			 */
			bool range(const string & begin, const string & end, const uint8_t prefix) const noexcept;
		public:
			/**
			 * mapping Метод проверки соотвествия IP-адреса указанной сети
			 * @param network сеть для проверки соответствия
			 * @return        результат проверки
			 */
			bool mapping(const string & network) const noexcept;
			/**
			 * mapping Метод проверки соотвествия IP-адреса указанной сети
			 * @param network сеть для проверки соответствия
			 * @param mask    маска сети для наложения
			 * @param addr    тип получаемого адреса
			 * @return        результат проверки
			 */
			bool mapping(const string & network, const string & mask, const addr_t addr) const noexcept;
			/**
			 * mapping Метод проверки соотвествия IP-адреса указанной сети
			 * @param network сеть для проверки соответствия
			 * @param prefix  префикс для наложения
			 * @param addr    тип получаемого адреса
			 * @return        результат проверки
			 */
			bool mapping(const string & network, const uint8_t prefix, const addr_t addr) const noexcept;
		public:
			/**
			 * mode Метод определения режима дислокации IP-адреса
			 * @return режим дислокации
			 */
			mode_t mode() const noexcept;
		public:
			/**
			 * arpa Получение записи в формате ARPA
			 * @return запись в формате ARPA
			 */
			string arpa() const noexcept;
			/**
			 * arpa Метод установки записи в формате ARPA
			 * @param addr адрес в формате ARPA (1.0.168.192.in-addr.arpa)
			 * @return     результат установки записи
			 */
			bool arpa(const string & addr) noexcept;
		public:
			/**
			 * parse Метод парсинга адреса
			 * @param addr адрес аппаратный или интернет подключения для парсинга
			 * @return     результат работы парсинга
			 */
			bool parse(const string & addr) noexcept;
			/**
			 * parse Метод парсинга адреса
			 * @param addr адрес аппаратный или интернет подключения для парсинга
			 * @param type тип адреса аппаратного или интернет подключения для парсинга
			 * @return     результат работы парсинга
			 */
			bool parse(const string & addr, const type_t type) noexcept;
		public:
			/**
			 * get Метод извлечения данных IP-адреса
			 * @param format формат формирования IP-адреса
			 * @return       сформированная строка IP-адреса
			 */
			string get(const format_t format = format_t::SHORT) const noexcept;
		public:
			/**
			 * Оператор вывода IP-адреса в качестве строки
			 * @return IP-адрес в качестве строки
			 */
			operator std::string() const noexcept;
		public:
			/**
			 * Оператор [<] сравнения IP-адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator < (const Net & addr) const noexcept;
			/**
			 * Оператор [>] сравнения IP-адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator > (const Net & addr) const noexcept;
			/**
			 * Оператор [<=] сравнения IP-адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator <= (const Net & addr) const noexcept;
			/**
			 * Оператор [>=] сравнения IP-адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator >= (const Net & addr) const noexcept;
			/**
			 * Оператор [!=] сравнения IP-адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator != (const Net & addr) const noexcept;
			/**
			 * Оператор [==] сравнения IP-адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator == (const Net & addr) const noexcept;
		public:
			/**
			 * Оператор [=] присвоения IP-адреса
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 */
			Net & operator = (const Net & addr) noexcept;
			/**
			 * Оператор [=] присвоения IP-адреса
			 * @param ip адрес для присвоения
			 * @return   текущий объект
			 */
			Net & operator = (const string & ip) noexcept;
			/**
			 * Оператор [=] присвоения IP-адреса
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 */
			Net & operator = (const uint32_t addr) noexcept;
			/**
			 * Оператор [=] присвоения IP-адреса
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 */
			Net & operator = (const array <uint64_t, 2> & addr) noexcept;
		public:
			/**
			 * Net конструктор
			 */
			Net() noexcept;
		private:
			/**
			 * Net конструктор
			 * @param exp регулярное выражение для установки
			 */
			Net(const regexp_t::exp_t & exp) noexcept;
		public:
			/**
			 * ~Net деструктор
			 */
			~Net() noexcept {}
	} net_t;
	/**
	 * Оператор [>>] чтения из потока IP-адреса
	 * @param is   поток для чтения
	 * @param addr адрес для присвоения
	 */
	istream & operator >> (istream & is, net_t & addr) noexcept;
	/**
	 * Оператор [<<] вывода в поток IP-адреса
	 * @param os   поток куда нужно вывести данные
	 * @param addr адрес для присвоения
	 */
	ostream & operator << (ostream & os, const net_t & addr) noexcept;
};

#endif // __AWH_NET__
