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
 * @brief Пример клиента Unix Domain Socket в датаграммном режиме на низкоуровневом движке ввода-вывода —
 *        демонстрация обмена сообщениями через файл сокета
 *
 * @copyright Copyright © 2025
 *
 */

#include <iostream>
#include <unistd.h>

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
	// Устанавливаем уровень логирования
	// awh::log::level(awh::log::level_t::NONE);
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io;
	// Добавляем новое событие клиента
	event::id_t eid = io.event(event::node_t::CLIENT, event::family_t::UDS, event::type_t::DATAGRAM);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устананавливаем опции события
		if(io.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY))
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Успешно установлены опции события!" << endl;
		// Записываем ошибку в лог установки опций события
		else cout << " Ошибка установки опций события!" << endl;
		// Устанавливаем адрес сервера назначения
		if(io.setTarget(eid, "/tmp/awh.sock")){
			// Устанавливаем функцию обратного вызова на изменение статуса события
			io.on(eid, [](const event::id_t eid, const event::status_t status) noexcept -> void {
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
			io.on(eid, static_cast <engine::callback::write_t> ([](const event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о записи данных
				awh::log::print("Записано: ID=%u, %zu байт", awh::log::flag_t::INFO, eid, size);
			}));
			// Счётчик эхо-ответов
			uint8_t count = 0;
			// Устанавливаем функцию обратного вызова на чтение из события
			io.on(eid, [&count, &io](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о чтении данных
				awh::log::print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log::flag_t::INFO, eid, size, message.c_str());
				// Задержка перед ответом 1 секунду
				::sleep(1);
				// Ограничиваем количество эхо-ответов
				if(count++ < 5){
					// Отправляем данные обратно клиенту
					if(io.send(eid, reinterpret_cast <const char *> (data), size))
						// Если данные успешно отправлены
						awh::log::print("Отправлено: ID=%u, %zu байт", awh::log::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else awh::log::print("Ошибка отправки: ID=%u", awh::log::flag_t::CRITICAL, eid);
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			io.on(eid, [](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
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
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						awh::log::print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log::flag_t::CRITICAL, eid, description.c_str());
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
			io.on(eid, [](const event::id_t eid, const event::action_t action) noexcept -> void {
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
			// Устанавливаем таймаут события на чтение
			io.setTimeout(eid, event::action_t::READ, 10000);
			// Устанавливаем таймаут события на запись
			io.setTimeout(eid, event::action_t::WRITE, 6000);
			// Выполняем фиксацию настроек события сервера
			if(io.commit(eid)){
				// Выполняем запуск события
				if(io.launch(eid)){
					// Записываем в лог сообщение об успешном запуске события
					cout << " Событие успешно запущено!" << endl;
					// Текст исходящего сообщения
					const string message("Hello from async client!");
					// Отправляем данные обратно клиенту
					if(io.send(eid, message.c_str(), message.size()))
						// Если данные успешно отправлены
						awh::log::print("Отправлено: ID=%u, %zu байт", awh::log::flag_t::INFO, eid, message.size());
					// Если данные не отправлены
					else awh::log::print("Ошибка отправки: ID=%u", awh::log::flag_t::CRITICAL, eid);
					/**
					 * Запускаем опрос событий
					 */
					while(io.poll());
				// Записываем ошибку в лог запуска события
				} else cout << " Ошибка запуска события!" << endl;
			}
		// Если адрес назначения не установлен
		} else cout << " Ошибка установки адреса сервера!" << endl;
	}
	// Возвращаем результат
	return 0;
}
