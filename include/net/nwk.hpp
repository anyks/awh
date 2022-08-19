/**
 * @file: nwk.hpp
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

#ifndef __AWH_NETWORK__
#define __AWH_NETWORK__

/**
 * Стандартная библиотека
 */
#include <regex>
#include <array>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <sstream>
#include <math.h>
#include <sys/types.h>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <getopt.h>
// Если - это Unix
#else
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

/**
 * Наши модули
 */
#include <sys/fmk.hpp>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Network Класс для работы с сетями
	 */
	typedef class Network {
		public:
			/**
			 * IPdata Класс содержащий данные ip адреса
			 */
			typedef class IPdata {
				private:
					// Параметры ip адреса
					u_int _ptr[4] = {256, 256, 256, 256};
				public:
					/**
					 * str Метод вывода результата в виде строки
					 * @return строка ip адреса
					 */
					const string str() const noexcept;
					/**
					 * length Метод определения размера
					 * @return размер массива
					 */
					const u_int size() const noexcept;
					/**
					 * get Метод получения данных масива
					 * @return указатель на масив ip адреса
					 */
					const u_int * get() const noexcept;
				public:
					/**
					 * set Метод установки данных ip адреса
					 * @param ptr массив значений ip адреса
					 */
					void set(const u_int ptr1 = 0, const u_int ptr2 = 0, const u_int ptr3 = 0, const u_int ptr4 = 0) noexcept;
			} ipdata_t;
			/**
			 * NTdata Структура данных типов сетей
			 */
			typedef struct NTdata {
				string mask;   // Маска сети
				string nmask;  // Маска подсети
				string smask;  // Начальный адрес
				string emask;  // Конечный адрес
				string invers; // Инверсия
				string format; // Распределение байт (С — сеть, Х — хост)
				string type;   // Тип сети
				float cls;     // Класс типа сети
				u_int number;  // Цифровое значение маски
				u_int sbytes;  // Первые биты
				size_t counts; // Количество доступных адресов
				size_t maxnwk; // Число возможных адресов сетей
				size_t maxhst; // Число возможных адресов хостов
				bool notEmpty; // Структура заполнена или нет
			} ntdata_t;
			/**
			 * NLdata Структура содержащая параметры локальных и запрещенных сетей
			 */
			typedef struct NLdata {
				string ip;      // ip адрес сети
				string network; // Адрес сети
				u_int mask;     // Маска сети
				bool allow;     // Разрешен (true - разрешен, false - запрещен)
			} nldata_t;
			/**
			 * NLdata6 Структура содержащая параметры локальных и запрещенных сетей IPv6
			 */
			typedef struct NLdata6 {
				string ip;    // ip адрес сети
				string eip;   // Конечный ip адрес
				u_int prefix; // Префикс сети
				bool allow;   // Разрешен (true - разрешен, false - запрещен)
			} nldata6_t;
			/**
			 * NKdata Структура содержащая данные подключения
			 */
			typedef struct NKdata {
				ipdata_t ip;      // Данные ip адреса
				ipdata_t mask;    // Данные маски сети
				ipdata_t network; // Данные сети
			} nkdata_t;
			/**
			 * Url Структура url адреса
			 */
			typedef struct Url {
				bool ssl;     // Соединение защищённое
				u_int port;   // Порт запроса
				string host;  // Хост запроса
				string query; // Параметры запроса
				/**
				 * Url Конструктор
				 */
				Url() noexcept : ssl(false), port(0), host(""), query("") {}
			} url_t;
			/**
			 * Флаги типа адреса
			 */
			enum class type_t : uint8_t {
				MAC         = 0x01, // Аппаратный адрес сетевого интерфейса
				IPV4        = 0x02, // Адрес подключения IPv4
				IPV6        = 0x03, // Адрес подключения IPv6
				NONE        = 0x00, // Не определено
				DOMNAME     = 0x04, // Доменное имя
				NETWORK     = 0x05, // Адрес/Маска сети
				ADDRESS     = 0x06, // Адрес в файловой системе
				HTTPADDRESS = 0x07  // HTTP адрес
			};
		private:
			/**
			 * Набор локальных сетей IPv6
			 */
			const vector <nldata6_t> locals6 = {
				{"::1", "", 128, true},
				{"2001::", "", 32, false},
				{"2001:db8::", "", 32, true},
				{"64:ff9b::", "", 96, false},
				{"2002::", "", 16, false},
				{"fe80::", "febf::", 10, true},
				{"fec0::", "feff::", 10, true},
				{"fc00::", "", 7, true},
				{"ff00::", "", 8, false}
			};
			/**
			 * Набор локальных сетей
			 */
			const vector <nldata_t> locals = {
				{"0.0.0.0", "0.0.0.0", 8, false},
				{"0.0.0.0", "0.0.0.0", 32, false},
				{"100.64.0.0", "100.64.0.0", 10, false},
				{"169.254.0.0", "169.254.0.0", 16, false},
				{"224.0.0.0", "224.0.0.0", 4, false},
				{"224.0.0.0", "224.0.0.0", 24, false},
				{"224.0.1.0", "224.0.0.0", 8, false},
				{"239.0.0.0", "239.0.0.0", 8, false},
				{"240.0.0.0", "240.0.0.0", 4, false},
				{"255.255.255.255", "255.255.255.255", 32, false},
				{"10.0.0.0", "10.0.0.0", 8, true},
				{"127.0.0.0", "127.0.0.0", 8, true},
				{"172.16.0.0", "172.16.0.0", 12, true},
				{"192.0.0.0", "192.0.0.0", 24, true},
				{"192.0.0.0", "192.0.0.0", 29, true},
				{"192.0.0.170", "192.0.0.170", 32, true},
				{"192.0.0.171", "192.0.0.171", 32, true},
				{"192.0.2.0", "192.0.2.0", 24, true},
				{"192.88.99.0", "192.88.99.0", 24, true},
				{"192.88.99.1", "192.88.99.1", 32, true},
				{"192.168.0.0", "192.168.0.0", 16, true},
				{"198.51.100.0", "198.51.100.0", 24, true},
				{"198.18.0.0", "198.18.0.0", 15, true},
				{"203.0.113.0", "203.0.113.0", 24, true}
			};
			/**
			 * Набор масок подсетей
			 */
			const vector <ntdata_t> masks = {
				{"000.000.000.000", "255.0.0.0", "0.0.0.0", "127.255.255.255", "255.255.255.255", "С.Х.Х.Х", "A", 256, 0, 0, 4294967296, 128, 16777214, false}, // 0
				{"128.000.000.000", "255.0.0.0", "0.0.0.0", "127.255.255.255", "127.255.255.255", "С.Х.Х.Х", "A", 128, 1, 0, 2147483648, 128, 16777214, false}, // 1
				{"192.000.000.000", "255.0.0.0", "0.0.0.0", "127.255.255.255", "63.255.255.255", "С.Х.Х.Х", "A", 64, 2, 0, 1073741824, 128, 16777214, false},   // 2
				{"224.000.000.000", "255.0.0.0", "0.0.0.0", "127.255.255.255", "31.255.255.255", "С.Х.Х.Х", "A", 32, 3, 0, 536870912, 128, 16777214, false},    // 3
				{"240.000.000.000", "255.0.0.0", "0.0.0.0", "127.255.255.255", "15.255.255.255", "С.Х.Х.Х", "A", 16, 4, 0, 268435456, 128, 16777214, false},    // 4
				{"248.000.000.000", "255.0.0.0", "0.0.0.0", "127.255.255.255", "7.255.255.255", "С.Х.Х.Х", "A", 8, 5, 0, 134217728, 128, 16777214, false},      // 5
				{"252.000.000.000", "255.0.0.0", "0.0.0.0", "127.255.255.255", "3.255.255.255", "С.Х.Х.Х", "A", 4, 6, 0, 67108864, 128, 16777214, false},       // 6
				{"254.000.000.000", "255.0.0.0", "0.0.0.0", "127.255.255.255", "1.255.255.255", "С.Х.Х.Х", "A", 2, 7, 0, 33554432, 128, 16777214, false},       // 7
				{"255.000.000.000", "255.0.0.0", "0.0.0.0", "127.255.255.255", "0.255.255.255", "С.Х.Х.Х", "A", 1, 8, 0, 16777216, 128, 16777214, false},       // 8
				{"255.128.000.000", "255.255.0.0", "128.0.0.0", "191.255.255.255", "0.127.255.255", "С.С.Х.Х", "B", 128, 9, 10, 8388608, 16384, 65534, false},  // 9
				{"255.192.000.000", "255.255.0.0", "128.0.0.0", "191.255.255.255", "0.63.255.255", "С.С.Х.Х", "B", 64, 10, 10, 4194304, 16384, 65534, false},   // 10
				{"255.224.000.000", "255.255.0.0", "128.0.0.0", "191.255.255.255", "0.31.255.255", "С.С.Х.Х", "B", 32, 11, 10, 2097152, 16384, 65534, false},   // 11
				{"255.240.000.000", "255.255.0.0", "128.0.0.0", "191.255.255.255", "0.15.255.255", "С.С.Х.Х", "B", 16, 12, 10, 1048576, 16384, 65534, false},   // 12
				{"255.248.000.000", "255.255.0.0", "128.0.0.0", "191.255.255.255", "0.7.255.255", "С.С.Х.Х", "B", 8, 13, 10, 524288, 16384, 65534, false},      // 13
				{"255.252.000.000", "255.255.0.0", "128.0.0.0", "191.255.255.255", "0.3.255.255", "С.С.Х.Х", "B", 4, 14, 10, 262144, 16384, 65534, false},      // 14
				{"255.254.000.000", "255.255.0.0", "128.0.0.0", "191.255.255.255", "0.1.255.255", "С.С.Х.Х", "B", 2, 15, 10, 131072, 16384, 65534, false},      // 15
				{"255.255.000.000", "255.255.0.0", "128.0.0.0", "191.255.255.255", "0.0.255.255", "С.С.Х.Х", "B", 1, 16, 10, 65536, 16384, 65534, false},       // 16
				{"255.255.128.000", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.127.255", "С.С.С.Х", "C", 128, 17, 110, 32768, 2097152, 254, false},  // 17
				{"255.255.192.000", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.63.255", "С.С.С.Х", "C", 64, 18, 110, 16384, 2097152, 254, false},    // 18
				{"255.255.224.000", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.31.255", "С.С.С.Х", "C", 32, 19, 110, 8192, 2097152, 254, false},     // 19
				{"255.255.240.000", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.15.255", "С.С.С.Х", "C", 16, 20, 110, 4096, 2097152, 254, false},     // 20
				{"255.255.248.000", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.7.255", "С.С.С.Х", "C", 8, 21, 110, 2048, 2097152, 254, false},       // 21
				{"255.255.252.000", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.3.255", "С.С.С.Х", "C", 4, 22, 110, 1024, 2097152, 254, false},       // 22
				{"255.255.254.000", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.1.255", "С.С.С.Х", "C", 2, 23, 110, 512, 2097152, 254, false},        // 23
				{"255.255.255.000", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.0.255", "С.С.С.Х", "C", 1, 24, 110, 256, 2097152, 254, false},        // 24
				{"255.255.255.128", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.0.127", "С.С.С.Х", "C", 0.5, 25, 110, 128, 2097152, 127, false},      // 25
				{"255.255.255.192", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.0.63", "С.С.С.Х", "C", 0.25, 26, 110, 64, 2097152, 63, false},        // 26
				{"255.255.255.224", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.0.31", "С.С.С.Х", "C", 0.125, 27, 110, 32, 2097152, 31, false},       // 27
				{"255.255.255.240", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.0.15", "С.С.С.Х", "C", 0.0625, 28, 110, 16, 2097152, 15, false},      // 28
				{"255.255.255.248", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.0.7", "С.С.С.Х", "C", 0.03125, 29, 110, 8, 2097152, 7, false},        // 29
				{"255.255.255.252", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.0.3", "С.С.С.Х", "C", 0.015625, 30, 110, 4, 2097152, 3, false},       // 30
				{"255.255.255.254", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.0.1", "С.С.С.Х", "C", 0.0078125, 31, 110, 2, 2097152, 1, false},      // 31
				{"255.255.255.255", "255.255.255.0", "192.0.0.0", "223.255.255.255", "0.0.0.0", "С.С.С.Х", "C", 0.00390625, 32, 110, 1, 2097152, 0, false}      // 32
			};
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
		private:
			/**
			 * setZerroToStrIp Метод дописывает указанное количестов нулей к строке
			 * @param str строка к которой нужно дописать нули
			 */
			void setZerroToStrIp(string & str) const noexcept;
			/**
			 * rmZerroToStrIp Метод удаляем указанное количестов нулей из строки
			 * @param str строка из которой нужно удалить нули
			 */
			void rmZerroToStrIp(string & str) const noexcept;
		private:
			/**
			 * checkMask Метод проверки на соответствии маски
			 * @param ip   блок с данными ip адреса
			 * @param mask блок с данными маски сети
			 * @return     результат проверки
			 */
			const bool checkMask(const ipdata_t & ip, const ipdata_t & mask) const noexcept;
		private:
			/**
			 * getLow1Ip6 Метод упрощения IPv6 адреса первого порядка
			 * @param ip адрес интернет протокола версии 6
			 * @return   упрощенный вид ip адреса первого порядка
			 */
			const string getLow1Ip6(const string & ip) const noexcept;
			/**
			 * getLow2Ip6 Метод упрощения IPv6 адреса второго порядка
			 * @param ip адрес интернет протокола версии 6
			 * @return   упрощенный вид ip адреса второго порядка
			 */
			const string getLow2Ip6(const string & ip) const noexcept;
			/**
			 * setLow1Ip6 Метод восстановления IPv6 адреса первого порядка
			 * @param ip адрес интернет протокола версии 6
			 * @return   восстановленный вид ip адреса первого порядка
			 */
			const string setLow1Ip6(const string & ip) const noexcept;
			/**
			 * setLow2Ip6 Метод восстановления IPv6 адреса второго порядка
			 * @param ip адрес интернет протокола версии 6
			 * @return   восстановленный вид ip адреса второго порядка
			 */
			const string setLow2Ip6(const string & ip) const noexcept;
		public:
			/**
			 * getNetwork Метод извлечения данных сети
			 * @param str адрес подключения (127.0.0.1/255.000.000.000 или 127.0.0.1/8 или 127.0.0.1)
			 * @return    параметры подключения
			 */
			const nkdata_t getNetwork(const string & str) const noexcept;
			/**
			 * getMaskByNumber Метод получения маски из цифровых обозначений
			 * @param value цифровое обозначение маски
			 * @return      объект с данными маски
			 */
			const ntdata_t getMaskByNumber(const u_int value) const noexcept;
			/**
			 * getMaskByString Метод получения маски из строки обозначения маски
			 * @param value строка с обозначением маски
			 * @return      объект с данными маски
			 */
			const ntdata_t getMaskByString(const string & value) const noexcept;
		public:
			/**
			 * getDataIp Метод получает цифровые данные ip адреса
			 * @param ip данные ip адреса в виде строки
			 * @return   оцифрованные данные ip адреса
			 */
			const ipdata_t getDataIp(const string & ip) const noexcept;
			/**
			 * imposeMask Метод наложения маски
			 * @param ip   блок с данными ip адреса
			 * @param mask блок с данными маски сети
			 * @return     блок с данными сети
			 */
			const ipdata_t imposeMask(const ipdata_t & ip, const ipdata_t & mask) const noexcept;
		public:
			/**
			 * isPort Метод проверки на качество порта
			 * @param port входная строка якобы содержащая порт
			 * @return     результат проверки
			 */
			const bool isPort(const string & str) const noexcept;
			/**
			 * isV4ToV6 Метод проверки на отображение адреса IPv4 в IPv6
			 * @param ip адрес для проверки
			 * @return   результат проверки
			 */
			const bool isV4ToV6(const string & ip) const noexcept;
			/**
			 * isAddr Метод проверки на то является ли строка адресом
			 * @param address строка адреса для проверки
			 * @return        результат проверки
			 */
			const bool isAddr(const string & address) const noexcept;
			/**
			 * compareIP6 Метод проверки на совпадение ip адресов
			 * @param ip1 адрес подключения IPv6
			 * @param ip2 адрес подключения IPv6
			 * @return    результат проверки
			 */
			const bool compareIP6(const string & ip1, const string & ip2) const noexcept;
			/**
			 * checkMaskByNumber Метод проверки на соответствие маски по номеру маски
			 * @param ip   данные ip адреса
			 * @param mask номер маски
			 * @return     результат проверки
			 */
			const bool checkMaskByNumber(const string & ip, const u_int mask) const noexcept;
			/**
			 * checkIPByNetwork Метод проверки, принадлежит ли ip адрес указанной сети
			 * @param ip  данные ip адреса интернет протокола версии 4
			 * @param nwk адрес сети (192.168.0.0/16)
			 * @return    результат проверки
			 */
			const bool checkIPByNetwork(const string & ip, const string & nwk) const noexcept;
			/**
			 * checkIPByNetwork6 Метод проверки, принадлежит ли ip адрес указанной сети
			 * @param ip  данные ip адреса интернет протокола версии 6
			 * @param nwk адрес сети (2001:db8::/32)
			 * @return    результат проверки
			 */
			const bool checkIPByNetwork6(const string & ip, const string & nwk) const noexcept;
			/**
			 * checkMaskByString Метод проверки на соответствие маски по строке маски
			 * @param ip   данные ip адреса
			 * @param mask номер маски
			 * @return     результат проверки
			 */
			const bool checkMaskByString(const string & ip, const string & mask) const noexcept;
			/**
			 * checkDomain Метод определения определения соответствия домена маски
			 * @param domain название домена
			 * @param mask   маска домена для проверки
			 * @return       результат проверки
			 */
			const bool checkDomainByMask(const string & domain, const string & mask) const noexcept;
			/**
			 * checkRange Метод проверки входит ли ip адрес в указанный диапазон
			 * @param ip  ip данные ip адреса интернет протокола версии 6
			 * @param bip начальный диапазон ip адресов
			 * @param eip конечный диапазон ip адресов
			 * @return    результат проверки
			 */
			const bool checkRange6(const string & ip, const string & bip, const string & eip) const noexcept;
		public:
			/**
			 * setLowIp Метод восстановления IPv4 адреса
			 * @param ip адрес интернет протокола версии 4
			 * @return   восстановленный вид ip адреса
			 */
			const string setLowIp(const string & ip) const noexcept;
			/**
			 * getLowIp Метод упрощения IPv4 адреса
			 * @param ip адрес интернет протокола версии 4
			 * @return   упрощенный вид ip адреса
			 */
			const string getLowIp(const string & ip) const noexcept;
			/**
			 * getLowIp6 Метод упрощения IPv6 адреса
			 * @param ip адрес интернет протокола версии 6
			 * @return   упрощенный вид ip адреса
			 */
			const string getLowIp6(const string & ip) const noexcept;
			/**
			 * setLowIp6 Метод восстановления IPv6 адреса
			 * @param ip адрес интернет протокола версии 6
			 * @return   восстановленный вид ip адреса
			 */
			const string setLowIp6(const string & ip) const noexcept;
			/**
			 * pathByString Метод извлечения правильной записи адреса
			 * @param path первоначальная строка адреса
			 * @return     правильная строка адреса
			 */
			const string pathByString(const string & path) const noexcept;
			/**
			 * queryByString Метод извлечения параметров запроса из строки адреса
			 * @param path первоначальная строка адреса
			 * @return     параметры запроса
			 */
			const string queryByString(const string & path) const noexcept;
			/**
			 * getIPByNetwork Метод извлечения данных ip адреса из сети
			 * @param nwk строка содержащая адрес сети
			 * @return    восстановленный вид ip адреса
			 */
			const string getIPByNetwork(const string & nwk) const noexcept;
			/**
			 * ipv4IntToString Метод преобразования int 2 bytes в строку IP адреса
			 * @param ip адрес интернет подключения в цифровом виде
			 * @return   ip адрес в виде строки
			 */
			const string ipv4IntToString(const uint32_t ip) const noexcept;
			/**
			 * ipv6IntToString Метод преобразования int 8 bytes в строку IP адреса
			 * @param ip адрес интернет подключения в цифровом виде
			 * @return   ip адрес в виде строки
			 */
			const string ipv6IntToString(const uint8_t * ip = nullptr) const noexcept;
			/**
			 * addToPath Метод формирования адреса из пути и названия файла
			 * @param path путь где хранится файл
			 * @param file название файла
			 * @return     сформированный путь
			 */
			const string addToPath(const string & path, const string & file) const noexcept;
			/**
			 * imposePrefix6 Метод наложения префикса
			 * @param ip6    адрес интернет протокола версии 6
			 * @param prefix префикс сети
			 * @return       результат наложения префикса
			 */
			const string imposePrefix6(const string & ip6, const u_int prefix) const noexcept;
			/**
			 * pathByDomain Метод создания пути из доменного имени
			 * @param domain название домена
			 * @param delim  разделитель
			 * @return       путь к файлу кэша
			 */
			const string pathByDomain(const string & domain, const string & delim) const noexcept;
		public:
			/**
			 * ipv6StringToInt Метод преобразования строки IP адреса в int 2 байта
			 * @param ip адрес интернет подключения
			 * @return   ip адрес в цифровом виде
			 */
			const array <u_char, 16> ipv6StringToInt(const string & ip) const noexcept;
		public:
			/**
			 * strIp6ToHex64 Метод преобразования строки ip адреса в 16-й вид
			 * @param ip данные ip адреса интернет протокола версии 6
			 * @return   результат в 16-м виде
			 */
			const __uint64_t strIp6ToHex64(const string & ip) const noexcept;
			/**
			 * ipv4StringToInt Метод преобразования строки IP адреса в int 2 байта
			 * @param ip адрес интернет подключения
			 * @return   ip адрес в цифровом виде
			 */
			const uint32_t ipv4StringToInt(const string & ip) const noexcept;
		public:
			/**
			 * checkNetworkByIp Метод определения типа сети по ip адресу
			 * @param ip данные ip адреса
			 * @return   тип сети в 10-м виде
			 */
			const u_short checkNetworkByIp(const string & ip) const noexcept;
			/**
			 * getPrefixByNetwork Метод извлечения данных префикса из строки адреса сети
			 * @param nwk строка содержащая адрес сети
			 * @return    восстановленный вид префикса сети
			 */
			const u_int getPrefixByNetwork(const string & nwk) const noexcept;
		public:
			/**
			 * isLocal Метод проверки на то является ли ip адрес локальным
			 * @param ip адрес подключения ip
			 * @return   результат проверки (-1 - запрещенный, 0 - локальный, 1 - глобальный)
			 */
			const int isLocal(const string & ip) const noexcept;
			/**
			 * isLocal6 Метод проверки на то является ли ip адрес локальным
			 * @param ip адрес подключения IPv6
			 * @return   результат проверки (-1 - запрещенный, 0 - локальный, 1 - глобальный)
			 */
			const int isLocal6(const string & ip) const noexcept;
		public:
			/**
			 * parseUri Метод парсинга uri адреса
			 * @param uri адрес запроса
			 * @return    параметры запроса
			 */
			const url_t parseUri(const string & uri) const noexcept;
			/**
			 * parseHost Метод определения типа данных из строки URL адреса
			 * @param str строка с данными
			 * @return    определенный тип данных
			 */
			const type_t parseHost(const string & str) const noexcept;
		public:
			/**
			 * Network Конструктор
			 * @param fmk объект фреймворка
			 */
			Network(const fmk_t * fmk) noexcept : _fmk(fmk) {}
	} network_t;
};

#endif // __AWH_NETWORK__
