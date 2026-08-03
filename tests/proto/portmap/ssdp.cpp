/**
 * @file: ssdp.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Проверка кодека договора SSDP — сборка запросов обнаружения, разбор ответов
 *        и оповещений устройств, сличение обозначения службы и отклонение мусора
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
 * @brief Проверка сборки запроса обнаружения устройств
 *
 */
TEST_F(PortmapFixture, SsdpSearch) {
	// Создаём объект кодека договора SSDP
	const std::unique_ptr <ssdp_t> ssdp = this->makeSsdp();
	// Выполняем сборку запроса обнаружения устройств
	const std::string request = ssdp->search(ssdp_t::TARGET_GATEWAY);
	// Выполняем проверку строки действия запроса
	ASSERT_EQ(request.compare(0, 19, "M-SEARCH * HTTP/1.1"), 0);
	// Выполняем проверку наличия группового адреса обнаружения устройств
	ASSERT_NE(request.find("HOST: 239.255.255.250:1900\r\n"), std::string::npos);
	// Выполняем проверку наличия указания на немедленную рассылку запроса
	ASSERT_NE(request.find("MAN: \"ssdp:discover\"\r\n"), std::string::npos);
	// Выполняем проверку наличия отведённого устройству срока на ответ
	ASSERT_NE(request.find("MX: 2\r\n"), std::string::npos);
	// Выполняем проверку наличия обозначения искомой службы
	ASSERT_NE(request.find(std::string("ST: ").append(ssdp_t::TARGET_GATEWAY).append("\r\n")), std::string::npos);
	/**
	 * Выполняем проверку завершения заголовка запроса пустой строкой
	 *
	 * @note Без пустой строки устройство считает заголовок незавершённым и ответа
	 *       не даёт вовсе
	 */
	ASSERT_EQ(request.compare(request.length() - 4, 4, "\r\n\r\n"), 0);
	/**
	 * Выполняем проверку сборки запроса для сети IPv6
	 *
	 * @note Адрес IPv6 обязан быть заключён в угловые скобки: без них двоеточия
	 *       адреса не отличить от разделителя порта
	 */
	{
		// Выполняем сборку запроса обнаружения устройств для сети IPv6
		const std::string six = ssdp->search(ssdp_t::TARGET_GATEWAY, 3, ssdp_t::MULTICAST_ADDRESS6);
		// Выполняем проверку наличия группового адреса сети IPv6
		ASSERT_NE(six.find("HOST: [FF02::C]:1900\r\n"), std::string::npos);
		// Выполняем проверку наличия отведённого устройству срока на ответ
		ASSERT_NE(six.find("MX: 3\r\n"), std::string::npos);
	}
	/**
	 * Выполняем проверку сборки запроса на группу площадки сети IPv6
	 *
	 * @note Групп обнаружения в сети IPv6 две, и поле обязано называть ту, на которую
	 *       запрос и рассылается: договор велит устройству сличать их, отвергая запрос
	 *       с чужим адресом
	 */
	{
		// Выполняем сборку запроса обнаружения устройств на группу площадки
		const std::string site = ssdp->search(ssdp_t::TARGET_GATEWAY, 3, ssdp_t::MULTICAST_ADDRESS6_SITE);
		// Выполняем проверку наличия группового адреса площадки сети IPv6
		ASSERT_NE(site.find("HOST: [FF05::C]:1900\r\n"), std::string::npos);
		// Выполняем проверку отсутствия группового адреса связи сети IPv6
		ASSERT_EQ(site.find("FF02::C"), std::string::npos);
	}
	/**
	 * Выполняем проверку отсечения зоны группового адреса
	 *
	 * @note Зона принадлежит машине отправителя, а не сети: записанная в поле, она
	 *       сделала бы адрес несовпадающим с тем, что видит устройство
	 */
	{
		// Выполняем сборку запроса обнаружения устройств с зоной группового адреса
		const std::string zoned = ssdp->search(ssdp_t::TARGET_GATEWAY, 3, "FF02::C%en0");
		// Выполняем проверку наличия группового адреса без зоны
		ASSERT_NE(zoned.find("HOST: [FF02::C]:1900\r\n"), std::string::npos);
		// Выполняем проверку отсутствия зоны группового адреса
		ASSERT_EQ(zoned.find("%en0"), std::string::npos);
	}
	/**
	 * Выполняем проверку отклонения сборки без обозначения искомой службы
	 */
	ASSERT_TRUE(ssdp->search("").empty());
}
/**
 * @brief Проверка разбора ответа устройства на запрос обнаружения
 *
 */
