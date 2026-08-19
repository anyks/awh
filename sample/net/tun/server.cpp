/**
 * @file server.cpp
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
 * @brief Пример сервера виртуального туннеля — демонстрация создания интерфейса TUN,
 *        приёма подключений клиентов и маршрутизации IP-пакетов между ними
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Для работы сервера:
 * # Назначаем адрес 10.0.0.1, destination 10.0.0.1 (Point-to-Point to self)
 * $ sudo ifconfig utun7 10.0.0.1 10.0.0.1 netmask 255.255.255.255 up
 *
 * macOS:
 * # Весь трафик для 10.0.0.x отправлять в интерфейс utun7 (сетевой интерфейс сервера)
 * $ sudo route -n add -net 10.0.0.0/24 -interface utun7
 *
 * FreeBSD:
 * # Синтаксис route net практически идентичен
 * $ sudo route add -net 10.0.0.0/24 -interface tun0
 *
 * Remove:
 * $ sudo route delete -net 10.0.0.0/24 -interface utun7
 */

/**
 * Стандартные модули
 */
#include <iostream>
#include <csignal>
#include <atomic>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>
#include <net/addr.hpp>
#include <net/eth/eth.hpp>
#include <net/eth/gateway.hpp>

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
/**
 * Признак остановки работы
 *
 * @note Снимается сигналом. Без штатного выхода разбор объектов не выполняется
 *       вовсе: устройство туннеля у NetBSD и OpenBSD закрытием дескриптора НЕ
 *       сносится и остаётся в системе вместе с адресами, перехватывая маршрут
 *       у следующего запуска
 *
 */
static std::atomic_bool __awh_stop__{false};

/**
 * @brief Функция обработки сигнала остановки
 *
 * @param signal номер полученного сигнала
 *
 */
static void onSignal(int32_t signal) noexcept {
	// Отмечаем работу подлежащей остановке
	(void) signal;
	__awh_stop__.store(true);
}

/**
 * @brief Параметры запуска сервера туннеля
 *
 * @details Значения по умолчанию оставлены теми же, какими они были записаны в
 *          коде: запуск без единого довода ведёт себя ровно как прежде
 *
 */
typedef struct Params {
	// Адрес своей стороны туннеля
	string tun;
	// Адрес другой стороны туннеля
	string peer;
	// Локальный адрес переноса
	string bind;
	// Сеть маршрута через туннель
	string net;
	// Порт переноса
	uint16_t port;
	// Префикс сети маршрута
	uint8_t prefix;
	// Название готового устройства туннеля
	string iface;
	// Признак поднятия устройства туннеля
	bool up;
	// Признак установки маршрута через туннель
	bool route;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Params() noexcept :
	 tun{"10.0.0.1"}, peer{"10.0.0.2"}, bind{"127.0.0.1"},
	 net{"10.0.0.0"}, iface{""}, port(2222), prefix(24), up(true), route(true) {}
} params_t;

/**
 * @brief Функция печати способа запуска
 *
 * @param name название исполняемого файла
 *
 */
static void usage(const char * name) noexcept {
	cout << "Сервер туннеля AWH" << endl << endl;
	cout << "Запуск: " << name << " [доводы]" << endl << endl;
	cout << "  --bind <адрес>    локальный адрес переноса (по умолчанию 127.0.0.1)" << endl;
	cout << "  --port <номер>    порт переноса (по умолчанию 2222)" << endl;
	cout << "  --tun <адрес>     адрес своей стороны туннеля (по умолчанию 10.0.0.1)" << endl;
	cout << "  --peer <адрес>    адрес другой стороны туннеля (по умолчанию 10.0.0.2)" << endl;
	cout << "  --net <адрес>     сеть маршрута через туннель (по умолчанию 10.0.0.0)" << endl;
	cout << "  --prefix <длина>  префикс сети маршрута (по умолчанию 24)" << endl;
	cout << "  --no-route        не ставить маршрут через туннель" << endl;
	cout << "  --no-up           не поднимать устройство туннеля" << endl;
	cout << "  --iface <имя>     готовое устройство туннеля (обязательно у Solaris и illumos:" << endl;
	cout << "                    связи там заводятся административно, например dladm)" << endl;
	cout << "  --help            напечатать этот текст" << endl << endl;
	cout << "Права суперпользователя обязательны: заведение устройства туннеля их требует." << endl;
}

