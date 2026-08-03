/**
 * @file: upnp.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Проверка кодека действий службы перенаправления UPnP — сборка вызовов, разбор
 *        ответов службы, коды ошибок и осмысленность повторной просьбы
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "portmap.hpp"

/**
 * Используем пространство имён договоров перенаправления портов
 */
using namespace awh::proto::portmap;

/**
 * @brief Обозначение вида службы соединения по адресу IP
 *
 */
static constexpr const char * WAN_SERVICE = "urn:schemas-upnp-org:service:WANIPConnection:1";

/**
 * @brief Обозначение вида службы заслона IPv6
 *
 */
static constexpr const char * WAN_SERVICE6 = "urn:schemas-upnp-org:service:WANIPv6FirewallControl:1";

/**
 * @brief Проверка сборки вызова заведения перенаправления порта
 *
 */
TEST_F(PortmapFixture, UpnpAdd) {
	// Создаём объект кодека действий службы перенаправления
	const std::unique_ptr <upnp_t> upnp = this->makeUpnp();
	// Параметры заводимого перенаправления порта
	upnp_t::mapping_t mapping;
	// Устанавливаем договор перенаправления порта
	mapping.proto = upnp_t::proto_t::TCP;
	// Устанавливаем внешний порт перенаправления
	mapping.externalPort = 8080;
	// Устанавливаем внутренний порт перенаправления
	mapping.internalPort = 8080;
	// Устанавливаем внутренний адрес машины перенаправления
	mapping.internalClient = "192.168.1.42";
	// Устанавливаем описание перенаправления
	mapping.description = "Сервер AWH";
	// Устанавливаем срок жизни перенаправления
	mapping.lifeTime = 3600;
	// Выполняем сборку вызова заведения перенаправления порта
	const upnp_t::request_t request = upnp->add(WAN_SERVICE, mapping);
	// Выполняем проверку пригодности собранного вызова
	ASSERT_TRUE(request.valid());
	// Выполняем проверку названия вызываемого действия службы
	ASSERT_EQ(request.action, "AddPortMapping");
	// Выполняем проверку обозначения вызываемого действия службы
	ASSERT_EQ(request.header, "\"urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping\"");
	// Выполняем проверку наличия внешнего порта перенаправления
	ASSERT_NE(request.body.find("<NewExternalPort>8080</NewExternalPort>"), std::string::npos);
	// Выполняем проверку наличия договора перенаправления порта
	ASSERT_NE(request.body.find("<NewProtocol>TCP</NewProtocol>"), std::string::npos);
	// Выполняем проверку наличия внутреннего адреса машины перенаправления
	ASSERT_NE(request.body.find("<NewInternalClient>192.168.1.42</NewInternalClient>"), std::string::npos);
	// Выполняем проверку наличия признака включения перенаправления
	ASSERT_NE(request.body.find("<NewEnabled>1</NewEnabled>"), std::string::npos);
	// Выполняем проверку наличия срока жизни перенаправления
	ASSERT_NE(request.body.find("<NewLeaseDuration>3600</NewLeaseDuration>"), std::string::npos);
	/**
	 * Выполняем проверку наличия пустого внешнего узла перенаправления
	 *
	 * @note Пустое значение означает «с любого узла», и именно его принимает
	 *       подавляющее большинство маршрутизаторов
	 */
	ASSERT_NE(request.body.find("<NewRemoteHost></NewRemoteHost>"), std::string::npos);
	/**
	 * Выполняем проверку отклонения неполных параметров перенаправления
	 */
	{
		// Сбрасываем внутренний адрес машины перенаправления
		mapping.internalClient.clear();
		// Выполняем проверку отклонения сборки без внутреннего адреса машины
		ASSERT_FALSE(upnp->add(WAN_SERVICE, mapping).valid());
		// Восстанавливаем внутренний адрес машины перенаправления
		mapping.internalClient = "192.168.1.42";
		// Сбрасываем договор перенаправления порта
		mapping.proto = upnp_t::proto_t::NONE;
		// Выполняем проверку отклонения сборки без договора перенаправления
		ASSERT_FALSE(upnp->add(WAN_SERVICE, mapping).valid());
	}
}
/**
 * @brief Проверка обрезки длинного описания перенаправления
 *
 * @details Описание обрезается, а не отвергает вызов: описание дело предъявительское,
 *          и терять из-за него перенаправление незачем
 *
 */
