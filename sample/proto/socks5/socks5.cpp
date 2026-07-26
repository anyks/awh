/**
 * @file: socks5.cpp
 * @date: 2026-07-20
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с протоколом SOCKS5 — демонстрация обмена сообщениями приветствия,
 *        авторизации и команд между клиентской и серверной сторонами протокола
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <string>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <iostream>

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Системный заголовочный файл
	 */
	#include <winsock2.h>
/**
 * Для операционной системы не являющейся MS Windows
 */
#else
	/**
	 * Системный заголовочный файл
	 */
	#include <netinet/in.h>
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <proto/socks5/client.hpp>
#include <proto/socks5/server.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;
/**
 * Используем пространство имён протоколов
 */
using namespace awh::proto;

/**
 * @brief Функция вывода бинарного кадра в консоль
 *
 * @param title название выводимого кадра
 * @param data  бинарный буфер данных кадра
 * @param size  размер бинарного буфера данных кадра
 *
 */
static void printFrame(const string & title, const uint8_t * data, const size_t size) noexcept {
	// Выводим название и размер кадра
	cout << title << " (" << size << " bytes):";
	/**
	 * Переходим по всем байтам кадра
	 */
	for(size_t i = 0; i < size; i++)
		// Выводим очередной байт кадра в шестнадцатеричном виде
		cout << " " << hex << uppercase << setw(2) << setfill('0') << static_cast <uint16_t> (data[i]);
	// Восстанавливаем параметры потока вывода
	cout << dec << nouppercase << setfill(' ') << endl;
}
/**
 * @brief Функция формирования текстового представления адреса хоста
 *
 * @param host параметры адреса хоста
 * @return     текстовое представление адреса хоста
 *
 */