TEST_F(PortmapFixture, SsdpResponse) {
	// Создаём объект кодека договора SSDP
	const std::unique_ptr <ssdp_t> ssdp = this->makeSsdp();
	// Ответ маршрутизатора на запрос обнаружения
	const std::string data =
		"HTTP/1.1 200 OK\r\n"
		"CACHE-CONTROL: max-age=1800\r\n"
		"DATE: Sat, 02 Aug 2026 10:00:00 GMT\r\n"
		"EXT:\r\n"
		"LOCATION: http://192.168.1.1:5000/rootDesc.xml\r\n"
		"SERVER: Linux/3.4 UPnP/1.1 MiniUPnPd/2.3\r\n"
		"ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
		"USN: uuid:12345678-1234-1234-1234-123456789012::urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
		"\r\n";
	// Разобранное сообщение договора
	ssdp_t::answer_t answer;
	// Код причины отказа кодека
	ssdp_t::error_t error = ssdp_t::error_t::NONE;
	// Выполняем разбор ответа маршрутизатора
	ASSERT_TRUE(ssdp->parse(data, answer, error)) << message(error);
	// Выполняем проверку вида полученного сообщения
	ASSERT_EQ(answer.kind, ssdp_t::kind_t::RESPONSE);
	// Выполняем проверку обозначения службы, о которой сообщает устройство
	ASSERT_EQ(answer.target, ssdp_t::TARGET_GATEWAY);
	// Выполняем проверку адреса описания устройства
	ASSERT_EQ(answer.location, "http://192.168.1.1:5000/rootDesc.xml");
	// Выполняем проверку сведений об устройстве
	ASSERT_EQ(answer.server, "Linux/3.4 UPnP/1.1 MiniUPnPd/2.3");
	// Выполняем проверку срока годности полученных сведений
	ASSERT_EQ(answer.maxAge, static_cast <uint32_t> (1800));
	// Выполняем проверку пригодности обнаруженного устройства
	ASSERT_TRUE(ssdp->suitable(answer, ssdp_t::TARGET_GATEWAY));
	/**
	 * Выполняем проверку отклонения устройства с чужой службой
	 *
	 * @note Ответ приходит от всякого устройства сети: без сличения обозначения
	 *       службы за маршрутизатор сошла бы, скажем, домашняя колонка
	 */
	ASSERT_FALSE(ssdp->suitable(answer, "urn:schemas-upnp-org:device:MediaRenderer:1"));
}
/**
 * @brief Проверка разбора ответа с вольностями в написании
 *
 * @details Устройства пишут заголовок вольно: разнобой в регистре имён полей,
 *          издание HTTP 1.0 вместо 1.1, перевод строки без возврата каретки, пробелы
 *          вокруг знака равенства и лишние поля. Отвергать из-за этого обнаруженный
 *          маршрутизатор незачем
 *
 * @note Строка вовсе без разделителя имени и значения отвергается: договор HTTP
 *       такого не допускает, и разбор ведётся его же разборщиком
 *
 */