TEST_F(PortmapFixture, UpnpDescription) {
	// Создаём объект кодека действий службы перенаправления
	const std::unique_ptr <upnp_t> upnp = this->makeUpnp();
	// Параметры заводимого перенаправления порта
	upnp_t::mapping_t mapping;
	// Устанавливаем договор перенаправления порта
	mapping.proto = upnp_t::proto_t::UDP;
	// Устанавливаем внешний порт перенаправления
	mapping.externalPort = 5060;
	// Устанавливаем внутренний порт перенаправления
	mapping.internalPort = 5060;
	// Устанавливаем внутренний адрес машины перенаправления
	mapping.internalClient = "192.168.1.42";
	// Устанавливаем описание перенаправления длиннее допустимого
	mapping.description.assign(upnp_t::MAX_DESCRIPTION * 2, 'x');
	// Выполняем сборку вызова заведения перенаправления порта
	const upnp_t::request_t request = upnp->add(WAN_SERVICE, mapping);
	// Выполняем проверку пригодности собранного вызова
	ASSERT_TRUE(request.valid());
	// Выполняем проверку обрезки описания перенаправления до допустимой длины
	ASSERT_NE(request.body.find(std::string("<NewPortMappingDescription>").append(upnp_t::MAX_DESCRIPTION, 'x').append("</NewPortMappingDescription>")), std::string::npos);
}
/**
 * @brief Проверка сборки прочих вызовов службы соединения
 *
 */
TEST_F(PortmapFixture, UpnpActions) {
	// Создаём объект кодека действий службы перенаправления
	const std::unique_ptr <upnp_t> upnp = this->makeUpnp();
	/**
	 * Выполняем проверку сборки вызова снятия перенаправления порта
	 */
	{
		// Выполняем сборку вызова снятия перенаправления порта
		const upnp_t::request_t request = upnp->remove(WAN_SERVICE, upnp_t::proto_t::TCP, 8080);
		// Выполняем проверку пригодности собранного вызова
		ASSERT_TRUE(request.valid());
		// Выполняем проверку названия вызываемого действия службы
		ASSERT_EQ(request.action, "DeletePortMapping");
		// Выполняем проверку наличия внешнего порта перенаправления
		ASSERT_NE(request.body.find("<NewExternalPort>8080</NewExternalPort>"), std::string::npos);
		// Выполняем проверку отклонения сборки без внешнего порта перенаправления
		ASSERT_FALSE(upnp->remove(WAN_SERVICE, upnp_t::proto_t::TCP, 0).valid());
	}
	/**
	 * Выполняем проверку сборки вызова чтения внешнего адреса маршрутизатора
	 */
	{
		// Выполняем сборку вызова чтения внешнего адреса маршрутизатора
		const upnp_t::request_t request = upnp->external(WAN_SERVICE);
		// Выполняем проверку пригодности собранного вызова
		ASSERT_TRUE(request.valid());
		// Выполняем проверку названия вызываемого действия службы
		ASSERT_EQ(request.action, "GetExternalIPAddress");
	}
	/**
	 * Выполняем проверку сборки вызова чтения перенаправления по номеру
	 */
	{
		// Выполняем сборку вызова чтения перенаправления по порядковому номеру
		const upnp_t::request_t request = upnp->entry(WAN_SERVICE, 3);
		// Выполняем проверку пригодности собранного вызова
		ASSERT_TRUE(request.valid());
		// Выполняем проверку названия вызываемого действия службы
		ASSERT_EQ(request.action, "GetGenericPortMappingEntry");
		// Выполняем проверку наличия порядкового номера перенаправления
		ASSERT_NE(request.body.find("<NewPortMappingIndex>3</NewPortMappingIndex>"), std::string::npos);
	}
	/**
	 * Выполняем проверку сборки вызова чтения перенаправления по внешнему порту
	 */
	{
		// Выполняем сборку вызова чтения перенаправления по внешнему порту
		const upnp_t::request_t request = upnp->specific(WAN_SERVICE, upnp_t::proto_t::UDP, 5060);
		// Выполняем проверку пригодности собранного вызова
		ASSERT_TRUE(request.valid());
		// Выполняем проверку названия вызываемого действия службы
		ASSERT_EQ(request.action, "GetSpecificPortMappingEntry");
		// Выполняем проверку наличия договора перенаправления порта
		ASSERT_NE(request.body.find("<NewProtocol>UDP</NewProtocol>"), std::string::npos);
	}
	/**
	 * Выполняем проверку сборки вызова чтения состояния соединения
	 */
	{
		// Выполняем сборку вызова чтения состояния соединения маршрутизатора
		const upnp_t::request_t request = upnp->status(WAN_SERVICE);
		// Выполняем проверку пригодности собранного вызова
		ASSERT_TRUE(request.valid());
		// Выполняем проверку названия вызываемого действия службы
		ASSERT_EQ(request.action, "GetStatusInfo");
	}
}
/**
 * @brief Проверка извлечения сведений из ответов службы
 *
 */
