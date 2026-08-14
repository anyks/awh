/**
 * @file device.cpp
 * @date 2026-08-02
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
 * @brief Проверка кодека описания устройства UPnP — разбор описания, обход вложенных
 *        устройств, поиск службы перенаправления и сборка адресов управления
 *
 * @copyright Copyright © 2026
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
 * @brief Описание маршрутизатора, каким его отдаёт устройство по договору UPnP
 *
 * @note Служба перенаправления лежит не в корне, а двумя уровнями ниже: внутри
 *       устройства доступа в сеть, а в нём - внутри устройства соединения
 *
 */
static constexpr const char * DESCRIPTION =
	"<?xml version=\"1.0\"?>"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
	"<specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>"
	"<friendlyName>Маршрутизатор ANYKS</friendlyName>"
	"<manufacturer>ANYKS</manufacturer>"
	"<modelName>AWH-1000</modelName>"
	"<UDN>uuid:12345678-1234-1234-1234-123456789012</UDN>"
	"<serviceList>"
	"<service>"
	"<serviceType>urn:schemas-upnp-org:service:Layer3Forwarding:1</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:L3Forwarding1</serviceId>"
	"<controlURL>/ctl/L3F</controlURL>"
	"<eventSubURL>/evt/L3F</eventSubURL>"
	"<SCPDURL>/L3F.xml</SCPDURL>"
	"</service>"
	"</serviceList>"
	"<deviceList>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>"
	"<UDN>uuid:12345678-1234-1234-1234-123456789013</UDN>"
	"<deviceList>"
	"<device>"
	"<deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType>"
	"<UDN>uuid:12345678-1234-1234-1234-123456789014</UDN>"
	"<serviceList>"
	"<service>"
	"<serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>"
	"<serviceId>urn:upnp-org:serviceId:WANIPConn1</serviceId>"
	"<controlURL>/ctl/IPConn</controlURL>"
	"<eventSubURL>/evt/IPConn</eventSubURL>"
	"<SCPDURL>/WANIPCn.xml</SCPDURL>"
	"</service>"
	"</serviceList>"
	"</device>"
	"</deviceList>"
	"</device>"
	"</deviceList>"
	"</device>"
	"</root>";

/**
 * @brief Проверка разбора описания устройства
 *
 */
TEST_F(PortmapFixture, DeviceParse) {
	// Создаём объект кодека описания устройства
	const std::unique_ptr <device_t> device = this->makeDevice();
	// Разобранное описание устройства
	device_t::description_t description;
	// Код причины отказа кодека
	device_t::error_t error = device_t::error_t::NONE;
	// Выполняем разбор описания устройства
	ASSERT_TRUE(device->parse(DESCRIPTION, description, error)) << message(error);
	// Выполняем проверку обозначения вида устройства
	ASSERT_EQ(description.type, "urn:schemas-upnp-org:device:InternetGatewayDevice:1");
	// Выполняем проверку понятного человеку названия устройства
	ASSERT_EQ(description.name, "Маршрутизатор ANYKS");
	// Выполняем проверку изготовителя устройства
	ASSERT_EQ(description.manufacturer, "ANYKS");
	// Выполняем проверку обозначения изделия
	ASSERT_EQ(description.model, "AWH-1000");
	// Выполняем проверку обозначения самого устройства
	ASSERT_EQ(description.udn, "uuid:12345678-1234-1234-1234-123456789012");
	/**
	 * Выполняем проверку количества собранных служб устройства
	 *
	 * @note Собраны обе службы: и лежащая в корне, и лежащая двумя уровнями ниже
	 */
	ASSERT_EQ(description.services.size(), static_cast <size_t> (2));
	// Выполняем поиск службы перенаправления портов
	const device_t::service_t * service = device->service(description, device_t::SERVICE_WAN_IP);
	// Выполняем проверку обнаружения службы перенаправления портов
	ASSERT_NE(service, nullptr);
	// Выполняем проверку обозначения самой службы
	ASSERT_EQ(service->id, "urn:upnp-org:serviceId:WANIPConn1");
	// Выполняем проверку адреса управления службой
	ASSERT_EQ(service->control, "/ctl/IPConn");
	// Выполняем проверку адреса подписки на события службы
	ASSERT_EQ(service->event, "/evt/IPConn");
	// Выполняем проверку адреса описания действий службы
	ASSERT_EQ(service->spec, "/WANIPCn.xml");
	/**
	 * Выполняем проверку отсутствия службы, устройством не заведённой
	 */
	ASSERT_EQ(device->service(description, device_t::SERVICE_WAN_PPP), nullptr);
}
/**
 * @brief Проверка сборки полного адреса управления службой
 *
 */