TEST_F(PortmapFixture, SsdpLenient) {
	// Создаём объект кодека договора SSDP
	const std::unique_ptr <ssdp_t> ssdp = this->makeSsdp();
	// Ответ маршрутизатора с вольностями в написании
	const std::string data =
		"HTTP/1.0 200 OK\n"
		"cache-control: max-age = 120\n"
		"Location: http://192.168.0.1:80/desc.xml\n"
		"st: urn:schemas-upnp-org:device:InternetGatewayDevice:1\n"
		"X-User-Agent: redsonic\n"
		"USN: uuid:abcdefgh::upnp:rootdevice\n"
		"\n";
	// Разобранное сообщение договора
	ssdp_t::answer_t answer;
	// Код причины отказа кодека
	ssdp_t::error_t error = ssdp_t::error_t::NONE;
	// Выполняем разбор ответа маршрутизатора
	ASSERT_TRUE(ssdp->parse(data, answer, error)) << message(error);
	// Выполняем проверку вида полученного сообщения
	ASSERT_EQ(answer.kind, ssdp_t::kind_t::RESPONSE);
	// Выполняем проверку адреса описания устройства
	ASSERT_EQ(answer.location, "http://192.168.0.1:80/desc.xml");
	// Выполняем проверку срока годности полученных сведений
	ASSERT_EQ(answer.maxAge, static_cast <uint32_t> (120));
	// Выполняем проверку пригодности обнаруженного устройства
	ASSERT_TRUE(ssdp->suitable(answer, ssdp_t::TARGET_GATEWAY));
}
/**
 * @brief Проверка разбора оповещений устройства
 *
 */
TEST_F(PortmapFixture, SsdpNotify) {
	// Создаём объект кодека договора SSDP
	const std::unique_ptr <ssdp_t> ssdp = this->makeSsdp();
	// Разобранное сообщение договора
	ssdp_t::answer_t answer;
	// Код причины отказа кодека
	ssdp_t::error_t error = ssdp_t::error_t::NONE;
	/**
	 * Выполняем проверку разбора объявления устройства в сети
	 */
	{
		// Оповещение устройства о появлении в сети
		const std::string data =
			"NOTIFY * HTTP/1.1\r\n"
			"HOST: 239.255.255.250:1900\r\n"
			"CACHE-CONTROL: max-age=1800\r\n"
			"LOCATION: http://192.168.1.1:5000/rootDesc.xml\r\n"
			"NT: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
			"NTS: ssdp:alive\r\n"
			"USN: uuid:12345678::urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
			"\r\n";
		// Выполняем разбор оповещения устройства
		ASSERT_TRUE(ssdp->parse(data, answer, error)) << message(error);
		// Выполняем проверку вида полученного сообщения
		ASSERT_EQ(answer.kind, ssdp_t::kind_t::NOTIFY);
		// Выполняем проверку вида оповещения устройства
		ASSERT_EQ(answer.notice, ssdp_t::notice_t::ALIVE);
		// Выполняем проверку обозначения службы, о которой сообщает устройство
		ASSERT_EQ(answer.target, ssdp_t::TARGET_GATEWAY);
		// Выполняем проверку пригодности обнаруженного устройства
		ASSERT_TRUE(ssdp->suitable(answer, ssdp_t::TARGET_GATEWAY));
	}
	/**
	 * Выполняем проверку разбора прощания устройства
	 *
	 * @note Прощание адреса описания не несёт, и требовать его незачем: ушедшее
	 *       устройство описывать нечем
	 */
	{
		// Оповещение устройства об уходе из сети
		const std::string data =
			"NOTIFY * HTTP/1.1\r\n"
			"HOST: 239.255.255.250:1900\r\n"
			"NT: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
			"NTS: ssdp:byebye\r\n"
			"USN: uuid:12345678::urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
			"\r\n";
		// Выполняем разбор оповещения устройства
		ASSERT_TRUE(ssdp->parse(data, answer, error)) << message(error);
		// Выполняем проверку вида оповещения устройства
		ASSERT_EQ(answer.notice, ssdp_t::notice_t::BYEBYE);
		/**
		 * Выполняем проверку непригодности покидающего сеть устройства
		 *
		 * @note Обозначение службы у прощания то же самое: без учёта вида
		 *       оповещения ушедшее устройство сошло бы за пригодное
		 */
		ASSERT_FALSE(ssdp->suitable(answer, ssdp_t::TARGET_GATEWAY));
	}
}
/**
 * @brief Проверка отклонения непригодных сообщений
 *
 */
