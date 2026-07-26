/**
 * @file: procre.cpp
 * @date: 2026-01-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с резольвером процессов — демонстрация сопоставления сетевого соединения по адресам и портам с
 *        владеющим им процессом операционной системы
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <cstdint>
#include <net/addr.hpp>
#include <sys/procre.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * main Главная функция приложения
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект резольвера процессов
	procre_t procre(&log);
	// Объект работы с сетевыми адресами
	net_addr_t addr(&fmk, &log);
	// Устанавливаем название сервиса
	log.name("Process Resolver");
	// Устанавливаем формат времени
	log.format("%H:%M:%S %d.%m.%Y");
	// Если количество параметров больше одного
	if(argc > 1){
		// Выполняем получение пида
		const pid_t pid = static_cast <pid_t> (::stoi(argv[1]));
		// Записываем в лог название приложения
		log.print("Process Resolver: NAME=%s", log_t::flag_t::INFO, procre.name(pid).c_str());
	// Записываем в лог название текущего проекта
	} else log.print("Process Resolver: NAME=%s", log_t::flag_t::INFO, procre.name().c_str());
	// Устанавливаем функцию обратного вызова для получения информации о процессе
	procre.on([&addr, &procre, &fmk, &log](const pid_t pid, const procre_t::info_t & info){
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
			// Для семейства UDS
			case static_cast <uint8_t> (event::family_t::UDS):
				// Устанавливаем семейство адресов процесса
				family = "UDS";
			break;
		}
		/**
		 * Определяем семейстов адресов сокета
		 */
		switch(static_cast <uint8_t> (info.family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Устанавливаем IP-адрес источника процесса
				addr.source(info.addresses.src.get());
				// Извлекаем IP-адрес источника процесса
				source = static_cast <string> (addr);
				// Устанавливаем IP-адрес назначения процесса
				addr.source(info.addresses.dst.get());
				// Извлекаем IP-адрес назначения процесса
				destination = static_cast <string> (addr);
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Устанавливаем IP-адрес источника процесса
				addr.source(info.addresses.src.get());
				// Извлекаем IP-адрес источника процесса
				source = fmk.format("[%s]", static_cast <string> (addr).c_str());
				// Устанавливаем IP-адрес назначения процесса
				addr.source(info.addresses.dst.get());
				// Извлекаем IP-адрес назначения процесса
				destination = fmk.format("[%s]", static_cast <string> (addr).c_str());
			} break;
			// Для семейства UDS
			case static_cast <uint8_t> (event::family_t::UDS): {
				// Устанавливаем адрес источника процесса
				source = awh_cast <net::addr_fs_t *> (info.addresses.src.get())->address;
				// Устанавливаем адрес назначения процесса
				destination = awh_cast <net::addr_fs_t *> (info.addresses.dst.get())->address;
			} break;
		}
		/**
		 * Определяем протокол сокета
		 */
		switch(static_cast <uint8_t> (info.protocol)){
			// Если протокол сокета является RAW-протоколом
			case static_cast <uint8_t> (event::protocol_t::RAW):
				// Устанавливаем протокол сокета
				protocol = "RAW";
			break;
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
		// Если порты процесса определены
		if((info.ports.dst > 0) && (info.ports.src > 0)){
			// Формируем адрес источника процесса с портом
			source = fmk.format("%s:%u", source.c_str(), info.ports.src);
			// Формируем адрес назначения процесса с портом
			destination = fmk.format("%s:%u", destination.c_str(), info.ports.dst);
		// Если только порт назначения процесса определён
		} else if(info.ports.dst > 0)
			// Формируем адрес назначения процесса с портом
			destination = fmk.format("%s:%u", destination.c_str(), info.ports.dst);
		// Если только порт источника процесса определён
		else if(info.ports.src > 0)
			// Формируем адрес источника процесса с портом
			source = fmk.format("%s:%u", source.c_str(), info.ports.src);
		// Если адрес источника процесса определён
		if(!source.empty()){
			// Если адрес источника процесса не определён а адрес назначения процесса определён
			if(!destination.empty()){
				// Записываем в лог информацию о процессе
				log.print("Process Resolver: NAME=%s, SOURCE=%s, DEST=%s, FAMILY=%s, PROTOCOL=%s",
					log_t::flag_t::INFO,
					procre.name(pid).c_str(),
					source.c_str(),
					destination.c_str(),
					family.c_str(),
					protocol.c_str()
				);
			// Если адрес назначения процесса не определён
			} else {
				// Записываем в лог информацию о процессе
				log.print("Process Resolver: NAME=%s, SOURCE=%s, FAMILY=%s, PROTOCOL=%s",
					log_t::flag_t::INFO,
					procre.name(pid).c_str(),
					source.c_str(),
					family.c_str(),
					protocol.c_str()
				);
			}
		// Если адрес источника процесса не определён а адрес назначения процесса определён
		} else if(!destination.empty()) {
			// Записываем в лог информацию о процессе
			log.print("Process Resolver: NAME=%s, DEST=%s, FAMILY=%s, PROTOCOL=%s",
				log_t::flag_t::INFO,
				procre.name(pid).c_str(),
				destination.c_str(),
				family.c_str(),
				protocol.c_str()
			);
		}
	});
	// Запускаем процесс сканирования активных процессов и получения информации о них
	procre.scanning();
	// Возвращаем результат
	return EXIT_SUCCESS;
}