TEST_F(PortmapFixture, UpnpAnswer) {
	// Создаём объект кодека действий службы перенаправления
	const std::unique_ptr <upnp_t> upnp = this->makeUpnp();
	// Создаём объект кодека договора SOAP
	const std::unique_ptr <soap_t> soap = this->makeSoap();
	// Разобранный ответ службы
	soap_t::answer_t answer;
	// Код причины отказа кодека
	soap_t::error_t error = soap_t::error_t::NONE;
	/**
	 * Выполняем проверку извлечения внешнего адреса маршрутизатора
	 */
	{
		// Ответ службы с внешним адресом маршрутизатора
		const std::string data =
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
			"<u:GetExternalIPAddressResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
			"<NewExternalIPAddress>203.0.113.7</NewExternalIPAddress>"
			"</u:GetExternalIPAddressResponse>"
			"</s:Body></s:Envelope>";
		// Выполняем разбор ответа службы
		ASSERT_TRUE(soap->parse(data, answer, error)) << message(error);
		// Извлекаемый внешний адрес маршрутизатора
		std::string address;
		// Выполняем извлечение внешнего адреса маршрутизатора
		ASSERT_TRUE(upnp->address(answer, address));
		// Выполняем проверку извлечённого внешнего адреса маршрутизатора
		ASSERT_EQ(address, "203.0.113.7");
	}
	/**
	 * Выполняем проверку отклонения пустого внешнего адреса
	 *
	 * @note Пустой адрес маршрутизатор выдаёт и тогда, когда соединения с внешней
	 *       сетью у него нет вовсе: принимать такой ответ за успех недопустимо
	 */
	{
		// Ответ службы с пустым внешним адресом маршрутизатора
		const std::string data =
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
			"<u:GetExternalIPAddressResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
			"<NewExternalIPAddress></NewExternalIPAddress>"
			"</u:GetExternalIPAddressResponse>"
			"</s:Body></s:Envelope>";
		// Выполняем разбор ответа службы
		ASSERT_TRUE(soap->parse(data, answer, error)) << message(error);
		// Извлекаемый внешний адрес маршрутизатора
		std::string address;
		// Выполняем проверку отклонения извлечения пустого внешнего адреса
		ASSERT_FALSE(upnp->address(answer, address));
	}
	/**
	 * Выполняем проверку извлечения перенаправления порта
	 */
	{
		// Ответ службы с перенаправлением порта
		const std::string data =
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
			"<u:GetGenericPortMappingEntryResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
			"<NewRemoteHost></NewRemoteHost>"
			"<NewExternalPort>12345</NewExternalPort>"
			"<NewProtocol>tcp</NewProtocol>"
			"<NewInternalPort>8080</NewInternalPort>"
			"<NewInternalClient>192.168.1.42</NewInternalClient>"
			"<NewEnabled>1</NewEnabled>"
			"<NewPortMappingDescription>Сервер AWH</NewPortMappingDescription>"
			"<NewLeaseDuration>0</NewLeaseDuration>"
			"</u:GetGenericPortMappingEntryResponse>"
			"</s:Body></s:Envelope>";
		// Выполняем разбор ответа службы
		ASSERT_TRUE(soap->parse(data, answer, error)) << message(error);
		// Извлекаемое перенаправление порта
		upnp_t::mapping_t mapping;
		// Выполняем извлечение перенаправления порта
		ASSERT_TRUE(upnp->mapping(answer, mapping));
		/**
		 * Выполняем проверку договора перенаправления порта
		 *
		 * @note Обозначение договора записано в нижнем регистре: службы записывают
		 *       его вольно, и сличение ведётся без учёта регистра
		 */
		ASSERT_EQ(mapping.proto, upnp_t::proto_t::TCP);
		// Выполняем проверку внешнего порта перенаправления
		ASSERT_EQ(mapping.externalPort, 12345);
		// Выполняем проверку внутреннего порта перенаправления
		ASSERT_EQ(mapping.internalPort, 8080);
		// Выполняем проверку внутреннего адреса машины перенаправления
		ASSERT_EQ(mapping.internalClient, "192.168.1.42");
		// Выполняем проверку описания перенаправления
		ASSERT_EQ(mapping.description, "Сервер AWH");
		// Выполняем проверку признака включения перенаправления
		ASSERT_TRUE(mapping.enabled);
		// Выполняем проверку срока жизни перенаправления
		ASSERT_EQ(mapping.lifeTime, static_cast <uint32_t> (0));
	}
	/**
	 * Выполняем проверку отклонения ответа без перенаправления
	 *
	 * @note Внутренний адрес есть у всякого перенаправления: без него подключения
	 *       отдавать некому
	 */
	{
		// Ответ службы без перенаправления порта
		const std::string data =
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
			"<u:GetStatusInfoResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
			"<NewConnectionStatus>Connected</NewConnectionStatus>"
			"</u:GetStatusInfoResponse>"
			"</s:Body></s:Envelope>";
		// Выполняем разбор ответа службы
		ASSERT_TRUE(soap->parse(data, answer, error)) << message(error);
		// Извлекаемое перенаправление порта
		upnp_t::mapping_t mapping;
		// Выполняем проверку отклонения извлечения перенаправления порта
		ASSERT_FALSE(upnp->mapping(answer, mapping));
	}
}
/**
 * @brief Проверка разбора кода ошибки и осмысленности повторной просьбы
 *
 * @details Часть отказов означает, что просить бесполезно, а часть - что следует
 *          просить иначе: занятый порт стоит попросить заново другим, а отказ
 *          настройки повторять незачем
 *
 */
