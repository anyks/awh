/**
 * @file: ip.cpp
 * @date: 2025-10-31
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

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/net.hpp>

/**
 * Подписываемся на пространство имён AWH
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
	net_t net(&fmk, &log);

	// Выводим тесты IP-адресов
	net = "[2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d]";
	// Выводим IP-адрес в консоль
	cout << " [2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d] || DEFAULT: " << net << " === LONG: " << net.print(net_t::format_size_t::LONG) << " === MIDDLE: " << net.print(net_t::format_size_t::MIDDLE) << " === SHORT: " << net.print(net_t::format_size_t::SHORT) << " == SHORT DEC: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::DECIMAL) << " == SHORT OCT: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::OCTAL) << " == SHORT IPv4 => IPv6: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::HEX_IPV4) << endl;

	// Выводим тесты IP-адресов
	net = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	// Выводим IP-адрес в консоль
	cout << " 2001:0db8:0000:0000:0000:0000:ae21:ad12 || " << net << " == IPv4: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::HEX_IPV4) << endl;

	// Выводим тесты IP-адресов
	net = "2001:db8::ae21:ad12";
	// Выводим IP-адрес в консоль
	cout << " 2001:db8::ae21:ad12 || " << net.print(net_t::format_size_t::LONG) << " and " << net.print(net_t::format_size_t::MIDDLE) << endl;

	// Выводим тесты IP-адресов
	net = "0000:0000:0000:0000:0000:0000:ae21:ad12";
	// Выводим IP-адрес в консоль
	cout << " 0000:0000:0000:0000:0000:0000:ae21:ad12 || " << net.print(net_t::format_size_t::SHORT) << " == IPv4: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::HEX) << endl;

	// Выводим тесты IP-адресов
	net = "::ae21:ad12";
	// Выводим IP-адрес в консоль
	cout << " ::ae21:ad12 || " << net.print(net_t::format_size_t::MIDDLE) << " == IPv4: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::HEX) << endl;

	// Выводим тесты IP-адресов
	net = "2001:0db8:11a3:09d7:1f34::";
	// Выводим IP-адрес в консоль
	cout << " 2001:0db8:11a3:09d7:1f34:: || " << net.print(net_t::format_size_t::LONG) << endl;

	// Выводим тесты IP-адресов
	net = "::ffff:192.0.2.1";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выводим IP-адрес в консоль
	cout << " ::ffff:192.0.2.1 || " << net << " ==== " << net.broadcastIPv6ToIPv4() << " == IPv4: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::HEX) << endl;

	// Выводим тесты IP-адресов
	net = "[::1]";
	// Выводим IP-адрес в консоль
	cout << " ::1 || " << net.print(net_t::format_size_t::LONG) << endl;

	// Выводим тесты IP-адресов
	net = "[::]";
	// Выводим IP-адрес в консоль
	cout << " [::] || " << net.print(net_t::format_size_t::LONG) << endl;

	// Выводим тесты IP-адресов
	net = "46.39.230.51";
	// Выводим IP-адрес в консоль
	cout << " 46.39.230.51 || " << net.print(net_t::format_size_t::LONG) << " ==== " << net.broadcastIPv6ToIPv4() << endl;

	// Выводим тесты IP-адресов
	net = "192.16.0.1";
	// Выводим IP-адрес в консоль
	cout << " 192.16.0.1 || LONG: " << net.print(net_t::format_size_t::LONG) << " === SHORT: " << net.print(net_t::format_size_t::SHORT) << " === MIDDLE: " << net.print(net_t::format_size_t::MIDDLE) << " === SHORT HEX: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::HEX) << " === SHORT OCT: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::OCTAL) << endl;

	// Выводим тесты IP-адресов
	net = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	// Выводим составную часть IP-адреса
	cout << " Составная часть адреса: 2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d || " << net.v6()[0] << " and " << net.v6()[1] << endl;

	// Выводим тесты IP-адресов
	net = "46.39.230.51";
	// Выводим составную часть IP-адреса
	cout << " Составная часть адреса: 46.39.230.51 || " << net.v4() << endl;

	// Выводим тесты IP-адресов
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	net.impose(53, net_t::addr_t::NETWORK);
	// Выводим составную часть IP-адреса
	cout << " Наложение префикса: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 || " << net << endl;

	// Выводим тесты IP-адресов
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	net.impose("FFFF:FFFF:FFFF:F8::", net_t::addr_t::NETWORK);
	// Выводим составную часть IP-адреса
	cout << " Наложение префикса: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F8:: || " << net << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Выводим составную часть IP-адреса
	cout << " Вывод IP-адреса в 16-м формате: 192.168.3.192 || " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::HEX) << endl;
	// Выводим составную часть IP-адреса
	cout << " Вывод IP-адреса в 8-м формате: 192.168.3.192 || " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::OCTAL) << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Накладываем префикс 9
	net.impose(9, net_t::addr_t::NETWORK);
	// Выводим составную часть IP-адреса
	cout << " Наложение префикса: 192.168.3.192/9 || " << net << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Накладываем префикс 9
	net.impose("255.128.0.0", net_t::addr_t::NETWORK);
	// Выводим составную часть IP-адреса
	cout << " Наложение префикса: 192.168.3.192/255.128.0.0 || " << net << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Накладываем префикс 9
	net.impose("255.255.255.0", net_t::addr_t::NETWORK);
	// Выводим составную часть IP-адреса
	cout << " Наложение префикса: 192.168.3.192/255.255.255.0 || " << net << endl;

	// Выводим тесты IP-адресов
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	net.impose(53, net_t::addr_t::HOST);
	// Выводим составную часть IP-адреса
	cout << " Получаем хост адреса: 2001:1234:abcd:5678:9877:3322:5541:aabb/53 || " << net << endl;

	// Выводим тесты IP-адресов
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Накладываем префикс 53
	net.impose("FFFF:FFFF:FFFF:F800::", net_t::addr_t::HOST);
	// Выводим составную часть IP-адреса
	cout << " Получаем хост адреса: 2001:1234:abcd:5678:9877:3322:5541:aabb/FFFF:FFFF:FFFF:F800:: || " << net << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Накладываем префикс 9
	net.impose(9, net_t::addr_t::HOST);
	// Выводим составную часть IP-адреса
	cout << " Получаем хост адреса: 192.168.3.192/9 || " << net << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Накладываем префикс 9
	net.impose("255.128.0.0", net_t::addr_t::HOST);
	// Выводим составную часть IP-адреса
	cout << " Получаем хост адреса: 192.168.3.192/255.128.0.0 || " << net << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Накладываем префикс 9
	net.impose(24, net_t::addr_t::HOST);
	// Выводим составную часть IP-адреса
	cout << " Получаем хост адреса: 192.168.3.192/24 || " << net << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Накладываем префикс 9
	net.impose("255.255.255.0", net_t::addr_t::HOST);
	// Выводим составную часть IP-адреса
	cout << " Получаем хост адреса: 192.168.3.192/255.255.255.0 || " << net << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Выводим составную часть IP-адреса
	cout << " Получаем маску адреса из префикса сети 9 || " << net.prefix2Mask(9) << endl;
	// Выводим составную часть IP-адреса
	cout << " Получаем префикс сети из маски адреса 255.128.0.0 || " << static_cast <uint16_t> (net.mask2Prefix("255.128.0.0")) << endl;

	// Выводим тесты IP-адресов
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Выводим составную часть IP-адреса
	cout << " Получаем маску адреса из префикса сети 53 || " << net.prefix2Mask(53) << endl;
	// Выводим составную часть IP-адреса
	cout << " Получаем префикс сети из маски адреса FFFF:FFFF:FFFF:F800:: || " << static_cast <uint16_t> (net.mask2Prefix("FFFF:FFFF:FFFF:F800::")) << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << " Выполняем проверку соответствия адреса 192.168.3.192 сети 192.168.0.0 || " << net.mapping("192.168.0.0") << endl;

	// Выводим тесты IP-адресов
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << " Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb сети 2001:1234:abcd:5678:: || " << net.mapping("2001:1234:abcd:5678::") << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << " Выполняем проверку соответствия адреса 192.168.3.192 сети 192.128.0.0/9 || " << net.mapping("192.128.0.0", 9, net_t::addr_t::NETWORK) << endl;

	// Выводим тесты IP-адресов
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << " Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb сети 2001:1234:abcd:5678::/53 || " << net.mapping("2001:1234:abcd:5678::", 53, net_t::addr_t::NETWORK) << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << " Выполняем проверку соответствия адреса 192.168.3.192 сети 192.128.0.0/255.128.0.0 || " << net.mapping("192.128.0.0", "255.128.0.0", net_t::addr_t::NETWORK) << endl;

	// Выводим тесты IP-адресов
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << " Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb сети 2001:1234:abcd:5678::/FFFF:FFFF:FFFF:F800:: || " << net.mapping("2001:1234:abcd:5678::", "FFFF:FFFF:FFFF:F800::", net_t::addr_t::NETWORK) << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << " Выполняем проверку соответствия адреса 192.168.3.192 хосту 9/0.40.3.192 || " << net.mapping("0.40.3.192", 9, net_t::addr_t::HOST) << endl;

	// Выводим тесты IP-адресов
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << " Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb хосту 53/::5600:9877:3322:5541:AABB || " << net.mapping("::5600:9877:3322:5541:AABB", 53, net_t::addr_t::HOST) << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << " Выполняем проверку соответствия адреса 192.168.3.192 хосту 255.128.0.0/0.40.3.192 || " << net.mapping("0.40.3.192", "255.128.0.0", net_t::addr_t::HOST) << endl;

	// Выводим тесты IP-адресов
	net = "2001:1234:abcd:5678:9877:3322:5541:aabb";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку маппинга адреса
	cout << " Выполняем проверку соответствия адреса 2001:1234:abcd:5678:9877:3322:5541:aabb хосту FFFF:FFFF:FFFF:F800::/::5600:9877:3322:5541:AABB || " << net.mapping("::5600:9877:3322:5541:AABB", "FFFF:FFFF:FFFF:F800::", net_t::addr_t::HOST) << endl;

	// Выводим тесты IP-адресов
	net = "192.168.3.192";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку на вхождение адреса в диапазон
	cout << " Выполняем проверку на вхождение адреса в диапазон 192.168.3.192 в диапазон [192.168.3.100 - 192.168.3.200] || " << net.range("192.168.3.100", "192.168.3.200", 24) << endl;

	// Выводим тесты IP-адресов
	net = "46.39.230.51";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес глобальным
	cout << " Выполняем проверку является ли IP-адрес глобальным 46.39.230.51 || " << (net.mode() == net_t::mode_t::WAN) << endl;

	// Выводим тесты IP-адресов
	net = "192.168.31.12";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес локальным
	cout << " Выполняем проверку является ли IP-адрес локальным 192.168.31.12 || " << (net.mode() == net_t::mode_t::LAN) << endl;

	// Выводим тесты IP-адресов
	net = "0.0.0.0";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес зарезервированным
	cout << " Выполняем проверку является ли IP-адрес зарезервированным 0.0.0.0 || " << (net.mode() == net_t::mode_t::SYS) << endl;

	// Выводим тесты IP-адресов
	net = "[2a00:1450:4010:c0a::8b]";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес глобальным
	cout << " Выполняем проверку является ли IP-адрес глобальным [2a00:1450:4010:c0a::8b] || " << (net.mode() == net_t::mode_t::WAN) << endl;

	// Выводим тесты IP-адресов
	net = "::1";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес локальным
	cout << " Выполняем проверку является ли IP-адрес локальным [::1] || " << (net.mode() == net_t::mode_t::LAN) << endl;

	// Выводим тесты IP-адресов
	net = "::";
	// Разрешаем вывод булевых переменных
	cout << boolalpha;
	// Выполняем проверку является ли IP-адрес зарезервированным
	cout << " Выполняем проверку является ли IP-адрес зарезервированным [::] || " << (net.mode() == net_t::mode_t::SYS) << endl;

	// Создаём IP-адрес
	string ip = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	// Выводим результат IP-адреса
	cout << " Длинная запись адреса || " << ip << endl;
	// Присваиваем IP-адрес объекту сети
	ip = net = ip;
	// Выводим результат IP-адреса
	cout << " Короткая запись адреса || " << ip << endl;

	// Создаём IP-адрес сети
	net = "2001:1234:abcd:5678::";
	// Выводим результат IP-адреса
	cout << " Форма записи сети1 || " << net << endl;

	// Создаём IP-адрес сети
	net = "fe80:0000:0000:0000:1cff:84b4:8614:0000";
	// Выводим результат IP-адреса
	cout << " Форма записи сети2 || " << net << endl;

	// Создаём IP-адрес сети
	net = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	// Выводим результат IP-адреса
	cout << " Форма записи адреса || " << net << endl;

	// Создаём MAC адрес
	net = "73:0b:04:0d:db:79";
	// Выводим MAC адрес в консоль
	cout << "MAC: 73:0b:04:0d:db:79 || " << net << endl;

	// Устанавливаем ARPA-адрес
	net.arpa("70.255.255.5.in-addr.arpa");
	// Выводим полученный в результате IP-адрес
	cout << "ARPA: 70.255.255.5.in-addr.arpa || " << net << endl;
	// Выводим сформированный ARPA-адрес
	cout << " ARPA IPv4: " << net.arpa() << endl;

	// Устанавливаем ARPA-адрес
	net.arpa("b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa");
	// Выводим полученный в результате IP-адрес
	cout << "ARPA: b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa || " << net << endl;
	// Выводим сформированный ARPA-адрес
	cout << " ARPA IPv6: " << net.arpa() << endl;

	// Проверяем тип IP-адреса
	cout << "192.168.7.231 это IPv4 адрес: " << (net.host("192.168.7.231") == net_t::type_t::IPV4 ? "Да" : "Нет")  << endl;
	cout << "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d это IPv6 адрес: " << (net.host("2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d") == net_t::type_t::IPV6 ? "Да" : "Нет")  << endl;
	cout << "192.168.7.231/24 это IPv4 сеть: " << (net.host("192.168.7.231/24") == net_t::type_t::NETV4 ? "Да" : "Нет")  << endl;
	cout << "192.168.7.231/255.255.255.0 это IPv4 сеть: " << (net.host("192.168.7.231/255.255.255.0") == net_t::type_t::NETV4 ? "Да" : "Нет")  << endl;
	cout << "fe80::1cff:84b4:8614:0/112 это IPv6 сеть: " << (net.host("fe80::1cff:84b4:8614:0/112") == net_t::type_t::NETV6 ? "Да" : "Нет")  << endl;
	cout << "73:0b:04:0d:db:79 это MAC адрес: " << (net.host("73:0b:04:0d:db:79") == net_t::type_t::MAC ? "Да" : "Нет")  << endl;
	cout << "https://anyks.com это URL-адрес: " << (net.host("https://anyks.com") == net_t::type_t::URL ? "Да" : "Нет")  << endl;
	cout << "anyks.com это FQDN: " << (net.host("anyks.com") == net_t::type_t::FQDN ? "Да" : "Нет")  << endl;
	cout << "ns1.anyks.com это FQDN: " << (net.host("ns1.anyks.com") == net_t::type_t::FQDN ? "Да" : "Нет")  << endl;
	cout << "c:\\Program\\ Files это адрес файловой системы: " << (net.host("c:\\Program\\ Files") == net_t::type_t::FS ? "Да" : "Нет")  << endl;
	cout << "/opt/mc/bin/mc это адрес файловой системы: " << (net.host("/opt/mc/bin/mc") == net_t::type_t::FS ? "Да" : "Нет")  << endl;

	cout << " ========================= " << endl;

	net = "192.168.0.1";
	cout << " IP1: " << net << endl;
	cout << " IP2: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::HEX_IPV6) << endl;
	cout << " IP3: " << net.print(net_t::format_size_t::MIDDLE, net_t::format_flag_t::HEX_IPV6) << endl;
	cout << " IP4: " << net.print(net_t::format_size_t::LONG, net_t::format_flag_t::HEX_IPV6) << endl;
	cout << " IP5: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::HEX_IPV4) << endl;
	cout << " IP6: " << net.print(net_t::format_size_t::MIDDLE, net_t::format_flag_t::HEX_IPV4) << endl;
	cout << " IP7: " << net.print(net_t::format_size_t::LONG, net_t::format_flag_t::HEX_IPV4) << endl << endl;

	net = "10.1";
	cout << " IP2: " << net << endl << endl;

	net = "0xC0.0xA8.0.0x1";
	cout << " IP3: " << net << endl << endl;

	net = "2001:db8:0:0:0:8:800:200C:417A";
	cout << " IP4: " << net << endl << endl;

	net = "2001:db8::8:800:200C:417A";
	cout << " IP5: " << net << endl << endl;

	net = "::ae21:ad12";
	cout << " IP6: " << net << endl << endl;

	net = "::ffff:192.0.2.128";
	cout << " IP7: " << net << endl;
	cout << " IP7: as IPv4: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::HEX_IPV6) << endl << endl;

	net = "fe80::1%eth0";
	cout << " IP8: " << net << " == " << net.zone() << endl << endl;

	net = "[fe80::1%25en0]";
	cout << " IP9: " << net << " == " << net.zone() << endl << endl;

	net = "::";
	cout << " IP10: " << net << endl << endl;

	cout << " ========================= " << endl;

	net = "73:0b:04:0d:db:79";
	cout << " MAC: " << net.print(net_t::format_size_t::MIDDLE, net_t::format_flag_t::HEX, '-') << endl << endl;

	net = "192.168.53.3";
	cout << " IP: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::DECIMAL) << endl << endl;

	// Создаём IP-адрес сети
	net = "2001:0db8:0000:0000:0000:0000:ae21:ad12";
	// net = "2001:0db8:11a3:09d7:1f34:8a2e:07a0:765d";
	// net = "2001:0db8:0000:0000:0000:0000:ae21:0";
	// net = "::ffff:192.0.2.1";
	// net = "0000:0000:0000:0000:0000:0000:ae21:ad12";
		
	// net = "FFFF:FFFF:FFFF:F800::";
	// net = "2001:0db8:11a3:09d7:1f34::";

	// Выводим результат IP-адреса
	cout << " IP: " << net.print(net_t::format_size_t::SHORT, net_t::format_flag_t::OCTAL, '-') << endl << endl;

	// Выводим результат
	return EXIT_SUCCESS;
}
