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
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <units/quic.hpp>
#include <net/tls/coder.hpp>

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
	// Адрес удалённого сервера
	string host = "127.0.0.1";
	// Порт удалённого сервера
	uint32_t port = 2222;
	/**
	 * Размер передаваемого блока данных потока: нулевой размер означает
	 * приветственное сообщение, ненулевой - блок заданной длины для сверки
	 * эха с чужой реализацией транспорта
	 */
	size_t size = 0;
	/**
	 * Путь к файлу состояния возобновления: билет сессии и токен проверки адреса
	 * сохраняются в него после завершения соединения и восстанавливаются перед
	 * следующим подключением (RFC 9001 §4.6, RFC 9000 §8.1.3)
	 */
	string state = "";
	/**
	 * Перебираем параметры командной строки
	 */
	for(int32_t i = 1; i < argc; i++){
		// Если задан адрес удалённого сервера
		if((::strcmp(argv[i], "--host") == 0) && ((i + 1) < argc))
			// Запоминаем адрес удалённого сервера
			host = argv[++i];
		// Если задан порт удалённого сервера
		else if((::strcmp(argv[i], "--port") == 0) && ((i + 1) < argc))
			// Запоминаем порт удалённого сервера
			port = static_cast <uint32_t> (::strtoul(argv[++i], nullptr, 10));
		// Если задан размер передаваемого блока данных потока
		else if((::strcmp(argv[i], "--size") == 0) && ((i + 1) < argc))
			// Запоминаем размер передаваемого блока данных потока
			size = static_cast <size_t> (::strtoull(argv[++i], nullptr, 10));
		// Если задан путь к файлу состояния возобновления
		else if((::strcmp(argv[i], "--state") == 0) && ((i + 1) < argc))
			// Запоминаем путь к файлу состояния возобновления
			state = argv[++i];
		// Если параметр не распознан
		else {
			// Выводим подсказку по параметрам запуска
			cout << " Использование: " << argv[0] << " [--host адрес] [--port порт] [--size байт] [--state файл]" << endl;
			// Выходим из приложения с ошибкой
			return EXIT_FAILURE;
		}
	}
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект кодера транспортной безопасности
	tls::Coder coder(&fmk, &log);
	// Создаём шаблон контекста безопасности протокола QUIC
	const tls::Coder::id_t context = coder.context(event::node_t::CLIENT, event::protocol_t::QUIC);
	// Если шаблон контекста безопасности не создан
	if(context == 0){
		// Записываем в лог сообщение об ошибке
		log.print("Контекст безопасности не создан", log_t::flag_t::CRITICAL);
		// Выходим из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Устанавливаем список поддерживаемых ALPN-протоколов (RFC 9001 §8.1)
	coder.alpn(context, {tls::Coder::alpn_t{0, "h3"}});
	// Устанавливаем доменное имя удалённого сервера
	coder.serverNameIndication(context, "localhost");
	// Снимаем проверку сертификата удалённого сервера для тестового прогона
	coder.validateServerNameIndication(context, false);
	// Создаём модуль клиента транспортного протокола QUIC
	unit::quic_client_t client(&fmk, &log);
	// Устанавливаем шаблон контекста безопасности соединения
	client.context(coder, context);
	// Транспортные параметры клиента
	quic::params::params_t params;
	// Устанавливаем таймаут простоя соединения в миллисекундах
	params.maxIdleTimeout = 30000;
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
	// Устанавливаем предельный размер принимаемой датаграммы приложения (RFC 9221 §3)
	params.maxDatagramFrameSize = 1200;
	// Устанавливаем локальные транспортные параметры соединения
	client.params(params);
	// Включаем возобновление сессии сохранённым билетом (RFC 9001 §4.6)
	client.resume(true);
	// Включаем уведомление о перегрузке пути (RFC 9000 §13.4)
	client.ecn(true);
	/**
	 * Типы функций обратного вызова модуля клиента: хранилище выбирает перегрузку
	 * по типу аргумента, поэтому лямбда передаётся уже завёрнутой
	 */
	using open_t = function <void (const event::id_t)>;
	// Тип функции обратного вызова на собранные данные потока приложения
	using read_t = function <void (const event::id_t, const uint64_t, const string &, const bool)>;
	// Тип функции обратного вызова на принятую датаграмму приложения
	using datagram_t = function <void (const event::id_t, const string &)>;
	// Тип функции обратного вызова на завершённое соединение
	using close_t = function <void (const event::id_t, const quic::error_t)>;
	// Создаём хранилище функций обратного вызова
	callback_t callback(&fmk, &log);
	// Флаг полученного эхо-ответа потока приложения
	bool stream = false;
	// Флаг полученного эхо-ответа датаграммой приложения
	bool datagram = false;
	/**
	 * Передаваемое содержимое потока: при заданном размере это блок с
	 * детерминированным заполнением, по которому эхо сверяется побайтно
	 */
	string payload = "Hello from QUIC client!";
	// Если задан размер передаваемого блока данных потока
	if(size > 0){
		// Резервируем память под передаваемый блок данных потока
		payload.assign(size, '\0');
		/**
		 * Заполняем блок повторяющейся последовательностью: расхождение эха
		 * на любом смещении выявляется сравнением с исходным блоком
		 */
		for(size_t i = 0; i < size; i++)
			// Устанавливаем очередной октет передаваемого блока
			payload[i] = static_cast <char> ('a' + (i % 26));
	}
	// Накопленное содержимое эхо-ответа потока приложения
	string echo = "";
	/**
	 * @brief Функция завершения соединения по получении обоих эхо-ответов
	 *
	 * @param eid идентификатор события клиента
	 */
	auto finish = [&client, &log, &stream, &datagram](const event::id_t eid) noexcept -> void {
		// Если получены оба эхо-ответа
		if(stream && datagram){
			// Записываем в лог сообщение о завершении обмена
			log.print("Оба эхо-ответа получены, завершаем соединение: ID=%u", log_t::flag_t::INFO, eid);
			// Выполняем завершение соединения приложением
			client.close(0, "done");
		}
	};
	// Флаг выполненной отправки запроса приложения
	bool requested = false;
	/**
	 * @brief Функция отправки запроса приложения
	 *
	 * @note Запрос отправляется однократно: он ставится в очередь либо ранними
	 *       данными на возобновлённой сессии, либо по установлении соединения
	 *
	 * @param eid идентификатор события клиента
	 */
	auto request = [&client, &log, &payload, &requested](const event::id_t eid) noexcept -> void {
		// Если запрос приложения уже отправлен
		if(requested)
			// Выходим из функции отправки
			return;
		// Выполняем открытие двунаправленного потока приложения
		const uint64_t sid = client.open(false);
		// Если поток приложения не открыт
		if(sid == quic::connection_t::INVALID_STREAM){
			// Записываем в лог сообщение об ошибке открытия потока
			log.print("Поток приложения не открыт: ID=%u", log_t::flag_t::CRITICAL, eid);
			// Выходим из функции отправки
			return;
		}
		// Устанавливаем флаг выполненной отправки запроса приложения
		requested = true;
		// Отправляем данные с завершением потока (FIN)
		if(client.send(sid, payload, true)){
			// Если передаётся приветственное сообщение
			if(payload.size() < 128)
				// Записываем в лог сообщение об отправке данных потока
				log.print("Отправлено: ID=%u, Поток=%llu, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, static_cast <unsigned long long> (sid), payload.size(), payload.c_str());
			// Записываем в лог сообщение об отправке блока данных потока
			else log.print("Отправлено: ID=%u, Поток=%llu, %zu байт", log_t::flag_t::INFO, eid, static_cast <unsigned long long> (sid), payload.size());
		}
	};
	/**
	 * Устанавливаем функцию обратного вызова на готовность отправки ранних данных:
	 * запрос ставится в очередь до завершения хендшейка и уходит вместе с первым
	 * пакетом, экономя круговую задержку (RFC 9001 §4.6)
	 */
	callback.on <void (const event::id_t)> ("early", open_t([&log, &request](const event::id_t eid) noexcept -> void {
		// Записываем в лог сообщение о готовности к отправке ранних данных
		log.print("Сессия возобновлена, отправляем ранние данные: ID=%u", log_t::flag_t::INFO, eid);
		// Выполняем отправку запроса приложения ранними данными
		request(eid);
	}));
	// Устанавливаем функцию обратного вызова на установленное соединение
	callback.on <void (const event::id_t)> ("open", open_t([&client, &log, &request](const event::id_t eid) noexcept -> void {
		// Записываем в лог сообщение об установленном соединении
		log.print("Соединение установлено: ID=%u, ALPN=%s, ранние данные=%s", log_t::flag_t::INFO, eid, client.alpn().protocol.c_str(), (client.early() ? "приняты" : "нет"));
		// Выполняем отправку запроса приложения, если он не ушёл ранними данными
		request(eid);
		// Текст исходящей датаграммы приложения
		const string unreliable("Hello from QUIC datagram!");
		/**
		 * Отправляем датаграмму приложения: она доставляется вне потоков, ненадёжно
		 * и без гарантии порядка, зато без ожидания повторной отправки (RFC 9221)
		 */
		if(client.datagram(unreliable))
			// Записываем в лог сообщение об отправке датаграммы приложения
			log.print("Отправлена датаграмма: ID=%u, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, unreliable.size(), unreliable.c_str());
		// Записываем в лог сообщение об ошибке отправки датаграммы приложения
		else log.print("Датаграмма приложения не отправлена: ID=%u", log_t::flag_t::WARNING, eid);
	}));
	// Устанавливаем функцию обратного вызова на принятую датаграмму приложения
	callback.on <void (const event::id_t, const string &)> ("datagram", datagram_t([&log, &datagram, &finish](const event::id_t eid, const string & data) noexcept -> void {
		// Записываем в лог сообщение о принятой датаграмме приложения
		log.print("Принята датаграмма: ID=%u, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, data.size(), data.c_str());
		// Устанавливаем флаг полученного эхо-ответа датаграммой
		datagram = true;
		// Выполняем завершение соединения по получении обоих эхо-ответов
		finish(eid);
	}));
	// Устанавливаем функцию обратного вызова на собранные данные потока приложения
	callback.on <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", read_t([&log, &stream, &finish, &payload, &echo](const event::id_t eid, const uint64_t sid, const string & data, const bool fin) noexcept -> void {
		// Если данные потока получены
		if(!data.empty()){
			// Накапливаем принятое содержимое эхо-ответа потока
			echo.append(data);
			// Если передавалось приветственное сообщение
			if(payload.size() < 128)
				// Записываем в лог сообщение о полученных данных потока
				log.print("Прочитано: ID=%u, Поток=%llu, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, static_cast <unsigned long long> (sid), data.size(), data.c_str());
		}
		// Если эхо-ответ потока получен полностью
		if(fin){
			// Записываем в лог сообщение о принятом объёме эхо-ответа
			log.print("Принято эхо: ID=%u, Поток=%llu, %zu из %zu байт", log_t::flag_t::INFO, eid, static_cast <unsigned long long> (sid), echo.size(), payload.size());
			/**
			 * Сверяем принятое эхо с отправленным содержимым: расхождение означает
			 * ошибку сборки потока, а не потерю - потерянное восстанавливается
			 */
			if(echo != payload)
				// Записываем в лог сообщение о расхождении эхо-ответа
				log.print("Эхо не совпадает с отправленным: ID=%u, Поток=%llu", log_t::flag_t::CRITICAL, eid, static_cast <unsigned long long> (sid));
			// Устанавливаем флаг полученного эхо-ответа потока
			stream = true;
			// Выполняем завершение соединения по получении обоих эхо-ответов
			finish(eid);
		}
	}));
	// Устанавливаем функцию обратного вызова на завершённое соединение
	callback.on <void (const event::id_t, const quic::error_t)> ("close", close_t([&client, &log, &state](const event::id_t eid, const quic::error_t error) noexcept -> void {
		// Записываем в лог сообщение о завершении соединения
		log.print("Соединение завершено: ID=%u, Ошибка=%s, Билет=%zu байт", log_t::flag_t::INFO, eid, quic::errorName(error).data(), client.session().size());
		// Если задан файл состояния возобновления
		if(!state.empty()){
			// Если билет возобновления сессии получен от сервера
			if(!client.session().empty()){
				// Открываем файл состояния на запись в двоичном режиме
				ofstream file(state, ios::binary | ios::trunc);
				// Если файл состояния открыт
				if(file.is_open())
					// Сохраняем билет возобновления сессии
					file.write(client.session().data(), static_cast <streamsize> (client.session().size()));
			}
			// Если токен проверки адреса получен от сервера
			if(!client.token().empty()){
				// Открываем файл токена на запись в двоичном режиме
				ofstream file(state + ".token", ios::binary | ios::trunc);
				// Если файл токена открыт
				if(file.is_open())
					// Сохраняем токен проверки адреса
					file.write(client.token().data(), static_cast <streamsize> (client.token().size()));
			}
		}
		/**
		 * Останавливаем модуль клиента: соединение завершено окончательно - период
		 * завершения выдержан, и работать модулю больше не над чем (RFC 9000 §10.2)
		 */
		client.stop();
	}));
	// Устанавливаем функции обратного вызова модуля клиента
	client.callback(callback);
	/**
	 * @brief Функция чтения файла состояния возобновления целиком
	 *
	 * @param filename путь к файлу состояния
	 * @return         содержимое файла либо пустая строка
	 */
	auto load = [](const string & filename) noexcept -> string {
		// Открываем файл состояния на чтение в двоичном режиме
		ifstream file(filename, ios::binary);
		// Если файл состояния не открыт
		if(!file.is_open())
			// Выводим пустое содержимое
			return "";
		// Буфер чтения содержимого файла
		ostringstream buffer;
		// Считываем содержимое файла целиком
		buffer << file.rdbuf();
		// Выводим считанное содержимое
		return buffer.str();
	};
	// Если задан файл состояния возобновления
	if(!state.empty()){
		// Считываем сохранённый билет возобновления сессии
		const string ticket = load(state);
		// Считываем сохранённый токен проверки адреса
		const string address = load(state + ".token");
		// Если билет возобновления сессии сохранён
		if(!ticket.empty()){
			// Устанавливаем билет возобновления сессии
			client.session(ticket);
			// Записываем в лог сообщение о восстановленном билете
			log.print("Восстановлен билет возобновления: %zu байт", log_t::flag_t::INFO, ticket.size());
		}
		// Если токен проверки адреса сохранён
		if(!address.empty()){
			// Устанавливаем токен проверки адреса
			client.token(address);
			// Записываем в лог сообщение о восстановленном токене
			log.print("Восстановлен токен проверки адреса: %zu байт", log_t::flag_t::INFO, address.size());
		}
	}
	// Если подключение к удалённому серверу не выполнено
	if(client.connect(event::family_t::IPV4, host, port) == 0){
		// Записываем в лог сообщение об ошибке подключения
		log.print("Клиент QUIC не подключён", log_t::flag_t::CRITICAL);
		// Выходим из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Записываем в лог сообщение об успешном запуске клиента
	cout << " Клиент QUIC успешно запущен!" << endl;
	// Выполняем запуск модуля клиента
	client.start();
	// Возвращаем результат
	return EXIT_SUCCESS;
}
