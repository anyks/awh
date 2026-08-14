/**
 * @file soap.cpp
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
 * @brief Проверка кодека договора SOAP — сборка вызова действия службы, разбор ответа,
 *        разбор отказа с кодом ошибки UPnP и отклонение непригодных ответов
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
 * @brief Обозначение вида службы соединения по адресу IP
 *
 */
static constexpr const char * SERVICE = "urn:schemas-upnp-org:service:WANIPConnection:1";

/**
 * @brief Проверка сборки вызова действия службы
 *
 */
TEST_F(PortmapFixture, SoapRequest) {
	// Создаём объект кодека договора SOAP
	const std::unique_ptr <soap_t> soap = this->makeSoap();
	// Перечень доводов вызова действия службы
	const std::vector <soap_t::argument_t> arguments = {
		soap_t::argument_t("NewRemoteHost", ""),
		soap_t::argument_t("NewExternalPort", "8080"),
		soap_t::argument_t("NewProtocol", "TCP"),
		soap_t::argument_t("NewPortMappingDescription", "Сервер «AWH» <рабочий>")
	};
	// Выполняем сборку вызова действия службы
	const std::string request = soap->request(SERVICE, "AddPortMapping", arguments);
	// Выполняем проверку наличия объявления разметки
	ASSERT_EQ(request.compare(0, 5, "<?xml"), 0);
	// Выполняем проверку наличия пространства имён конверта договора
	ASSERT_NE(request.find(soap_t::NAMESPACE), std::string::npos);
	// Выполняем проверку наличия правил записи содержимого конверта
	ASSERT_NE(request.find(soap_t::ENCODING), std::string::npos);
	// Выполняем проверку наличия обозначения вида службы
	ASSERT_NE(request.find(SERVICE), std::string::npos);
	// Выполняем проверку наличия вызываемого действия службы
	ASSERT_NE(request.find("AddPortMapping"), std::string::npos);
	// Выполняем проверку наличия довода вызова действия
	ASSERT_NE(request.find("<NewExternalPort>8080</NewExternalPort>"), std::string::npos);
	/**
	 * Выполняем проверку экранирования содержимого довода
	 *
	 * @note Без экранирования угловая скобка в описании разорвала бы разметку, а
	 *       служба ответила бы отказом на неправильно построенный запрос
	 */
	ASSERT_NE(request.find("&lt;рабочий&gt;"), std::string::npos);
	/**
	 * Выполняем проверку плотности записи собранного вызова
	 *
	 * @note Отступы добавляют между узлами пробельное содержимое, а часть служб
	 *       принимает его за значение довода
	 */
	ASSERT_EQ(request.find("\n"), std::string::npos);
	/**
	 * Выполняем проверку сборки обозначения вызываемого действия службы
	 *
	 * @note Кавычки договором отведены как есть: без них часть служб обозначение
	 *       не распознаёт и отвечает отказом
	 */
	ASSERT_EQ(
		soap->action(SERVICE, "AddPortMapping"),
		"\"urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping\""
	);
	/**
	 * Выполняем проверку отклонения сборки без названия действия
	 */
	ASSERT_TRUE(soap->request(SERVICE, "").empty());
	// Выполняем проверку отклонения сборки обозначения без названия действия
	ASSERT_TRUE(soap->action(SERVICE, "").empty());
}
/**
 * @brief Проверка разбора ответа службы на вызов действия
 *
 */
TEST_F(PortmapFixture, SoapResponse) {
	// Создаём объект кодека договора SOAP
	const std::unique_ptr <soap_t> soap = this->makeSoap();
	// Ответ службы на вызов действия
	const std::string data =
		"<?xml version=\"1.0\"?>"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>"
		"<u:GetExternalIPAddressResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewExternalIPAddress>203.0.113.7</NewExternalIPAddress>"
		"</u:GetExternalIPAddressResponse>"
		"</s:Body>"
		"</s:Envelope>";
	// Разобранный ответ службы
	soap_t::answer_t answer;
	// Код причины отказа кодека
	soap_t::error_t error = soap_t::error_t::NONE;
	// Выполняем разбор ответа службы
	ASSERT_TRUE(soap->parse(data, answer, error)) << message(error);
	// Выполняем проверку отсутствия отказа службы
	ASSERT_FALSE(answer.fault);
	/**
	 * Выполняем проверку названия действия, на которое получен ответ
	 *
	 * @note Приставка ответа отброшена: так название совпадает с названием
	 *       вызванного действия, и сличать их можно напрямую
	 */
	ASSERT_EQ(answer.action, "GetExternalIPAddress");
	// Выполняем проверку количества выданных службой значений
	ASSERT_EQ(answer.arguments.size(), static_cast <size_t> (1));
	// Выполняем проверку значения, выданного службой
	ASSERT_EQ(soap->value(answer, "NewExternalIPAddress"), "203.0.113.7");
	// Выполняем проверку отсутствия значения, службой не выданного
	ASSERT_TRUE(soap->value(answer, "NewInternalPort").empty());
}
/**
 * @brief Проверка разбора собранного вызова действия службы
 *
 * @details Собранный вызов разбирается тем же кодеком: так сличается вся цепочка
 *          сборки и разбора, а не каждая её половина по отдельности
 *
 */
