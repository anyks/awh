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
	io_t io(&fmk, &log);
	// Добавляем новое событие клиента UDP
	event::id_t eid = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::DATAGRAM);
	// Устанавливаем порт события
	io.port(eid, 9999);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устанавливаем мультикастовый режим события
		if(io.cast(eid, event::cast_t::MULTICAST)){
			// Устанавливаем TTL для мультикастового события
			if(io.hops(eid, event::family_t::IPV4, event::hops_t::NETWORK)){
				// Устананавливаем опции события
				if(io.options(eid, event::options::NOSIGILL | event::options::NOSIGPIPE | event::options::REUSEADDR | event::options::REUSEPORT | event::options::MULTICASTLOOP))
					// Выводим сообщение об успешной установке опций события
					cout << " Успешно установлены опции события!" << endl;
				// Выводим сообщение об ошибке установки опций события
				else cout << " Ошибка установки опций события!" << endl;
				// Устанавливаем IP-адрес события
				if(io.address(eid, event::address_t::IPV4, "239.255.1.1")){
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
					// Устанавливаем функцию обратного вызова на запись в событие
					io.on(eid, static_cast <event::callback::write_t> ([&log](const event::id_t eid, const size_t size) noexcept -> void {
						// Выводим сообщение о переподключении события
						log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
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
					// Устанавливаем таймаут события на чтение
					// io.timeout(eid, event::action_t::READ, 5000);
					// Устанавливаем таймаут события на запись
					// io.timeout(eid, event::action_t::WRITE, 5000);
					// Выполняем фиксацию настроек события сервера
					if(io.commit(eid)){
						// Количество отправленных сообщений
						uint8_t counter = 0;
						/**
						 * Запускаем бесконечный цикл отправки сообщений в мультикастовую группу
						 */
						for(;;){
							// Формируем отправляемое сообщение
							const string & message = fmk.format("Message #%d", ++counter);
							// Выполняем отправку сообщения в мультикастовую группу
							if(!io.send(eid, message.c_str(), message.size()))
								// Выводим сообщение об ошибке отправки сообщения
								log.print("Сообщение не отправлено: ID=%u, %s", log_t::flag_t::INFO, eid, message.c_str());
							// Задержка между отправками сообщений
							::sleep(2);
						}
					}
				// Если адрес не установлен
				} else cout << " Ошибка установки адреса события!" << endl;
			// Если TTL мультикастового события не установлен
			} else {
				// Выводим сообщение об ошибке установки TTL мультикастового события
				cout << " Ошибка установки TTL мультикастового события!" << endl;
			}
		// Если мультикастовый режим события не установлен
		} else {
			// Выводим сообщение об ошибке установки мультикастового режима события
			cout << " Ошибка установки мультикастового режима события!" << endl;
		}
	}
	// Выводим результат
	return EXIT_SUCCESS;
}
