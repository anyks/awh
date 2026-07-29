/**
 * @file: addr.hpp
 * @date: 2025-10-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля работы с сетевыми адресами — класс Network_Address для разбора, нормализации,
 *        сравнения и форматирования IPv4, IPv6 и MAC-адресов, работы с префиксами и масками сети,
 *        определения типов и принадлежности адреса зарезервированным диапазонам
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_NET_ADDR__
#define __AWH_NET_ADDR__

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <string>
#include <memory>
#include <cstdint>
#include <iostream>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "net.hpp"
#include "../sys/log.hpp"

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
	 * @brief Класс для работы с сетевыми адресами
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Network_Address {
		public:
			/**
			 * @brief Режим дислокации IP-адреса
			 *
			 */
			enum class own_t : uint8_t {
				NONE = 0x00, // Адрес не установлен
				LAN  = 0x01, // Адрес является локальным
				WAN  = 0x02, // Адрес является глобальным
				SYS  = 0x03  // Адрес является зарезервированным
			};
			/**
			 * @brief Составная часть IP-адреса
			 *
			 */
			enum class addr_t : uint8_t {
				NONE    = 0x00, // Адрес не установлен
				HOST    = 0x01, // Адрес хоста
				NETWORK = 0x02  // Адрес сети
			};
			/**
			 * @brief Порядок следования байт
			 *
			 */
			enum class endian_t : uint8_t {
				NONE   = 0x00, // Порядок следования байт не установлен
				BIG    = 0x01, // Порядок байт от старшего к младшему
				LITTLE = 0x02  // Порядок байт от младшего к старшему
			};
			/**
			 * @brief Размер формата IP-адреса
			 *
			 */
			enum class format_size_t : uint8_t {
				NONE   = 0x00, // Размер формата не установлен
				LONG   = 0x01, // Полный формат IP-адреса [0000:0000:0000:0000:0000:0000:ae21:ad12 / 192.168.000.001]
				SHORT  = 0x02, // Короткий формат IP-адреса [::ae21:ad12 / 192.168.0.1]
				MIDDLE = 0x03  // Средний формат IP-адреса [0:0:0:0:0:0:ae21:ad12 / 192.168.0.1]
			};
			/**
			 * @brief Флаги форматирования IP-адреса
			 *
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
			 * @brief Идентификаторы разновидностей адресов
			 *
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
			 * @details Структура локального адреса содержит информацию о локальном адресе сети,
			 *          включая его префикс, начальный и конечный диапазон адресов, а также флаг, указывающий, является ли адрес зарезервированным.
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ LocalNet {
				bool reserved;                      // Адрес является зарезервированным
				uint8_t prefix;                     // Префикс сети
				unique_ptr <Network_Address> end;   // Конечный диапазон адреса
				unique_ptr <Network_Address> begin; // Начальный IP-адрес
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit LocalNet(const fmk_t * fmk, const log_t * log) noexcept;
			} localNet_t;
			/**
			 * @brief Структура бинарного буфера адреса постоянной ёмкости
			 *
			 * @details Буфер держит адрес в двоичном виде, и длина его известна
			 *          заранее: четыре байта у IPv4, шесть у аппаратного адреса,
			 *          шестнадцать у IPv6. Динамический массив на этом месте
			 *          заводил выделение памяти на каждый разобранный адрес, а
			 *          проверка принадлежности сети создаёт временный объект
			 *          адреса на каждый вызов, и выделение приходилось на каждую
			 *          проверку. Постоянный буфер снимает и то, и другое, а объект
			 *          адреса становится копируемым без обращений к куче
			 *
			 * @note Набор методов повторяет ту часть работы с динамическим массивом,
			 *       которой модуль пользовался, чтобы замена оставалась подстановкой,
			 *       а не переписыванием мест обращения к буферу
			 *
			 */
			typedef struct __AWH_SHARED_EXPORT__ Buffer {
				public:
					// Предельный размер бинарного буфера адреса
					static constexpr size_t CAPACITY = 16;
				private:
					// Количество занятых байт буфера
					size_t _size;
					// Байты бинарного буфера адреса
					uint8_t _data[CAPACITY];
				public:
					/**
					 * @brief Метод проверки заполненности буфера
					 *
					 * @return результат проверки
					 *
					 */
					bool empty() const noexcept;
					/**
					 * @brief Метод получения размера буфера
					 *
					 * @return размер буфера
					 *
					 */
					size_t size() const noexcept;
				public:
					/**
					 * @brief Метод очистки буфера
					 *
					 */
					void clear() noexcept;
					/**
					 * @brief Метод изменения размера буфера
					 *
					 * @param size  новый размер буфера
					 * @param value значение заполнения добавленных байт
					 *
					 * @note Размер сверх ёмкости обрезается: адреса длиннее
					 *       шестнадцати байт не бывает, и запрос такого размера
					 *       означал бы ошибку вызывающей стороны
					 *
					 */
					void resize(const size_t size, const uint8_t value = 0) noexcept;
				public:
					/**
					 * @brief Метод получения указателя на данные буфера
					 *
					 * @return указатель на данные буфера
					 *
					 */
					uint8_t * data() noexcept;
					/**
					 * @brief Метод получения указателя на данные буфера
					 *
					 * @return указатель на данные буфера
					 *
					 */
					const uint8_t * data() const noexcept;
				public:
					/**
					 * @brief Оператор получения байта буфера по индексу
					 *
					 * @param index индекс байта буфера
					 * @return      байт буфера
					 *
					 */
					uint8_t & operator [] (const size_t index) noexcept;
					/**
					 * @brief Оператор получения байта буфера по индексу
					 *
					 * @param index индекс байта буфера
					 * @return      байт буфера
					 *
					 */
					const uint8_t & operator [] (const size_t index) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Buffer() noexcept;
			} buffer_t;
		private:
			// Тип обрабатываемого адреса
			type_t _type;
		private:
			// Флаг строгого режима парсинга/проверки адресов
			bool _strict;
		private:
			// Зона IPv6 адреса
			string _zone;
		private:
			// Бинарный буфер данных
			buffer_t _buffer;
		private:
			// Список локальных адресов
			unordered_multimap <type_t, localNet_t> _localsNet;
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
			 *
			 */
			bool broadcastIPv6ToIPv4() const noexcept;
		public:
			/**
			 * @brief Метод извлечения зоны IPv6 адреса
			 *
			 * @return зона IPv6 адреса
			 *
			 */
			const string & zone() const noexcept;
			/**
			 * @brief Метод установки зоны IPv6 адреса
			 *
			 * @param zone зона IPv6 адреса для установки
			 *
			 */
			void zone(string_view zone) noexcept;
		public:
			/**
			 * @brief Метод извлечения типа IP-адреса
			 *
			 * @return тип IP-адреса
			 *
			 */
			type_t type() const noexcept;
			/**
			 * @brief Метод установки типа IP-адреса
			 *
			 * @param type тип IP-адреса для установки
			 *
			 */
			void type(const type_t type) noexcept;
		public:
			/**
			 * @brief Метод извлечения флага строгого режима парсинга/проверки адресов
			 *
			 * @return флаг строгого режима
			 *
			 */
			bool strict() const noexcept;
			/**
			 * @brief Метод установки строгого режима парсинга/проверки адресов
			 *
			 * @param mode флаг строгого режима (в строгом режиме для IPv4 запрещены legacy-формы
			 *             [a.b.c, a.b, a] и не-десятичные системы счисления [0x..., 0...])
			 *
			 */
			void strict(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод определения типа хоста
			 *
			 * @param host хост для определения
			 * @return     определённый тип хоста
			 *
			 */
			type_t host(string_view host) const noexcept;
		public:
			/**
			 * @brief Метод извлечения аппаратного адреса в чистом виде
			 *
			 * @return аппаратный адрес в чистом виде
			 *
			 */
			array <uint8_t, 6> mac() const noexcept;
			/**
			 * @brief Метод установки аппаратного адреса в чистом виде
			 *
			 * @param addr аппаратный адрес в чистом виде
			 *
			 */
			void mac(const array <uint8_t, 6> & addr) noexcept;
		public:
			/**
			 * @brief Метод извлечения адреса IPv4 в чистом виде
			 *
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 * @return       адрес IPv4 в чистом виде
			 *
			 */
			uint32_t v4(const endian_t endian = endian_t::LITTLE) const noexcept;
			/**
			 * @brief Метод установки адреса IPv4 в чистом виде
			 *
			 * @param addr   адрес IPv4 в чистом виде
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 *
			 */
			void v4(const uint32_t addr, const endian_t endian = endian_t::LITTLE) noexcept;
		public:
			/**
			 * @brief Метод извлечения адреса IPv6 в чистом виде
			 *
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 * @return       адрес IPv6 в чистом виде
			 *
			 */
			array <uint8_t, 16> v6(const endian_t endian = endian_t::LITTLE) const noexcept;
			/**
			 * @brief Метод установки адреса IPv6 в чистом виде
			 *
			 * @param addr   адрес IPv6 в чистом виде
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 *
			 */
			void v6(const array <uint8_t, 16> & addr, const endian_t endian = endian_t::LITTLE) noexcept;
		public:
			/**
			 * @brief Метод извлечения адреса в чистом виде
			 *
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 * @return       адрес в чистом виде
			 *
			 */
			unique_ptr <net::addr_t> source(const endian_t endian = endian_t::LITTLE) const noexcept;
			/**
			 * @brief Метод установки адреса в чистом виде
			 *
			 * @param value  адрес в чистом виде для установки
			 * @param endian флаг формирования адреса в установленном порядке следовании байт
			 *
			 */
			void source(const net::addr_t * value, const endian_t endian = endian_t::LITTLE) noexcept;
		public:
			/**
			 * @brief Метод проверки валидности IP-адреса
			 *
			 * @param addr адрес аппаратный или интернет подключения для проверки
			 * @param type тип адреса аппаратного или интернет подключения для проверки
			 * @return     результат проверки
			 *
			 */
			bool check(const string_view addr, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод наложения маски сети
			 *
			 * @param mask маска сети для наложения
			 * @param addr тип получаемого адреса
			 *
			 */
			void impose(string_view mask, const addr_t addr) noexcept;
			/**
			 * @brief Метод наложения маски сети
			 *
			 * @param mask маска сети для наложения
			 * @param addr тип получаемого адреса
			 * @param type тип адреса аппаратного или интернет подключения
			 *
			 */
			void impose(string_view mask, const addr_t addr, const type_t type) noexcept;
		public:
			/**
			 * @brief Метод наложения префикса
			 *
			 * @param prefix префикс для наложения
			 * @param addr   тип получаемого адреса
			 *
			 */
			void impose(const uint8_t prefix, const addr_t addr) noexcept;
			/**
			 * @brief Метод наложения префикса
			 *
			 * @param prefix префикс для наложения
			 * @param addr   тип получаемого адреса
			 * @param type   тип адреса аппаратного или интернет подключения
			 *
			 */
			void impose(const uint8_t prefix, const addr_t addr, const type_t type) noexcept;
		public:
			/**
			 * @brief Метод перевода маски сети в префикс адреса
			 *
			 * @param mask маска сети для перевода
			 * @return     полученный префикс адреса
			 *
			 */
			uint8_t mask2Prefix(string_view mask) const noexcept;
			/**
			 * @brief Метод перевода маски сети в префикс адреса
			 *
			 * @param mask маска сети для перевода
			 * @param type тип адреса аппаратного или интернет подключения
			 * @return     полученный префикс адреса
			 *
			 */
			uint8_t mask2Prefix(string_view mask, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод преобразования префикса адреса в маску сети
			 *
			 * @param prefix префикс адреса для преобразования
			 * @return       полученная маска сети
			 *
			 */
			string prefix2Mask(const uint8_t prefix) const noexcept;
			/**
			 * @brief Метод преобразования префикса адреса в маску сети
			 *
			 * @param prefix префикс адреса для преобразования
			 * @param type   тип адреса аппаратного или интернет подключения
			 * @return       полученная маска сети
			 *
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
			 *
			 */
			bool range(const Network_Address & begin, const Network_Address & end, string_view mask) const noexcept;
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @param type  тип адреса аппаратного или интернет подключения
			 * @return      результат првоерки
			 *
			 */
			bool range(const Network_Address & begin, const Network_Address & end, string_view mask, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @return       результат првоерки
			 *
			 */
			bool range(const Network_Address & begin, const Network_Address & end, const uint8_t prefix) const noexcept;
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @param type   тип адреса аппаратного или интернет подключения
			 * @return       результат првоерки
			 *
			 */
			bool range(const Network_Address & begin, const Network_Address & end, const uint8_t prefix, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @return      результат првоерки
			 *
			 */
			bool range(string_view begin, string_view end, string_view mask) const noexcept;
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin начало диапазона адресов
			 * @param end   конец диапазона адресов
			 * @param mask  маска сети для перевода
			 * @param type  тип адреса аппаратного или интернет подключения
			 * @return      результат првоерки
			 *
			 */
			bool range(string_view begin, string_view end, string_view mask, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @return       результат првоерки
			 *
			 */
			bool range(string_view begin, string_view end, const uint8_t prefix) const noexcept;
			/**
			 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
			 *
			 * @param begin  начало диапазона адресов
			 * @param end    конец диапазона адресов
			 * @param prefix префикс адреса для преобразования
			 * @param type   тип адреса аппаратного или интернет подключения
			 * @return       результат првоерки
			 *
			 */
			bool range(string_view begin, string_view end, const uint8_t prefix, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @return        результат проверки
			 *
			 */
			bool mapping(string_view network) const noexcept;
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param type    тип адреса аппаратного или интернет подключения
			 * @return        результат проверки
			 *
			 */
			bool mapping(string_view network, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param mask    маска сети для наложения
			 * @param addr    тип получаемого адреса
			 * @return        результат проверки
			 *
			 */
			bool mapping(string_view network, string_view mask, const addr_t addr) const noexcept;
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param mask    маска сети для наложения
			 * @param addr    тип получаемого адреса
			 * @param type    тип адреса аппаратного или интернет подключения
			 * @return        результат проверки
			 *
			 */
			bool mapping(string_view network, string_view mask, const addr_t addr, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param prefix  префикс для наложения
			 * @param addr    тип получаемого адреса
			 * @return        результат проверки
			 *
			 */
			bool mapping(string_view network, const uint8_t prefix, const addr_t addr) const noexcept;
			/**
			 * @brief Метод проверки соотвествия IP-адреса указанной сети
			 *
			 * @param network сеть для проверки соответствия
			 * @param prefix  префикс для наложения
			 * @param addr    тип получаемого адреса
			 * @param type    тип адреса аппаратного или интернет подключения
			 * @return        результат проверки
			 *
			 */
			bool mapping(string_view network, const uint8_t prefix, const addr_t addr, const type_t type) const noexcept;
		public:
			/**
			 * @brief Метод определения принадлежности адреса
			 *
			 * @return флаг принадлежности адреса
			 *
			 */
			own_t own() const noexcept;
		public:
			/**
			 * @brief Получение записи в формате ARPA
			 *
			 * @return запись в формате ARPA
			 *
			 */
			string arpa() const noexcept;
			/**
			 * @brief Метод установки записи в формате ARPA
			 *
			 * @param addr адрес в формате ARPA (1.0.168.192.in-addr.arpa)
			 * @return     результат установки записи
			 *
			 */
			bool arpa(string_view addr) noexcept;
		public:
			/**
			 * @brief Метод парсинга адреса
			 *
			 * @param addr адрес аппаратный или интернет подключения для парсинга
			 * @return     результат работы парсинга
			 *
			 */
			bool parse(string_view addr) noexcept;
			/**
			 * @brief Метод парсинга адреса
			 *
			 * @param addr адрес аппаратный или интернет подключения для парсинга
			 * @param type тип адреса аппаратного или интернет подключения для парсинга
			 * @return     результат работы парсинга
			 *
			 */
			bool parse(string_view addr, const type_t type) noexcept;
		public:
			/**
			 * @brief Метод извлечения данных IP-адреса
			 *
			 * @param size  размер формата формирования IP-адреса
			 * @param flag  флаг форматирования IP-адреса
			 * @param delim разделитель формата формирования IP-адреса
			 * @return      сформированная строка IP-адреса
			 *
			 */
			string print(const format_size_t size = format_size_t::NONE, const format_flag_t flag = format_flag_t::NONE, const char delim = -1) const noexcept;
		public:
			/**
			 * @brief Оператор вывода IP-адреса в качестве строки
			 *
			 * @return IP-адрес в качестве строки
			 *
			 */
			operator string() const noexcept;
		public:
			/**
			 * @brief Оператор [<] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 */
			bool operator < (const Network_Address & addr) const noexcept;
			/**
			 * @brief Оператор [>] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 */
			bool operator > (const Network_Address & addr) const noexcept;
			/**
			 * @brief Оператор [<=] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 */
			bool operator <= (const Network_Address & addr) const noexcept;
			/**
			 * @brief Оператор [>=] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 */
			bool operator >= (const Network_Address & addr) const noexcept;
			/**
			 * @brief Оператор [!=] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 */
			bool operator != (const Network_Address & addr) const noexcept;
			/**
			 * @brief Оператор [==] сравнения IP-адреса
			 *
			 * @param addr адрес для сравнения
			 * @return     результат сравнения
			 *
			 */
			bool operator == (const Network_Address & addr) const noexcept;
		public:
			/**
			 * @brief Оператор присваивания присвоения IP-адреса
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 *
			 */
			Network_Address & operator = (const Network_Address & addr) noexcept;
			/**
			 * @brief Оператор присваивания присвоения IP-адреса
			 *
			 * @param ip адрес для присвоения
			 * @return   текущий объект
			 *
			 */
			Network_Address & operator = (string_view ip) noexcept;
			/**
			 * @brief Оператор присваивания установки типа IP-адреса
			 *
			 * @param type тип IP-адреса для установки
			 * @return     текущий объект
			 *
			 */
			Network_Address & operator = (const type_t type) noexcept;
			/**
			 * @brief Оператор присваивания присвоения IP-адреса
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 *
			 */
			Network_Address & operator = (const uint32_t addr) noexcept;
			/**
			 * @brief Оператор присваивания присвоения MAC-адреса
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 *
			 */
			Network_Address & operator = (const array <uint8_t, 6> & addr) noexcept;
			/**
			 * @brief Оператор присваивания присвоения IP-адреса
			 *
			 * @param addr адрес для присвоения
			 * @return     текущий объект
			 *
			 */
			Network_Address & operator = (const array <uint8_t, 16> & addr) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Network_Address(const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * @brief Деструктор
			 *
			 */
			~Network_Address() noexcept;
	} net_addr_t;
	/**
	 * @brief Оператор [>>] чтения из потока IP-адреса
	 *
	 * @param is   поток для чтения
	 * @param addr адрес для присвоения
	 *
	 */
	__AWH_SHARED_EXPORT__ istream & operator >> (istream & is, net_addr_t & addr) noexcept;
	/**
	 * @brief Оператор [<<] вывода в поток IP-адреса
	 *
	 * @param os   поток куда нужно вывести данные
	 * @param addr адрес для присвоения
	 *
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const net_addr_t & addr) noexcept;
};

#endif // __AWH_NET_ADDR__
