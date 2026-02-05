/**
 * @file: server.cpp
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

/**
 * Для работы сервера:
 * # Назначаем адрес 10.0.0.1, destination 10.0.0.1 (Point-to-Point to self)
 * $ sudo ifconfig utun7 10.0.0.1 10.0.0.1 netmask 255.255.255.255 up
 *
 * MacOS X:
 * # Весь трафик для 10.0.0.x отправлять в интерфейс utun7 (сетевой интерфейс сервера)
 * $ sudo route -n add -net 10.0.0.0/24 -interface utun7
 *
 * FreeBSD:
 * # Синтаксис route net практически идентичен
 * $ sudo route add -net 10.0.0.0/24 -interface tun0
 *
 * Remove:
 * $ sudo route delete -net 10.0.0.0/24 -interface utun7
 */

/**
 * Стандартные модули
 */
#include <iostream>
#include <cinttypes>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>
#include <net/addr.hpp>
#include <net/eth/gateway.hpp>

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
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Создаём объект работы с сетевыми адресами
	net_addr_t addr(&fmk, &log);
	// Создаём объект работы с шлюзами
	eth::gateway_t gateway(&fmk, &log);
	// Добавляем новое событие туннеля
	event::id_t tid = io.event(event::node_t::TUNNEL, event::family_t::IPV4);
	// Добавляем новое событие посредника
	event::id_t mid = io.event(event::node_t::MEDIATOR, event::family_t::IPV4);
	// Добавляем новое событие сервера UDP
	event::id_t eid = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Устанавливаем порт события
	io.port(eid, 2222);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устананавливаем опции события туннеля
		if(io.options(tid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
			// Выводим сообщение об успешной установке опций события
			cout << " Успешно установлены опции события туннеля!" << endl;
		// Выводим сообщение об ошибке установки опций события
		else cout << " Ошибка установки опций события туннеля!" << endl;
		// Устананавливаем опции события
		if(io.options(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::KEEPALIVE))
			// Выводим сообщение об успешной установке опций события
			cout << " Успешно установлены опции события сервера! " << endl;
		// Выводим сообщение об ошибке установки опций события
		else cout << " Ошибка установки опций события сервера!" << endl;
		// Устанавливаем IP-адрес события
		if(io.address(eid, event::address_t::IPV4, "127.0.0.1") && io.address(tid, event::address_t::IPV4, "10.0.0.1")){
			// Устанавливаем адрес сервера назначения
			if(io.target(mid, "10.0.0.2")){
				// Устанавливаем функцию обратного вызова на событие сервера
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
				// Устанавливаем функцию обратного вызова на событие туннеля
				io.on(tid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Выводим сообщение о принятии события
							log.print("Событие туннеля принято: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Выводим сообщение об уничтожении события
							log.print("Событие туннеля подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Выводим сообщение об инициализации события
							log.print("Событие туннеля инициализировано: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::LAUNCHED):
							// Выводим сообщение о запуске события
							log.print("Событие туннеля запущено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Выводим сообщение о паузе события
							log.print("Событие туннеля на паузе: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Выводим сообщение о возобновлении события
							log.print("Событие туннеля возобновлено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Выводим сообщение о успешном выполнении события
							log.print("Событие туннеля успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Выводим сообщение о неудачном выполнении события
							log.print("Событие туннеля выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Выводим сообщение о выполнении события в ожидании
							log.print("Событие туннеля в ожидании: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Выводим сообщение о подключении события
							log.print("Событие туннеля подключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Выводим сообщение об отмене события
							log.print("Событие туннеля отменено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Выводим сообщение о переподключении события
							log.print("Событие туннеля переподключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус прослушивания события
						case static_cast <uint8_t> (event::status_t::LISTENING):
							// Выводим сообщение о прослушивании события
							log.print("Событие туннеля прослушивается: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на событие посредника
				io.on(mid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Выводим сообщение о принятии события
							log.print("Событие посредника принято: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Выводим сообщение об уничтожении события
							log.print("Событие посредника подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Выводим сообщение об инициализации события
							log.print("Событие посредника инициализировано: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::LAUNCHED):
							// Выводим сообщение о запуске события
							log.print("Событие посредника запущено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Выводим сообщение о паузе события
							log.print("Событие посредника на паузе: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Выводим сообщение о возобновлении события
							log.print("Событие посредника возобновлено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Выводим сообщение о успешном выполнении события
							log.print("Событие посредника успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Выводим сообщение о неудачном выполнении события
							log.print("Событие посредника выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Выводим сообщение о выполнении события в ожидании
							log.print("Событие посредника в ожидании: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Выводим сообщение о подключении события
							log.print("Событие посредника подключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Выводим сообщение об отмене события
							log.print("Событие посредника отменено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Выводим сообщение о переподключении события
							log.print("Событие посредника переподключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус прослушивания события
						case static_cast <uint8_t> (event::status_t::LISTENING):
							// Выводим сообщение о прослушивании события
							log.print("Событие посредника прослушивается: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на подключение нового клиента
				io.on(eid, static_cast <event::callback::accept_t> ([tid, mid, &io, &log](const event::id_t eid, const event::id_t cid) noexcept -> void {
					// Объединяем с пиром
					io.splice(mid, cid);
					// Выводим сообщение о принятии события
					log.print("Событие принято: ID=%u, Клиентский ID=%u, IP=%s, PORT=%d", log_t::flag_t::INFO, eid, cid, io.address(cid, event::address_t::IPV4).c_str(), io.port(cid));
					// Устанавливаем функцию обратного вызова на событие таймера
					io.on(cid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
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
					io.on(cid, static_cast <event::callback::write_t> ([&log](const event::id_t eid, const size_t size) noexcept -> void {
						// Выводим сообщение о переподключении события
						log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
					}));
					// Устанавливаем функцию обратного вызова на чтение из события
					io.on(cid, [tid, &io, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
						// Текст входящего сообщения
						const string message(reinterpret_cast <const char *> (data), size);
						// Выводим сообщение о переподключении события
						log.print("Прочитано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
						// Отправляем данные в туннель
						if(io.send(tid, reinterpret_cast <const char *> (data), size))
							// Если данные успешно отправлены
							log.print("Отправлено в туннель: ID=%u, %zu байт", log_t::flag_t::INFO, tid, size);
						// Если данные не отправлены
						else log.print("Ошибка отправки в туннель: ID=%u", log_t::flag_t::CRITICAL, tid);
					});
					// Устанавливаем функцию обратного вызова на ошибку события
					io.on(cid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
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
					// Устанавливаем функцию обратного вызова на общее событие
					io.on(cid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
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
				}));
				// Устанавливаем функцию обратного вызова на чтение из события посредника
				io.on(mid, [&io, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const string message(reinterpret_cast <const char *> (data), size);
					// Выводим сообщение о переподключении события
					log.print("Прочитано из посредника: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
					// Отправляем данные обратно клиенту
					if(io.send(eid, reinterpret_cast <const char *> (data), size))
						// Если данные успешно отправлены
						log.print("Отправлено в сервер: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else log.print("Ошибка отправки в сервер: ID=%u", log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на ошибку события сервера
				io.on(eid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Выводим сообщение об ошибке неизвестного события
							log.print("Неизвестная ошибка события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Выводим сообщение об ошибке недопустимой операции
							log.print("Недопустимая операция события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Выводим сообщение об ошибке доступа запрещёния
							log.print("Доступ к событию сервера запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Выводим сообщение об ошибке уже существующего объекта
							log.print("Объект события сервера уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Выводим сообщение об ошибке доступа к сокету
							log.print("Ошибка доступа к сокету события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Выводим сообщение об ошибке некорректного адреса
							log.print("Некорректный адрес события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Выводим сообщение об ошибке подключения
							log.print("Ошибка подключения события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Выводим сообщение об ошибке недостаточно ресурсов
							log.print("Недостаточно ресурсов для события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Выводим сообщение об ошибке события
							log.print("Ошибка события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Выводим сообщение об ошибке события
							log.print("Объект события сервера не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на ошибку события туннеля
				io.on(tid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Выводим сообщение об ошибке неизвестного события
							log.print("Неизвестная ошибка события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Выводим сообщение об ошибке недопустимой операции
							log.print("Недопустимая операция события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Выводим сообщение об ошибке доступа запрещёния
							log.print("Доступ к событию туннеля запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Выводим сообщение об ошибке уже существующего объекта
							log.print("Объект события туннеля уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Выводим сообщение об ошибке доступа к сокету
							log.print("Ошибка доступа к сокету события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Выводим сообщение об ошибке некорректного адреса
							log.print("Некорректный адрес события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Выводим сообщение об ошибке подключения
							log.print("Ошибка подключения события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Выводим сообщение об ошибке недостаточно ресурсов
							log.print("Недостаточно ресурсов для события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Выводим сообщение об ошибке события
							log.print("Ошибка события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Выводим сообщение об ошибке события
							log.print("Объект события туннеля не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на ошибку события посредника
				io.on(mid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Выводим сообщение об ошибке неизвестного события
							log.print("Неизвестная ошибка события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Выводим сообщение об ошибке недопустимой операции
							log.print("Недопустимая операция события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Выводим сообщение об ошибке доступа запрещёния
							log.print("Доступ к событию посредника запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Выводим сообщение об ошибке уже существующего объекта
							log.print("Объект события посредника уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Выводим сообщение об ошибке доступа к сокету
							log.print("Ошибка доступа к сокету события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Выводим сообщение об ошибке некорректного адреса
							log.print("Некорректный адрес события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Выводим сообщение об ошибке подключения
							log.print("Ошибка подключения события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Выводим сообщение об ошибке недостаточно ресурсов
							log.print("Недостаточно ресурсов для события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Выводим сообщение об ошибке события
							log.print("Ошибка события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Выводим сообщение об ошибке события
							log.print("Объект события посредника не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на общее событие сервера
				io.on(eid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Выводим сообщение о чтении события
							log.print("Событие сервера на чтение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Выводим сообщение о записи события
							log.print("Событие сервера на запись: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Выводим сообщение о подключении события
							log.print("Событие сервера на подключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Выводим сообщение об отключении события
							log.print("Событие сервера на отключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Выводим сообщение о переподключении события
							log.print("Событие сервера на переподключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Выводим сообщение о закрытии события
							log.print("Событие сервера на закрытие подключения: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение об изменении события
							log.print("Событие сервера на изменение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение об удалении события
							log.print("Событие сервера на удаление: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Выводим сообщение о переименовании события
							log.print("Событие сервера на переименование: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Выводим сообщение об изменении атрибутов события
							log.print("Событие сервера на изменение атрибутов: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Выводим сообщение об отзыве доступа события
							log.print("Событие сервера на отзыв доступа: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Выводим сообщение о изменении счётчика жёстких ссылок события
							log.print("Событие сервера на изменение счётчика жёстких ссылок: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на общее событие посредника
				io.on(mid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Выводим сообщение о чтении события
							log.print("Событие посредника на чтение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Выводим сообщение о записи события
							log.print("Событие посредника на запись: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Выводим сообщение о подключении события
							log.print("Событие посредника на подключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Выводим сообщение об отключении события
							log.print("Событие посредника на отключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Выводим сообщение о переподключении события
							log.print("Событие посредника на переподключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Выводим сообщение о закрытии события
							log.print("Событие посредника на закрытие подключения: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение об изменении события
							log.print("Событие посредника на изменение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение об удалении события
							log.print("Событие посредника на удаление: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Выводим сообщение о переименовании события
							log.print("Событие посредника на переименование: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Выводим сообщение об изменении атрибутов события
							log.print("Событие посредника на изменение атрибутов: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Выводим сообщение об отзыве доступа события
							log.print("Событие посредника на отзыв доступа: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Выводим сообщение о изменении счётчика жёстких ссылок события
							log.print("Событие посредника на изменение счётчика жёстких ссылок: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Выполняем фиксацию настроек события сервера
				if(io.commit(eid) && io.commit(tid) && io.commit(mid)){
					// Выполняем запуск события
					if(io.launch(eid) && io.launch(tid)){
						// Маршрут туннеля
						eth::gateway_t::route_t route;
						// Устанавливаем интерфейс туннеля
						route.ifname = io.iface(tid);
						// Устанавливаем префикс маршрута туннеля
						route.prefix = 24;
						// Создаём шлюз маршрута туннеля
						route.gateway = make_unique <net::addr_net_ipv4_t> ();
						// Создаём адрес назначения маршрута туннеля
						route.destination = make_unique <net::addr_net_ipv4_t> ();
						// Выполням парсинг адреса назначения маршрута туннеля
						addr = "10.0.0.0";
						// Устанавливаем адрес назначения маршрута туннеля
						awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address = addr.v4(net_addr_t::endian_t::LITTLE);
						// Устанавливаем маршрут туннеля (sudo route -n add -net 10.0.0.0/24 -interface utun7)
						if(gateway.add(route))
							// Выводим сообщение об успешной установке маршрута туннеля
							cout << " Маршрут туннеля успешно установлен!" << endl;
						// Выводим сообщение об успешном запуске события
						cout << " Событие сервера и туннеля успешно запущено!" << endl;
						/**
						 * Запускаем опрос событий
						 */
						while(io.poll());
					// Выводим сообщение об ошибке запуска события
					} else cout << " Ошибка запуска события!" << endl;
				}
			// Если адрес назначения не установлен
			} else cout << " Ошибка установки адреса сервера!" << endl;
		// Если адрес не установлен
		} else cout << " Ошибка установки адреса события!" << endl;
	}
	// Выводим результат
	return EXIT_SUCCESS;
}
