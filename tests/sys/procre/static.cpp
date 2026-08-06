/**
 * @file: static.cpp
 * @date: 2026-01-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты модуля резольвера процессов — проверка создания и сброса объекта модуля,
 *        а также корректности сопоставления сетевого соединения по адресам и портам с владеющим им процессом
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем восполнение средств POSIX, отсутствующих у MS Windows
 */
#include "../../posix.hpp"

/**
 * Подключаем заголовочный файл
 */
#include "procre.hpp"

/**
 * @brief Тест создания объекта резольвера процессов
 *
 */
TEST_F(ProcreFixture, CreateProcreTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_procre != nullptr);
	
	// Сбрасываем объект
	this->_procre.reset();
	
	// Проверяем сброс объекта
	ASSERT_TRUE(this->_procre == nullptr);
}

/**
 * @brief Тест получения имени текущего процесса
 *
 */
TEST_F(ProcreFixture, CurrentProcessNameTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_procre != nullptr);

	// Получаем имя текущего процесса
	std::string name = this->_procre->name();
	
	// Проверяем что имя не пустое
	ASSERT_FALSE(name.empty());
}

/**
 * @brief Тест получения имени процесса init/launchd
 *
 */
TEST_F(ProcreFixture, InitProcessNameTest){
	// Проверяем создание объекта
	ASSERT_TRUE(this->_procre != nullptr);

	// Получаем имя процесса с PID 1
	std::string name = this->_procre->name(1);

	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Процесса с идентификатором 1 у MS Windows не существует: идентификаторы
		 * выдаются там кратными четырём, единица среди них не встречается вовсе.
		 * Ни init, ни launchd, ни иного родоначальника всех процессов там нет -
		 * ближайшее по смыслу зовётся System и носит идентификатор 4
		 */
		// Проверяем что имя пустое
		ASSERT_TRUE(name.empty());
	/**
	 * Для операционных систем Linux, FreeBSD, NetBSD, OpenBSD, macOS и Solaris
	 */
	#else
		// Если мы работаем от имени суперпользователя
		if(::geteuid() == 0)
			// Проверяем что имя не пустое
			ASSERT_FALSE(name.empty());
	#endif

	// Устанавливаем функцию обратного вызова для получения информации о процессе
	this->_procre->on([this](const pid_t pid, const awh::procre_t::info_t & info){
		// Семейство адресов процесса
		std::string family = "";
		// Протокол процесса
		std::string protocol = "";
		// Адрес источника процесса
		std::string source = "";
		// Адрес назначения процесса
		std::string destination = "";
		/**
		 * Определяем семейстов адресов сокета
		 */
		switch(static_cast <uint8_t> (info.family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (awh::event::family_t::IPV4):
				// Устанавливаем семейство адресов процесса
				family = "IPv4";
			break;
			// Для семейства IPv6
			case static_cast <uint8_t> (awh::event::family_t::IPV6):
				// Устанавливаем семейство адресов процесса
				family = "IPv6";
			break;
			// Для семейства UDS
			case static_cast <uint8_t> (awh::event::family_t::UDS):
				// Устанавливаем семейство адресов процесса
				family = "UDS";
			break;
		}
		/**
		 * Определяем семейстов адресов сокета
		 */
		switch(static_cast <uint8_t> (info.family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (awh::event::family_t::IPV4): {
				// Устанавливаем IP-адрес источника процесса
				this->_addr->source(info.addresses.src.get());
				// Извлекаем IP-адрес источника процесса
				source = static_cast <std::string> (* this->_addr.get());
				// Устанавливаем IP-адрес назначения процесса
				this->_addr->source(info.addresses.dst.get());
				// Извлекаем IP-адрес назначения процесса
				destination = static_cast <std::string> (* this->_addr.get());
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (awh::event::family_t::IPV6): {
				// Устанавливаем IP-адрес источника процесса
				this->_addr->source(info.addresses.src.get());
				// Извлекаем IP-адрес источника процесса
				source = this->_fmk->format("[%s]", static_cast <std::string> (* this->_addr.get()).c_str());
				// Устанавливаем IP-адрес назначения процесса
				this->_addr->source(info.addresses.dst.get());
				// Извлекаем IP-адрес назначения процесса
				destination = this->_fmk->format("[%s]", static_cast <std::string> (* this->_addr.get()).c_str());
			} break;
			// Для семейства UDS
			case static_cast <uint8_t> (awh::event::family_t::UDS): {
				/**
				 * @par Намеренные решения
				 *
				 * Адреса берутся с проверкой на наличие. У сокета Unix собеседника
				 * может не быть вовсе - слушающий сокет его не имеет, - и пустой
				 * указатель адреса назначения тут правда о нём, а не пробел разбора.
				 * Прежде он разыменовывался без проверки, и на OpenBSD, где такие
				 * сокеты в перечислении встречаются, набор падал с дампом
				 *
				 */
				// Если адрес источника процесса получен
				if(info.addresses.src != nullptr)
					// Устанавливаем адрес источника процесса
					source = awh_cast <awh::net::addr_fs_t *> (info.addresses.src.get())->address;
				// Если адрес назначения процесса получен
				if(info.addresses.dst != nullptr)
					// Устанавливаем адрес назначения процесса
					destination = awh_cast <awh::net::addr_fs_t *> (info.addresses.dst.get())->address;
			} break;
		}
		/**
		 * Определяем протокол сокета
		 */
		switch(static_cast <uint8_t> (info.protocol)){
			// Если протокол сокета является RAW-протоколом
			case static_cast <uint8_t> (awh::event::protocol_t::RAW):
				// Устанавливаем протокол сокета
				protocol = "RAW";
			break;
			// Если протокол сокета является TCP-протоколом
			case static_cast <uint8_t> (awh::event::protocol_t::TCP):
				// Устанавливаем протокол сокета
				protocol = "TCP";
			break;
			// Если протокол сокета является UDP-протоколом
			case static_cast <uint8_t> (awh::event::protocol_t::UDP):
				// Устанавливаем протокол сокета
				protocol = "UDP";
			break;
			// Если протокол сокета является ICMP-протоколом
			case static_cast <uint8_t> (awh::event::protocol_t::ICMP):
				// Устанавливаем протокол сокета
				protocol = "ICMP";
			break;
			// Если протокол сокета является IGMP-протоколом
			case static_cast <uint8_t> (awh::event::protocol_t::IGMP):
				// Устанавливаем протокол сокета
				protocol = "IGMP";
			break;
			// Если протокол сокета является SCTP-протоколом
			case static_cast <uint8_t> (awh::event::protocol_t::SCTP):
				// Устанавливаем протокол сокета
				protocol = "SCTP";
			break;
			// Если протокол сокета не определён
			default : protocol = "NONE";
		}
		// Если порты процесса определены
		if((info.ports.dst > 0) && (info.ports.src > 0)){
			// Формируем адрес источника процесса с портом
			source = this->_fmk->format("%s:%u", source.c_str(), info.ports.src);
			// Формируем адрес назначения процесса с портом
			destination = this->_fmk->format("%s:%u", destination.c_str(), info.ports.dst);
		// Если только порт назначения процесса определён
		} else if(info.ports.dst > 0)
			// Формируем адрес назначения процесса с портом
			destination = this->_fmk->format("%s:%u", destination.c_str(), info.ports.dst);
		// Если только порт источника процесса определён
		else if(info.ports.src > 0)
			// Формируем адрес источника процесса с портом
			source = this->_fmk->format("%s:%u", source.c_str(), info.ports.src);
		// Если адрес источника процесса определён
		if(!source.empty()){
			// Проверяем что адрес источника процесса не пустой
			ASSERT_FALSE(source.empty());
			// Если адрес источника процесса не определён а адрес назначения процесса определён
			if(!destination.empty()){
				// Проверяем что адрес назначения процесса не пустой
				ASSERT_FALSE(destination.empty());
				// Записываем в лог информацию о процессе
				this->_log->print("Process Resolver: NAME=%s, SOURCE=%s, DEST=%s, FAMILY=%s, PROTOCOL=%s",
					awh::log_t::flag_t::INFO,
					this->_procre->name(pid).c_str(),
					source.c_str(),
					destination.c_str(),
					family.c_str(),
					protocol.c_str()
				);
			// Если адрес назначения процесса не определён
			} else {
				// Записываем в лог информацию о процессе
				this->_log->print("Process Resolver: NAME=%s, SOURCE=%s, FAMILY=%s, PROTOCOL=%s",
					awh::log_t::flag_t::INFO,
					this->_procre->name(pid).c_str(),
					source.c_str(),
					family.c_str(),
					protocol.c_str()
				);
			}
		// Если адрес источника процесса не определён а адрес назначения процесса определён
		} else if(!destination.empty()) {
			// Проверяем что адрес назначения процесса не пустой
			ASSERT_FALSE(destination.empty());
			// Записываем в лог информацию о процессе
			this->_log->print("Process Resolver: NAME=%s, DEST=%s, FAMILY=%s, PROTOCOL=%s",
				awh::log_t::flag_t::INFO,
				this->_procre->name(pid).c_str(),
				destination.c_str(),
				family.c_str(),
				protocol.c_str()
			);
		}
	});
	// Запускаем процесс сканирования активных процессов и получения информации о них
	this->_procre->scanning();
}
