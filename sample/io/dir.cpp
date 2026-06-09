/**
 * @file: dir.cpp
 * @date: 2025-11-24
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
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Добавляем новое событие каталога
	event::id_t eid = io.event(event::node_t::DIR, event::family_t::FSYS);
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
		// Устанавливаем функцию обратного вызова на изменение события
		io.on(eid, [&log](const event::id_t eid, const event::action_t action, const event::vnode_t vnode, const std::string & path) noexcept -> void {
			/**
			 * Обрабатываем тип узла события
			 */
			switch(static_cast <uint8_t> (vnode)){
				// Если тип узла не определён
				case static_cast <uint8_t> (event::vnode_t::NONE):
					// Выводим сообщение о типе узла события
					log.print("Тип узла события: Не определён, Путь=%s", log_t::flag_t::INFO, path.c_str());
				break;
				case static_cast <uint8_t> (event::vnode_t::CHR): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							log.print("Тип узла события: Символьный узел устройства добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение об удалении события
							log.print("Тип узла события: Символьный узел устройства удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				case static_cast <uint8_t> (event::vnode_t::BLK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							log.print("Тип узла события: Блочный узел устройства добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение об удалении события
							log.print("Тип узла события: Блочный узел устройства удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является каналом FIFO
				case static_cast <uint8_t> (event::vnode_t::FIFO): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							log.print("Тип узла события: Канал FIFO добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение об удалении события
							log.print("Тип узла события: Канал FIFO удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является сокетом
				case static_cast <uint8_t> (event::vnode_t::SOCK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							log.print("Тип узла события: Сокет добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение об удалении события
							log.print("Тип узла события: Сокет удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является файлом
				case static_cast <uint8_t> (event::vnode_t::FILE): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							log.print("Тип узла события: Файл добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение об удалении события
							log.print("Тип узла события: Файл удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является каталогом
				case static_cast <uint8_t> (event::vnode_t::DIR): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							log.print("Тип узла события: Каталог добавлен, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение о типе узла события
							log.print("Тип узла события: Каталог удалён, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является символической ссылкой
				case static_cast <uint8_t> (event::vnode_t::LINK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Выводим сообщение о изменении события
							log.print("Тип узла события: Символическая ссылка добавлена, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Выводим сообщение о типе узла события
							log.print("Тип узла события: Символическая ссылка удалена, Путь=%s", log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		io.on(eid, [&io, &log](const event::id_t eid, const event::action_t action) noexcept -> void {
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
					log.print("Событие на переименование: ID=%u, новый адрес: %s", log_t::flag_t::INFO, eid, io.getAddress(eid, event::address_t::FS).c_str());
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
		// Устанавливаем путь к отслеживаемому каталогу
		if(io.setAddress(eid, event::address_t::FS, "../tmp")){
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
		}
	}
	// Выводим результат
	return EXIT_SUCCESS;
}
