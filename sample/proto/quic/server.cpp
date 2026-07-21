/**
 * @file: server.cpp
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
#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/io.hpp>
#include <sys/fs.hpp>
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
	// Создаём объект для работы с файловой системой
	fs_t fs(&fmk, &log);
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Выполняем чтение сертификата сервера в формате PEM
	const string certificate = fs.read <string> ("../sh/certificates/server/cert.pem");
	// Выполняем чтение приватного ключа сервера в формате PEM
	const string privateKey = fs.read <string> ("../sh/certificates/server/key.pem");
	// Если сертификат или приватный ключ не прочитаны
	if(certificate.empty() || privateKey.empty()){
		// Записываем в лог сообщение об ошибке чтения сертификата
		log.print("Не удалось прочитать сертификат или приватный ключ сервера", log_t::flag_t::CRITICAL);
		// Выходим из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Список соединений QUIC по идентификаторам событий клиентов
	std::map <event::id_t, std::unique_ptr <quic::connection_t>> connections;
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
	 * @param eid        идентификатор события клиента
	 * @param connection соединение QUIC клиента
	 */
	auto flush = [&io, &log, &now](const event::id_t eid, quic::connection_t & connection) noexcept -> void {
		// Буфер исходящей датаграммы
		string datagram = "";
		/**
		 *  Извлекаем исходящие датаграммы соединения
		 */
		while(connection.write(datagram, now())){
			// Отправляем датаграмму клиенту
			if(!io.send(eid, datagram.data(), datagram.size()))
				// Записываем в лог сообщение об ошибке отправки датаграммы
				log.print("Ошибка отправки датаграммы: ID=%u", log_t::flag_t::CRITICAL, eid);
		}
	};
	/**
	 * @brief Функция обработки собранных данных потоков приложения (эхо-ответ)
	 *
	 * @param eid        идентификатор события клиента
	 * @param connection соединение QUIC клиента
	 */
	auto process = [&log](const event::id_t eid, quic::connection_t & connection) noexcept -> void {
		/**
		 * Переходим по всем потокам с собранными данными
		 */
		for(auto & sid : connection.readable()){
			// Флаг завершения потока удалённым эндпоинтом
			bool fin = false;
			// Собранные данные потока приложения
			string data = "";
			// Выполняем выдачу собранных данных потока
			if(connection.receive(sid, data, fin) != quic::status_t::OK)
				// Пропускаем поток с ошибкой выдачи
				continue;
			// Если данные потока получены
			if(!data.empty())
				// Записываем в лог сообщение о полученных данных потока
				log.print("Прочитано: ID=%u, Поток=%llu, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, static_cast <unsigned long long> (sid), data.size(), data.c_str());
			// Если данные получены либо поток завершён, отправляем эхо-ответ
			if(!data.empty() || fin){
				// Отправляем данные обратно клиенту в тот же поток
				if(connection.send(sid, data, fin) == quic::status_t::OK)
					// Записываем в лог сообщение об отправке эхо-ответа
					log.print("Отправлено: ID=%u, Поток=%llu, %zu байт, FIN=%s", log_t::flag_t::INFO, eid, static_cast <unsigned long long> (sid), data.size(), (fin ? "да" : "нет"));
			}
		}
	};
	// Добавляем новое событие сервера UDP
	event::id_t eid = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Добавляем новое событие интервала таймеров соединений
	event::id_t tid = io.event(event::node_t::INTERVAL, event::family_t::TIMER);
	// Устанавливаем порт события
	io.setSourcePort(eid, 2222);
	// Устанавливаем интервал проверки таймеров соединений
	io.setTimeout(tid, event::action_t::NONE, 25);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устананавливаем опции события
		if(!io.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
			// Записываем ошибку в лог установки опций события
			cout << " Ошибка установки опций события!" << endl;
		// Устанавливаем IP-адрес события
		if(io.setAddress(eid, event::address_t::IPV4, "127.0.0.1")){
			// Устанавливаем функцию обратного вызова на подключение нового клиента
			io.on(eid, static_cast <engine::callback::accept_t> ([&](const event::id_t eid, const event::id_t cid) noexcept -> void {
				// Записываем в лог сообщение о принятии нового клиента
				log.print("Новый клиент: ID=%u, Клиентский ID=%u, IP=%s, PORT=%d", log_t::flag_t::INFO, eid, cid, io.getAddress(cid, event::address_t::IPV4).c_str(), io.getSourcePort(cid));
				// Создаём соединение QUIC для нового клиента
				auto connection = std::make_unique <quic::connection_t> (quic::endpoint_t::SERVER);
				// Устанавливаем список поддерживаемых ALPN-протоколов
				connection->alpn({"h3"});
				// Устанавливаем сертификат и приватный ключ сервера
				connection->certificate(certificate, privateKey);
				// Транспортные параметры сервера
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
				connection->params(params);
				// Добавляем соединение в список соединений
				connections.emplace(cid, ::move(connection));
				// Устанавливаем функцию обратного вызова на чтение из события
				io.on(cid, [&](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Выполняем поиск соединения клиента
					auto i = connections.find(eid);
					// Если соединение клиента не найдено
					if(i == connections.end())
						// Выходим из функции обработки
						return;
					// Выполняем обработку входящей датаграммы
					i->second->read(data, size, now());
					// Выполняем обработку собранных данных потоков приложения
					process(eid, * i->second);
					// Отправляем все готовые исходящие датаграммы
					flush(eid, * i->second);
					// Если удалённый эндпоинт завершил соединение
					if(i->second->state() == quic::connection_t::state_t::DRAINING){
						// Записываем в лог сообщение о завершении соединения
						log.print("Соединение завершено клиентом: ID=%u", log_t::flag_t::INFO, eid);
						// Удаляем соединение из списка соединений
						connections.erase(i);
					}
				});
				// Устанавливаем функцию обратного вызова на ошибку события
				io.on(cid, [&](const event::id_t eid, [[maybe_unused]] const event::error_t error, const string & description) noexcept -> void {
					// Записываем в лог сообщение об ошибке события
					log.print("Ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
				});
			}));
			// Устанавливаем функцию обратного вызова на ошибку события
			io.on(eid, [&](const event::id_t eid, [[maybe_unused]] const event::error_t error, const string & description) noexcept -> void {
				// Записываем в лог сообщение об ошибке события
				log.print("Ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
			});
			// Устанавливаем функцию обратного вызова на событие интервала таймеров
			io.on(tid, [&]([[maybe_unused]] const event::id_t eid, const event::status_t status) noexcept -> void {
				// Если статус события успешен
				if(status == event::status_t::SUCCESS){
					// Текущее время в миллисекундах
					const uint64_t date = now();
					/**
					 * Переходим по всем соединениям клиентов
					 */
					for(auto & connection : connections){
						// Дедлайн ближайшего события таймера соединения
						const uint64_t timeout = connection.second->timeout();
						// Если дедлайн таймера соединения наступил
						if((timeout > 0) && (date >= timeout)){
							// Выполняем обработку просроченных таймеров соединения
							connection.second->tick(date);
							// Отправляем все готовые исходящие датаграммы
							flush(connection.first, * connection.second);
						}
					}
				}
			});
			// Выполняем фиксацию настроек события сервера
			if(io.commit(eid) && io.commit(tid)){
				// Выполняем запуск событий сервера и интервала таймеров
				if(io.launch(eid) && io.launch(tid)){
					// Записываем в лог сообщение об успешном запуске события
					cout << " Сервер QUIC успешно запущен!" << endl;
					/**
					 * Запускаем опрос событий
					 */
					while(io.poll());
				// Записываем ошибку в лог запуска события
				} else cout << " Ошибка запуска события!" << endl;
			}
		// Если адрес не установлен
		} else cout << " Ошибка установки адреса события!" << endl;
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
