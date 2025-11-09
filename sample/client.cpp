/**
 * @file: client.cpp
 * @date: 2025-10-25
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

#include <cinttypes>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>

/**
 * Подписываемся на пространство имён AWH
 */
using namespace awh;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

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
	// Создаём объект логирования
	log_t log(&fmk);
	// Устанавливаем логгер
	fmk.setLogger(&log);
	// Устанавливаем уровень логирования
	log.level(log_t::level_t::NONE);
	/**
	 * Клиентская часть асинхронного движка ввода-вывода
	 */
	{
		// Создаём объект асинхронного движка ввода-вывода
		io_t io(&fmk, &log);
		/**
		 * IPv4 событие
		 */
		{
			cout << endl << " ******************** IPv4 CLIENT ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid1 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid1, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.iface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.address(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.address(eid1, event::address_t::IPV4);

				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.target(eid1) << " || " << io.address(eid1, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid1) << endl;

				io.bufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid2 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid2, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.address(eid2, event::address_t::MAC, mac)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid2) << endl;
				cout << " MAC-адрес: " << io.address(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid2, event::address_t::IPV4) << " == " << io.target(eid2) << " || " << io.address(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid2) << endl;

				io.bufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid3 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid3, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid3, 8080);
			// Устанавливаем IP-адрес события
			if(io.address(eid3, event::address_t::IPV4, ip)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid3) << endl;
				cout << " MAC-адрес: " << io.address(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid3, event::address_t::IPV4) << " == " << io.target(eid3) << " || " << io.address(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid3) << endl;

				io.bufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid4 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid4, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid4, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid4, event::address_t::NETWORK, ip + "/255.255.255.0")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid4) << endl;
				cout << " MAC-адрес: " << io.address(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid4, event::address_t::IPV4) << " == " << io.target(eid4) << " || " << io.address(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid4) << endl;

				io.bufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid5 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid5, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid5) << endl;
				cout << " MAC-адрес: " << io.address(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid5, event::address_t::UDS) << " == " << io.target(eid5) << " || " << io.address(eid5, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.port(eid5) << endl;

				io.bufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid6 = io.event(event::family_t::FILE, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid6, event::node_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FILE, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid6, event::address_t::FILE) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.port(eid6) << endl;

				io.bufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid7 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid7, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid7, ip)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid7) << endl;
				cout << " MAC-адрес: " << io.address(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid7, event::address_t::IPV4) << " == " << io.target(eid7) << " || " << io.address(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid7) << endl;

				io.bufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid8 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid8, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid8, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid8) << endl;
				cout << " MAC-адрес: " << io.address(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid8, event::address_t::IPV4) << " == " << io.target(eid8) << " || " << io.address(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid8) << endl;

				io.bufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
		/**
		 * IPv6 событие
		 */
		{
			cout << endl << " ******************** IPv6 CLIENT ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid1 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid1, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.iface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.address(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.address(eid1, event::address_t::IPV6);

				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.target(eid1) << " || " << io.address(eid1, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid1) << endl;

				io.bufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid2 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid2, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.address(eid2, event::address_t::MAC, mac)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid2) << endl;
				cout << " MAC-адрес: " << io.address(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid2, event::address_t::IPV6) << " == " << io.target(eid2) << " || " << io.address(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid2) << endl;

				io.bufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid3 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid3, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid3, 8080);
			// Устанавливаем IP-адрес события
			if(io.address(eid3, event::address_t::IPV6, ip)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid3) << endl;
				cout << " MAC-адрес: " << io.address(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid3, event::address_t::IPV6) << " == " << io.target(eid3) << " || " << io.address(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid3) << endl;

				io.bufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid4 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid4, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid4, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid4, event::address_t::NETWORK, ip + "/112")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid4) << endl;
				cout << " MAC-адрес: " << io.address(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid4, event::address_t::IPV6) << " == " << io.target(eid4) << " || " << io.address(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid4) << endl;

				io.bufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid5 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid5, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid5) << endl;
				cout << " MAC-адрес: " << io.address(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid5, event::address_t::UDS) << " == " << io.target(eid5) << " || " << io.address(eid5, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.port(eid5) << endl;

				io.bufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid6 = io.event(event::family_t::FILE, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid6, event::node_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FILE, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid6, event::address_t::FILE) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.port(eid6) << endl;

				io.bufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid7 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid7, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid7, ip)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid7) << endl;
				cout << " MAC-адрес: " << io.address(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid7, event::address_t::IPV6) << " == " << io.target(eid7) << " || " << io.address(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid7) << endl;

				io.bufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid8 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid8, event::node_t::CLIENT);
			// Устанавливаем порт события
			io.port(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid8, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid8) << endl;
				cout << " MAC-адрес: " << io.address(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid8, event::address_t::IPV6) << " == " << io.target(eid8) << " || " << io.address(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid8) << endl;

				io.bufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
	}
	/**
	 * Серверная часть асинхронного движка ввода-вывода
	 */
	{
		// Создаём объект асинхронного движка ввода-вывода
		io_t io(&fmk, &log);
		/**
		 * IPv4 событие
		 */
		{
			cout << endl << " ******************** IPv4 SERVER ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid1 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid1, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.iface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.address(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.address(eid1, event::address_t::IPV4);

				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.target(eid1) << " || " << io.address(eid1, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid1) << endl;

				io.bufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid2 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid2, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.address(eid2, event::address_t::MAC, mac)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid2) << endl;
				cout << " MAC-адрес: " << io.address(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid2, event::address_t::IPV4) << " == " << io.target(eid2) << " || " << io.address(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid2) << endl;

				io.bufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid3 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid3, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid3, 8080);
			// Устанавливаем IP-адрес события
			if(io.address(eid3, event::address_t::IPV4, ip)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid3) << endl;
				cout << " MAC-адрес: " << io.address(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid3, event::address_t::IPV4) << " == " << io.target(eid3) << " || " << io.address(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid3) << endl;

				io.bufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid4 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid4, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid4, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid4, event::address_t::NETWORK, ip + "/255.255.255.0")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid4) << endl;
				cout << " MAC-адрес: " << io.address(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid4, event::address_t::IPV4) << " == " << io.target(eid4) << " || " << io.address(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid4) << endl;

				io.bufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid5 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid5, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid5) << endl;
				cout << " MAC-адрес: " << io.address(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid5, event::address_t::UDS) << " == " << io.target(eid5) << " || " << io.address(eid5, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.port(eid5) << endl;

				io.bufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid6 = io.event(event::family_t::FILE, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid6, event::node_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FILE, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid6, event::address_t::FILE) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.port(eid6) << endl;

				io.bufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid7 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid7, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid7, ip)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid7) << endl;
				cout << " MAC-адрес: " << io.address(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid7, event::address_t::IPV4) << " == " << io.target(eid7) << " || " << io.address(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid7) << endl;

				io.bufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid8 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid8, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid8, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid8) << endl;
				cout << " MAC-адрес: " << io.address(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid8, event::address_t::IPV4) << " == " << io.target(eid8) << " || " << io.address(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid8) << endl;

				io.bufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
		/**
		 * IPv6 событие
		 */
		{
			cout << endl << " ******************** IPv6 SERVER ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid1 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid1, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.iface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.address(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.address(eid1, event::address_t::IPV6);

				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.target(eid1) << " || " << io.address(eid1, event::address_t::UDS) << " || " << io.address(eid1, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.port(eid1) << endl;

				io.bufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid2 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid2, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.address(eid2, event::address_t::MAC, mac)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid2) << endl;
				cout << " MAC-адрес: " << io.address(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid2, event::address_t::IPV6) << " == " << io.target(eid2) << " || " << io.address(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid2) << endl;

				io.bufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid3 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid3, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid3, 8080);

			cout << " Устанавливаем IP-адрес события: " << ip << endl;

			// Устанавливаем IP-адрес события
			if(io.address(eid3, event::address_t::IPV6, ip)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid3) << endl;
				cout << " MAC-адрес: " << io.address(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid3, event::address_t::IPV6) << " == " << io.target(eid3) << " || " << io.address(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid3) << endl;

				io.bufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid4 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid4, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid4, 8080);

			cout << " Устанавливаем IP-сеть события: " << (ip + "/112") << endl;

			// Устанавливаем сетевой адрес события
			if(io.address(eid4, event::address_t::NETWORK, ip + "/112")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid4) << endl;
				cout << " MAC-адрес: " << io.address(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid4, event::address_t::IPV6) << " == " << io.target(eid4) << " || " << io.address(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid4) << endl;

				io.bufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid5 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid5, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid5) << endl;
				cout << " MAC-адрес: " << io.address(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid5, event::address_t::UDS) << " == " << io.target(eid5) << " || " << io.address(eid5, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.port(eid5) << endl;

				io.bufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid6 = io.event(event::family_t::FILE, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid6, event::node_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FILE, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid6, event::address_t::FILE) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.port(eid6) << endl;

				io.bufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid7 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid7, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid7, ip)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid7) << endl;
				cout << " MAC-адрес: " << io.address(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid7, event::address_t::IPV6) << " == " << io.target(eid7) << " || " << io.address(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid7) << endl;

				io.bufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid8 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid8, event::node_t::SERVER);
			// Устанавливаем порт события
			io.port(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid8, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid8) << endl;
				cout << " MAC-адрес: " << io.address(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid8, event::address_t::IPV6) << " == " << io.target(eid8) << " || " << io.address(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid8) << endl;

				io.bufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
	}
	/**
	 * Соседская часть асинхронного движка ввода-вывода
	 */
	{
		// Создаём объект асинхронного движка ввода-вывода
		io_t io(&fmk, &log);
		/**
		 * IPv4 событие
		 */
		{
			cout << endl << " ******************** IPv4 PEER ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid1 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid1, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.iface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.address(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.address(eid1, event::address_t::IPV4);

				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.target(eid1) << " || " << io.address(eid1, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid1) << endl;

				io.bufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid2 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid2, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.address(eid2, event::address_t::MAC, mac)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid2) << endl;
				cout << " MAC-адрес: " << io.address(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid2, event::address_t::IPV4) << " == " << io.target(eid2) << " || " << io.address(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid2) << endl;

				io.bufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid3 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid3, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid3, 8080);
			// Устанавливаем IP-адрес события
			if(io.address(eid3, event::address_t::IPV4, ip)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid3) << endl;
				cout << " MAC-адрес: " << io.address(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid3, event::address_t::IPV4) << " == " << io.target(eid3) << " || " << io.address(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid3) << endl;

				io.bufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid4 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid4, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid4, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid4, event::address_t::NETWORK, ip + "/255.255.255.0")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid4) << endl;
				cout << " MAC-адрес: " << io.address(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid4, event::address_t::IPV4) << " == " << io.target(eid4) << " || " << io.address(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid4) << endl;

				io.bufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid5 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid5, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid5) << endl;
				cout << " MAC-адрес: " << io.address(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid5, event::address_t::UDS) << " == " << io.target(eid5) << " || " << io.address(eid5, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.port(eid5) << endl;

				io.bufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid6 = io.event(event::family_t::FILE, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid6, event::node_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FILE, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid6, event::address_t::FILE) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.port(eid6) << endl;

				io.bufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid7 = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid7, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid7, "192.168.7.11")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid7) << endl;
				cout << " MAC-адрес: " << io.address(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid7, event::address_t::IPV4) << " == " << io.target(eid7) << " || " << io.address(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid7) << endl;

				io.bufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid8 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid8, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid8, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid8) << endl;
				cout << " MAC-адрес: " << io.address(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid8, event::address_t::IPV4) << " == " << io.target(eid8) << " || " << io.address(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid8) << endl;

				io.bufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
		/**
		 * IPv6 событие
		 */
		{
			cout << endl << " ******************** IPv6 PEER ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid1 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid1, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.iface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.address(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.address(eid1, event::address_t::IPV6);

				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.target(eid1) << " || " << io.address(eid1, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid1) << endl;

				io.bufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid2 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid2, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.address(eid2, event::address_t::MAC, mac)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid2) << endl;
				cout << " MAC-адрес: " << io.address(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid2, event::address_t::IPV6) << " == " << io.target(eid2) << " || " << io.address(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid2) << endl;

				io.bufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid3 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid3, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid3, 8080);
			// Устанавливаем IP-адрес события
			if(io.address(eid3, event::address_t::IPV6, ip)){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid3) << endl;
				cout << " MAC-адрес: " << io.address(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid3, event::address_t::IPV6) << " == " << io.target(eid3) << " || " << io.address(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid3) << endl;

				io.bufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid4 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid4, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid4, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid4, event::address_t::NETWORK, ip + "/112")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid4) << endl;
				cout << " MAC-адрес: " << io.address(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid4, event::address_t::IPV6) << " == " << io.target(eid4) << " || " << io.address(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid4) << endl;

				io.bufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid5 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid5, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid5) << endl;
				cout << " MAC-адрес: " << io.address(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid5, event::address_t::UDS) << " == " << io.target(eid5) << " || " << io.address(eid5, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.port(eid5) << endl;

				io.bufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid6 = io.event(event::family_t::FILE, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid6, event::node_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FILE, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.address(eid6, event::address_t::FILE) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.port(eid6) << endl;

				io.bufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid7 = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid7, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid7, "fd44:135d:afb:0:14e3:5f29:f2cc:1746")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid7) << endl;
				cout << " MAC-адрес: " << io.address(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid7, event::address_t::IPV6) << " == " << io.target(eid7) << " || " << io.address(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid7) << endl;

				io.bufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid8 = io.event(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем тип ноды
			io.node(eid8, event::node_t::PEER);
			// Устанавливаем порт события
			io.port(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.target(eid8, "/tmp/awh.sock")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid8) << endl;
				cout << " MAC-адрес: " << io.address(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.address(eid8, event::address_t::IPV6) << " == " << io.target(eid8) << " || " << io.address(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.port(eid8) << endl;

				io.bufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.bufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.bufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.bufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
	}
	/**
	 * Межпроцессная часть асинхронного движка ввода-вывода
	 */
	{
		// Создаём объект асинхронного движка ввода-вывода
		io_t io(&fmk, &log);

		cout << endl << " ******************** IPC ******************** " << endl;
		cout << " ======================================== UDS " << endl;

		// Добавляем новое событие клиента TCP
		auto eids1 = io.events(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::NONE);

		cout << " Найдено событий: " << eids1[0] << " == " << eids1[1] << endl;

		// Устанавливаем тип ноды
		io.node(eids1[0], event::node_t::IPC);
		// Устанавливаем порт события
		io.port(eids1[0], 8080);

		// Устанавливаем адрес сервера назначения
		if(io.target(eids1[0], "192.168.7.11")){
			cout << " Успешно установлено адреса события! №1." << endl;
		} else cout << " Ошибка установки адреса события! №1." << endl;
		
		io.bufferSize(eids1[0], event::action_t::READ, 1024 * 64);
		io.bufferSize(eids1[0], event::action_t::WRITE, 1024 * 64);

		cout << " Размер буфера на чтение: " << io.bufferSize(eids1[0], event::action_t::READ) << " байт. " << endl;
		cout << " Размер буфера на запись: " << io.bufferSize(eids1[0], event::action_t::WRITE) << " байт. " << endl;


		cout << endl << " ******************** IPC ******************** " << endl;
		cout << " ======================================== IPC " << endl;

		// Добавляем новое событие клиента TCP
		auto eids2 = io.events(event::family_t::IPC, event::type_t::NONE, event::protocol_t::NONE);

		cout << " Найдено событий: " << eids2[0] << " == " << eids2[1] << endl;

		// Устанавливаем тип ноды
		io.node(eids2[0], event::node_t::IPC);
		// Устанавливаем порт события
		io.port(eids2[0], 8080);

		// Устанавливаем адрес сервера назначения
		if(io.target(eids2[0], "192.168.7.11")){
			cout << " Успешно установлено адреса события! №1." << endl;
		} else cout << " Ошибка установки адреса события! №1." << endl;
		
		io.bufferSize(eids2[0], event::action_t::READ, 1024 * 64);
		io.bufferSize(eids2[0], event::action_t::WRITE, 1024 * 64);

		cout << " Размер буфера на чтение: " << io.bufferSize(eids2[0], event::action_t::READ) << " байт. " << endl;
		cout << " Размер буфера на запись: " << io.bufferSize(eids2[0], event::action_t::WRITE) << " байт. " << endl;
	}
	/**
	 * Таймерная часть асинхронного движка ввода-вывода
	 */
	{
		// Создаём объект асинхронного движка ввода-вывода
		io_t io(&fmk, &log);

		cout << endl << " ******************** TIMER ******************** " << endl;
		cout << " ======================================== TIMER " << endl;

		// Добавляем новое событие клиента TCP
		event::id_t eid = io.event(event::family_t::INTERVAL, event::type_t::NONE, event::protocol_t::NONE);
		// Устанавливаем тип ноды
		io.node(eid, event::node_t::TIMER);

		cout << " Таймерное событие ID: " << eid << endl;
	}

	/**
	 При коннекте использовать проверку для локальных интерфейсов 127.0.0.1 и ::1

	 	bool is_loopback(const std::string& ip_str) {
			struct in_addr addr;
			if (inet_pton(AF_INET, ip_str.c_str(), &addr) != 1)
				return false; // некорректный IPv4

			uint32_t ip = ntohl(addr.s_addr);
			return (ip >> 24) == 127; // первые 8 бит == 127
		}

		bool is_loopback_v6(const std::string& ip_str) {
			struct in6_addr addr;
			if (inet_pton(AF_INET6, ip_str.c_str(), &addr) != 1)
				return false;

			const uint8_t* bytes = addr.s6_addr;
			for (int i = 0; i < 15; ++i)
				if (bytes[i] != 0) return false;
			return bytes[15] == 1; // только последний байт == 1
		}
	 */

	/*
	// Создаём объект асинхронного движка ввода-вывода
	io_t io(&fmk, &log);
	// Добавляем новое событие клиента TCP
	event::id_t eid = io.event(event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
	// Устанавливаем тип ноды
	io.node(eid, event::node_t::SERVER);
	// Устанавливаем порт события
	io.port(eid, 8080);
	// Устанавливаем адрес события (en0 -> ea:ab:fd:74:1d:0d -> 10.9.5.161)
	// if(io.address(eid, event::address_t::NETWORK, "10.9.5.0/255.255.255.0")){
	// if(io.address(eid, event::address_t::NETWORK, "fe80::105d:12e9:40c7:a76/76")){
	// if(io.address(eid, event::address_t::IPV4, "192.168.7.231")){
	// if(io.address(eid, event::address_t::NETWORK, "192.168.7.231/255.255.255.0")){
	// if(io.address(eid, event::address_t::NETWORK, "FE80::96:81FE:CCE3:0/112")){
	// if(io.address(eid, event::address_t::MAC, "EA:AB:FD:74:1D:0D")){
	// if(io.address(eid, event::address_t::UDS, "/tmp/awh.sock")){
	if(io.iface(eid, "EN0")){

		cout << " !!!!! " << io.address(eid, event::address_t::MAC) << ":" << io.port(eid) << " !!!!! " << io.target(eid) << " == " << io.iface(eid) << endl;

		// cout << " !!!!! " << io.address(eid, event::address_t::UDS) << " !!!!! " << io.host(eid) << endl;

		io.bufferSize(eid, event::action_t::READ, 1024 * 64);
		io.bufferSize(eid, event::action_t::WRITE, 1024 * 64);

		cout << " Размер буфера на чтение: " << io.bufferSize(eid, event::action_t::READ) << " байт. " << endl;
		cout << " Размер буфера на запись: " << io.bufferSize(eid, event::action_t::WRITE) << " байт. " << endl;

	// Если адрес не установлен
	} else {

		cout << " Ошибка установки адреса события! " << endl;

	}
	*/
	// Выводим результат
	return 0;
}