TEST_F(PortmapFixture, SsdpMalformed) {
	// Создаём объект кодека договора SSDP
	const std::unique_ptr <ssdp_t> ssdp = this->makeSsdp();
	// Разобранное сообщение договора
	ssdp_t::answer_t answer;
	// Код причины отказа кодека
	ssdp_t::error_t error = ssdp_t::error_t::NONE;
	// Выполняем разбор пустого сообщения
	ASSERT_FALSE(ssdp->parse("", answer, error));
	// Выполняем проверку кода причины отказа кодека
	ASSERT_EQ(error, ssdp_t::error_t::EMPTY);
	/**
	 * Выполняем проверку отклонения сообщения, договору не принадлежащего
	 */
	{
		// Сообщение, договору не принадлежащее
		const std::string data = "GET / HTTP/1.1\r\nHOST: 192.168.1.1\r\n\r\n";
		// Выполняем разбор сообщения, договору не принадлежащего
		ASSERT_FALSE(ssdp->parse(data, answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, ssdp_t::error_t::UNKNOWN_METHOD);
	}
	/**
	 * Выполняем проверку отклонения ответа устройства с отказом
	 *
	 * @note Отказ здесь не испорченное сообщение, а осмысленный ответ: устройство
	 *       обнаружено, но искомой службы не имеет
	 */
	{
		// Ответ устройства с отказом
		const std::string data = "HTTP/1.1 404 Not Found\r\n\r\n";
		// Выполняем разбор ответа устройства с отказом
		ASSERT_FALSE(ssdp->parse(data, answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, ssdp_t::error_t::BAD_STATUS);
	}
	/**
	 * Выполняем проверку отклонения ответа без обозначения службы
	 */
	{
		// Ответ устройства без обозначения службы
		const std::string data =
			"HTTP/1.1 200 OK\r\n"
			"LOCATION: http://192.168.1.1:5000/rootDesc.xml\r\n"
			"\r\n";
		// Выполняем разбор ответа без обозначения службы
		ASSERT_FALSE(ssdp->parse(data, answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, ssdp_t::error_t::MISSING_TARGET);
	}
	/**
	 * Выполняем проверку отклонения ответа без адреса описания устройства
	 *
	 * @note Без адреса описания обнаруженное устройство бесполезно: перечень его
	 *       служб взять неоткуда
	 */
	{
		// Ответ устройства без адреса описания
		const std::string data =
			"HTTP/1.1 200 OK\r\n"
			"ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
			"\r\n";
		// Выполняем разбор ответа без адреса описания устройства
		ASSERT_FALSE(ssdp->parse(data, answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, ssdp_t::error_t::MISSING_LOCATION);
	}
	/**
	 * Выполняем проверку отклонения сообщения длиннее допустимого договором
	 */
	{
		// Сообщение длиннее допустимого договором
		std::string data = "HTTP/1.1 200 OK\r\nSERVER: ";
		// Выполняем дополнение сообщения до превышения предела
		data.append(ssdp_t::MAX_MESSAGE_SIZE, 'x');
		// Выполняем разбор сообщения длиннее допустимого договором
		ASSERT_FALSE(ssdp->parse(data, answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, ssdp_t::error_t::TOO_LARGE);
	}
	/**
	 * Выполняем проверку разбора собственного запроса обнаружения
	 *
	 * @note Разосланный запрос приходит и на свою же машину: отличать его от
	 *       ответа необходимо, иначе он сойдёт за обнаруженное устройство
	 */
	{
		// Выполняем разбор собственного запроса обнаружения
		ASSERT_TRUE(ssdp->parse(ssdp->search(ssdp_t::TARGET_GATEWAY), answer, error)) << message(error);
		// Выполняем проверку вида полученного сообщения
		ASSERT_EQ(answer.kind, ssdp_t::kind_t::SEARCH);
		// Выполняем проверку непригодности собственного запроса обнаружения
		ASSERT_FALSE(ssdp->suitable(answer, ssdp_t::TARGET_GATEWAY));
	}
}
