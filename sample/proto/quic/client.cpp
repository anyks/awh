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
	// Устанавливаем функцию обратного вызова на установленное соединение
	callback.on <void (const event::id_t)> ("open", open_t([&client, &log](const event::id_t eid) noexcept -> void {
		// Записываем в лог сообщение об установленном соединении
		log.print("Соединение установлено: ID=%u, ALPN=%s", log_t::flag_t::INFO, eid, client.alpn().protocol.c_str());
		// Выполняем открытие двунаправленного потока приложения
		const uint64_t sid = client.open(false);
		// Если поток приложения не открыт
		if(sid == quic::connection_t::INVALID_STREAM){
			// Записываем в лог сообщение об ошибке открытия потока
			log.print("Поток приложения не открыт: ID=%u", log_t::flag_t::CRITICAL, eid);
			// Выходим из функции обработки
			return;
		}
		// Текст исходящего сообщения
		const string message("Hello from QUIC client!");
		// Отправляем данные с завершением потока (FIN)
		if(client.send(sid, message, true))
			// Записываем в лог сообщение об отправке данных потока
			log.print("Отправлено: ID=%u, Поток=%llu, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, static_cast <unsigned long long> (sid), message.size(), message.c_str());
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
	callback.on <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", read_t([&log, &stream, &finish](const event::id_t eid, const uint64_t sid, const string & data, const bool fin) noexcept -> void {
		// Если данные потока получены
		if(!data.empty())
			// Записываем в лог сообщение о полученных данных потока
			log.print("Прочитано: ID=%u, Поток=%llu, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, static_cast <unsigned long long> (sid), data.size(), data.c_str());
		// Если эхо-ответ потока получен полностью
		if(fin){
			// Устанавливаем флаг полученного эхо-ответа потока
			stream = true;
			// Выполняем завершение соединения по получении обоих эхо-ответов
			finish(eid);
		}
	}));
	// Устанавливаем функцию обратного вызова на завершённое соединение
	callback.on <void (const event::id_t, const quic::error_t)> ("close", close_t([&client, &log](const event::id_t eid, const quic::error_t error) noexcept -> void {
		// Записываем в лог сообщение о завершении соединения
		log.print("Соединение завершено: ID=%u, Ошибка=%s, Билет=%zu байт", log_t::flag_t::INFO, eid, quic::errorName(error).data(), client.session().size());
		/**
		 * Останавливаем модуль клиента: соединение завершено окончательно - период
		 * завершения выдержан, и работать модулю больше не над чем (RFC 9000 §10.2)
		 */
		client.stop();
	}));
	// Устанавливаем функции обратного вызова модуля клиента
	client.callback(callback);
	// Если подключение к удалённому серверу не выполнено
	if(client.connect(event::family_t::IPV4, "127.0.0.1", 2222) == 0){
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
