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
	const tls::Coder::id_t context = coder.context(event::node_t::SERVER, event::protocol_t::QUIC);
	// Если шаблон контекста безопасности не создан
	if(context == 0){
		// Записываем в лог сообщение об ошибке
		log.print("Контекст безопасности не создан", log_t::flag_t::CRITICAL);
		// Выходим из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Устанавливаем список поддерживаемых ALPN-протоколов (RFC 9001 §8.1)
	coder.alpn(context, {tls::Coder::alpn_t{0, "h3"}});
	// Устанавливаем сертификат сервера
	coder.certificate(context, "../sh/certificates/server/cert.pem");
	// Устанавливаем приватный ключ сервера
	coder.privateKey(context, "../sh/certificates/server/key.pem");
	/**
	 * Снимаем проверку сертификата удалённого узла: шаблон контекста создаётся
	 * с включённой проверкой, а на серверном узле это означает требование
	 * клиентского сертификата, то есть взаимную аутентификацию
	 */
	coder.validateServerNameIndication(context, false);
	// Создаём модуль сервера транспортного протокола QUIC
	unit::quic_server_t server(&fmk, &log);
	// Устанавливаем шаблон контекста безопасности соединений
	server.context(coder, context);
	// Транспортные параметры сервера
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
	// Устанавливаем локальные транспортные параметры соединений
	server.params(params);
	// Включаем проверку адреса клиента через пакет Retry (RFC 9000 §8.1.2)
	server.retry(true);
	// Включаем уведомление о перегрузке пути (RFC 9000 §13.4)
	server.ecn(true);
	/**
	 * Типы функций обратного вызова модуля сервера: хранилище выбирает перегрузку
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
	// Устанавливаем функцию обратного вызова на установленное соединение
	callback.on <void (const event::id_t)> ("open", open_t([&server, &log](const event::id_t oid) noexcept -> void {
		// Записываем в лог сообщение об установленном соединении
		log.print("Соединение установлено: ID=%u, Адрес=%s, ALPN=%s", log_t::flag_t::INFO, oid, server.address(oid).c_str(), server.alpn(oid).protocol.c_str());
	}));
	// Устанавливаем функцию обратного вызова на собранные данные потока приложения
	callback.on <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", read_t([&server, &log](const event::id_t oid, const uint64_t sid, const string & data, const bool fin) noexcept -> void {
		// Если данные потока получены
		if(!data.empty())
			// Записываем в лог сообщение о полученных данных потока
			log.print("Прочитано: ID=%u, Поток=%llu, %zu байт, сообщение: %s", log_t::flag_t::INFO, oid, static_cast <unsigned long long> (sid), data.size(), data.c_str());
		// Отправляем данные обратно клиенту в тот же поток
		if(server.send(oid, sid, data, fin))
			// Записываем в лог сообщение об отправке эхо-ответа
			log.print("Отправлено: ID=%u, Поток=%llu, %zu байт, FIN=%s", log_t::flag_t::INFO, oid, static_cast <unsigned long long> (sid), data.size(), (fin ? "да" : "нет"));
	}));
	// Устанавливаем функцию обратного вызова на принятую датаграмму приложения
	callback.on <void (const event::id_t, const string &)> ("datagram", datagram_t([&server, &log](const event::id_t oid, const string & data) noexcept -> void {
		// Записываем в лог сообщение о принятой датаграмме приложения
		log.print("Принята датаграмма: ID=%u, %zu байт, сообщение: %s", log_t::flag_t::INFO, oid, data.size(), data.c_str());
		// Отправляем датаграмму обратно клиенту
		if(server.datagram(oid, data))
			// Записываем в лог сообщение об отправке эхо-ответа датаграммой
			log.print("Отправлена датаграмма: ID=%u, %zu байт", log_t::flag_t::INFO, oid, data.size());
	}));
	// Устанавливаем функцию обратного вызова на завершённое соединение
	callback.on <void (const event::id_t, const quic::error_t)> ("close", close_t([&log](const event::id_t oid, const quic::error_t error) noexcept -> void {
		// Записываем в лог сообщение о завершении соединения
		log.print("Соединение завершено: ID=%u, Ошибка=%s", log_t::flag_t::INFO, oid, quic::errorName(error).data());
	}));
	// Устанавливаем функции обратного вызова модуля сервера
	server.callback(callback);
	// Если запуск сервера соединений не выполнен
	if(server.listen(event::family_t::IPV4, "127.0.0.1", 2222) == 0){
		// Записываем в лог сообщение об ошибке запуска сервера
		log.print("Сервер QUIC не запущен", log_t::flag_t::CRITICAL);
		// Выходим из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Записываем в лог сообщение об успешном запуске сервера
	cout << " Сервер QUIC успешно запущен!" << endl;
	// Выполняем запуск модуля сервера
	server.start();
	// Возвращаем результат
	return EXIT_SUCCESS;
}