TEST_F(PortmapFixture, SoapRoundtrip) {
	// Создаём объект кодека договора SOAP
	const std::unique_ptr <soap_t> soap = this->makeSoap();
	// Перечень доводов вызова действия службы
	const std::vector <soap_t::argument_t> arguments = {
		soap_t::argument_t("NewExternalPort", "8080"),
		soap_t::argument_t("NewProtocol", "TCP"),
		soap_t::argument_t("NewPortMappingDescription", "Сервер «AWH» <рабочий> & запасной")
	};
	// Разобранный ответ службы
	soap_t::answer_t answer;
	// Код причины отказа кодека
	soap_t::error_t error = soap_t::error_t::NONE;
	// Выполняем разбор собранного вызова действия службы
	ASSERT_TRUE(soap->parse(soap->request(SERVICE, "AddPortMapping", arguments), answer, error)) << message(error);
	// Выполняем проверку названия разобранного действия
	ASSERT_EQ(answer.action, "AddPortMapping");
	// Выполняем проверку количества разобранных доводов
	ASSERT_EQ(answer.arguments.size(), arguments.size());
	// Выполняем проверку разобранного довода вызова действия
	ASSERT_EQ(soap->value(answer, "NewProtocol"), "TCP");
	/**
	 * Выполняем проверку восстановления экранированного содержимого
	 *
	 * @note Содержимое обязано вернуться из разбора ровно тем, каким его записали:
	 *       иначе описание перенаправления дошло бы до маршрутизатора изменённым
	 */
	ASSERT_EQ(soap->value(answer, "NewPortMappingDescription"), "Сервер «AWH» <рабочий> & запасной");
}
/**
 * @brief Проверка разбора отказа службы
 *
 * @details Отказ службы сообщением построен верно и разбирается успешно: он означает,
 *          что служба до нас дошла и нас поняла. Отличать его от испорченного ответа
 *          необходимо - причина отказа лежит в коде ошибки
 *
 */
TEST_F(PortmapFixture, SoapFault) {
	// Создаём объект кодека договора SOAP
	const std::unique_ptr <soap_t> soap = this->makeSoap();
	// Отказ службы выполнить вызванное действие
	const std::string data =
		"<?xml version=\"1.0\"?>"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
		"<s:Body>"
		"<s:Fault>"
		"<faultcode>s:Client</faultcode>"
		"<faultstring>UPnPError</faultstring>"
		"<detail>"
		"<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
		"<errorCode>718</errorCode>"
		"<errorDescription>ConflictInMappingEntry</errorDescription>"
		"</UPnPError>"
		"</detail>"
		"</s:Fault>"
		"</s:Body>"
		"</s:Envelope>";
	// Разобранный ответ службы
	soap_t::answer_t answer;
	// Код причины отказа кодека
	soap_t::error_t error = soap_t::error_t::NONE;
	// Выполняем разбор отказа службы
	ASSERT_TRUE(soap->parse(data, answer, error)) << message(error);
	// Выполняем проверку отсутствия отказа кодека
	ASSERT_EQ(error, soap_t::error_t::NONE);
	// Выполняем проверку признака отказа службы
	ASSERT_TRUE(answer.fault);
	// Выполняем проверку кода ошибки, выданного службой
	ASSERT_EQ(answer.code, static_cast <uint32_t> (718));
	// Выполняем проверку описания ошибки, выданного службой
	ASSERT_EQ(answer.description, "ConflictInMappingEntry");
	/**
	 * Выполняем проверку разбора отказа без подробностей
	 *
	 * @note Код ошибки UPnP лежит не в самом отказе, а в его подробностях: сам
	 *       договор SOAP видов отказа не задаёт, и часть служб подробностей не пишет
	 */
	{
		// Отказ службы без подробностей
		const std::string plain =
			"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
			"<s:Body><s:Fault>"
			"<faultcode>s:Server</faultcode>"
			"<faultstring>Internal Error</faultstring>"
			"</s:Fault></s:Body>"
			"</s:Envelope>";
		// Выполняем разбор отказа службы без подробностей
		ASSERT_TRUE(soap->parse(plain, answer, error)) << message(error);
		// Выполняем проверку признака отказа службы
		ASSERT_TRUE(answer.fault);
		// Выполняем проверку отсутствия кода ошибки
		ASSERT_EQ(answer.code, static_cast <uint32_t> (0));
		// Выполняем проверку описания отказа, выданного службой
		ASSERT_EQ(answer.description, "Internal Error");
	}
}
/**
 * @brief Проверка отклонения непригодных ответов службы
 *
 */