static string hostToString(const net::attr_t * host) noexcept {
	// Если параметры адреса хоста не установлены
	if(host == nullptr)
		// Возвращаем пустой адрес
		return "(none)";
	/**
	 * Определяем тип адреса хоста
	 */
	switch(static_cast <uint8_t> (host->type)){
		// Если тип адреса соответствует FQDN
		case static_cast <uint8_t> (net::type_t::FQDN): {
			// Получаем объект FQDN-адреса подключения
			const net::attr_fqdn_t * fqdn = static_cast <const net::attr_fqdn_t *> (host);
			// Возвращаем доменное имя и порт хоста
			return fqdn->domain + ":" + to_string(fqdn->port);
		}
		// Если тип адреса соответствует IPv4
		case static_cast <uint8_t> (net::type_t::IPV4): {
			// Получаем объект IP-адреса подключения
			const net::attr_net_t * attr = static_cast <const net::attr_net_t *> (host);
			// Извлекаем IP-адрес хоста в порядке байт хоста
			const uint32_t address = ntohl(static_cast <const net::addr_net_ipv4_t *> (attr->ip.get())->address);
			// Возвращаем IP-адрес и порт хоста
			return to_string((address >> 24) & 0xFF) + "." +
			       to_string((address >> 16) & 0xFF) + "." +
			       to_string((address >> 8) & 0xFF) + "." +
			       to_string(address & 0xFF) + ":" + to_string(attr->port);
		}
		// Если тип адреса соответствует IPv6
		case static_cast <uint8_t> (net::type_t::IPV6):
			// Возвращаем признак IPv6-адреса
			return "(IPv6)";
	}
	// Возвращаем признак неизвестного адреса
	return "(unknown)";
}
/**
 * @brief Демонстрация полного рукопожатия SOCKS5 без аутентификации
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleHandshake(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== HANDSHAKE (NO AUTH) ======== " << endl;
	// Создаём объект клиента SOCKS5
	client_socks5_t client(fmk, log);
	// Создаём объект сервера SOCKS5
	server_socks5_t server(fmk, log);
	// Создаём контекст обмена данными клиента
	socks5_t::ctx_t cctx;
	// Создаём контекст обмена данными сервера
	socks5_t::ctx_t sctx;
	// Буфер сформированного кадра
	uint8_t * data = nullptr;
	// Размер сформированного кадра
	size_t size = 0;
	// Формируем приветствие клиента со списком поддерживаемых методов аутентификации
	client.buffer(&data, size, cctx);
	// Выводим сформированный кадр приветствия клиента
	printFrame("Client greeting", data, size);
	// Выполняем разбор приветствия клиента на сервере
	server.parse(data, size, sctx);
	// Формируем ответ сервера с выбранным методом аутентификации
	server.buffer(&data, size, sctx);
	// Выводим сформированный кадр выбора метода аутентификации
	printFrame("Server method choice", data, size);
	// Выполняем разбор выбранного метода аутентификации на клиенте
	client.parse(data, size, cctx);
	// Устанавливаем хост конечного сервера для подключения
	cctx.host = make_unique <net::attr_fqdn_t> ();
	// Устанавливаем доменное имя конечного сервера
	static_cast <net::attr_fqdn_t *> (cctx.host.get())->domain = "anyks.com";
	// Устанавливаем порт конечного сервера
	static_cast <net::attr_fqdn_t *> (cctx.host.get())->port = 443;
	// Формируем запрос CONNECT для подключения к конечному серверу
	client.buffer(&data, size, cctx);
	// Выводим сформированный кадр запроса CONNECT
	printFrame("Client CONNECT request", data, size);
	// Выполняем разбор запроса CONNECT на сервере
	server.parse(data, size, sctx);
	// Выводим полученный сервером адрес конечного сервера
	cout << "Server received CONNECT to: " << hostToString(sctx.host.get()) << endl;
	/**
	 * В этот момент реальный прокси-сервер выполняет подключение к конечному серверу,
	 * а в ответе сообщает клиенту фактический адрес и порт установленного соединения
	 */
	sctx.host = make_unique <net::attr_net_t> ();
	// Устанавливаем порт установленного соединения
	static_cast <net::attr_net_t *> (sctx.host.get())->port = 53211;
	// Устанавливаем IP-адрес установленного соединения (127.0.0.1)
	static_cast <net::addr_net_ipv4_t *> (static_cast <net::attr_net_t *> (sctx.host.get())->ip.get())->address = htonl(0x7F000001);
	// Формируем ответ сервера на запрос CONNECT
	server.buffer(&data, size, sctx);
	// Выводим сформированный кадр ответа на запрос CONNECT
	printFrame("Server CONNECT response", data, size);
	// Выполняем разбор ответа на запрос CONNECT на клиенте
	client.parse(data, size, cctx);
	// Выводим итоговый результат выполнения рукопожатия
	cout << "Handshake: " << (cctx.state == socks5_t::state_t::HANDSHAKE ? "done" : "failed") << endl;
	// Выводим полученный клиентом адрес установленного соединения
	cout << "Bound address: " << hostToString(cctx.host.get()) << endl << endl;
}
/**
 * @brief Демонстрация аутентификации USER/PASS (RFC 1929)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleAuth(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== AUTH USER/PASS ======== " << endl;
	// Создаём объект клиента SOCKS5
	client_socks5_t client(fmk, log);
	// Создаём объект сервера SOCKS5
	server_socks5_t server(fmk, log);
	// Устанавливаем параметры авторизации клиента на сервере
	client.setUser("forman", "12345");
	// Устанавливаем функцию обратного вызова для проверки авторизации на сервере
	server.on([](const string & username, const string & password) noexcept -> bool {
		// Выводим полученные сервером данные авторизации
		cout << "Server checks credentials: [" << username << "] / [" << password << "]" << endl;
		// Выполняем проверку данных авторизации
		return ((username.compare("forman") == 0) && (password.compare("12345") == 0));
	});
	// Создаём контекст обмена данными клиента
	socks5_t::ctx_t cctx;
	// Создаём контекст обмена данными сервера
	socks5_t::ctx_t sctx;
	// Буфер сформированного кадра
	uint8_t * data = nullptr;
	// Размер сформированного кадра
	size_t size = 0;
	// Формируем приветствие клиента со списком поддерживаемых методов аутентификации
	client.buffer(&data, size, cctx);
	// Выводим сформированный кадр приветствия клиента
	printFrame("Client greeting", data, size);
	// Выполняем разбор приветствия клиента на сервере
	server.parse(data, size, sctx);
	// Формируем ответ сервера с требованием аутентификации USER/PASS
	server.buffer(&data, size, sctx);
	// Выводим сформированный кадр выбора метода аутентификации
	printFrame("Server method choice", data, size);
	// Выполняем разбор выбранного метода аутентификации на клиенте
	client.parse(data, size, cctx);
	// Формируем пакет авторизации с логином и паролем пользователя
	client.buffer(&data, size, cctx);
	// Выводим сформированный кадр авторизации
	printFrame("Client auth packet", data, size);
	// Выполняем разбор пакета авторизации на сервере
	server.parse(data, size, sctx);
	// Формируем ответ сервера с результатом авторизации
	server.buffer(&data, size, sctx);
	// Выводим сформированный кадр ответа авторизации
	printFrame("Server auth response", data, size);
	// Выполняем разбор ответа авторизации на клиенте
	client.parse(data, size, cctx);
	// Выводим итоговый результат авторизации на сервере
	cout << "Authorization: " << (cctx.state == socks5_t::state_t::CONNECT ? "success" : "failed") << endl << endl;
}
/**
 * @brief Демонстрация отказа в авторизации и получения кода ошибки
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleAuthFailed(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== AUTH FAILED ======== " << endl;
	// Создаём объект клиента SOCKS5
	client_socks5_t client(fmk, log);
	// Создаём объект сервера SOCKS5
	server_socks5_t server(fmk, log);
	// Устанавливаем неверные параметры авторизации клиента на сервере
	client.setUser("forman", "wrong-password");
	// Устанавливаем функцию обратного вызова для проверки авторизации на сервере
	server.on([](const string & username, const string & password) noexcept -> bool {
		// Выполняем проверку данных авторизации
		return ((username.compare("forman") == 0) && (password.compare("12345") == 0));
	});
	// Создаём контекст обмена данными клиента
	socks5_t::ctx_t cctx;
	// Создаём контекст обмена данными сервера
	socks5_t::ctx_t sctx;
	// Буфер сформированного кадра
	uint8_t * data = nullptr;
	// Размер сформированного кадра
	size_t size = 0;
	// Формируем приветствие клиента со списком поддерживаемых методов аутентификации
	client.buffer(&data, size, cctx);
	// Выполняем разбор приветствия клиента на сервере
	server.parse(data, size, sctx);
	// Формируем ответ сервера с требованием аутентификации USER/PASS
	server.buffer(&data, size, sctx);
	// Выполняем разбор выбранного метода аутентификации на клиенте
	client.parse(data, size, cctx);
	// Формируем пакет авторизации с неверным паролем пользователя
	client.buffer(&data, size, cctx);
	// Выполняем разбор пакета авторизации на сервере
	server.parse(data, size, sctx);
	// Формируем ответ сервера с отказом в авторизации
	server.buffer(&data, size, sctx);
	// Выводим сформированный кадр ответа авторизации
	printFrame("Server auth response", data, size);
	// Выполняем разбор ответа авторизации на клиенте
	client.parse(data, size, cctx);
	// Выводим итоговое состояние клиента после отказа в авторизации
	cout << "Client state: " << (cctx.state == socks5_t::state_t::BROKEN ? "BROKEN" : "?") << endl;
	// Выводим человекочитаемое сообщение кода статуса
	cout << "Status: " << socks5_t::statusMessage(cctx.status) << endl << endl;
}
/**
 * @brief Демонстрация кадрирования входящего потока данных
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleFraming(const fmk_t * fmk, const log_t * log) noexcept {
	// Игнорируем неиспользуемые параметры
	(void) fmk; (void) log;
	// Печатаем заголовок демонстрации
	cout << " ======== FRAMING ======== " << endl;
	// Формируем данные приветствия клиента с двумя методами аутентификации
	const uint8_t greeting[] = {0x05, 0x02, 0x00, 0x02};
	/**
	 * Данные из сети приходят произвольными кусками — frameSize() позволяет определить,
	 * накоплен ли полный SOCKS5-кадр, до передачи данных в парсер
	 */
	for(size_t i = 1; i <= sizeof(greeting); i++){
		// Определяем полный размер кадра для текущего количества данных
		const size_t need = socks5_t::frameSize(socks5_t::state_t::NONE, greeting, i);
		// Выводим результат определения размера кадра
		cout << "Greeting bytes=" << i << " => " << (need == 0 ? "incomplete" : ("frame size " + to_string(need))) << endl;
	}
	// Формируем данные запроса CONNECT с FQDN-адресом (anyks.com:443)
	const uint8_t request[] = {
		0x05, 0x01, 0x00, 0x03, 0x09,
		'a', 'n', 'y', 'k', 's', '.', 'c', 'o', 'm',
		0x01, 0xBB
	};
	// Определяем полный размер кадра для неполного запроса CONNECT
	cout << "CONNECT bytes=5 => " << (socks5_t::frameSize(socks5_t::state_t::CONNECT, request, 5) == 0 ? "incomplete" : "?") << endl;
	// Определяем полный размер кадра для полного запроса CONNECT
	cout << "CONNECT bytes=" << sizeof(request) << " => frame size " << socks5_t::frameSize(socks5_t::state_t::CONNECT, request, sizeof(request)) << endl;
	// Формируем некорректные данные пакета авторизации (версия не соответствует RFC 1929)
	const uint8_t broken[] = {0x05, 0x01};
	// Определяем размер кадра для некорректного пакета авторизации
	cout << "AUTH bad version => " << (socks5_t::frameSize(socks5_t::state_t::AUTH, broken, sizeof(broken)) == SIZE_MAX ? "broken frame" : "?") << endl << endl;
}
/**
 * @brief Демонстрация инкапсуляции UDP-датаграмм (UDP ASSOCIATE)
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
static void sampleUDP(const fmk_t * fmk, const log_t * log) noexcept {
	// Печатаем заголовок демонстрации
	cout << " ======== UDP ASSOCIATE ======== " << endl;
	// Создаём объект клиента SOCKS5
	client_socks5_t client(fmk, log);
	// Создаём объект сервера SOCKS5
	server_socks5_t server(fmk, log);
	// Создаём объект UDP заголовка исходящей датаграммы
	socks5_t::udp_head_t udpOut;
	// Устанавливаем хост конечного получателя датаграммы
	udpOut.host = make_unique <net::attr_fqdn_t> ();
	// Устанавливаем доменное имя конечного получателя
	static_cast <net::attr_fqdn_t *> (udpOut.host.get())->domain = "anyks.com";
	// Устанавливаем порт конечного получателя
	static_cast <net::attr_fqdn_t *> (udpOut.host.get())->port = 53;
	// Буфер сформированного UDP заголовка
	uint8_t * header = nullptr;
	// Размер сформированного UDP заголовка
	size_t size = 0;
	// Формируем UDP заголовок датаграммы
	client.buffer(&header, size, udpOut);
	// Формируем полезную нагрузку датаграммы
	const string payload = "DNS-QUERY";
	// Собираем полную датаграмму из UDP заголовка и полезной нагрузки
	vector <uint8_t> datagram(header, header + size);
	// Добавляем полезную нагрузку в датаграмму
	datagram.insert(datagram.end(), payload.begin(), payload.end());
	// Выводим сформированную датаграмму
	printFrame("Datagram", datagram.data(), datagram.size());
	// Создаём объект UDP заголовка входящей датаграммы
	socks5_t::udp_head_t udpIn;
	// Выполняем разбор UDP заголовка датаграммы на сервере
	server.parse(datagram.data(), datagram.size(), udpIn);
	// Выводим полученный адрес конечного получателя датаграммы
	cout << "Destination: " << hostToString(udpIn.host.get()) << endl;
	// Выводим размер UDP заголовка датаграммы
	cout << "Header size: " << udpIn.size << endl;
	// Выводим полезную нагрузку датаграммы (данные следуют сразу за заголовком)
	cout << "Payload: [" << string(datagram.begin() + udpIn.size, datagram.end()) << "]" << endl << endl;
}
/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Демонстрируем полное рукопожатие SOCKS5 без аутентификации
	sampleHandshake(&fmk, &log);
	// Демонстрируем аутентификацию USER/PASS
	sampleAuth(&fmk, &log);
	// Демонстрируем отказ в авторизации
	sampleAuthFailed(&fmk, &log);
	// Демонстрируем кадрирование входящего потока данных
	sampleFraming(&fmk, &log);
	// Демонстрируем инкапсуляцию UDP-датаграмм
	sampleUDP(&fmk, &log);
	// Возвращаем результат
	return EXIT_SUCCESS;
}
