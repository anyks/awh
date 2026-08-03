/**
 * @file: portmap.cpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с модулем перенаправления портов —
 *        демонстрация запроса внешнего адреса, чтения перечня заведённых перенаправлений,
 *        заведения перенаправления на маршрутизаторе и снятия заведённого
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <units/portmap.hpp>

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
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Отключаем неиспользуемые переменные
	(void) argc;
	(void) argv;
	// Объект фреймворка
	fmk_t fmk;
	// Объект работы с логами
	log_t log(&fmk);
	// Устанавливаем объект работы с логами
	fmk.setLogger(&log);
	// Устанавливаем название сервиса
	log.name("Portmap");
	// Создаём объект модуля перенаправления портов
	unit::portmap_t portmap(&fmk, &log);
	/**
	 * Устанавливаем вид опроса маршрутизатора
	 *
	 * @note Опрос AUTO ведёт все три договора разом под общим сроком и принимает
	 *       ответ, пришедший первым: какой из них поддерживает маршрутизатор, заранее
	 *       неизвестно. Нужный договор задаётся и явно
	 */
	portmap.setType(unit::portmap_t::type_t::AUTO);
	// portmap.setType(unit::portmap_t::type_t::PCP);
	// portmap.setType(unit::portmap_t::type_t::UPNP);
	// portmap.setType(unit::portmap_t::type_t::NAT_PMP);
	/**
	 * Устанавливаем разновидность сети, в которой ведётся обмен
	 *
	 * @note Обмен ведётся целиком одной разновидностью. В сети IPv6 преобразования
	 *       адресов нет, и перенаправление означает не подмену адреса, а разрешение
	 *       подключений сквозь заслон маршрутизатора. Договор NAT-PMP там не
	 *       опрашивается вовсе - разновидности IPv6 он не имеет
	 */
	portmap.setFamily(unit::portmap_t::family_t::IPV4);
	// portmap.setFamily(unit::portmap_t::family_t::IPV6);
	// Устанавливаем срок ожидания ответа маршрутизатора
	portmap.setTimeout(3000);
	// Устанавливаем количество попыток обращения к маршрутизатору
	portmap.setAttempts(4);
	/**
	 * Устанавливаем адрес маршрутизатора
	 *
	 * @note Задавать его незачем: без этого маршрутизатор отыскивается по таблице
	 *       маршрутов. Настройка нужна лишь тогда, когда отыскание ошибается
	 */
	// portmap.setRouter("192.168.1.1");
	/**
	 * Устанавливаем сетевое устройство, которым ведётся обмен
	 *
	 * @note Задавать его незачем: без этого устройство берётся из маршрута до внешней
	 *       сети. Настройка нужна лишь тогда, когда движение завёрнуто в туннель, а
	 *       маршрутизатор виден другим устройством
	 */
	// portmap.setIface("en0");
	// Заводимое перенаправление порта
	unit::portmap_t::mapping_t mapping;
	// Устанавливаем договор перенаправляемого порта
	mapping.proto = unit::portmap_t::proto_t::TCP;
	// Устанавливаем внутренний порт перенаправления
	mapping.internalPort = 8081;
	// Устанавливаем желаемый внешний порт перенаправления
	mapping.externalPort = 8080;
	// Устанавливаем срок жизни перенаправления в секундах
	mapping.lifeTime = 3600;
	// Устанавливаем описание перенаправления, видное в настройках маршрутизатора
	mapping.description = "AWH Web Server";
	/**
	 * Устанавливаем функцию обратного вызова на получение внешнего адреса маршрутизатора
	 *
	 * @param address внешний адрес маршрутизатора
	 * @param type    договор, которым получен ответ
	 *
	 */
	portmap.on <void (const string &, const unit::portmap_t::type_t)> ("external", [&portmap](const string & address, const unit::portmap_t::type_t type) noexcept -> void {
		// Выводим полученный внешний адрес маршрутизатора
		cout << "External address: " << address << " (protocol " << static_cast <uint16_t> (type) << ")" << endl;
		// Выполняем чтение перечня заведённых перенаправлений
		portmap.list();
	}, _1, _2);
	/**
	 * Устанавливаем функцию обратного вызова на получение перечня перенаправлений
	 *
	 * @note Перечень общий на всю сеть: в нём окажутся и перенаправления, заведённые
	 *       другими машинами. Чьё именно, видно по полю internalClient
	 *
	 * @param mappings перечень заведённых перенаправлений
	 * @param type     договор, которым получен ответ
	 *
	 */
	portmap.on <void (const vector <unit::portmap_t::mapping_t> &, const unit::portmap_t::type_t)> ("mappings", [&portmap, &mapping](const vector <unit::portmap_t::mapping_t> & mappings, const unit::portmap_t::type_t type) noexcept -> void {
		// Выводим количество заведённых на маршрутизаторе перенаправлений
		cout << "Port mappings: " << mappings.size() << " (protocol " << static_cast <uint16_t> (type) << ")" << endl;
		/**
		 * Выполняем перебор всех заведённых перенаправлений
		 */
		for(auto & entry : mappings)
			// Выводим сведения о заведённом перенаправлении
			cout << " " << ((entry.proto == unit::portmap_t::proto_t::TCP) ? "TCP" : "UDP")
				<< " " << entry.externalPort << " -> " << entry.internalClient << ":" << entry.internalPort
				<< ", TTL: " << entry.lifeTime << " seconds"
				<< (entry.enabled ? "" : " (disabled)")
				<< " [" << entry.description << "]" << endl;
		// Выполняем заведение перенаправления порта
		portmap.open(mapping);
	}, _1, _2);
	/**
	 * Устанавливаем функцию обратного вызова на заведение перенаправления порта
	 *
	 * @warning Запрошенный внешний порт для маршрутизатора лишь пожелание: занятый порт
	 *          он заменит другим по своему выбору, и объявлять другим следует именно
	 *          назначенный
	 *
	 * @param mapping заведённое перенаправление порта
	 * @param type    договор, которым получен ответ
	 *
	 */
	portmap.on <void (const unit::portmap_t::mapping_t &, const unit::portmap_t::type_t)> ("mapped", [&portmap](const unit::portmap_t::mapping_t & mapping, const unit::portmap_t::type_t type) noexcept -> void {
		// Выводим сведения о заведённом перенаправлении порта
		cout << "Mapped: internal " << mapping.internalPort << " -> external " << mapping.externalPort
			<< ", TTL: " << mapping.lifeTime << " seconds (protocol " << static_cast <uint16_t> (type) << ")" << endl;
		/**
		 * Выполняем снятие заведённого перенаправления порта
		 *
		 * @note Заведённое перенаправление переживает завершение процесса: маршрутизатор
		 *       держит его до истечения срока жизни, и снимать заведённое следует самому
		 */
		portmap.close(mapping);
	}, _1, _2);
	/**
	 * Устанавливаем функцию обратного вызова на снятие перенаправления порта
	 *
	 * @param mapping снятое перенаправление порта
	 * @param type    договор, которым получен ответ
	 *
	 */
	portmap.on <void (const unit::portmap_t::mapping_t &, const unit::portmap_t::type_t)> ("unmapped", [&portmap](const unit::portmap_t::mapping_t & mapping, const unit::portmap_t::type_t type) noexcept -> void {
		// Выводим сведения о снятом перенаправлении порта
		cout << "Unmapped: external " << mapping.externalPort << " (protocol " << static_cast <uint16_t> (type) << ")" << endl;
		// Выполняем остановку работы модуля
		portmap.stop();
	}, _1, _2);
	/**
	 * Устанавливаем функцию обратного вызова на отказ перенаправления
	 *
	 * @note Отказ маршрутизатора отделён от сбоя обмена: первый означает осмысленный
	 *       ответ, второй - что ответа не получено вовсе
	 *
	 * @param error код причины отказа
	 * @param type  договор, по которому вёлся обмен
	 *
	 */
	portmap.on <void (const unit::portmap_t::error_t, const unit::portmap_t::type_t)> ("failure", [&portmap, &log](const unit::portmap_t::error_t error, const unit::portmap_t::type_t type) noexcept -> void {
		// Записываем в лог сообщение об отказе перенаправления
		log.print("Portmapping failed: code %u (protocol %u)", log_t::flag_t::WARNING, static_cast <uint16_t> (error), static_cast <uint16_t> (type));
		// Выполняем остановку работы модуля
		portmap.stop();
	}, _1, _2);
	/**
	 * Устанавливаем функцию обратного вызова на получение ошибок событий обмена
	 *
	 * @param eid         идентификатор события обмена
	 * @param error       код ошибки события обмена
	 * @param description описание ошибки события обмена
	 *
	 */
	portmap.on <void (const event::id_t, const event::error_t, const string &)> ("error", [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
		// Записываем в лог сообщение об ошибке события обмена
		log.print("Event %u error: %s (code %u)", log_t::flag_t::CRITICAL, eid, description.c_str(), static_cast <uint16_t> (error));
	}, _1, _2, _3);
	/**
	 * Выполняем запрос внешнего адреса маршрутизатора
	 *
	 * @note Просьба лишь отправляется: обмен ведётся без блокировки потока, а итог
	 *       приходит функцией обратного вызова
	 */
	portmap.external();
	// Выполняем запуск работы модуля
	portmap.start();
	// Выводим результат
	return EXIT_SUCCESS;
}
