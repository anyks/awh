/**
 * @file client.cpp
 * @date 2025-10-25
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Пример клиента SCTP в потоковом режиме — демонстрация установки ассоциации,
 *        настройки параметров инициализации и обмена непрерывным потоком данных
 *
 * @copyright Copyright © 2025
 *
 */

#include <iostream>
#include <cinttypes>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

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
		engine::io_t io(&fmk, &log);
		/**
		 * IPv4 событие
		 */
		{
			cout << endl << " ******************** IPv4 CLIENT ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid1 = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.setIface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.getAddress(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.getAddress(eid1, event::address_t::IPV4);

				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.getTarget(eid1) << " || " << io.getAddress(eid1, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid1) << endl;

				io.setBufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid2 = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.setAddress(eid2, event::address_t::MAC, mac)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid2) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid2, event::address_t::IPV4) << " == " << io.getTarget(eid2) << " || " << io.getAddress(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid2) << endl;

				io.setBufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid3 = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid3, 8080);
			// Устанавливаем IP-адрес события
			if(io.setAddress(eid3, event::address_t::IPV4, ip)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid3) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid3, event::address_t::IPV4) << " == " << io.getTarget(eid3) << " || " << io.getAddress(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid3) << endl;

				io.setBufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid4 = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid4, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid4, event::address_t::NETWORK, ip + "/255.255.255.0")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid4) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid4, event::address_t::IPV4) << " == " << io.getTarget(eid4) << " || " << io.getAddress(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid4) << endl;

				io.setBufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid5 = io.event(event::node_t::CLIENT, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid5) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.getAddress(eid5, event::address_t::UDS) << " == " << io.getTarget(eid5) << " || " << io.getAddress(eid5, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.getTargetPort(eid5) << endl;

				io.setBufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие файла
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.setTargetPort(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid6) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.getAddress(eid6, event::address_t::FS) << " == " << io.getTarget(eid6) << " || " << io.getAddress(eid6, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.getTargetPort(eid6) << endl;

				io.setBufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv4 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid7 = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid7, ip)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid7) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid7, event::address_t::IPV4) << " == " << io.getTarget(eid7) << " || " << io.getAddress(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid7) << endl;

				io.setBufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid8 = io.event(event::node_t::CLIENT, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid8, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid8) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid8, event::address_t::IPV4) << " == " << io.getTarget(eid8) << " || " << io.getAddress(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid8) << endl;

				io.setBufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
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
			io.setTargetPort(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.setIface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.getAddress(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.getAddress(eid1, event::address_t::IPV6);

				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.getTarget(eid1) << " || " << io.getAddress(eid1, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid1) << endl;

				io.setBufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid2 = io.event(event::node_t::CLIENT, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.setAddress(eid2, event::address_t::MAC, mac)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid2) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid2, event::address_t::IPV6) << " == " << io.getTarget(eid2) << " || " << io.getAddress(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid2) << endl;

				io.setBufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid3 = io.event(event::node_t::CLIENT, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid3, 8080);
			// Устанавливаем IP-адрес события
			if(io.setAddress(eid3, event::address_t::IPV6, ip)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid3) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid3, event::address_t::IPV6) << " == " << io.getTarget(eid3) << " || " << io.getAddress(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid3) << endl;

				io.setBufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid4 = io.event(event::node_t::CLIENT, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid4, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid4, event::address_t::NETWORK, ip + "/112")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid4) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid4, event::address_t::IPV6) << " == " << io.getTarget(eid4) << " || " << io.getAddress(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid4) << endl;

				io.setBufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid5 = io.event(event::node_t::CLIENT, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid5) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.getAddress(eid5, event::address_t::UDS) << " == " << io.getTarget(eid5) << " || " << io.getAddress(eid5, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.getTargetPort(eid5) << endl;

				io.setBufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие файла
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.setTargetPort(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid6) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.getAddress(eid6, event::address_t::FS) << " == " << io.getTarget(eid6) << " || " << io.getAddress(eid6, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.getTargetPort(eid6) << endl;

				io.setBufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv6 " << endl;

			// Добавляем новое событие клиента TCP
			event::id_t eid7 = io.event(event::node_t::CLIENT, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setTargetPort(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid7, ip)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid7) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid7, event::address_t::IPV6) << " == " << io.getTarget(eid7) << " || " << io.getAddress(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid7) << endl;

				io.setBufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие клиента
			event::id_t eid8 = io.event(event::node_t::CLIENT, event::family_t::UDS, event::type_t::STREAM);
			// Устанавливаем порт события
			io.setTargetPort(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid8, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid8) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid8, event::address_t::IPV6) << " == " << io.getTarget(eid8) << " || " << io.getAddress(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getTargetPort(eid8) << endl;

				io.setBufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
	}
	/**
	 * Серверная часть асинхронного движка ввода-вывода
	 */
	{
		// Создаём объект асинхронного движка ввода-вывода
		engine::io_t io(&fmk, &log);
		/**
		 * IPv4 событие
		 */
		{
			cout << endl << " ******************** IPv4 SERVER ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие сервера TCP
			event::id_t eid1 = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.setIface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.getAddress(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.getAddress(eid1, event::address_t::IPV4);

				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.getTarget(eid1) << " || " << io.getAddress(eid1, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid1) << endl;

				io.setBufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие сервера TCP
			event::id_t eid2 = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.setAddress(eid2, event::address_t::MAC, mac)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid2) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid2, event::address_t::IPV4) << " == " << io.getTarget(eid2) << " || " << io.getAddress(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid2) << endl;

				io.setBufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv4 " << endl;

			// Добавляем новое событие сервера UDP
			event::id_t eid3 = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
			// Устанавливаем порт события
			io.setSourcePort(eid3, 8080);
			// Устанавливаем IP-адрес события
			if(io.setAddress(eid3, event::address_t::IPV4, ip)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid3) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid3, event::address_t::IPV4) << " == " << io.getTarget(eid3) << " || " << io.getAddress(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid3) << endl;

				io.setBufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv4 " << endl;

			// Добавляем новое событие сервера TCP
			event::id_t eid4 = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid4, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid4, event::address_t::NETWORK, ip + "/255.255.255.0")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid4) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid4, event::address_t::IPV4) << " == " << io.getTarget(eid4) << " || " << io.getAddress(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid4) << endl;

				io.setBufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие сервера
			event::id_t eid5 = io.event(event::node_t::SERVER, event::family_t::UDS, event::type_t::STREAM);
			// Устанавливаем порт события
			io.setSourcePort(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid5) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.getAddress(eid5, event::address_t::UDS) << " == " << io.getTarget(eid5) << " || " << io.getAddress(eid5, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.getSourcePort(eid5) << endl;

				io.setBufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие файла
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.setSourcePort(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid6) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.getAddress(eid6, event::address_t::FS) << " == " << io.getTarget(eid6) << " || " << io.getAddress(eid6, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.getSourcePort(eid6) << endl;

				io.setBufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv4 " << endl;

			// Добавляем новое событие сервера TCP
			event::id_t eid7 = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid7, ip)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid7) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid7, event::address_t::IPV4) << " == " << io.getTarget(eid7) << " || " << io.getAddress(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid7) << endl;

				io.setBufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие сервера
			event::id_t eid8 = io.event(event::node_t::SERVER, event::family_t::UDS, event::type_t::STREAM);
			// Устанавливаем порт события
			io.setSourcePort(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid8, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid8) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid8, event::address_t::IPV4) << " == " << io.getTarget(eid8) << " || " << io.getAddress(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid8) << endl;

				io.setBufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
		/**
		 * IPv6 событие
		 */
		{
			cout << endl << " ******************** IPv6 SERVER ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие сервера TCP
			event::id_t eid1 = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.setIface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.getAddress(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.getAddress(eid1, event::address_t::IPV6);

				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.getTarget(eid1) << " || " << io.getAddress(eid1, event::address_t::UDS) << " || " << io.getAddress(eid1, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.getSourcePort(eid1) << endl;

				io.setBufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие сервера TCP
			event::id_t eid2 = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.setAddress(eid2, event::address_t::MAC, mac)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid2) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid2, event::address_t::IPV6) << " == " << io.getTarget(eid2) << " || " << io.getAddress(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid2) << endl;

				io.setBufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv6 " << endl;

			// Добавляем новое событие сервера TCP
			event::id_t eid3 = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid3, 8080);

			cout << " Устанавливаем IP-адрес события: " << ip << endl;

			// Устанавливаем IP-адрес события
			if(io.setAddress(eid3, event::address_t::IPV6, ip)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid3) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid3, event::address_t::IPV6) << " == " << io.getTarget(eid3) << " || " << io.getAddress(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid3) << endl;

				io.setBufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv6 " << endl;

			// Добавляем новое событие сервера TCP
			event::id_t eid4 = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid4, 8080);

			cout << " Устанавливаем IP-сеть события: " << (ip + "/112") << endl;

			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid4, event::address_t::NETWORK, ip + "/112")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid4) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid4, event::address_t::IPV6) << " == " << io.getTarget(eid4) << " || " << io.getAddress(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid4) << endl;

				io.setBufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие сервера TCP
			event::id_t eid5 = io.event(event::node_t::SERVER, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid5) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.getAddress(eid5, event::address_t::UDS) << " == " << io.getTarget(eid5) << " || " << io.getAddress(eid5, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.getSourcePort(eid5) << endl;

				io.setBufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие файла
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.setSourcePort(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid6) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.getAddress(eid6, event::address_t::FS) << " == " << io.getTarget(eid6) << " || " << io.getAddress(eid6, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.getSourcePort(eid6) << endl;

				io.setBufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv6 " << endl;

			// Добавляем новое событие сервера TCP
			event::id_t eid7 = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid7, ip)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid7) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid7, event::address_t::IPV6) << " == " << io.getTarget(eid7) << " || " << io.getAddress(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid7) << endl;

				io.setBufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие сервера
			event::id_t eid8 = io.event(event::node_t::SERVER, event::family_t::UDS, event::type_t::STREAM);
			// Устанавливаем порт события
			io.setSourcePort(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid8, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid8) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid8, event::address_t::IPV6) << " == " << io.getTarget(eid8) << " || " << io.getAddress(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid8) << endl;

				io.setBufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
	}
	/**
	 * Соседская часть асинхронного движка ввода-вывода
	 */
	{
		// Создаём объект асинхронного движка ввода-вывода
		engine::io_t io(&fmk, &log);
		/**
		 * IPv4 событие
		 */
		{
			cout << endl << " ******************** IPv4 PEER ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid1 = io.event(event::node_t::PEER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.setIface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.getAddress(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.getAddress(eid1, event::address_t::IPV4);

				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.getTarget(eid1) << " || " << io.getAddress(eid1, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid1) << endl;

				io.setBufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid2 = io.event(event::node_t::PEER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.setAddress(eid2, event::address_t::MAC, mac)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid2) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid2, event::address_t::IPV4) << " == " << io.getTarget(eid2) << " || " << io.getAddress(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid2) << endl;

				io.setBufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv4 " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid3 = io.event(event::node_t::PEER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid3, 8080);
			// Устанавливаем IP-адрес события
			if(io.setAddress(eid3, event::address_t::IPV4, ip)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid3) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid3, event::address_t::IPV4) << " == " << io.getTarget(eid3) << " || " << io.getAddress(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid3) << endl;

				io.setBufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv4 " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid4 = io.event(event::node_t::PEER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid4, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid4, event::address_t::NETWORK, ip + "/255.255.255.0")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid4) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid4, event::address_t::IPV4) << " == " << io.getTarget(eid4) << " || " << io.getAddress(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid4) << endl;

				io.setBufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие пира
			event::id_t eid5 = io.event(event::node_t::PEER, event::family_t::UDS, event::type_t::STREAM);
			// Устанавливаем порт события
			io.setSourcePort(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid5) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.getAddress(eid5, event::address_t::UDS) << " == " << io.getTarget(eid5) << " || " << io.getAddress(eid5, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.getSourcePort(eid5) << endl;

				io.setBufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие файла
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.setSourcePort(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid6) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.getAddress(eid6, event::address_t::FS) << " == " << io.getTarget(eid6) << " || " << io.getAddress(eid6, event::address_t::IPV4) << endl;
				cout << " Порт: " << io.getSourcePort(eid6) << endl;

				io.setBufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv4 " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid7 = io.event(event::node_t::PEER, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid7, "192.168.7.11")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid7) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid7, event::address_t::IPV4) << " == " << io.getTarget(eid7) << " || " << io.getAddress(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid7) << endl;

				io.setBufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid8 = io.event(event::node_t::PEER, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid8, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid8) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid8, event::address_t::IPV4) << " == " << io.getTarget(eid8) << " || " << io.getAddress(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid8) << endl;

				io.setBufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
		/**
		 * IPv6 событие
		 */
		{
			cout << endl << " ******************** IPv6 PEER ******************** " << endl;
			cout << " ======================================== IFACE " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid1 = io.event(event::node_t::PEER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid1, 8080);
			// MAC-адрес сетевого интерфейса
			string mac = "";
			// IP-адрес сетевого интерфейса
			string ip = "";
			// Устанавливаем сетевой интерфейс события
			if(io.setIface(eid1, "EN0")){
				// Извлекаем MAC-адрес сетевого интерфейса
				mac = io.getAddress(eid1, event::address_t::MAC);
				// Извлекаем IP-адрес сетевого интерфейса
				ip = io.getAddress(eid1, event::address_t::IPV6);

				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid1) << endl;
				cout << " MAC-адрес: " << mac << endl;
				cout << " IP-адрес: " << ip << " == " << io.getTarget(eid1) << " || " << io.getAddress(eid1, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid1) << endl;

				io.setBufferSize(eid1, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid1, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid1, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid1, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №1." << endl;

			cout << endl;

			cout << " ======================================== MAC " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid2 = io.event(event::node_t::PEER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid2, 8080);
			// Устанавливаем MAC-адрес события
			if(io.setAddress(eid2, event::address_t::MAC, mac)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid2) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid2, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid2, event::address_t::IPV6) << " == " << io.getTarget(eid2) << " || " << io.getAddress(eid2, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid2) << endl;

				io.setBufferSize(eid2, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid2, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid2, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid2, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №2." << endl;

			cout << endl;

			cout << " ======================================== IPv6 " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid3 = io.event(event::node_t::PEER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid3, 8080);
			// Устанавливаем IP-адрес события
			if(io.setAddress(eid3, event::address_t::IPV6, ip)){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid3) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid3, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid3, event::address_t::IPV6) << " == " << io.getTarget(eid3) << " || " << io.getAddress(eid3, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid3) << endl;

				io.setBufferSize(eid3, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid3, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid3, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid3, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №3." << endl;

			cout << endl;

			cout << " ======================================== NETWORK IPv6 " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid4 = io.event(event::node_t::PEER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid4, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid4, event::address_t::NETWORK, ip + "/112")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid4) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid4, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid4, event::address_t::IPV6) << " == " << io.getTarget(eid4) << " || " << io.getAddress(eid4, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid4) << endl;

				io.setBufferSize(eid4, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid4, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid4, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid4, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== UDS " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid5 = io.event(event::node_t::PEER, event::family_t::UDS, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid5, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid5, event::address_t::UDS, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid5) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid5, event::address_t::MAC) << endl;
				cout << " UDS-адрес: " << io.getAddress(eid5, event::address_t::UDS) << " == " << io.getTarget(eid5) << " || " << io.getAddress(eid5, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.getSourcePort(eid5) << endl;

				io.setBufferSize(eid5, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid5, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid5, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid5, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №4." << endl;

			cout << endl;

			cout << " ======================================== FILE " << endl;

			// Добавляем новое событие файла
			event::id_t eid6 = io.event(event::node_t::FILE, event::family_t::FSYS);
			// Устанавливаем порт события
			io.setSourcePort(eid6, 8080);
			// Устанавливаем сетевой адрес события
			if(io.setAddress(eid6, event::address_t::FS, "/tmp/awh.txt")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid6) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid6, event::address_t::MAC) << endl;
				cout << " FILE-адрес: " << io.getAddress(eid6, event::address_t::FS) << " == " << io.getTarget(eid6) << " || " << io.getAddress(eid6, event::address_t::IPV6) << endl;
				cout << " Порт: " << io.getSourcePort(eid6) << endl;

				io.setBufferSize(eid6, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid6, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid6, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid6, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №5." << endl;

			cout << endl;

			cout << " ======================================== TARGET IPv6 " << endl;

			// Добавляем новое событие пира TCP
			event::id_t eid7 = io.event(event::node_t::PEER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
			// Устанавливаем порт события
			io.setSourcePort(eid7, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid7, "fd44:135d:afb:0:14e3:5f29:f2cc:1746")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid7) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid7, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid7, event::address_t::IPV6) << " == " << io.getTarget(eid7) << " || " << io.getAddress(eid7, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid7) << endl;

				io.setBufferSize(eid7, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid7, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid7, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid7, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №6." << endl;

			cout << endl;

			cout << " ======================================== TARGET UDS " << endl;

			// Добавляем новое событие пира
			event::id_t eid8 = io.event(event::node_t::PEER, event::family_t::UDS, event::type_t::STREAM);
			// Устанавливаем порт события
			io.setSourcePort(eid8, 8080);
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid8, "/tmp/awh.sock")){
				// Записываем в лог основные параметры события
				cout << " Сетевой интерфейс: " << io.getIface(eid8) << endl;
				cout << " MAC-адрес: " << io.getAddress(eid8, event::address_t::MAC) << endl;
				cout << " IP-адрес: " << io.getAddress(eid8, event::address_t::IPV6) << " == " << io.getTarget(eid8) << " || " << io.getAddress(eid8, event::address_t::UDS) << endl;
				cout << " Порт: " << io.getSourcePort(eid8) << endl;

				io.setBufferSize(eid8, event::action_t::READ, 1024 * 64);
				io.setBufferSize(eid8, event::action_t::WRITE, 1024 * 64);

				cout << " Размер буфера на чтение: " << io.getBufferSize(eid8, event::action_t::READ) << " байт. " << endl;
				cout << " Размер буфера на запись: " << io.getBufferSize(eid8, event::action_t::WRITE) << " байт. " << endl;
			// Если адрес не установлен
			} else cout << " Ошибка установки адреса события! №7." << endl;
		}
	}
	/**
	 * Межпроцессная часть асинхронного движка ввода-вывода
	 */
	{
		// Создаём объект асинхронного движка ввода-вывода
		engine::io_t io(&fmk, &log);

		cout << endl << " ******************** IPC ******************** " << endl;
		cout << " ======================================== UDS " << endl;

		// Добавляем новую пару событий UDS
		auto eids1 = io.events(event::family_t::UDS);

		cout << " Найдено событий: " << eids1[0] << " == " << eids1[1] << endl;

		// Устанавливаем порт события
		io.setTargetPort(eids1[0], 8080);

		// Устанавливаем адрес сервера назначения
		if(io.setTarget(eids1[0], "192.168.7.11")){
			cout << " Успешно установлено адреса события! №1." << endl;
		} else cout << " Ошибка установки адреса события! №1." << endl;
		
		io.setBufferSize(eids1[0], event::action_t::READ, 1024 * 64);
		io.setBufferSize(eids1[0], event::action_t::WRITE, 1024 * 64);

		cout << " Размер буфера на чтение: " << io.getBufferSize(eids1[0], event::action_t::READ) << " байт. " << endl;
		cout << " Размер буфера на запись: " << io.getBufferSize(eids1[0], event::action_t::WRITE) << " байт. " << endl;


		cout << endl << " ******************** IPC ******************** " << endl;
		cout << " ======================================== IPC " << endl;

		// Добавляем новую пару событий PIPE
		auto eids2 = io.events(event::family_t::PIPE);

		cout << " Найдено событий: " << eids2[0] << " == " << eids2[1] << endl;

		// Устанавливаем порт события
		io.setSourcePort(eids2[0], 8080);

		// Устанавливаем адрес сервера назначения
		if(io.setTarget(eids2[0], "192.168.7.11")){
			cout << " Успешно установлено адреса события! №1." << endl;
		} else cout << " Ошибка установки адреса события! №1." << endl;
		
		io.setBufferSize(eids2[0], event::action_t::READ, 1024 * 64);
		io.setBufferSize(eids2[0], event::action_t::WRITE, 1024 * 64);

		cout << " Размер буфера на чтение: " << io.getBufferSize(eids2[0], event::action_t::READ) << " байт. " << endl;
		cout << " Размер буфера на запись: " << io.getBufferSize(eids2[0], event::action_t::WRITE) << " байт. " << endl;
	}
	/**
	 * Таймерная часть асинхронного движка ввода-вывода
	 */
	/*
	{
		// Создаём объект асинхронного движка ввода-вывода
		engine::io_t io(&fmk, &log);

		cout << endl << " ******************** TIMER ******************** " << endl;
		cout << " ======================================== TIMER " << endl;

		// Добавляем новое событие клиента интервала
		event::id_t eid = io.event(event::node_t::TIMER, event::family_t::INTERVAL, event::type_t::NONE, event::protocol_t::NONE);

		cout << " Таймерное событие ID: " << eid << endl;

		io.setTimeout(eid, event::action_t::NONE, 12000);

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
	engine::io_t io(&fmk, &log);
	// Добавляем новое событие клиента SCTP
	event::id_t eid = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::SCTP);
	// Устанавливаем порт события
	io.setTargetPort(eid, 2222);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устананавливаем опции события
		if(io.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY | event::options::AUTO_RECONNECT))
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Успешно установлены опции события!" << endl;
		// Записываем ошибку в лог установки опций события
		else cout << " Ошибка установки опций события!" << endl;
		// Создаём объект управления SCTP протоколом
		engine::sctp_t sctp(&fmk, &log);
		// Выполняем подписку на SCTP события
		sctp.eventsSubscribe(eid, {
			net::sctp::event_type_t::ASSOC_CHANGE,
			net::sctp::event_type_t::SHUTDOWN_EVENT,
			net::sctp::event_type_t::SEND_FAILED_EVENT,
			net::sctp::event_type_t::REMOTE_ERROR
		});
		// Устанавливаем IP-адрес события
		if(io.setAddress(eid, event::address_t::IPV4, "0.0.0.0")){
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid, "127.0.0.1")){
				// Устанавливаем функцию обратного вызова на изменение статуса события
				io.on(eid, [&io, &sctp, &log](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Записываем в лог сообщение о принятии события
							log.print("Событие принято: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Записываем в лог сообщение об уничтожении события
							log.print("Событие подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Записываем в лог сообщение об инициализации события
							log.print("Событие инициализировано: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::LAUNCHED):
							// Записываем в лог сообщение о запуске события
							log.print("Событие запущено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Записываем в лог сообщение о паузе события
							log.print("Событие на паузе: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Записываем в лог сообщение о возобновлении события
							log.print("Событие возобновлено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Записываем в лог сообщение о успешном выполнении события
							log.print("Событие успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Записываем в лог сообщение о неудачном выполнении события
							log.print("Событие выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Записываем в лог сообщение о выполнении события в ожидании
							log.print("Событие в ожидании: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Записываем в лог сообщение о подключении события
							log.print("Событие подключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Записываем в лог сообщение об отмене события
							log.print("Событие отменено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Записываем в лог сообщение о переподключении события
							log.print("Событие переподключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус прослушивания события
						case static_cast <uint8_t> (event::status_t::LISTENING):
							// Записываем в лог сообщение о прослушивании события
							log.print("Событие прослушивается: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус возрождения события
						case static_cast <uint8_t> (event::status_t::REBIRTHED): {
							// Записываем в лог сообщение об возрождении события
							log.print("Событие возрождено: ID=%u", log_t::flag_t::INFO, eid);
							// Выполняем подписку на SCTP события
							sctp.eventsSubscribe(eid, {
								net::sctp::event_type_t::ASSOC_CHANGE,
								net::sctp::event_type_t::SHUTDOWN_EVENT,
								net::sctp::event_type_t::SEND_FAILED_EVENT,
								net::sctp::event_type_t::REMOTE_ERROR
							});
						} break;
					}
				});
				// Устанавливаем функцию обратного вызова на запись в событие
				io.on(eid, static_cast <engine::callback::write_t> ([&log](const event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о записи данных
					log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				sctp.on(eid, static_cast <engine::callback::sctp::minfo_t> ([&log](const event::id_t eid, const net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					log.print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				sctp.on(eid, [&log](const event::id_t eid, net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					cout << " SCTP EVENT ID: " << event->id << endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							cout << "  - DATA IO EVENT " << endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							cout << "  - REMOTE ERROR EVENT " << endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							cout << "  - ASSOC CHANGE EVENT " << endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							cout << "  - SHUTDOWN EVENT " << endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							cout << "  - SENDER DRY EVENT " << endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							cout << "  - PEER ADDR CHANGE EVENT " << endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							cout << "  - SEND FAILED EVENT " << endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							cout << "  - STREAM RESET EVENT " << endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							cout << "  - AUTHENTICATION EVENT " << endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							cout << "  - ADAPTATION INDICATION EVENT " << endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							cout << "  - PARTIAL DELIVERY EVENT " << endl;
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на чтение из события
				io.on(eid, [&sctp, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Получаем информацию о сообщении SCTP-сокета
					const net::sctp::minfo_t & minfo = sctp.messageInfo(eid);
					// Записываем в лог информацию о сообщении SCTP-сокета
					cout << " SCTP Message Info2: " << endl;
					cout << "  - Stream Number: " << minfo.num << endl;
					cout << "  - Payload Protocol ID: " << (u_short) minfo.ppid << endl;
					cout << "  - Context: " << minfo.ctx << endl;
					cout << "  - Time to Live: " << minfo.ttl << endl;
					cout << "  - Flags: " << minfo.flags.size() << endl;
					// Получаем статус SCTP-сокета
					const net::sctp::status_t & status = sctp.status(eid);
					// Возвращаем статус SCTP-сокета
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
					// Записываем в лог сообщение о чтении данных
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
							// Записываем ошибку в лог неизвестного события
							log.print("Неизвестная ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Записываем ошибку в лог недопустимой операции
							log.print("Недопустимая операция события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Записываем ошибку в лог доступа запрещёния
							log.print("Доступ к событию запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Записываем ошибку в лог уже существующего объекта
							log.print("Объект события уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Записываем ошибку в лог доступа к сокету
							log.print("Ошибка доступа к сокету события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Записываем ошибку в лог некорректного адреса
							log.print("Некорректный адрес события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Записываем ошибку в лог подключения
							log.print("Ошибка подключения события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Записываем ошибку в лог недостаточно ресурсов
							log.print("Недостаточно ресурсов для события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Записываем ошибку в лог события
							log.print("Ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Записываем ошибку в лог события
							log.print("Объект события не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на удачное подключение к серверу
				io.on(eid, static_cast <engine::callback::connect_t> ([&io, &log](const event::id_t eid, const bool ok) noexcept -> void {
					// Записываем в лог сообщение о принятии события
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
							// Записываем в лог сообщение о чтении события
							log.print("Событие на чтение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							log.print("Событие на запись: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							log.print("Событие на подключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							log.print("Событие на отключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							log.print("Событие на переподключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							log.print("Событие на закрытие подключения: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							log.print("Событие на изменение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							log.print("Событие на удаление: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							log.print("Событие на переименование: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							log.print("Событие на изменение атрибутов: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							log.print("Событие на отзыв доступа: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							log.print("Событие на изменение счётчика жёстких ссылок: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем таймаут события на чтение
				io.setTimeout(eid, event::action_t::READ, 10000);
				// Устанавливаем таймаут события на запись
				io.setTimeout(eid, event::action_t::WRITE, 7000);
				// Устанавливаем таймаут события на подключение
				io.setTimeout(eid, event::action_t::CONNECT, 5000);
				// Выполняем фиксацию настроек события сервера
				if(io.commit(eid)){
					// Если подключение к серверу прошло успешно
					if(io.connect(eid)){
						// Выполняем запуск события
						if(io.launch(eid)){
							// Записываем в лог сообщение об успешном запуске события
							cout << " Событие успешно запущено!" << endl;
							/**
							 * Запускаем опрос событий
							 */
							while(io.poll());
						// Записываем ошибку в лог запуска события
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
	engine::io_t io(&fmk, &log);
	// Добавляем новое событие сервера TCP
	event::id_t eid = io.event(event::node_t::SERVER, event::family_t::IPV6, event::type_t::STREAM, event::protocol_t::TCP);
	// Устанавливаем порт события
	io.setSourcePort(eid, 8080);
	// Устанавливаем адрес события (en0 -> ea:ab:fd:74:1d:0d -> 10.9.5.161)
	// if(io.setAddress(eid, event::address_t::NETWORK, "10.9.5.0/255.255.255.0")){
	// if(io.setAddress(eid, event::address_t::NETWORK, "fe80::105d:12e9:40c7:a76/76")){
	// if(io.setAddress(eid, event::address_t::IPV4, "192.168.7.231")){
	// if(io.setAddress(eid, event::address_t::NETWORK, "192.168.7.231/255.255.255.0")){
	// if(io.setAddress(eid, event::address_t::NETWORK, "FE80::96:81FE:CCE3:0/112")){
	// if(io.setAddress(eid, event::address_t::MAC, "EA:AB:FD:74:1D:0D")){
	// if(io.setAddress(eid, event::address_t::UDS, "/tmp/awh.sock")){
	if(io.setIface(eid, "EN0")){

		cout << " !!!!! " << io.getAddress(eid, event::address_t::MAC) << ":" << io.getSourcePort(eid) << " !!!!! " << io.getTarget(eid) << " == " << io.getIface(eid) << endl;

		// cout << " !!!!! " << io.getAddress(eid, event::address_t::UDS) << " !!!!! " << io.host(eid) << endl;

		io.setBufferSize(eid, event::action_t::READ, 1024 * 64);
		io.setBufferSize(eid, event::action_t::WRITE, 1024 * 64);

		cout << " Размер буфера на чтение: " << io.getBufferSize(eid, event::action_t::READ) << " байт. " << endl;
		cout << " Размер буфера на запись: " << io.getBufferSize(eid, event::action_t::WRITE) << " байт. " << endl;

	// Если адрес не установлен
	} else {

		cout << " Ошибка установки адреса события! " << endl;

	}
	*/
	// Возвращаем результат
	return 0;
}
