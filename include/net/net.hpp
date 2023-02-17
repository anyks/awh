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
#include <cmath>
#include <bitset>
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
			 * Режим дислокации IP адреса
			 */
			enum class mode_t : uint8_t {
				NONE   = 0x00, // Адрес не установлен
				LOCAL  = 0x01, // Адрес является локальным
				GLOBAL = 0x02, // Адрес является глобальным
				RESERV = 0x03  // Адрес является зарезервированным
			};
			/**
			 * Формат IP адреса
			 */
			enum class addr_t : uint8_t {
				NONE = 0x00, // Адрес не установлен
				HOST = 0x01, // Адрес хоста
				NETW = 0x02  // Адрес сети
			};
			/**
			 * Формат формирования IP адреса
			 */
			enum class format_t : uint8_t {
				NONE        = 0x00, // Формат IP адреса не указан
				LONG        = 0x01, // Полный формат IP адреса [0000:0000:0000:0000:0000:0000:ae21:ad12 / 192.168.000.001]
				SHORT       = 0x02, // Короткий формат IP адреса [::ae21:ad12 / 192.168.0.1]
				MIDDLE      = 0x03, // Средний формат IP адреса [0:0:0:0:0:0:ae21:ad12 / 192.168.0.1]
				LONG_IPV4   = 0x04, // Полный формат IP адреса для IPv4
				LONG_IPV6   = 0x05, // Полный формат IP адреса для IPv6
				SHORT_IPV4  = 0x06, // Короткий формат IP адреса для IPv4
				SHORT_IPV6  = 0x07, // Короткий формат IP адреса для IPv6
				MIDDLE_IPV4 = 0x08, // Средний формат IP адреса для IPv4
				MIDDLE_IPV6 = 0x09  // Средний формат IP адреса для IPv6
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
			/**
			 * Local Структура локального адреса
			 */
			typedef struct Local {
				bool reserved;          // Адрес является зарезервированным
				uint8_t prefix;         // Префикс сети
				unique_ptr <Net> end;   // Конечный диапазон адреса
				unique_ptr <Net> begin; // Начальный IP адрес
				/**
				 * Local Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Local(const fmk_t * fmk, const log_t * log) noexcept :
				 reserved(false), prefix(0), end(new Net(fmk, log)), begin(new Net(fmk, log)) {}
			} local_t;
		private:
			// Тип обрабатываемого адреса
			type_t _type;
		private:
			// Бинарный буфер данных
			vector <uint8_t> _buffer;
		private:
			// Список локальных адресов
			multimap <type_t, local_t> _locals;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			/**
			 * initLocalNet Метод инициализации списка локальных адресов
			 */
			void initLocalNet() noexcept;
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
					// Выполняем копирование IP адреса
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
			 * range Метод проверки вхождения IP адреса в диапазон адресов
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @return      результат првоерки
			 */
			bool range(const Net & begin, const Net & end, string & mask) const noexcept;
			/**
			 * range Метод проверки вхождения IP адреса в диапазон адресов
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @return       результат првоерки
			 */
			bool range(const Net & begin, const Net & end, const uint8_t prefix) const noexcept;
			/**
			 * range Метод проверки вхождения IP адреса в диапазон адресов
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @return      результат првоерки
			 */
			bool range(const string & begin, const string & end, string & mask) const noexcept;
			/**
			 * range Метод проверки вхождения IP адреса в диапазон адресов
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @return       результат првоерки
			 */
			bool range(const string & begin, const string & end, const uint8_t prefix) const noexcept;
		public:
			/**
			 * mapping Метод проверки соотвествия IP адреса указанной сети
			 * @param network сеть для проверки соответствия
			 * @return        результат проверки
			 */
			bool mapping(const string & network) const noexcept;
			/**
			 * mapping Метод проверки соотвествия IP адреса указанной сети
			 * @param network сеть для проверки соответствия
			 * @param mask    маска сети для наложения
			 * @param addr    тип получаемого адреса
			 * @return        результат проверки
			 */
			bool mapping(const string & network, const string & mask, const addr_t addr) const noexcept;
			/**
			 * mapping Метод проверки соотвествия IP адреса указанной сети
			 * @param network сеть для проверки соответствия
			 * @param prefix  префикс для наложения
			 * @param addr    тип получаемого адреса
			 * @return        результат проверки
			 */
			bool mapping(const string & network, const uint8_t prefix, const addr_t addr) const noexcept;
		public:
			/**
			 * mode Метод определения режима дислокации IP адреса
			 * @return режим дислокации
			 */
			mode_t mode() const noexcept;
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
			bool operator < (const Net & addr) const noexcept;
			/**
			 * Оператор [>] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator > (const Net & addr) const noexcept;
			/**
			 * Оператор [<=] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator <= (const Net & addr) const noexcept;
			/**
			 * Оператор [>=] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator >= (const Net & addr) const noexcept;
			/**
			 * Оператор [!=] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator != (const Net & addr) const noexcept;
			/**
			 * Оператор [==] сравнения IP адреса
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 */
			bool operator == (const Net & addr) const noexcept;
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
