/**
 * @file: ip.cpp
 * @date: 2025-10-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/addr.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Выполняем создание объекта IP-адреса
	net_addr_t addr(&fmk, &log);

	// Выполняем тесты IP-адресов
	addr = "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d]";
	// Печатаем IP-адрес в консоль
	cout << "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] DEFAULT: " << addr << endl;
	cout << "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] LONG: " << addr.print(net_addr_t::format_size_t::LONG) << endl;
	cout << "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] MIDDLE: " << addr.print(net_addr_t::format_size_t::MIDDLE) << endl;
	cout << "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] SHORT: " << addr.print(net_addr_t::format_size_t::SHORT) << endl;
	cout << "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] SHORT DEC: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::DECIMAL) << endl;
	cout << "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] SHORT OCT: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::OCTAL) << endl;
	cout << "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] SHORT IPv4 => IPv6: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::HEX_IPV4) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	// Печатаем IP-адрес в консоль
	cout << "2001:0db8:0000:0000:0000:0000:ae21:ad12 DEFAULT: " << addr << endl;
	cout << "2001:0db8:0000:0000:0000:0000:ae21:ad12 SHORT IPv4: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::HEX_IPV4) << endl;
	cout << "2001:0db8:0000:0000:0000:0000:ae21:ad12 MIDDLE IPv4: " << addr.print(net_addr_t::format_size_t::MIDDLE, net_addr_t::format_flag_t::HEX_IPV4) << endl;
	cout << "2001:0db8:0000:0000:0000:0000:ae21:ad12 LONG IPv4: " << addr.print(net_addr_t::format_size_t::LONG, net_addr_t::format_flag_t::HEX_IPV4) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:db8::ae21:ad12";
	// Печатаем IP-адрес в консоль
	cout << "2001:db8::ae21:ad12 LONG: " << addr.print(net_addr_t::format_size_t::LONG) << endl;
	cout << "2001:db8::ae21:ad12 MIDDLE: " << addr.print(net_addr_t::format_size_t::MIDDLE) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "0000:0000:0000:0000:0000:0000:ae21:ad12";
	// Печатаем IP-адрес в консоль
	cout << "0000:0000:0000:0000:0000:0000:ae21:ad12 SHORT: " << addr.print(net_addr_t::format_size_t::SHORT) << endl;
	cout << "0000:0000:0000:0000:0000:0000:ae21:ad12 IPv4: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::HEX) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "::ae21:ad12";
	// Печатаем IP-адрес в консоль
	cout << "::ae21:ad12 MIDDLE: " << addr.print(net_addr_t::format_size_t::MIDDLE) << endl;
	cout << "::ae21:ad12 IPv4: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::HEX) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:0db8:11a3:09d7:1f34::";
	// Печатаем IP-адрес в консоль
	cout << "2001:0db8:11a3:09d7:1f34:: LONG: " << addr.print(net_addr_t::format_size_t::LONG) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "::ffff:192.0.2.1";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Печатаем IP-адрес в консоль
	cout << "::ffff:192.0.2.1 BROADCAST IPv4 => IPv6: " << addr << " == " << addr.broadcastIPv6ToIPv4() << endl;
	cout << "::ffff:192.0.2.1 IPv4: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::HEX) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "[::1]";
	// Печатаем IP-адрес в консоль
	cout << "::1 " << addr.print(net_addr_t::format_size_t::LONG) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "[::]";
	// Печатаем IP-адрес в консоль
	cout << "[::] " << addr.print(net_addr_t::format_size_t::LONG) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "46.39.230.51";
	// Печатаем IP-адрес в консоль
	cout << "46.39.230.51 LONG: " << addr.print(net_addr_t::format_size_t::LONG) << endl;
	cout << "46.39.230.51 BROADCAST IPv4 => IPv6: " << addr.broadcastIPv6ToIPv4() << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.16.0.1";
	// Печатаем IP-адрес в консоль
	cout << "192.16.0.1 LONG: " << addr.print(net_addr_t::format_size_t::LONG) << endl;
	cout << "192.16.0.1 SHORT: " << addr.print(net_addr_t::format_size_t::SHORT) << endl;
	cout << "192.16.0.1 MIDDLE: " << addr.print(net_addr_t::format_size_t::MIDDLE) << endl;
	cout << "192.16.0.1 SHORT HEX: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::HEX) << endl;
	cout << "192.16.0.1 SHORT OCT: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::OCTAL) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	// Возвращаем составную часть IP-адреса
	cout << "Составная часть адреса: 2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d SIZE=" << addr.v6().size() << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "46.39.230.51";
	// Возвращаем составную часть IP-адреса
	cout << "Составная часть адреса: 46.39.230.51 BINARY: " << addr.v4() << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	addr.impose(53, net_addr_t::addr_t::NETWORK);
	// Возвращаем составную часть IP-адреса
	cout << "Наложение префикса: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	addr.impose("FFFF:FFFF:FFFF:F8::", net_addr_t::addr_t::NETWORK);
	// Возвращаем составную часть IP-адреса
	cout << "Наложение префикса: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F8:: == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Возвращаем составную часть IP-адреса
	cout << "Вывод IP-адреса в 16-м формате: 192.168.3.192 == " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::HEX) << endl << endl;
	// Возвращаем составную часть IP-адреса
	cout << "Вывод IP-адреса в 8-м формате: 192.168.3.192 == " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::OCTAL) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Накладываем префикс 9
	addr.impose(9, net_addr_t::addr_t::NETWORK);
	// Возвращаем составную часть IP-адреса
	cout << "Наложение префикса: 192.168.3.192/9 == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Накладываем префикс 9
	addr.impose("255.128.0.0", net_addr_t::addr_t::NETWORK);
	// Возвращаем составную часть IP-адреса
	cout << "Наложение префикса: 192.168.3.192/255.128.0.0 == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Накладываем префикс 9
	addr.impose("255.255.255.0", net_addr_t::addr_t::NETWORK);
	// Возвращаем составную часть IP-адреса
	cout << "Наложение префикса: 192.168.3.192/255.255.255.0 == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	addr.impose(53, net_addr_t::addr_t::HOST);
	// Возвращаем составную часть IP-адреса
	cout << "Получаем хост адреса: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем маску FFFF:FFFF:FFFF:F800::
	addr.impose("FFFF:FFFF:FFFF:F800::", net_addr_t::addr_t::HOST);
	// Возвращаем составную часть IP-адреса
	cout << "Получаем хост адреса: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F800:: == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Накладываем префикс 9
	addr.impose(9, net_addr_t::addr_t::HOST);
	// Возвращаем составную часть IP-адреса
	cout << "Получаем хост адреса: 192.168.3.192/9 == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Накладываем маску 255.128.0.0
	addr.impose("255.128.0.0", net_addr_t::addr_t::HOST);
	// Возвращаем составную часть IP-адреса
	cout << "Получаем хост адреса: 192.168.3.192/255.128.0.0 == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Накладываем префикс 24
	addr.impose(24, net_addr_t::addr_t::HOST);
	// Возвращаем составную часть IP-адреса
	cout << "Получаем хост адреса: 192.168.3.192/24 == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Накладываем маску 255.255.255.0
	addr.impose("255.255.255.0", net_addr_t::addr_t::HOST);
	// Возвращаем составную часть IP-адреса
	cout << "Получаем хост адреса: 192.168.3.192/255.255.255.0 == " << addr << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Возвращаем составную часть IP-адреса
	cout << "Получаем маску адреса из префикса сети 9 == " << addr.prefix2Mask(9) << endl << endl;
	// Возвращаем составную часть IP-адреса
	cout << "Получаем префикс сети из маски адреса 255.128.0.0 == " << static_cast <uint16_t> (addr.mask2Prefix("255.128.0.0")) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Возвращаем составную часть IP-адреса
	cout << "Получаем маску адреса из префикса сети 53 == " << addr.prefix2Mask(53) << endl << endl;
	// Возвращаем составную часть IP-адреса
	cout << "Получаем префикс сети из маски адреса FFFF:FFFF:FFFF:F800:: == " << static_cast <uint16_t> (addr.mask2Prefix("FFFF:FFFF:FFFF:F800::")) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << "Выполняем проверку соответствия адреса 192.168.3.192 сети 192.168.0.0 == " << addr.mapping("192.168.0.0") << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << "Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb сети 2001:1234:abcd:5678:: == " << addr.mapping("2001:1234:abcd:5678::") << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << "Выполняем проверку соответствия адреса 192.168.3.192 сети 192.128.0.0/9 == " << addr.mapping("192.128.0.0", 9, net_addr_t::addr_t::NETWORK) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << "Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb сети 2001:1234:abcd:5678::/53 == " << addr.mapping("2001:1234:abcd:5678::", 53, net_addr_t::addr_t::NETWORK) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << "Выполняем проверку соответствия адреса 192.168.3.192 сети 192.128.0.0/255.128.0.0 == " << addr.mapping("192.128.0.0", "255.128.0.0", net_addr_t::addr_t::NETWORK) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << "Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb сети 2001:1234:abcd:5678::/FFFF:FFFF:FFFF:F800:: == " << addr.mapping("2001:1234:abcd:5678::", "FFFF:FFFF:FFFF:F800::", net_addr_t::addr_t::NETWORK) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << "Выполняем проверку соответствия адреса 192.168.3.192 хосту 9/0.40.3.192 == " << addr.mapping("0.40.3.192", 9, net_addr_t::addr_t::HOST) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << "Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb хосту 53/::5600:9877:3322:5541:AABB == " << addr.mapping("::5600:9877:3322:5541:AABB", 53, net_addr_t::addr_t::HOST) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << "Выполняем проверку соответствия адреса 192.168.3.192 хосту 255.128.0.0/0.40.3.192 == " << addr.mapping("0.40.3.192", "255.128.0.0", net_addr_t::addr_t::HOST) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << "Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb хосту FFFF:FFFF:FFFF:F800::/::5600:9877:3322:5541:AABB == " << addr.mapping("::5600:9877:3322:5541:AABB", "FFFF:FFFF:FFFF:F800::", net_addr_t::addr_t::HOST) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку на вхождение адреса в диапазон
	cout << "Выполняем проверку на вхождение адреса в диапазон 192.168.3.192 в диапазон [192.168.3.100 - 192.168.3.200] == " << addr.range("192.168.3.100", "192.168.3.200", 24) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "46.39.230.51";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес глобальным
	cout << "Выполняем проверку является ли IP-адрес глобальным 46.39.230.51 == " << (addr.own() == net_addr_t::own_t::WAN) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.31.12";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес локальным
	cout << "Выполняем проверку является ли IP-адрес локальным 192.168.31.12 == " << (addr.own() == net_addr_t::own_t::LAN) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "0.0.0.0";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес зарезервированным
	cout << "Выполняем проверку является ли IP-адрес зарезервированным 0.0.0.0 == " << (addr.own() == net_addr_t::own_t::SYS) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "[2a00:1450:4010:c0a::8b]";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес глобальным
	cout << "Выполняем проверку является ли IP-адрес глобальным [2a00:1450:4010:c0a::8b] == " << (addr.own() == net_addr_t::own_t::WAN) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "::1";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес локальным
	cout << "Выполняем проверку является ли IP-адрес локальным [::1] == " << (addr.own() == net_addr_t::own_t::LAN) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "::";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес зарезервированным
	cout << "Выполняем проверку является ли IP-адрес зарезервированным [::] == " << (addr.own() == net_addr_t::own_t::SYS) << endl << endl;

	// Выполняем тесты IP-адресов
	addr = "192.168.3.25";
	// Извлекаем адрес IPv4 в числовом виде
	uint32_t v4 = addr.v4(net_addr_t::endian_t::BIG);
	// Формируем адрес Broadcast
	addr.v4((v4 & 0xFFFFFF00U) | 0x000000FFU, net_addr_t::endian_t::BIG);
	// Возвращаем результат IP-адреса
	cout << "Broadcast: " << addr << endl << endl;

	// Создаём IP-адрес
	string ip = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	// Возвращаем результат IP-адреса
	cout << "Длинная запись адреса " << ip << endl;
	// Присваиваем IP-адрес объекту сети
	ip = addr = ip;
	// Возвращаем результат IP-адреса
	cout << "Короткая запись адреса " << ip << endl << endl;

	// Создаём IP-адрес сети
	addr = "2001:1234:abcd:5678::";
	// Возвращаем результат IP-адреса
	cout << "Форма записи сети1 " << addr << endl << endl;

	// Создаём IP-адрес сети
	addr = "fe80:0000:0000:0000:1cff:84b4:8614:0000";
	// Возвращаем результат IP-адреса
	cout << "Форма записи сети2 " << addr << endl << endl;

	// Создаём IP-адрес сети
	addr = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	// Возвращаем результат IP-адреса
	cout << "Форма записи адреса " << addr << endl << endl;

	// Создаём MAC адрес
	addr = "73:0b:04:0d:db:79";
	// Печатаем MAC-адрес в консоль
	cout << "MAC: 73:0b:04:0d:db:79 == " << addr << endl << endl;

	// Устанавливаем ARPA-адрес
	addr.arpa("70.255.255.5.in-addr.arpa");
	// Возвращаем полученный в результате IP-адрес
	cout << "ARPA: 70.255.255.5.in-addr.arpa == " << addr << endl;
	// Возвращаем сформированный ARPA-адрес
	cout << "ARPA IPv4: " << addr.arpa() << endl << endl;

	// Устанавливаем ARPA-адрес
	addr.arpa("b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa");
	// Возвращаем полученный в результате IP-адрес
	cout << "ARPA: b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa == " << addr << endl;
	// Возвращаем сформированный ARPA-адрес
	cout << "ARPA IPv6: " << addr.arpa() << endl << endl;

	// Проверяем тип IP-адреса
	cout << "192.168.7.231 это IPv4 адрес: " << (addr.host("192.168.7.231") == net_addr_t::type_t::IPV4 ? "Да" : "Нет")  << endl;
	cout << "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d это IPv6 адрес: " << (addr.host("2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d") == net_addr_t::type_t::IPV6 ? "Да" : "Нет")  << endl;
	cout << "192.168.7.231/24 это IPv4 сеть: " << (addr.host("192.168.7.231/24") == net_addr_t::type_t::NETV4 ? "Да" : "Нет")  << endl;
	cout << "192.168.7.231/255.255.255.0 это IPv4 сеть: " << (addr.host("192.168.7.231/255.255.255.0") == net_addr_t::type_t::NETV4 ? "Да" : "Нет")  << endl;
	cout << "fe80::1cff:84b4:8614:0/112 это IPv6 сеть: " << (addr.host("fe80::1cff:84b4:8614:0/112") == net_addr_t::type_t::NETV6 ? "Да" : "Нет")  << endl;
	cout << "73:0b:04:0d:db:79 это MAC адрес: " << (addr.host("73:0b:04:0d:db:79") == net_addr_t::type_t::MAC ? "Да" : "Нет")  << endl;
	cout << "https://anyks.com это URL-адрес: " << (addr.host("https://anyks.com") == net_addr_t::type_t::URL ? "Да" : "Нет")  << endl;
	cout << "anyks.com это FQDN: " << (addr.host("anyks.com") == net_addr_t::type_t::FQDN ? "Да" : "Нет")  << endl;
	cout << "ns1.anyks.com это FQDN: " << (addr.host("ns1.anyks.com") == net_addr_t::type_t::FQDN ? "Да" : "Нет")  << endl;
	cout << "c:\\Program\\ Files это адрес файловой системы: " << (addr.host("c:\\Program\\ Files") == net_addr_t::type_t::FS ? "Да" : "Нет")  << endl;
	cout << "/opt/mc/bin/mc это адрес файловой системы: " << (addr.host("/opt/mc/bin/mc") == net_addr_t::type_t::FS ? "Да" : "Нет")  << endl << endl;

	addr = "192.168.0.1";
	cout << "IP1: " << addr << endl;
	cout << "IP2: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::HEX_IPV6) << endl;
	cout << "IP3: " << addr.print(net_addr_t::format_size_t::MIDDLE, net_addr_t::format_flag_t::HEX_IPV6) << endl;
	cout << "IP4: " << addr.print(net_addr_t::format_size_t::LONG, net_addr_t::format_flag_t::HEX_IPV6) << endl;
	cout << "IP5: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::HEX_IPV4) << endl;
	cout << "IP6: " << addr.print(net_addr_t::format_size_t::MIDDLE, net_addr_t::format_flag_t::HEX_IPV4) << endl;
	cout << "IP7: " << addr.print(net_addr_t::format_size_t::LONG, net_addr_t::format_flag_t::HEX_IPV4) << endl << endl;

	addr = "10.1";
	cout << "IP2: " << addr << endl << endl;

	addr = "0xC0.0xA8.0.0x1";
	cout << "IP3: " << addr << endl << endl;

	addr = "2001:db8:0:0:0:8:800:200C:417A";
	cout << "IP4: " << addr << endl << endl;

	addr = "2001:db8::8:800:200C:417A";
	cout << "IP5: " << addr << endl << endl;

	addr = "::ae21:ad12";
	cout << "IP6: " << addr << endl << endl;

	addr = "::ffff:192.0.2.128";
	cout << "IP7: " << addr << endl;
	cout << "IP7: as IPv4: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::HEX_IPV6) << endl << endl;

	addr = "fe80::1%eth0";
	cout << "IP8: " << addr << " == " << addr.zone() << endl << endl;

	addr = "[fe80::1%25en0]";
	cout << "IP9: " << addr << " == " << addr.zone() << endl << endl;

	addr = "::";
	cout << "IP10: " << addr << endl << endl;

	addr = "73:0b:04:0d:db:79";
	cout << "MAC: " << addr.print(net_addr_t::format_size_t::MIDDLE, net_addr_t::format_flag_t::HEX, '-') << endl << endl;

	addr = "192.168.53.3";
	cout << "IP: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::DECIMAL) << endl << endl;

	// Создаём IP-адрес сети
	addr = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	// addr = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	// addr = "2001:0db8:0000:0000:0000:0000:ae21:0";
	// addr = "::ffff:192.0.2.1";
	// addr = "0000:0000:0000:0000:0000:0000:ae21:ad12";
		
	// addr = "FFFF:FFFF:FFFF:F800::";
	// addr = "2001:0db8:11a3:09d7:1f34::";

	// Возвращаем результат IP-адреса
	cout << "IP: " << addr.print(net_addr_t::format_size_t::SHORT, net_addr_t::format_flag_t::OCTAL, '-') << endl << endl;

	// Возвращаем результат
	return EXIT_SUCCESS;
}