/**
 * @brief Функция разбора доводов запуска
 *
 * @param argc   длина массива параметров
 * @param argv   массив параметров
 * @param params параметры запуска
 * @return       результат разбора
 *
 */
static bool parse(int32_t argc, char * argv[], params_t & params) noexcept {
	/**
	 * Перебираем все переданные доводы
	 */
	for(int32_t i = 1; i < argc; i++){
		// Получаем название довода
		const string arg(argv[i]);
		// Если запрошен способ запуска
		if(arg.compare("--help") == 0){
			// Печатаем способ запуска
			usage(argv[0]);
			// Выводим отказ: работать дальше незачем
			return false;
		// Если запрошено не ставить маршрут
		} else if(arg.compare("--no-route") == 0)
			// Отключаем установку маршрута
			params.route = false;
		// Если запрошено не поднимать устройство
		else if(arg.compare("--no-up") == 0)
			// Отключаем поднятие устройства
			params.up = false;
		// Иначе довод требует значения
		else {
			// Если значение довода не передано
			if((i + 1) >= argc){
				// Сообщаем о доводе без значения
				cout << "Довод \"" << arg << "\" требует значения" << endl;
				// Выводим отказ
				return false;
			}
			// Получаем значение довода
			const string value(argv[++i]);
			// Разбираем довод по названию
			if(arg.compare("--bind") == 0) params.bind = value;
			else if(arg.compare("--port") == 0) params.port = static_cast <uint16_t> (::stoi(value));
			else if(arg.compare("--tun") == 0) params.tun = value;
			else if(arg.compare("--peer") == 0) params.peer = value;
			else if(arg.compare("--net") == 0) params.net = value;
			else if(arg.compare("--iface") == 0) params.iface = value;
			else if(arg.compare("--prefix") == 0) params.prefix = static_cast <uint8_t> (::stoi(value));
			// Если довод неизвестен
			else {
				// Сообщаем о неизвестном доводе
				cout << "Довод \"" << arg << "\" неизвестен" << endl;
				// Печатаем способ запуска
				usage(argv[0]);
				// Выводим отказ
				return false;
			}
		}
	}
	// Выводим успешный результат
	return true;
}