TEST_F(PortmapFixture, DeviceAddress) {
	// Создаём объект кодека описания устройства
	const std::unique_ptr <device_t> device = this->makeDevice();
	// Разобранное описание устройства
	device_t::description_t description;
	// Код причины отказа кодека
	device_t::error_t error = device_t::error_t::NONE;
	// Выполняем разбор описания устройства
	ASSERT_TRUE(device->parse(DESCRIPTION, description, error)) << message(error);
	/**
	 * Выполняем проверку сборки адреса относительно адреса описания
	 *
	 * @note Основание устройством не объявлено, поэтому основанием служит сам
	 *       адрес, по которому получено описание
	 */
	ASSERT_EQ(
		device->address(description, "http://192.168.1.1:5000/rootDesc.xml", "/ctl/IPConn"),
		"http://192.168.1.1:5000/ctl/IPConn"
	);
	/**
	 * Выполняем проверку сборки адреса, записанного полным
	 *
	 * @note Устройства записывают адрес управления и путём, и полным адресом:
	 *       разрешение относительно основания годится для обоих видов
	 *
	 * @note Порт, отведённый схеме по умолчанию, из собранного адреса отбрасывается:
	 *       так адрес приводится к принятому виду, а сам порт берётся не из строки,
	 *       а разобранным числом
	 */
	ASSERT_EQ(
		device->address(description, "http://192.168.1.1:5000/rootDesc.xml", "http://10.0.0.1:80/ctl"),
		"http://10.0.0.1/ctl"
	);
	/**
	 * Выполняем проверку сборки адреса относительно объявленного основания
	 */
	{
		// Объявляем устройству основание для сборки относительных адресов
		description.base = "http://192.168.1.1:49152/";
		// Выполняем проверку сборки адреса относительно объявленного основания
		ASSERT_EQ(
			device->address(description, "http://192.168.1.1:5000/rootDesc.xml", "/ctl/IPConn"),
			"http://192.168.1.1:49152/ctl/IPConn"
		);
	}
	/**
	 * Выполняем проверку отклонения сборки пустого адреса управления
	 */
	ASSERT_TRUE(device->address(description, "http://192.168.1.1:5000/rootDesc.xml", "").empty());
}
/**
 * @brief Проверка разбора описания без пространства имён
 *
 * @details Пространство имён описания устройства опускают и пишут с опечаткой: разбор
 *          сличает лишь местные имена узлов. Описание получено у одного определённого
 *          устройства по адресу из обнаружения, и чужой разметке взяться в нём неоткуда
 *
 */
TEST_F(PortmapFixture, DeviceNamespace) {
	// Создаём объект кодека описания устройства
	const std::unique_ptr <device_t> device = this->makeDevice();
	// Описание устройства без объявления пространства имён
	const std::string data =
		"<root>"
		"<device>"
		"<deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>"
		"<UDN>uuid:00000000-0000-0000-0000-000000000001</UDN>"
		"<serviceList>"
		"<service>"
		"<serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>"
		"<controlURL>/ctl</controlURL>"
		"</service>"
		"</serviceList>"
		"</device>"
		"</root>";
	// Разобранное описание устройства
	device_t::description_t description;
	// Код причины отказа кодека
	device_t::error_t error = device_t::error_t::NONE;
	// Выполняем разбор описания устройства
	ASSERT_TRUE(device->parse(data, description, error)) << message(error);
	// Выполняем проверку количества собранных служб устройства
	ASSERT_EQ(description.services.size(), static_cast <size_t> (1));
	// Выполняем проверку обнаружения службы перенаправления портов
	ASSERT_NE(device->service(description, device_t::SERVICE_WAN_IP), nullptr);
}
/**
 * @brief Проверка отклонения непригодных описаний устройства
 *
 */
TEST_F(PortmapFixture, DeviceMalformed) {
	// Создаём объект кодека описания устройства
	const std::unique_ptr <device_t> device = this->makeDevice();
	// Разобранное описание устройства
	device_t::description_t description;
	// Код причины отказа кодека
	device_t::error_t error = device_t::error_t::NONE;
	// Выполняем разбор пустого описания устройства
	ASSERT_FALSE(device->parse("", description, error));
	// Выполняем проверку кода причины отказа кодека
	ASSERT_EQ(error, device_t::error_t::EMPTY);
	/**
	 * Выполняем проверку отклонения неправильно построенной разметки
	 */
	{
		// Выполняем разбор неправильно построенной разметки
		ASSERT_FALSE(device->parse("<root><device></root>", description, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, device_t::error_t::MALFORMED);
	}
	/**
	 * Выполняем проверку отклонения разметки с чужим корневым узлом
	 *
	 * @note Отвечать по адресу описания способно что угодно: без сличения корневого
	 *       узла за описание устройства сошла бы страница настройки маршрутизатора
	 */
	{
		// Выполняем разбор разметки с чужим корневым узлом
		ASSERT_FALSE(device->parse("<html><body>Не найдено</body></html>", description, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, device_t::error_t::MISSING_ROOT);
	}
	/**
	 * Выполняем проверку отклонения описания без самого устройства
	 */
	{
		// Выполняем разбор описания без самого устройства
		ASSERT_FALSE(device->parse("<root><specVersion><major>1</major></specVersion></root>", description, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, device_t::error_t::MISSING_SPEC);
	}
	/**
	 * Выполняем проверку отклонения устройства без обозначения
	 *
	 * @note Обозначение у устройства единственное и неизменное: им устройство и
	 *       опознаётся между обнаружениями
	 */
	{
		// Выполняем разбор описания устройства без обозначения
		ASSERT_FALSE(device->parse("<root><device><friendlyName>Без обозначения</friendlyName></device></root>", description, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, device_t::error_t::MISSING_UDN);
	}
	/**
	 * Выполняем проверку отклонения устройства без пригодных служб
	 *
	 * @note Служба без адреса управления бесполезна: обратиться к ней попросту
	 *       некуда, и в перечень она не попадает
	 */
	{
		// Описание устройства со службой без адреса управления
		const std::string data =
			"<root><device>"
			"<UDN>uuid:00000000-0000-0000-0000-000000000002</UDN>"
			"<serviceList><service>"
			"<serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>"
			"</service></serviceList>"
			"</device></root>";
		// Выполняем разбор описания устройства без пригодных служб
		ASSERT_FALSE(device->parse(data, description, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, device_t::error_t::EMPTY_SERVICE);
	}
}