TEST_F(PortmapFixture, UpnpResult) {
	// Создаём объект кодека действий службы перенаправления
	const std::unique_ptr <upnp_t> upnp = this->makeUpnp();
	// Создаём объект кодека договора SOAP
	const std::unique_ptr <soap_t> soap = this->makeSoap();
	// Отказ службы о занятости перенаправления
	const std::string data =
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
		"<s:Fault><faultcode>s:Client</faultcode><faultstring>UPnPError</faultstring><detail>"
		"<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
		"<errorCode>718</errorCode><errorDescription>ConflictInMappingEntry</errorDescription>"
		"</UPnPError></detail></s:Fault>"
		"</s:Body></s:Envelope>";
	// Разобранный ответ службы
	soap_t::answer_t answer;
	// Код причины отказа кодека
	soap_t::error_t error = soap_t::error_t::NONE;
	// Выполняем разбор отказа службы
	ASSERT_TRUE(soap->parse(data, answer, error)) << message(error);
	// Выполняем проверку кода ошибки, выданного службой
	ASSERT_EQ(upnp->result(answer), upnp_t::result_t::CONFLICT);
	// Выполняем проверку описания кода ошибки, выданного службой
	ASSERT_STREQ(message(upnp->result(answer)), "conflict in mapping entry");
	/**
	 * Выполняем проверку осмысленности повторной просьбы с иным портом
	 */
	{
		// Выполняем проверку осмысленности повторной просьбы при занятости порта
		ASSERT_TRUE(upnp->retriable(upnp_t::result_t::CONFLICT));
		// Выполняем проверку осмысленности повторной просьбы при требовании равенства портов
		ASSERT_TRUE(upnp->retriable(upnp_t::result_t::SAME_PORT_REQUIRED));
		// Выполняем проверку бессмысленности повторной просьбы при отказе настройки
		ASSERT_FALSE(upnp->retriable(upnp_t::result_t::NOT_AUTHORIZED));
		// Выполняем проверку бессмысленности повторной просьбы при нехватке места
		ASSERT_FALSE(upnp->retriable(upnp_t::result_t::NO_PORT_MAPS));
	}
	/**
	 * Выполняем проверку сохранения кода ошибки, кодеку неизвестного
	 *
	 * @note Неизвестный код подменяться известным не должен: договор оставляет
	 *       место под коды изготовителя, и подмена скрыла бы причину отказа
	 */
	{
		// Отказ службы с кодом ошибки изготовителя
		const std::string vendor =
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
			"<s:Fault><faultstring>UPnPError</faultstring><detail>"
			"<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\"><errorCode>801</errorCode></UPnPError>"
			"</detail></s:Fault></s:Body></s:Envelope>";
		// Выполняем разбор отказа службы
		ASSERT_TRUE(soap->parse(vendor, answer, error)) << message(error);
		// Выполняем проверку сохранения кода ошибки изготовителя
		ASSERT_EQ(static_cast <uint32_t> (upnp->result(answer)), static_cast <uint32_t> (801));
		// Выполняем проверку описания неизвестного кода ошибки
		ASSERT_STREQ(message(upnp->result(answer)), "unknown error code");
	}
	/**
	 * Выполняем проверку кода успешного выполнения просьбы
	 */
	{
		// Ответ службы об успешном заведении перенаправления
		const std::string success =
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
			"<u:AddPortMappingResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\"/>"
			"</s:Body></s:Envelope>";
		// Выполняем разбор ответа службы
		ASSERT_TRUE(soap->parse(success, answer, error)) << message(error);
		// Выполняем проверку кода успешного выполнения просьбы
		ASSERT_EQ(upnp->result(answer), upnp_t::result_t::SUCCESS);
		// Выполняем проверку названия действия, на которое получен ответ
		ASSERT_EQ(answer.action, "AddPortMapping");
	}
}

