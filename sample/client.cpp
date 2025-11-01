/**
 * @file: client.cpp
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

#include <cinttypes>

/**
 * Подключаем заголовочный файл проекта
 */
#include <engine/io.hpp>

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
	// Устанавливаем логгер
	fmk.setLogger(&log);
	// Создаём объект асинхронного движка ввода-вывода
	io_t io(&fmk, &log);
	// Добавляем новое событие клиента TCP
	event::id_t eid = io.event(event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP, event::mode_t::ASYNCHRONOUS);
	// Устанавливаем тип ноды
	io.node(eid, event::node_t::SERVER);
	// Устанавливаем порт события
	io.port(eid, 8080);
	// Устанавливаем адрес события (en0 -> ea:ab:fd:74:1d:0d -> 10.9.5.161)
	// if(io.address(eid, event::address_t::NETWORK, "10.9.5.0/255.255.255.0")){
	// if(io.address(eid, event::address_t::NETWORK, "fe80::105d:12e9:40c7:a76/76")){
	if(io.address(eid, event::address_t::IPV4, "192.168.7.231")){
	// if(io.address(eid, event::address_t::NETWORK, "192.168.7.0/255.255.255.0")){
	// if(io.address(eid, event::address_t::NETWORK, "fe80::1cff:84b4:8614:a918/76")){
	// if(io.address(eid, event::address_t::MAC, "ba:0c:db:93:61:2a")){
	// if(io.address(eid, event::address_t::UDS, "/tmp/awh.sock")){

		cout << " !!!!! " << io.address(eid, event::address_t::MAC) << ":" << io.port(eid) << " !!!!! " << io.host(eid) << endl;

		// cout << " !!!!! " << io.address(eid, event::address_t::UDS) << " !!!!! " << io.host(eid) << endl;

	// Если адрес не установлен
	} else {

		cout << " Ошибка установки адреса события! " << endl;

	}
	// Выводим результат
	return 0;
}
