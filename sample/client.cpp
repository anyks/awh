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

#include <iostream>
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
	// log.level(log_t::level_t::NONE);
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
			event::id_t eid1 = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid2 = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid3 = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid4 = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid5 = io.event(event::node_t::CLIENT, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.address(eid6, event::address_t::FS) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV4) << endl;
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
			event::id_t eid7 = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid8 = io.event(event::node_t::CLIENT, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid1 = io.event(event::node_t::CLIENT, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid2 = io.event(event::node_t::CLIENT, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid3 = io.event(event::node_t::CLIENT, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid4 = io.event(event::node_t::CLIENT, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid5 = io.event(event::node_t::CLIENT, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.address(eid6, event::address_t::FS) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV6) << endl;
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
			event::id_t eid7 = io.event(event::node_t::CLIENT, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid8 = io.event(event::node_t::CLIENT, event::family_t::UDS, event::type_t::STREAM);
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
			event::id_t eid1 = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid2 = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid3 = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
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
			event::id_t eid4 = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid5 = io.event(event::node_t::SERVER, event::family_t::UDS, event::type_t::STREAM);
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
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.address(eid6, event::address_t::FS) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV4) << endl;
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
			event::id_t eid7 = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid8 = io.event(event::node_t::SERVER, event::family_t::UDS, event::type_t::STREAM);
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
			event::id_t eid1 = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid2 = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid3 = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid4 = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid5 = io.event(event::node_t::SERVER, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.address(eid6, event::address_t::FS) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV6) << endl;
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
			event::id_t eid7 = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid8 = io.event(event::node_t::SERVER, event::family_t::UDS, event::type_t::STREAM);
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
			event::id_t eid1 = io.event(event::node_t::PEER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid2 = io.event(event::node_t::PEER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid3 = io.event(event::node_t::PEER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid4 = io.event(event::node_t::PEER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid5 = io.event(event::node_t::PEER, event::family_t::UDS, event::type_t::STREAM);
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
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.address(eid6, event::address_t::FS) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV4) << endl;
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
			event::id_t eid7 = io.event(event::node_t::PEER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid8 = io.event(event::node_t::PEER, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid1 = io.event(event::node_t::PEER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid2 = io.event(event::node_t::PEER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid3 = io.event(event::node_t::PEER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid4 = io.event(event::node_t::PEER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid5 = io.event(event::node_t::PEER, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.port(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.address(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Выводим основные параметры события
				cout << " Сетевой интерфейс: " << io.iface(eid6) << endl;
				cout << " MAC-адрес: " << io.address(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.address(eid6, event::address_t::FS) << " == " << io.target(eid6) << " || " << io.address(eid6, event::address_t::IPV6) << endl;
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
			event::id_t eid7 = io.event(event::node_t::PEER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
			event::id_t eid8 = io.event(event::node_t::PEER, event::family_t::UDS, event::type_t::STREAM);
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
		auto eids1 = io.events(event::family_t::UDS);

		cout << " Найдено событий: " << eids1[0] << " == " << eids1[1] << endl;

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
		auto eids2 = io.events(event::family_t::PIPE);

		cout << " Найдено событий: " << eids2[0] << " == " << eids2[1] << endl;

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
	/*
	{
		// Создаём объект асинхронного движка ввода-вывода
		io_t io(&fmk, &log);

		cout << endl << " ******************** TIMER ******************** " << endl;
		cout << " ======================================== TIMER " << endl;

		// Добавляем новое событие клиента интервала
		event::id_t eid = io.event(event::node_t::TIMER, event::family_t::INTERVAL, event::type_t::NONE, event::protocol_t::NONE);

		cout << " Таймерное событие ID: " << eid << endl;

		io.timeout(eid, event::action_t::NONE, 12000);

		if(io.initialize()){
			io.commit(eid);

			io.on(eid, [](const event::id_t eid, const event::status_t status) noexcept -> void {
				if(status == event::status_t::SUCCESS)
					cout << " Таймер сработал! " << eid << endl;
			});

			while(io.poll());
		}
	}
	*/
	// Создаём объект асинхронного движка ввода-вывода
	io_t io(&fmk, &log);
	// Добавляем новое событие клиента TCP
	event::id_t eid = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::SCTP);
	// Устанавливаем порт события
	io.port(eid, 2222);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устананавливаем опции события
		if(io.options(eid, event::options::NOSIGILL | event::options::NOSIGPIPE | event::options::REUSEADDR | event::options::NOIOBLOCK | event::options::CLOSEONEXEC | event::options::TCPNODELAY | event::options::KEEPALIVE))
			// Выводим сообщение об успешной установке опций события
			cout << " Успешно установлены опции события!" << endl;
		// Выводим сообщение об ошибке установки опций события
		else cout << " Ошибка установки опций события!" << endl;
		// Выполняем подписку на SCTP события
		io.sctpEventsSubscribe(eid, {
			net::sctp::event_type_t::ASSOC_CHANGE,
			net::sctp::event_type_t::SHUTDOWN_EVENT,
			net::sctp::event_type_t::SEND_FAILED_EVENT,
			net::sctp::event_type_t::REMOTE_ERROR
		});
		// Устанавливаем IP-адрес события
		if(io.address(eid, event::address_t::IPV4, "0.0.0.0")){
			// Устанавливаем адрес сервера назначения
			if(io.target(eid, "127.0.0.1")){
				// Устанавливаем функцию обратного вызова на событие таймера
				io.on(eid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Выводим сообщение о принятии события
							log.print("Событие принято: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Выводим сообщение об уничтожении события
							log.print("Событие подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Выводим сообщение об инициализации события
							log.print("Событие инициализировано: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::LAUNCHED):
							// Выводим сообщение о запуске события
							log.print("Событие запущено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Выводим сообщение о паузе события
							log.print("Событие на паузе: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Выводим сообщение о возобновлении события
							log.print("Событие возобновлено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Выводим сообщение о успешном выполнении события
							log.print("Событие успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Выводим сообщение о неудачном выполнении события
							log.print("Событие выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Выводим сообщение о выполнении события в ожидании
							log.print("Событие в ожидании: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Выводим сообщение о подключении события
							log.print("Событие подключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Выводим сообщение об отмене события
							log.print("Событие отменено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Выводим сообщение о переподключении события
							log.print("Событие переподключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус прослушивания события
						case static_cast <uint8_t> (event::status_t::LISTENING):
							// Выводим сообщение о прослушивании события
							log.print("Событие прослушивается: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на запись в событие
				io.on(eid, static_cast <event::callback::write_t> ([&log](const event::id_t eid, const size_t size) noexcept -> void {
					// Выводим сообщение о переподключении события
					log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				io.on(eid, static_cast <net::sctp::callback::info_t> ([&log](const event::id_t eid, const net::sctp::minfo_t & minfo) noexcept -> void {
					// Выводим информацию о сообщении SCTP-сокета
					log.print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				io.on(eid, [&log](const event::id_t eid, net::sctp_event_t event) noexcept -> void {
					// Выводим сообщение с идентификатором событий SCTP
					cout << " SCTP EVENT ID: " << event->id << endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (net::sctp::event_type_t::DATA_IO):
							// Выводим сообщение о событии DATA IO
							cout << "  - DATA IO EVENT " << endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (net::sctp::event_type_t::REMOTE_ERROR):
							// Выводим сообщение о событии REMOTE ERROR
							cout << "  - REMOTE ERROR EVENT " << endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (net::sctp::event_type_t::ASSOC_CHANGE):
							// Выводим сообщение о событии ASSOC CHANGE
							cout << "  - ASSOC CHANGE EVENT " << endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Выводим сообщение о событии SHUTDOWN EVENT
							cout << "  - SHUTDOWN EVENT " << endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Выводим сообщение о событии SENDER DRY EVENT
							cout << "  - SENDER DRY EVENT " << endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Выводим сообщение о событии PEER ADDR CHANGE
							cout << "  - PEER ADDR CHANGE EVENT " << endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Выводим сообщение о событии SEND FAILED EVENT
							cout << "  - SEND FAILED EVENT " << endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Выводим сообщение о событии STREAM RESET EVENT
							cout << "  - STREAM RESET EVENT " << endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Выводим сообщение о событии AUTHENTICATION EVENT
							cout << "  - AUTHENTICATION EVENT " << endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Выводим сообщение о событии ADAPTATION INDICATION
							cout << "  - ADAPTATION INDICATION EVENT " << endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Выводим сообщение о событии PARTIAL DELIVERY EVENT
							cout << "  - PARTIAL DELIVERY EVENT " << endl;
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на чтение из события
				io.on(eid, [&io, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Получаем информацию о сообщении SCTP-сокета
					const net::sctp::minfo_t & minfo = io.sctpMessageInfo(eid);
					// Выводим информацию о сообщении SCTP-сокета
					cout << " SCTP Message Info2: " << endl;
					cout << "  - Stream Number: " << minfo.num << endl;
					cout << "  - Payload Protocol ID: " << (u_short) minfo.ppid << endl;
					cout << "  - Context: " << minfo.ctx << endl;
					cout << "  - Time to Live: " << minfo.ttl << endl;
					cout << "  - Flags: " << minfo.flags.size() << endl;
					// Получаем статус SCTP-сокета
					const net::sctp::status_t & status = io.sctpStatus(eid);
					// Выводим статус SCTP-сокета
					cout << " SCTP Status: " << endl;
					cout << "  - ID: " << status.id << endl;
					cout << "  - State: " << (u_short) status.state << endl;
					cout << "  - Outbound Streams: " << status.ostreams << endl;
					cout << "  - Inbound Streams: " << status.istreams << endl;
					cout << "  - Fragmentation Point: " << status.fragpoint << endl;
					cout << "  - Rate Window: " << status.ratewind << endl;
					cout << "  - Unpack Data: " << status.unackdata << endl;
					cout << "  - Pending Data: " << status.penddata << endl;
					// Текст входящего сообщения
					const string message(reinterpret_cast <const char *> (data), size);
					// Выводим сообщение о переподключении события
					log.print("Прочитано: ID=%u, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, size, message.c_str());
				});
				// Устанавливаем функцию обратного вызова на ошибку события
				io.on(eid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Выводим сообщение об ошибке неизвестного события
							log.print("Неизвестная ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Выводим сообщение об ошибке недопустимой операции
							log.print("Недопустимая операция события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Выводим сообщение об ошибке доступа запрещёния
							log.print("Доступ к событию запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Выводим сообщение об ошибке уже существующего объекта
							log.print("Объект события уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Выводим сообщение об ошибке доступа к сокету
							log.print("Ошибка доступа к сокету события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Выводим сообщение об ошибке некорректного адреса
							log.print("Некорректный адрес события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Выводим сообщение об ошибке подключения
							log.print("Ошибка подключения события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Выводим сообщение об ошибке недостаточно ресурсов
							log.print("Недостаточно ресурсов для события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Выводим сообщение об ошибке события
							log.print("Ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Выводим сообщение об ошибке события
							log.print("Объект события не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на удачное подключение к серверу
				io.on(eid, static_cast <event::callback::connect_t> ([&io, &log](const event::id_t eid, const bool ok) noexcept -> void {
					// Выводим сообщение о принятии события
					log.print("Событие подключения: ID=%u, результат: %s", log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
					// Если подключение успешно
					if(ok){
						// Текст исходящего сообщения
						const string message("Hello from async client!");
						// Отправляем данные обратно клиенту
						if(io.send(eid, message.c_str(), message.size()))
							// Если данные успешно отправлены
							log.print("Отправлено: ID=%u, %zu байт", log_t::flag_t::INFO, eid, message.size());
						// Если данные не отправлены
						else log.print("Ошибка отправки: ID=%u", log_t::flag_t::CRITICAL, eid);
					}
				}));
				// Устанавливаем функцию обратного вызова на общее событие
				io.on(eid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Выводим сообщение о чтении события
							log.print("Событие на чтение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Выводим сообщение о записи события
							log.print("Событие на запись: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Выводим сообщение о подключении события
							log.print("Событие на подключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Выводим сообщение об отключении события
							log.print("Событие на отключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Выводим сообщение о переподключении события
							log.print("Событие на переподключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Выводим сообщение о закрытии события
							log.print("Событие на закрытие подключения: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение об изменении события
							log.print("Событие на изменение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение об удалении события
							log.print("Событие на удаление: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Выводим сообщение о переименовании события
							log.print("Событие на переименование: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Выводим сообщение об изменении атрибутов события
							log.print("Событие на изменение атрибутов: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Выводим сообщение об отзыве доступа события
							log.print("Событие на отзыв доступа: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Выводим сообщение о изменении счётчика жёстких ссылок события
							log.print("Событие на изменение счётчика жёстких ссылок: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем таймаут события на чтение
				io.timeout(eid, event::action_t::READ, 10000);
				// Устанавливаем таймаут события на запись
				io.timeout(eid, event::action_t::WRITE, 7000);
				// Устанавливаем таймаут события на подключение
				io.timeout(eid, event::action_t::CONNECT, 5000);
				// Выполняем фиксацию настроек события сервера
				if(io.commit(eid)){
					// Если подключение к серверу прошло успешно
					// if(io.connect(eid, true)){
					if(io.multiconnect(eid, {eid}, true)){
						// Выполняем запуск события
						if(io.launch(eid)){
							// Выводим сообщение об успешном запуске события
							cout << " Событие успешно запущено!" << endl;
							/**
							 * Запускаем опрос событий
							 */
							while(io.poll());
						// Выводим сообщение об ошибке запуска события
						} else cout << " Ошибка запуска события!" << endl;
					}
				}
			// Если адрес назначения не установлен
			} else cout << " Ошибка установки адреса сервера!" << endl;
		// Если адрес не установлен
		} else cout << " Ошибка установки адреса клиента!" << endl;
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
	event::id_t eid = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
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
