/**
 * @file ipc.cpp
 * @date 2025-11-21
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
 * @brief Пример межпроцессного взаимодействия через Unix Domain Socket в потоковом режиме —
 *        демонстрация установки локального соединения и обмена непрерывным потоком байт
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Стандартные модули
 */
#include <chrono>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <cinttypes>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

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
 * @return код выхода из приложения
 *
 */
int32_t main(){
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 */
	awh::fmk::initialize();
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io;
	// Добавляем новое пользовательское событие
	const auto & events = io.events(event::family_t::UDS, event::type_t::STREAM, event::protocol_t::NONE);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Получаем идентификатор родительского процесса
		const pid_t mpid = ::getpid();
		// Устанавливаем идентификатор процесса
		pid_t pid = -1;
		/**
		 * Определяем тип потока
		 */
		switch((pid = ::fork())){
			// Если поток не создан
			case -1: {
				// Записываем в лог сообщение
				awh::log::print("Child process could not be created", awh::log::flag_t::CRITICAL);
				// Выходим из приложения
				::exit(EXIT_FAILURE);
			} break;
			// Если процесс является дочерним
			case 0: {
				// Выполняем переинициализацию асинхронного движка ввода-вывода
				io.reinitialize();
				// Уничтожаем событие родительского процесса
				io.destroy(events[0]);
				// Устананавливаем опции события
				if(io.setOptions(events[1], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
					// Записываем в лог сообщение об успешной установке опций события
					cout << " Успешно установлены опции события!" << endl;
				// Записываем ошибку в лог установки опций события
				else cout << " Ошибка установки опций события!" << endl;
				// Устанавливаем функцию обратного вызова на изменение статуса события
				io.on(events[1], [](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Записываем в лог сообщение о принятии события
							awh::log::print("Событие принято: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Записываем в лог сообщение об уничтожении события
							awh::log::print("Событие подлежит уничтожению: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Записываем в лог сообщение об инициализации события
							awh::log::print("Событие инициализировано: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::LAUNCHED):
							// Записываем в лог сообщение о запуске события
							awh::log::print("Событие запущено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Записываем в лог сообщение о паузе события
							awh::log::print("Событие на паузе: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Записываем в лог сообщение о возобновлении события
							awh::log::print("Событие возобновлено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Записываем в лог сообщение о успешном выполнении события
							awh::log::print("Событие успешно выполнено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Записываем в лог сообщение о неудачном выполнении события
							awh::log::print("Событие выполнено с ошибкой: ID=%u", awh::log::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Записываем в лог сообщение о выполнении события в ожидании
							awh::log::print("Событие в ожидании: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Записываем в лог сообщение о подключении события
							awh::log::print("Событие подключено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Записываем в лог сообщение об отмене события
							awh::log::print("Событие отменено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Записываем в лог сообщение о переподключении события
							awh::log::print("Событие переподключено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус прослушивания события
						case static_cast <uint8_t> (event::status_t::LISTENING):
							// Записываем в лог сообщение о прослушивании события
							awh::log::print("Событие прослушивается: ID=%u", awh::log::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на запись в событие
				io.on(events[1], static_cast <engine::callback::write_t> ([](const event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о записи данных
					awh::log::print("Записано: ID=%u, %zu байт", awh::log::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				io.on(events[1], [mpid](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const string message(reinterpret_cast <const char *> (data), size);
					// Записываем в лог сообщение о чтении данных
					awh::log::print("Прочитано: ID=%u, MPID=%u, PID=%u, %zu байт, сообщение: %s", awh::log::flag_t::INFO, eid, mpid, ::getpid(), size, message.c_str());
				});
				// Устанавливаем функцию обратного вызова на ошибку события
				io.on(events[1], [](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Записываем ошибку в лог неизвестного события
							awh::log::print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Записываем ошибку в лог недопустимой операции
							awh::log::print("Недопустимая операция события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Записываем ошибку в лог доступа запрещёния
							awh::log::print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Записываем ошибку в лог уже существующего объекта
							awh::log::print("Объект события уже существует: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Записываем ошибку в лог некорректного адреса
							awh::log::print("Некорректный адрес события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Записываем ошибку в лог подключения
							awh::log::print("Ошибка подключения события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Записываем ошибку в лог недостаточно ресурсов
							awh::log::print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Записываем ошибку в лог события
							awh::log::print("Ошибка события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Записываем ошибку в лог события
							awh::log::print("Объект события не найден: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на общее событие
				io.on(events[1], [](const event::id_t eid, const event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							awh::log::print("Событие на чтение: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							awh::log::print("Событие на запись: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							awh::log::print("Событие на подключение: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							awh::log::print("Событие на отключение: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							awh::log::print("Событие на переподключение: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							awh::log::print("Событие на закрытие подключения: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							awh::log::print("Событие на изменение: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							awh::log::print("Событие на удаление: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							awh::log::print("Событие на переименование: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							awh::log::print("Событие на изменение атрибутов: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							awh::log::print("Событие на отзыв доступа: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							awh::log::print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log::flag_t::INFO, eid);
						break;
					}
				});
				// Выполняем фиксацию настроек события сервера
				if(io.commit(events[1])){
					// Сообщение для отправки родительскому процессу
					const string message = "Hello from child process!";
					// Отправляем сообщение родительскому процессу
					io.send(events[1], reinterpret_cast <const char *> (message.c_str()), message.length());
					// Выполняем запуск события
					if(io.launch(events[1])){
						// Записываем в лог сообщение об успешном запуске события
						cout << " Событие успешно запущено!" << endl;
						/**
						 * Запускаем опрос событий
						 */
						while(io.poll());
					// Записываем ошибку в лог запуска события
					} else cout << " Ошибка запуска события!" << endl;
				}
			} break;
			// Если процесс является родительским
			default: {
				// Уничтожаем событие дочернего процесса
				io.destroy(events[1]);
				// Устананавливаем опции события
				if(io.setOptions(events[0], event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
					// Записываем в лог сообщение об успешной установке опций события
					cout << " Успешно установлены опции события!" << endl;
				// Записываем ошибку в лог установки опций события
				else cout << " Ошибка установки опций события!" << endl;
				// Устанавливаем функцию обратного вызова на изменение статуса события
				io.on(events[0], [](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Записываем в лог сообщение о принятии события
							awh::log::print("Событие принято: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Записываем в лог сообщение об уничтожении события
							awh::log::print("Событие подлежит уничтожению: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Записываем в лог сообщение об инициализации события
							awh::log::print("Событие инициализировано: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::LAUNCHED):
							// Записываем в лог сообщение о запуске события
							awh::log::print("Событие запущено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Записываем в лог сообщение о паузе события
							awh::log::print("Событие на паузе: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Записываем в лог сообщение о возобновлении события
							awh::log::print("Событие возобновлено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Записываем в лог сообщение о успешном выполнении события
							awh::log::print("Событие успешно выполнено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Записываем в лог сообщение о неудачном выполнении события
							awh::log::print("Событие выполнено с ошибкой: ID=%u", awh::log::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Записываем в лог сообщение о выполнении события в ожидании
							awh::log::print("Событие в ожидании: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Записываем в лог сообщение о подключении события
							awh::log::print("Событие подключено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Записываем в лог сообщение об отмене события
							awh::log::print("Событие отменено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Записываем в лог сообщение о переподключении события
							awh::log::print("Событие переподключено: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если статус прослушивания события
						case static_cast <uint8_t> (event::status_t::LISTENING):
							// Записываем в лог сообщение о прослушивании события
							awh::log::print("Событие прослушивается: ID=%u", awh::log::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на запись в событие
				io.on(events[0], static_cast <engine::callback::write_t> ([](const event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о записи данных
					awh::log::print("Записано: ID=%u, %zu байт", awh::log::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				io.on(events[0], [mpid](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const string message(reinterpret_cast <const char *> (data), size);
					// Записываем в лог сообщение о чтении данных
					awh::log::print("Прочитано: ID=%u, MPID=%u, PID=%u, %zu байт, сообщение: %s", awh::log::flag_t::INFO, eid, mpid, ::getpid(), size, message.c_str());
				});
				// Устанавливаем функцию обратного вызова на ошибку события
				io.on(events[0], [](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Записываем ошибку в лог неизвестного события
							awh::log::print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Записываем ошибку в лог недопустимой операции
							awh::log::print("Недопустимая операция события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Записываем ошибку в лог доступа запрещёния
							awh::log::print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Записываем ошибку в лог уже существующего объекта
							awh::log::print("Объект события уже существует: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Записываем ошибку в лог некорректного адреса
							awh::log::print("Некорректный адрес события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Записываем ошибку в лог подключения
							awh::log::print("Ошибка подключения события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Записываем ошибку в лог недостаточно ресурсов
							awh::log::print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Записываем ошибку в лог события
							awh::log::print("Ошибка события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Записываем ошибку в лог события
							awh::log::print("Объект события не найден: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на общее событие
				io.on(events[0], [](const event::id_t eid, const event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							awh::log::print("Событие на чтение: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							awh::log::print("Событие на запись: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							awh::log::print("Событие на подключение: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							awh::log::print("Событие на отключение: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							awh::log::print("Событие на переподключение: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							awh::log::print("Событие на закрытие подключения: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							awh::log::print("Событие на изменение: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							awh::log::print("Событие на удаление: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							awh::log::print("Событие на переименование: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							awh::log::print("Событие на изменение атрибутов: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							awh::log::print("Событие на отзыв доступа: ID=%u", awh::log::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							awh::log::print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log::flag_t::INFO, eid);
						break;
					}
				});
				// Выполняем фиксацию настроек события сервера
				if(io.commit(events[0])){
					// Сообщение для отправки дочернему процессу
					const string message = "Hello from parent process!";
					// Отправляем сообщение родительскому процессу
					io.send(events[0], reinterpret_cast <const char *> (message.c_str()), message.length());
					// Выполняем запуск события
					if(io.launch(events[0])){
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
		}
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
