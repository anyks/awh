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
 * Стандартные модули
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
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Добавляем новое событие клиента TCP
	event::id_t eid = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Устанавливаем порт события
	io.port(eid, 2222);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устананавливаем опции события
		if(io.options(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::KEEPALIVE))
			// Выводим сообщение об успешной установке опций события
			cout << " Успешно установлены опции события!" << endl;
		// Выводим сообщение об ошибке установки опций события
		else cout << " Ошибка установки опций события!" << endl;
		// Устанавливаем IP-адрес события
		if(io.setAddress(eid, event::address_t::IPV4, "127.0.0.1")){
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
			// Устанавливаем функцию обратного вызова на подключение нового клиента
			io.on(eid, static_cast <event::callback::accept_t> ([&io, &log](const event::id_t eid, const event::id_t cid) noexcept -> void {
				// Выводим сообщение о принятии события
				log.print("Событие принято: ID=%u, Клиентский ID=%u, IP=%s, PORT=%d", log_t::flag_t::INFO, eid, cid, io.getAddress(cid, event::address_t::IPV4).c_str(), io.port(cid));
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
				io.on(cid, [&io, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const string message(reinterpret_cast <const char *> (data), size);
					// Выводим сообщение о переподключении события
					log.print("Прочитано: ID=%u, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, size, message.c_str());
					// Отправляем данные обратно клиенту
					if(io.send(eid, reinterpret_cast <const char *> (data), size))
						// Если данные успешно отправлены
						log.print("Отправлено: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else log.print("Ошибка отправки: ID=%u", log_t::flag_t::CRITICAL, eid);
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
			io.timeout(eid, event::action_t::READ, 6000);
			// Устанавливаем таймаут события на запись
			io.timeout(eid, event::action_t::WRITE, 6000);
			// Выполняем фиксацию настроек события сервера
			if(io.commit(eid)){
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
		// Если адрес не установлен
		} else cout << " Ошибка установки адреса события!" << endl;
	}
	// Выводим результат
	return EXIT_SUCCESS;
}