/**
 * @brief Проверка сборки вызова проделывания пробоя заслона IPv6
 *
 * @details Пробой - это не перенаправление: преобразования адресов в сети IPv6 нет,
 *          и просьба означает разрешение подключений сквозь заслон маршрутизатора.
 *          Внешнего порта у пробоя нет, а договор передаётся числом по описи IANA
 *
 * @warning Проверить вызов на живом устройстве не удалось: маршрутизатор, на котором
 *          отлаживался модуль, службы заслона IPv6 не выдаёт. Проверка сличает
 *          собранное с разметкой договора UPnP IGD:2, а не с ответом устройства
 *
 */
TEST_F(PortmapFixture, UpnpPinhole) {
	// Создаём объект кодека действий службы перенаправления
	const std::unique_ptr <upnp_t> upnp = this->makeUpnp();
	// Параметры проделываемого пробоя заслона
	upnp_t::pinhole_t pinhole;
	// Устанавливаем договор пробоя заслона
	pinhole.proto = upnp_t::proto_t::TCP;
	// Устанавливаем внутренний порт машины, которой отдаются подключения
	pinhole.internalPort = 8080;
	// Устанавливаем внутренний адрес машины, которой отдаются подключения
	pinhole.internalClient = "2001:db8::42";
	// Устанавливаем срок жизни пробоя заслона
	pinhole.lifeTime = 3600;
	// Выполняем сборку вызова проделывания пробоя заслона
	const upnp_t::request_t request = upnp->pinhole(WAN_SERVICE6, pinhole);
	// Выполняем проверку пригодности собранного вызова
	ASSERT_TRUE(request.valid());
	// Выполняем проверку названия вызываемого действия службы
	ASSERT_EQ(request.action, "AddPinhole");
	// Выполняем проверку обозначения вызываемого действия службы
	ASSERT_EQ(request.header, "\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1#AddPinhole\"");
	/**
	 * Выполняем проверку наличия договора пробоя числом по описи IANA
	 *
	 * @note Здесь это отличается от заведения перенаправления, где то же поле несёт
	 *       название договора: договор пробоя передаётся числом
	 */
	ASSERT_NE(request.body.find("<Protocol>6</Protocol>"), std::string::npos);
	// Выполняем проверку наличия внутреннего порта машины
	ASSERT_NE(request.body.find("<InternalPort>8080</InternalPort>"), std::string::npos);
	// Выполняем проверку наличия внутреннего адреса машины
	ASSERT_NE(request.body.find("<InternalClient>2001:db8::42</InternalClient>"), std::string::npos);
	// Выполняем проверку наличия срока жизни пробоя заслона
	ASSERT_NE(request.body.find("<LeaseTime>3600</LeaseTime>"), std::string::npos);
	/**
	 * Выполняем проверку наличия пустого порта узла, с которого пропускаются подключения
	 *
	 * @note Нулевой порт означает «с любого порта»
	 */
	ASSERT_NE(request.body.find("<RemotePort>0</RemotePort>"), std::string::npos);
	/**
	 * Выполняем проверку записи договора UDP числом по описи IANA
	 */
	{
		// Устанавливаем договор пробоя заслона
		pinhole.proto = upnp_t::proto_t::UDP;
		// Выполняем проверку наличия договора пробоя числом по описи IANA
		ASSERT_NE(upnp->pinhole(WAN_SERVICE6, pinhole).body.find("<Protocol>17</Protocol>"), std::string::npos);
		// Восстанавливаем договор пробоя заслона
		pinhole.proto = upnp_t::proto_t::TCP;
	}
	/**
	 * Выполняем проверку отклонения неполных параметров пробоя заслона
	 */
	{
		// Сбрасываем внутренний адрес машины
		pinhole.internalClient.clear();
		// Выполняем проверку отклонения сборки без внутреннего адреса машины
		ASSERT_FALSE(upnp->pinhole(WAN_SERVICE6, pinhole).valid());
		// Восстанавливаем внутренний адрес машины
		pinhole.internalClient = "2001:db8::42";
		// Сбрасываем срок жизни пробоя заслона
		pinhole.lifeTime = 0;
		/**
		 * Выполняем проверку отклонения сборки без срока жизни пробоя
		 *
		 * @note Бессрочных пробоев договор не заводит вовсе, и нулевой срок означал бы
		 *       просьбу, которую устройство отвергнет
		 */
		ASSERT_FALSE(upnp->pinhole(WAN_SERVICE6, pinhole).valid());
		// Восстанавливаем срок жизни пробоя заслона
		pinhole.lifeTime = 3600;
		// Сбрасываем договор пробоя заслона
		pinhole.proto = upnp_t::proto_t::NONE;
		// Выполняем проверку отклонения сборки без договора пробоя
		ASSERT_FALSE(upnp->pinhole(WAN_SERVICE6, pinhole).valid());
	}
}
/**
 * @brief Проверка сборки вызова заделывания пробоя заслона IPv6
 *
 * @warning Проверить вызов на живом устройстве не удалось - см. замечание к проверке
 *          сборки вызова проделывания пробоя
 *
 */
