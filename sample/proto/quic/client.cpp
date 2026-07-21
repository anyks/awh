/**
 * @file: client.cpp
 * @date: 2026-07-21
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные модули
 */
#include <string>
#include <cstdint>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/io.hpp>
#include <proto/quic/connection.hpp>

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
	// Создаём соединение QUIC клиента
	quic::connection_t connection(quic::endpoint_t::CLIENT);
	// Устанавливаем список поддерживаемых ALPN-протоколов
	connection.alpn({"h3"});
	// Устанавливаем доменное имя удалённого сервера
	connection.serverNameIndication("localhost");
	// Транспортные параметры клиента
	quic::params::params_t params;
	// Устанавливаем лимит данных соединения
	params.initialMaxData = 1048576;
	// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiLocal = 262144;
	// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
	params.initialMaxStreamDataBidiRemote = 262144;
	// Устанавливаем лимит данных однонаправленных потоков
	params.initialMaxStreamDataUni = 262144;
	// Устанавливаем лимит числа двунаправленных потоков
	params.initialMaxStreamsBidi = 100;
	// Устанавливаем лимит числа однонаправленных потоков
	params.initialMaxStreamsUni = 100;
	// Устанавливаем транспортные параметры соединения
	connection.params(params);
	// Идентификатор открытого потока приложения
	uint64_t sid = quic::connection_t::INVALID_STREAM;
	/**
	 * @brief Функция получения текущего времени в миллисекундах
	 *
	 * @return текущее время в миллисекундах
	 */
	auto now = [&fmk]() noexcept -> uint64_t {
		// Выводим текущий штамп времени в миллисекундах
		return fmk.timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
	};
	/**
	 * @brief Функция отправки всех готовых исходящих датаграмм соединения
	 *
	 * @param eid идентификатор события клиента
	 */
	auto flush = [&](const event::id_t eid) noexcept -> void {
		// Буфер исходящей датаграммы
		string datagram = "";
		/**
		 *  Извлекаем исходящие датаграммы соединения
		 */
		while(connection.write(datagram, now())){
			// Отправляем датаграмму серверу
			if(!io.send(eid, datagram.data(), datagram.size()))
				// Записываем в лог сообщение об ошибке отправки датаграммы
				log.print("Ошибка отправки датаграммы: ID=%u", log_t::flag_t::CRITICAL, eid);
		}
	};
	// Добавляем новое событие клиента UDP
	event::id_t eid = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Добавляем новое событие интервала таймеров соединения
	event::id_t tid = io.event(event::node_t::INTERVAL, event::family_t::TIMER);
	// Устанавливаем порт события
	io.setTargetPort(eid, 2222);
	// Устанавливаем интервал проверки таймеров соединения
	io.setTimeout(tid, event::action_t::NONE, 25);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устананавливаем опции события
		if(!io.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
			// Записываем ошибку в лог установки опций события
			cout << " Ошибка установки опций события!" << endl;
		// Устанавливаем IP-адрес события
		if(io.setAddress(eid, event::address_t::IPV4, "0.0.0.0")){
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid, "127.0.0.1")){
				// Устанавливаем функцию обратного вызова на чтение из события
				io.on(eid, [&](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Выполняем обработку входящей датаграммы
					connection.read(data, size, now());
					// Если соединение установлено и поток приложения ещё не открыт
					if((connection.state() == quic::connection_t::state_t::CONNECTED) && (sid == quic::connection_t::INVALID_STREAM)){
						// Записываем в лог сообщение об установленном соединении
						log.print("Соединение установлено: ID=%u, ALPN=%s", log_t::flag_t::INFO, eid, connection.alpn().c_str());
						// Выполняем открытие двунаправленного потока приложения
						sid = connection.open(false);
						// Если поток приложения открыт
						if(sid != quic::connection_t::INVALID_STREAM){
							// Текст исходящего сообщения
							const string message("Hello from QUIC client!");
							// Отправляем данные с завершением потока (FIN)
							if(connection.send(sid, message, true) == quic::status_t::OK)
								// Записываем в лог сообщение об отправке данных потока
								log.print("Отправлено: ID=%u, Поток=%llu, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, static_cast <unsigned long long> (sid), message.size(), message.c_str());
						// Если поток приложения не открыт
						} else log.print("Ошибка открытия потока приложения: ID=%u", log_t::flag_t::CRITICAL, eid);
					}
					/**
					 * Переходим по всем потокам с собранными данными
					 */
					for(auto & item : connection.readable()){
						// Флаг завершения потока удалённым эндпоинтом
						bool fin = false;
						// Собранные данные потока приложения
						string message = "";
						// Выполняем выдачу собранных данных потока
						if(connection.receive(item, message, fin) != quic::status_t::OK)
							// Пропускаем поток с ошибкой выдачи
							continue;
						// Если данные потока получены
						if(!message.empty())
							// Записываем в лог сообщение о полученном эхо-ответе
							log.print("Прочитано: ID=%u, Поток=%llu, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, static_cast <unsigned long long> (item), message.size(), message.c_str());
						// Если эхо-ответ получен полностью
						if(fin){
							// Записываем в лог сообщение о завершении работы
							log.print("Эхо-ответ получен полностью, завершаем соединение: ID=%u", log_t::flag_t::INFO, eid);
							// Выполняем завершение соединения приложением
							connection.close(0, "goodbye");
						}
					}
					// Отправляем все готовые исходящие датаграммы
					flush(eid);
					// Если локальный эндпоинт завершил соединение
					if(connection.state() == quic::connection_t::state_t::CLOSING)
						// Завершаем работу приложения
						::exit(EXIT_SUCCESS);
				});
				// Устанавливаем функцию обратного вызова на ошибку события
				io.on(eid, [&](const event::id_t eid, [[maybe_unused]] const event::error_t error, const string & description) noexcept -> void {
					// Записываем в лог сообщение об ошибке события
					log.print("Ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
				});
				// Устанавливаем функцию обратного вызова на событие интервала таймеров
				io.on(tid, [&]([[maybe_unused]] const event::id_t tid, const event::status_t status) noexcept -> void {
					// Если статус события успешен
					if(status == event::status_t::SUCCESS){
						// Дедлайн ближайшего события таймера соединения
						const uint64_t timeout = connection.timeout();
						// Если дедлайн таймера соединения наступил
						if((timeout > 0) && (now() >= timeout)){
							// Выполняем обработку просроченных таймеров соединения
							connection.tick(now());
							// Отправляем все готовые исходящие датаграммы
							flush(eid);
						}
					}
				});
				// Выполняем фиксацию настроек событий клиента и интервала таймеров
				if(io.commit(eid) && io.commit(tid)){
					// Выполняем запуск событий клиента и интервала таймеров
					if(io.launch(eid) && io.launch(tid)){
						// Записываем в лог сообщение об успешном запуске события
						cout << " Клиент QUIC успешно запущен!" << endl;
						// Выполняем начало соединения клиентом
						if(connection.connect() == quic::status_t::OK){
							// Отправляем первую датаграмму хендшейка серверу
							flush(eid);
							/**
							 * Запускаем опрос событий
							 */
							while(io.poll());
						// Записываем ошибку в лог начала соединения
						} else cout << " Ошибка начала соединения QUIC!" << endl;
					// Записываем ошибку в лог запуска события
					} else cout << " Ошибка запуска события!" << endl;
				}
			// Если адрес назначения не установлен
			} else cout << " Ошибка установки адреса сервера!" << endl;
		// Если адрес не установлен
		} else cout << " Ошибка установки адреса клиента!" << endl;
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