TEST_F(PortmapFixture, SoapMalformed) {
	// Создаём объект кодека договора SOAP
	const std::unique_ptr <soap_t> soap = this->makeSoap();
	// Разобранный ответ службы
	soap_t::answer_t answer;
	// Код причины отказа кодека
	soap_t::error_t error = soap_t::error_t::NONE;
	// Выполняем разбор пустого ответа службы
	ASSERT_FALSE(soap->parse("", answer, error));
	// Выполняем проверку кода причины отказа кодека
	ASSERT_EQ(error, soap_t::error_t::EMPTY);
	/**
	 * Выполняем проверку отклонения неправильно построенной разметки
	 */
	{
		// Выполняем разбор неправильно построенной разметки
		ASSERT_FALSE(soap->parse("<Envelope><Body></Envelope>", answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, soap_t::error_t::MALFORMED);
	}
	/**
	 * Выполняем проверку отклонения ответа без конверта договора
	 *
	 * @note По адресу управления способно ответить что угодно: без сличения корневого
	 *       узла за ответ службы сошла бы страница ошибки маршрутизатора
	 */
	{
		// Выполняем разбор ответа без конверта договора
		ASSERT_FALSE(soap->parse("<html><body>500</body></html>", answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, soap_t::error_t::MISSING_ENVELOPE);
	}
	/**
	 * Выполняем проверку отклонения конверта без тела
	 */
	{
		// Выполняем разбор конверта без тела
		ASSERT_FALSE(soap->parse("<Envelope><Header/></Envelope>", answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, soap_t::error_t::MISSING_BODY);
	}
	/**
	 * Выполняем проверку отклонения тела без ответа и без отказа
	 */
	{
		// Выполняем разбор тела без ответа и без отказа
		ASSERT_FALSE(soap->parse("<Envelope><Body></Body></Envelope>", answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, soap_t::error_t::MISSING_ANSWER);
	}
}

/**
 * @brief Проверка отклонения довода, непригодного к записи в заголовок обмена
 *
 * @details Обозначение вызываемого действия записывается значением заголовка обмена как
 *          есть, а само обозначение службы берётся из описания устройства, полученного
 *          из сети. Конец строки внутри него разбил бы наш же запрос надвое, и за местом
 *          разрыва встал бы заголовок, вписанный выдавшим описание, - врезка эта
 *          управляется маршрутизатором и потому опасна
 *
 * @note Отвергаются все управляющие знаки, а не одни лишь концы строки: договор обмена
 *       дозволяет значению заголовка знаки видимые, пробел и знак горизонтального хода
 *
 */
TEST_F(PortmapFixture, SoapHeaderInjection) {
	// Объект кодека договора вызова действий службы
	const std::unique_ptr <soap_t> soap = this->makeSoap();
	// Перечень доводов, к записи в заголовок непригодных
	const char * hostile[] = {
		"urn:x:service:S:1\r\nX-Injected: yes",
		"urn:x:service:S:1\nX-Injected: yes",
		"urn:x:service:S:1\rX-Injected: yes",
		"urn:x:service:S:1\x01",
		"urn:x:service:S:1\x7F"
	};
	/**
	 * Выполняем перебор всех непригодных доводов
	 */
	for(const char * item : hostile){
		// Выполняем проверку отклонения непригодного обозначения службы
		ASSERT_TRUE(soap->action(item, "AddPortMapping").empty()) << item;
		// Выполняем проверку отклонения непригодного названия действия
		ASSERT_TRUE(soap->action("urn:x:service:S:1", item).empty()) << item;
	}
	/**
	 * Выполняем проверку того, что пригодные доводы принимаются
	 */
	{
		// Выполняем сборку обозначения вызываемого действия службы
		const std::string header = soap->action("urn:x:service:S:1", "AddPortMapping");
		// Выполняем проверку того, что обозначение собрано
		ASSERT_FALSE(header.empty());
		// Выполняем проверку того, что обозначение конца строки не несёт
		ASSERT_EQ(header.find('\r'), std::string::npos);
		// Выполняем проверку того, что обозначение перевода строки не несёт
		ASSERT_EQ(header.find('\n'), std::string::npos);
		// Выполняем проверку построения собранного обозначения
		ASSERT_EQ(header, "\"urn:x:service:S:1#AddPortMapping\"");
	}
	/**
	 * Выполняем проверку того, что знак горизонтального хода допущен
	 *
	 * @note Знак этот договор обмена значению заголовка дозволяет наравне с пробелом,
	 *       и отвергать его значило бы отвергать построенное правильно
	 */
	ASSERT_FALSE(soap->action("urn:x:service:S:1\tq", "AddPortMapping").empty());
}