int32_t main(int32_t argc, char * argv[]){
	// Параметры запуска
	params_t params;
	// Выполняем разбор доводов запуска
	if(!parse(argc, argv, params))
		// Выводим отказ
		return 1;
	// Устанавливаем обработчики сигналов остановки
	::signal(SIGINT, onSignal);
	::signal(SIGTERM, onSignal);
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Создаём объект работы с сетевыми адресами
	net_addr_t addr(&fmk, &log);
	// Создаём объект работы с шлюзами
	eth::gateway_t gateway(&fmk, &log);
	// Добавляем новое событие туннеля
	event::id_t tid = io.event(event::node_t::TUNNEL, event::family_t::IPV4);
	// Добавляем новое событие посредника
	event::id_t mid = io.event(event::node_t::MEDIATOR, event::family_t::IPV4);
	// Добавляем новое событие сервера UDP
	event::id_t eid = io.event(event::node_t::SERVER, event::family_t::IPV4, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Устанавливаем порт события
	io.setSourcePort(eid, params.port);
	/**
	 * Задаём готовое устройство туннеля, если оно названо
	 *
	 * @note Linux и BSD заводят устройство сами и имя выдают. У Solaris и illumos
	 *       связи канального уровня заводятся административно, движку остаётся
	 *       открыть готовую по имени - и без имени он отказывает прямо. Имя
	 *       задаётся узлу, а к делу его приводит пересоздание устройства
	 */
	if(!params.iface.empty()){
		// Задаём название устройства туннеля
		if(io.setIface(tid, params.iface)){
			// Пересоздаём устройство туннеля по названному имени
			if(io.rebuild(tid))
				// Записываем в лог сообщение об успешном заведении устройства
				cout << " Устройство туннеля \"" << params.iface << "\" заведено!" << endl;
			// Записываем ошибку в лог заведения устройства
			else cout << " Ошибка заведения устройства туннеля \"" << params.iface << "\"!" << endl;
		// Записываем ошибку в лог установки названия устройства
		} else cout << " Ошибка установки названия устройства туннеля!" << endl;
	}
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устананавливаем опции события туннеля
		if(io.setOptions(tid, event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC))
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Успешно установлены опции события туннеля!" << endl;
		// Записываем ошибку в лог установки опций события
		else cout << " Ошибка установки опций события туннеля!" << endl;
		// Устананавливаем опции события
		if(io.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::AUTO_RECONNECT))
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Успешно установлены опции события сервера! " << endl;
		// Записываем ошибку в лог установки опций события
		else cout << " Ошибка установки опций события сервера!" << endl;
		// Устанавливаем IP-адрес события
		if(io.setAddress(eid, event::address_t::IPV4, params.bind) && io.setAddress(tid, event::address_t::IPV4, params.tun)){
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(mid, params.peer)){
				// Устанавливаем функцию обратного вызова на событие сервера
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
				// Устанавливаем функцию обратного вызова на событие туннеля
				io.on(tid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Записываем в лог сообщение о принятии события
							log.print("Событие туннеля принято: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Записываем в лог сообщение об уничтожении события
							log.print("Событие туннеля подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Записываем в лог сообщение об инициализации события
							log.print("Событие туннеля инициализировано: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::LAUNCHED):
							// Записываем в лог сообщение о запуске события
							log.print("Событие туннеля запущено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Записываем в лог сообщение о паузе события
							log.print("Событие туннеля на паузе: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Записываем в лог сообщение о возобновлении события
							log.print("Событие туннеля возобновлено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Записываем в лог сообщение о успешном выполнении события
							log.print("Событие туннеля успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Записываем в лог сообщение о неудачном выполнении события
							log.print("Событие туннеля выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Записываем в лог сообщение о выполнении события в ожидании
							log.print("Событие туннеля в ожидании: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Записываем в лог сообщение о подключении события
							log.print("Событие туннеля подключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Записываем в лог сообщение об отмене события
							log.print("Событие туннеля отменено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Записываем в лог сообщение о переподключении события
							log.print("Событие туннеля переподключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус прослушивания события
						case static_cast <uint8_t> (event::status_t::LISTENING):
							// Записываем в лог сообщение о прослушивании события
							log.print("Событие туннеля прослушивается: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на событие посредника
				io.on(mid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Записываем в лог сообщение о принятии события
							log.print("Событие посредника принято: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Записываем в лог сообщение об уничтожении события
							log.print("Событие посредника подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Записываем в лог сообщение об инициализации события
							log.print("Событие посредника инициализировано: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::LAUNCHED):
							// Записываем в лог сообщение о запуске события
							log.print("Событие посредника запущено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Записываем в лог сообщение о паузе события
							log.print("Событие посредника на паузе: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Записываем в лог сообщение о возобновлении события
							log.print("Событие посредника возобновлено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Записываем в лог сообщение о успешном выполнении события
							log.print("Событие посредника успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Записываем в лог сообщение о неудачном выполнении события
							log.print("Событие посредника выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Записываем в лог сообщение о выполнении события в ожидании
							log.print("Событие посредника в ожидании: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Записываем в лог сообщение о подключении события
							log.print("Событие посредника подключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Записываем в лог сообщение об отмене события
							log.print("Событие посредника отменено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Записываем в лог сообщение о переподключении события
							log.print("Событие посредника переподключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус прослушивания события
						case static_cast <uint8_t> (event::status_t::LISTENING):
							// Записываем в лог сообщение о прослушивании события
							log.print("Событие посредника прослушивается: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на подключение нового клиента
				io.on(eid, static_cast <engine::callback::accept_t> ([tid, mid, &io, &log](const event::id_t eid, const event::id_t cid) noexcept -> void {
					// Объединяем с пиром
					io.splice(mid, cid);
					// Записываем в лог сообщение о принятии события
					log.print("Событие принято: ID=%u, Клиентский ID=%u, IP=%s, PORT=%d", log_t::flag_t::INFO, eid, cid, io.getAddress(cid, event::address_t::IPV4).c_str(), io.getSourcePort(cid));
					// Устанавливаем функцию обратного вызова на изменение статуса события
					io.on(cid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
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
					io.on(cid, static_cast <engine::callback::write_t> ([&log](const event::id_t eid, const size_t size) noexcept -> void {
						// Записываем в лог сообщение о записи данных
						log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
					}));
					// Устанавливаем функцию обратного вызова на чтение из события
					io.on(cid, [tid, &io, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
						// Текст входящего сообщения
						const string message(reinterpret_cast <const char *> (data), size);
						// Записываем в лог сообщение о чтении данных
						log.print("Прочитано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
						// Отправляем данные в туннель
						if(io.send(tid, reinterpret_cast <const char *> (data), size))
							// Если данные успешно отправлены
							log.print("Отправлено в туннель: ID=%u, %zu байт", log_t::flag_t::INFO, tid, size);
						// Если данные не отправлены
						else log.print("Ошибка отправки в туннель: ID=%u", log_t::flag_t::CRITICAL, tid);
					});
					// Устанавливаем функцию обратного вызова на ошибку события
					io.on(cid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
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
							// Если ошибка доступа к сокету
							case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
								// Записываем ошибку в лог доступа к сокету
								log.print("Ошибка доступа к сокету события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
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
					io.on(cid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
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
				}));
				// Устанавливаем функцию обратного вызова на чтение из события посредника
				io.on(mid, [&io, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const string message(reinterpret_cast <const char *> (data), size);
					// Записываем в лог сообщение о чтении данных
					log.print("Прочитано из посредника: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
					// Отправляем данные обратно клиенту
					if(io.send(eid, reinterpret_cast <const char *> (data), size))
						// Если данные успешно отправлены
						log.print("Отправлено в сервер: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else log.print("Ошибка отправки в сервер: ID=%u", log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на ошибку события сервера
				io.on(eid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Записываем ошибку в лог неизвестного события
							log.print("Неизвестная ошибка события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Записываем ошибку в лог недопустимой операции
							log.print("Недопустимая операция события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Записываем ошибку в лог доступа запрещёния
							log.print("Доступ к событию сервера запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Записываем ошибку в лог уже существующего объекта
							log.print("Объект события сервера уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Записываем ошибку в лог доступа к сокету
							log.print("Ошибка доступа к сокету события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Записываем ошибку в лог некорректного адреса
							log.print("Некорректный адрес события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Записываем ошибку в лог подключения
							log.print("Ошибка подключения события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Записываем ошибку в лог недостаточно ресурсов
							log.print("Недостаточно ресурсов для события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Записываем ошибку в лог события
							log.print("Ошибка события сервера: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Записываем ошибку в лог события
							log.print("Объект события сервера не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на ошибку события туннеля
				io.on(tid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Записываем ошибку в лог неизвестного события
							log.print("Неизвестная ошибка события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Записываем ошибку в лог недопустимой операции
							log.print("Недопустимая операция события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Записываем ошибку в лог доступа запрещёния
							log.print("Доступ к событию туннеля запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Записываем ошибку в лог уже существующего объекта
							log.print("Объект события туннеля уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Записываем ошибку в лог доступа к сокету
							log.print("Ошибка доступа к сокету события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Записываем ошибку в лог некорректного адреса
							log.print("Некорректный адрес события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Записываем ошибку в лог подключения
							log.print("Ошибка подключения события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Записываем ошибку в лог недостаточно ресурсов
							log.print("Недостаточно ресурсов для события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Записываем ошибку в лог события
							log.print("Ошибка события туннеля: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Записываем ошибку в лог события
							log.print("Объект события туннеля не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на получение информации пакетов в туннеле
				io.on(tid, [&addr, &fmk, &log](const event::id_t tid, const event::id_t mid, const event::action_t action, const net::tun_info_t & info) noexcept -> void {
					// Количество хопов до процесса
					string hops = "";
					// Семейство адресов процесса
					string family = "";
					// Протокол процесса
					string protocol = "";
					// Адрес источника процесса
					string source = "";
					// Адрес назначения процесса
					string destination = "";
					/**
					 * Определяем семейстов адресов сокета
					 */
					switch(static_cast <uint8_t> (info.family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4):
							// Устанавливаем семейство адресов процесса
							family = "IPv4";
						break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6):
							// Устанавливаем семейство адресов процесса
							family = "IPv6";
						break;
					}
					/**
					 * Определяем количество хопов до процесса
					 */
					switch(static_cast <uint8_t> (info.hops)){
						case static_cast <uint8_t> (event::hops_t::LOOPBACK):
							// Устанавливаем количество хопов до процесса
							hops = "LOOPBACK";
						break;
						case static_cast <uint8_t> (event::hops_t::NETWORK):
							// Устанавливаем количество хопов до процесса
							hops = "NETWORK";
						break;
						case static_cast <uint8_t> (event::hops_t::COMPANY):
							// Устанавливаем количество хопов до процесса
							hops = "COMPANY";
						break;
						case static_cast <uint8_t> (event::hops_t::REGION):
							// Устанавливаем количество хопов до процесса
							hops = "REGION";
						break;
						case static_cast <uint8_t> (event::hops_t::CONTINENT):
							// Устанавливаем количество хопов до процесса
							hops = "CONTINENT";
						break;
						case static_cast <uint8_t> (event::hops_t::WORLD):
							// Устанавливаем количество хопов до процесса
							hops = "WORLD";
						break;
					}
					/**
					 * Определяем протокол сокета
					 */
					switch(static_cast <uint8_t> (info.protocol)){
						// Если протокол сокета является TCP-протоколом
						case static_cast <uint8_t> (event::protocol_t::TCP):
							// Устанавливаем протокол сокета
							protocol = "TCP";
						break;
						// Если протокол сокета является UDP-протоколом
						case static_cast <uint8_t> (event::protocol_t::UDP):
							// Устанавливаем протокол сокета
							protocol = "UDP";
						break;
						// Если протокол сокета является ICMP-протоколом
						case static_cast <uint8_t> (event::protocol_t::ICMP):
							// Устанавливаем протокол сокета
							protocol = "ICMP";
						break;
						// Если протокол сокета является IGMP-протоколом
						case static_cast <uint8_t> (event::protocol_t::IGMP):
							// Устанавливаем протокол сокета
							protocol = "IGMP";
						break;
						// Если протокол сокета является SCTP-протоколом
						case static_cast <uint8_t> (event::protocol_t::SCTP):
							// Устанавливаем протокол сокета
							protocol = "SCTP";
						break;
						// Если протокол сокета не определён
						default : protocol = "NONE";
					}
					// Получаем объект для хранения информации о сетевом адресе назначения
					net::attr_net_t * targetAddress = awh_cast <net::attr_net_t *> (info.target.get());
					// Получаем объект для хранения информации о сетевом адресе источника
					net::attr_net_t * sourceAddress = awh_cast <net::attr_net_t *> (info.source.get());
					// Если порты процесса определены
					if((targetAddress->port > 0) && (sourceAddress->port > 0)){
						// Устанавливаем адрес источника процесса
						addr.source(sourceAddress->ip.get());
						// Формируем адрес источника процесса с портом
						source = fmk.format("%s:%u", static_cast <string> (addr).c_str(), sourceAddress->port);
						// Устанавливаем адрес назначения процесса
						addr.source(targetAddress->ip.get());
						// Формируем адрес назначения процесса с портом
						destination = fmk.format("%s:%u", static_cast <string> (addr).c_str(), targetAddress->port);
					// Если только порт назначения процесса определён
					} else if(targetAddress->port > 0) {
						// Устанавливаем адрес назначения процесса
						addr.source(targetAddress->ip.get());
						// Формируем адрес назначения процесса с портом
						destination = fmk.format("%s:%u", static_cast <string> (addr).c_str(), targetAddress->port);
					// Если только порт источника процесса определён
					} else if(sourceAddress->port > 0) {
						// Устанавливаем адрес источника процесса
						addr.source(sourceAddress->ip.get());
						// Формируем адрес источника процесса с портом
						source = fmk.format("%s:%u", static_cast <string> (addr).c_str(), sourceAddress->port);
					}
					// Если адрес источника процесса определён
					if(!source.empty()){
						// Если адрес источника процесса не определён а адрес назначения процесса определён
						if(!destination.empty()){
							// Записываем в лог информацию о процессе
							log.print("Package: (TID=%u, MID=%u), SOURCE=%s, DEST=%s, FAMILY=%s, PROTOCOL=%s, TTL=%s, ACTION=%s",
								log_t::flag_t::INFO,
								tid, mid,
								source.c_str(),
								destination.c_str(),
								family.c_str(),
								protocol.c_str(),
								hops.c_str(),
								(action == event::action_t::READ ? "READ" : "WRITE")
							);
						// Если адрес назначения процесса не определён
						} else {
							// Записываем в лог информацию о процессе
							log.print("Package: (TID=%u, MID=%u), SOURCE=%s, FAMILY=%s, PROTOCOL=%s, TTL=%s, ACTION=%s",
								log_t::flag_t::INFO,
								tid, mid,
								source.c_str(),
								family.c_str(),
								protocol.c_str(),
								hops.c_str(),
								(action == event::action_t::READ ? "READ" : "WRITE")
							);
						}
					// Если адрес источника процесса не определён а адрес назначения процесса определён
					} else if(!destination.empty()) {
						// Записываем в лог информацию о процессе
						log.print("Package: (TID=%u, MID=%u), DEST=%s, FAMILY=%s, PROTOCOL=%s, TTL=%s, ACTION=%s",
							log_t::flag_t::INFO,
							tid, mid,
							destination.c_str(),
							family.c_str(),
							protocol.c_str(),
							hops.c_str(),
							(action == event::action_t::READ ? "READ" : "WRITE")
						);
					}
				});
				// Устанавливаем функцию обратного вызова на ошибку события посредника
				io.on(mid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Записываем ошибку в лог неизвестного события
							log.print("Неизвестная ошибка события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Записываем ошибку в лог недопустимой операции
							log.print("Недопустимая операция события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Записываем ошибку в лог доступа запрещёния
							log.print("Доступ к событию посредника запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Записываем ошибку в лог уже существующего объекта
							log.print("Объект события посредника уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Записываем ошибку в лог доступа к сокету
							log.print("Ошибка доступа к сокету события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Записываем ошибку в лог некорректного адреса
							log.print("Некорректный адрес события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Записываем ошибку в лог подключения
							log.print("Ошибка подключения события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Записываем ошибку в лог недостаточно ресурсов
							log.print("Недостаточно ресурсов для события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Записываем ошибку в лог события
							log.print("Ошибка события посредника: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Записываем ошибку в лог события
							log.print("Объект события посредника не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на общее событие сервера
				io.on(eid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							log.print("Событие сервера на чтение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							log.print("Событие сервера на запись: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							log.print("Событие сервера на подключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							log.print("Событие сервера на отключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							log.print("Событие сервера на переподключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							log.print("Событие сервера на закрытие подключения: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							log.print("Событие сервера на изменение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							log.print("Событие сервера на удаление: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							log.print("Событие сервера на переименование: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							log.print("Событие сервера на изменение атрибутов: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							log.print("Событие сервера на отзыв доступа: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							log.print("Событие сервера на изменение счётчика жёстких ссылок: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на общее событие посредника
				io.on(mid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							log.print("Событие посредника на чтение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							log.print("Событие посредника на запись: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							log.print("Событие посредника на подключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							log.print("Событие посредника на отключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							log.print("Событие посредника на переподключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							log.print("Событие посредника на закрытие подключения: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							log.print("Событие посредника на изменение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							log.print("Событие посредника на удаление: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							log.print("Событие посредника на переименование: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							log.print("Событие посредника на изменение атрибутов: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							log.print("Событие посредника на отзыв доступа: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							log.print("Событие посредника на изменение счётчика жёстких ссылок: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Выполняем фиксацию настроек события сервера
				if(io.commit(eid) && io.commit(tid) && io.commit(mid)){
					// Выполняем запуск события
					if(io.launch(eid) && io.launch(tid)){
						// Получаем название устройства туннеля
						const string iface = io.getIface(tid);
						// Записываем в лог название заведённого устройства
						cout << " Устройство туннеля: " << iface << endl;
						/**
						 * Поднимаем устройство туннеля
						 *
						 * @note Движок устройство заводит и адреса ему назначает, но признака
						 *       UP не ставит: без него устройство остаётся опущенным, маршрута
						 *       через него нет и пакету идти некуда
						 */
						if(params.up && !iface.empty()){
							// Создаём объект работы с сетью
							eth_t eth(&fmk, &log);
							// Поднимаем устройство туннеля
							if(eth.iface.flag(iface, event::eth_flag_t::UP, event::mode_t::ENABLED))
								// Записываем в лог сообщение об успешном поднятии устройства
								cout << " Устройство туннеля поднято!" << endl;
							// Записываем ошибку в лог поднятия устройства
							else cout << " Ошибка поднятия устройства туннеля!" << endl;
						}
						// Маршрут туннеля
						eth::gateway_t::route_t route;
						// Устанавливаем интерфейс туннеля
						route.ifname = iface;
						// Устанавливаем префикс маршрута туннеля
						route.prefix = params.prefix;
						// Создаём шлюз маршрута туннеля
						route.gateway = make_unique <net::addr_net_ipv4_t> ();
						// Создаём адрес назначения маршрута туннеля
						route.destination = make_unique <net::addr_net_ipv4_t> ();
						// Выполням парсинг адреса назначения маршрута туннеля
						addr = params.net;
						// Устанавливаем адрес назначения маршрута туннеля
						awh_cast <net::addr_net_ipv4_t *> (route.destination.get())->address = addr.v4(net_addr_t::endian_t::LITTLE);
						// Устанавливаем маршрут туннеля (sudo route -n add -net 10.0.0.0/24 -interface utun7)
						if(params.route && gateway.add(route))
							// Записываем в лог сообщение об успешной установке маршрута туннеля
							cout << " Маршрут туннеля успешно установлен!" << endl;
						// Записываем в лог сообщение об успешном запуске события
						cout << " Событие сервера и туннеля успешно запущено!" << endl;
						/**
						 * Запускаем опрос событий
						 */
						/**
						 * Крутим опрос до сигнала остановки
						 *
						 * @note Выход обязан быть штатным: только он доводит дело до разбора
						 *       объектов, а с ним - и до сноса устройства туннеля
						 */
						while(!__awh_stop__.load() && io.poll(250));
						// Записываем в лог сообщение об остановке
						cout << " Остановка по сигналу, разбираем события..." << endl;
						// Выполняем разбор всех событий
						io.deinitialize();
						// Записываем в лог сообщение о завершении работы
						cout << " Работа завершена!" << endl;
					// Записываем ошибку в лог запуска события
					} else cout << " Ошибка запуска события!" << endl;
				}
			// Если адрес назначения не установлен
			} else cout << " Ошибка установки адреса сервера!" << endl;
		// Если адрес не установлен
		} else cout << " Ошибка установки адреса события!" << endl;
	}
	// Возвращаем результат
	return EXIT_SUCCESS;
}
