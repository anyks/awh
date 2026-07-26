/**
 * @file: event.cpp
 * @date: 2025-11-20
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с событиями движка ввода-вывода — демонстрация регистрации событий, управления подписками,
 *        обработки действий чтения и записи и корректной остановки цикла событий
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Стандартные модули
 */
#include <chrono>
#include <thread>
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
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Добавляем новое пользовательское событие
	event::id_t eid = io.event(event::node_t::NOTIFY, event::family_t::USER);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устанавливаем функцию обратного вызова на изменение статуса события
		io.on(eid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
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
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		io.on(eid, static_cast <engine::callback::write_t> ([&log](const event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о записи данных
			log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		io.on(eid, [&io, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
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
		// Выполняем фиксацию настроек события сервера
		if(io.commit(eid)){
			// Запускаем дочерний поток для уведомления события
			std::thread([&io](const event::id_t eid) noexcept -> void {
				// Текст сообщения
				const string message = "Hello AWH IO Event!";
				// Задержка перед уведомлением события
				std::this_thread::sleep_for(std::chrono::seconds(3));
				// Уведомляем событие
				io.send(eid, reinterpret_cast <const char *> (message.c_str()), message.length());
				// Задержка перед уведомлением события
				std::this_thread::sleep_for(std::chrono::seconds(3));
				// Уведомляем событие
				io.send(eid, reinterpret_cast <const char *> (message.c_str()), message.length());
			}, eid).detach();
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
	// Возвращаем результат
	return EXIT_SUCCESS;
}