TEST_F(PortmapFixture, UpnpUnpinhole) {
	// Создаём объект кодека действий службы перенаправления
	const std::unique_ptr <upnp_t> upnp = this->makeUpnp();
	// Выполняем сборку вызова заделывания пробоя заслона
	const upnp_t::request_t request = upnp->unpinhole(WAN_SERVICE6, 42);
	// Выполняем проверку пригодности собранного вызова
	ASSERT_TRUE(request.valid());
	// Выполняем проверку названия вызываемого действия службы
	ASSERT_EQ(request.action, "DeletePinhole");
	// Выполняем проверку обозначения вызываемого действия службы
	ASSERT_EQ(request.header, "\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1#DeletePinhole\"");
	// Выполняем проверку наличия опознавателя заделываемого пробоя
	ASSERT_NE(request.body.find("<UniqueID>42</UniqueID>"), std::string::npos);
}
/**
 * @brief Проверка разбора ответов службы заслона IPv6
 *
 * @warning Проверить разбор на живом устройстве не удалось - см. замечание к проверке
 *          сборки вызова проделывания пробоя. Разбираемые ответы собраны по разметке
 *          договора UPnP IGD:2, а не сняты с устройства
 *
 */
TEST_F(PortmapFixture, UpnpFirewall) {
	// Создаём объект кодека действий службы перенаправления
	const std::unique_ptr <upnp_t> upnp = this->makeUpnp();
	// Создаём объект кодека договора SOAP
	const std::unique_ptr <soap_t> soap = this->makeSoap();
	// Выполняем сборку вызова чтения состояния заслона IPv6
	const upnp_t::request_t request = upnp->firewall(WAN_SERVICE6);
	// Выполняем проверку пригодности собранного вызова
	ASSERT_TRUE(request.valid());
	// Выполняем проверку названия вызываемого действия службы
	ASSERT_EQ(request.action, "GetFirewallStatus");
	// Код причины отказа кодека
	soap_t::error_t error = soap_t::error_t::NONE;
	/**
	 * Выполняем проверку извлечения состояния заслона IPv6
	 */
	{
		// Разбираемый ответ службы заслона IPv6
		const std::string text =
			"<?xml version=\"1.0\"?>"
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
			"<s:Body><u:GetFirewallStatusResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1\">"
			"<FirewallEnabled>1</FirewallEnabled>"
			"<InboundPinholeAllowed>1</InboundPinholeAllowed>"
			"</u:GetFirewallStatusResponse></s:Body></s:Envelope>";
		// Разобранный ответ службы устройства
		soap_t::answer_t answer;
		// Выполняем разбор ответа службы устройства
		ASSERT_TRUE(soap->parse(text, answer, error));
		// Признак того, что заслон IPv6 включён
		bool enabled = false;
		// Признак того, что пробои заслона IPv6 разрешены
		bool allowed = false;
		// Выполняем извлечение состояния заслона IPv6
		ASSERT_TRUE(upnp->firewall(answer, enabled, allowed));
		// Выполняем проверку того, что заслон IPv6 включён
		ASSERT_TRUE(enabled);
		// Выполняем проверку того, что пробои заслона IPv6 разрешены
		ASSERT_TRUE(allowed);
	}
	/**
	 * Выполняем проверку извлечения опознавателя проделанного пробоя
	 */
	{
		// Разбираемый ответ службы заслона IPv6
		const std::string text =
			"<?xml version=\"1.0\"?>"
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
			"<s:Body><u:AddPinholeResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1\">"
			"<UniqueID>65000</UniqueID>"
			"</u:AddPinholeResponse></s:Body></s:Envelope>";
		// Разобранный ответ службы устройства
		soap_t::answer_t answer;
		// Выполняем разбор ответа службы устройства
		ASSERT_TRUE(soap->parse(text, answer, error));
		// Извлекаемый опознаватель пробоя заслона
		uint16_t unique = 0;
		// Выполняем извлечение опознавателя пробоя заслона
		ASSERT_TRUE(upnp->unique(answer, unique));
		// Выполняем проверку извлечённого опознавателя пробоя заслона
		ASSERT_EQ(unique, 65000);
	}
	/**
	 * Выполняем проверку отклонения извлечения из ответа без опознавателя
	 */
	{
		// Разбираемый ответ службы заслона IPv6
		const std::string text =
			"<?xml version=\"1.0\"?>"
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
			"<s:Body><u:AddPinholeResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPv6FirewallControl:1\">"
			"</u:AddPinholeResponse></s:Body></s:Envelope>";
		// Разобранный ответ службы устройства
		soap_t::answer_t answer;
		// Выполняем разбор ответа службы устройства
		ASSERT_TRUE(soap->parse(text, answer, error));
		// Извлекаемый опознаватель пробоя заслона
		uint16_t unique = 0;
		/**
		 * Выполняем проверку отклонения извлечения опознавателя пробоя
		 *
		 * @note Без опознавателя пробой заделать нечем, и принимать такой ответ за
		 *       успех недопустимо
		 */
		ASSERT_FALSE(upnp->unique(answer, unique));
	}
}
