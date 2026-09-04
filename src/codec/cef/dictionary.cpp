/**
 * @file dictionary.cpp
 * @date 2026-09-04
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
 * @brief Реализация словаря расширений контейнера CEF — таблицы ключей расширения с их полными
 *        именами, видами значений и пределами длины, и розыска по ним двоичным поиском
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/cef/dictionary.hpp>

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже
 */
#include <sys/macro/suppress.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён контейнера CEF
	 */
	using namespace awh::codec::cef;

	/**
	 * @brief Таблица ключей расширения, ПО КЛЮЧУ упорядоченная
	 *
	 * @details Упорядоченность обязательна: розыск ведётся двоичным поиском, и нарушение
	 *          порядка обратило бы его в молчаливую выдачу отсутствия. Порядок закреплён
	 *          проверочным испытанием, а не оставлен на усмотрение правящего таблицу
	 */
	constexpr entry_t ENTRIES[] = {
	{"act", "deviceAction", type_t::STRING, 63},
	{"agentDnsDomain", "agentDnsDomain", type_t::STRING, 255},
	{"agentNtDomain", "agentNtDomain", type_t::STRING, 255},
	{"agentTranslatedAddress", "agentTranslatedAddress", type_t::ADDRESS, 0},
	{"agentTranslatedZoneExternalID", "agentTranslatedZoneExternalID", type_t::STRING, 200},
	{"agentTranslatedZoneKey", "Agent Translated Zone Key", type_t::INTEGER, 0},
	{"agentTranslatedZoneURI", "agentTranslatedZoneURI", type_t::STRING, 2048},
	{"agentZoneExternalID", "agentZoneExternalID", type_t::STRING, 200},
	{"agentZoneKey", "Agent Zone Key", type_t::INTEGER, 0},
	{"agentZoneURI", "agentZoneURI", type_t::STRING, 2048},
	{"agt", "agentAddress", type_t::ADDRESS, 0},
	{"ahost", "agentHostName", type_t::STRING, 1023},
	{"aid", "agentId", type_t::STRING, 40},
	{"amac", "agentMacAddress", type_t::MAC, 0},
	{"app", "applicationProtocol", type_t::STRING, 31},
	{"art", "agentReceiptTime", type_t::TIMESTAMP, 0},
	{"at", "agentType", type_t::STRING, 63},
	{"atz", "agentTimeZone", type_t::STRING, 255},
	{"av", "agentVersion", type_t::STRING, 31},
	{"c6a1", "deviceCustomIPv6Address1", type_t::IPV6, 0},
	{"c6a1Label", "deviceCustomIPv6Address1Label", type_t::STRING, 1023},
	{"c6a2", "deviceCustomIPv6Address2", type_t::IPV6, 0},
	{"c6a3", "deviceCustomIPv6Address3", type_t::IPV6, 0},
	{"c6a3Label", "deviceCustomIPv6Address3Label", type_t::STRING, 1023},
	{"c6a4", "deviceCustomIPv6Address4", type_t::IPV6, 0},
	{"c6a4Label", "deviceCustomIPv6Address4Label", type_t::STRING, 1023},
	{"cat", "deviceEventCategory", type_t::STRING, 1023},
	{"cfp1", "deviceCustomFloatingPoint1", type_t::DOUBLE, 0},
	{"cfp1Label", "deviceCustomFloatingPoint1Label", type_t::STRING, 1023},
	{"cfp2", "deviceCustomFloatingPoint2", type_t::DOUBLE, 0},
	{"cfp2Label", "deviceCustomFloatingPoint2 Label", type_t::STRING, 1023},
	{"cfp3", "deviceCustomFloatingPoint3", type_t::DOUBLE, 0},
	{"cfp3Label", "deviceCustomFloatingPoint3Label", type_t::STRING, 1023},
	{"cfp4", "deviceCustomFloatingPoint4", type_t::DOUBLE, 0},
	{"cfp4Label", "deviceCustomFloatingPoint4Label", type_t::STRING, 1023},
	{"cn1", "deviceCustomNumber1", type_t::INTEGER, 0},
	{"cn1Label", "deviceCustomNumber1Label", type_t::STRING, 1023},
	{"cn2", "deviceCustomNumber2", type_t::INTEGER, 0},
	{"cn2Label", "deviceCustomNumber2Label", type_t::STRING, 1023},
	{"cn3", "deviceCustomNumber3", type_t::INTEGER, 0},
	{"cn3Label", "deviceCustomNumber3Label", type_t::STRING, 1023},
	{"cnt", "baseEventCount", type_t::INTEGER, 0},
	{"cs1", "deviceCustomString1", type_t::STRING, 4000},
	{"cs1Label", "deviceCustomString1Label", type_t::STRING, 1023},
	{"cs2", "deviceCustomString2", type_t::STRING, 4000},
	{"cs2Label", "deviceCustomString2Label", type_t::STRING, 1023},
	{"cs3", "deviceCustomString3", type_t::STRING, 4000},
	{"cs3Label", "deviceCustomString3Label", type_t::STRING, 1023},
	{"cs4", "deviceCustomString4", type_t::STRING, 4000},
	{"cs4Label", "deviceCustomString4Label", type_t::STRING, 1023},
	{"cs5", "deviceCustomString5", type_t::STRING, 4000},
	{"cs5Label", "deviceCustomString5Label", type_t::STRING, 1023},
	{"cs6", "deviceCustomString6", type_t::STRING, 4000},
	{"cs6Label", "deviceCustomString6Label", type_t::STRING, 1023},
	{"customerExternalID", "customerExternalID", type_t::STRING, 200},
	{"customerKey", "Customer Key", type_t::INTEGER, 0},
	{"customerURI", "customerURI", type_t::STRING, 2048},
	{"dTranslatedZoneKey", "Destination Translated Zone Key", type_t::INTEGER, 0},
	{"dZoneKey", "Destination Zone Key", type_t::INTEGER, 0},
	{"destinationDnsDomain", "destinationDnsDomain", type_t::STRING, 255},
	{"destinationServiceName", "destinationServiceName", type_t::STRING, 1023},
	{"destinationTranslatedPort", "destinationTranslatedPort", type_t::INTEGER, 0},
	{"destinationTranslatedZoneExternalID", "destinationTranslatedZoneExternalID", type_t::STRING, 200},
	{"destinationTranslatedZoneURI", "destinationTranslatedZoneURI", type_t::STRING, 2048},
	{"destinationZoneExternalID", "destinationZoneExternalID", type_t::STRING, 200},
	{"destinationZoneURI", "destinationZoneURI", type_t::STRING, 2048},
	{"deviceCustomDate1", "deviceCustomDate1", type_t::TIMESTAMP, 0},
	{"deviceCustomDate1Label", "deviceCustomDate1Label", type_t::STRING, 1023},
	{"deviceCustomDate2", "deviceCustomDate2", type_t::TIMESTAMP, 0},
	{"deviceCustomDate2Label", "deviceCustomDate2Label", type_t::STRING, 1023},
	{"deviceDirection", "deviceDirection", type_t::INTEGER, 0},
	{"deviceDnsDomain", "deviceDnsDomain", type_t::STRING, 255},
	{"deviceExternalId", "deviceExternalId", type_t::STRING, 255},
	{"deviceFacility", "deviceFacility", type_t::STRING, 1023},
	{"deviceInboundInterface", "deviceInboundInterface", type_t::STRING, 128},
	{"deviceNtDomain", "deviceNtDomain", type_t::STRING, 255},
	{"deviceOutboundInterface", "deviceOutboundInterface", type_t::STRING, 128},
	{"deviceProcessName", "deviceProcessName", type_t::STRING, 1023},
	{"deviceTranslatedAddress", "deviceTranslatedAddress", type_t::IPV4, 0},
	{"deviceTranslatedZoneExternalID", "deviceTranslatedZoneExternalID", type_t::STRING, 200},
	{"deviceTranslatedZoneKey", "Device Translated Zone Key", type_t::INTEGER, 0},
	{"deviceTranslatedZoneURI", "deviceTranslatedZoneURI", type_t::STRING, 2048},
	{"deviceZoneExternalID", "deviceZoneExternalID", type_t::STRING, 200},
	{"deviceZoneKey", "Device Zone Key", type_t::INTEGER, 0},
	{"deviceZoneURI", "deviceZoneURI", type_t::STRING, 2048},
	{"deviceeventId", "deviceeventId", type_t::STRING, 128},
	{"dhost", "destinationHostName", type_t::STRING, 1023},
	{"dlat", "destinationGeoLatitude", type_t::DOUBLE, 0},
	{"dlong", "destinationGeoLongitude", type_t::DOUBLE, 0},
	{"dmac", "deviceMacAddress", type_t::MAC, 0},
	{"dntdom", "destinationNtDomain", type_t::STRING, 255},
	{"dpid", "destinationProcessId", type_t::INTEGER, 0},
	{"dpriv", "destinationUserPrivileges", type_t::STRING, 1023},
	{"dproc", "destinationProcessName", type_t::STRING, 1023},
	{"dpt", "destinationPort", type_t::INTEGER, 0},
	{"dst", "destinationAddress", type_t::IPV4, 0},
	{"dtz", "deviceTimeZone", type_t::STRING, 255},
	{"duid", "destinationUserId", type_t::STRING, 1023},
	{"duser", "destinationUserName", type_t::STRING, 1023},
	{"dvc", "deviceAddress", type_t::IPV4, 0},
	{"dvchost", "deviceHostName", type_t::STRING, 100},
	{"dvcpid", "deviceProcessId", type_t::INTEGER, 0},
	{"end", "endTime", type_t::TIMESTAMP, 0},
	{"eventId", "eventId", type_t::INTEGER, 0},
	{"externalId", "externalId", type_t::STRING, 40},
	{"fileCreateTime", "fileCreateTime", type_t::TIMESTAMP, 0},
	{"fileHash", "fileHash", type_t::STRING, 255},
	{"fileId", "fileId", type_t::STRING, 1023},
	{"fileModificationTime", "fileModificationTime", type_t::TIMESTAMP, 0},
	{"filePath", "filePath", type_t::STRING, 1023},
	{"filePermission", "filePermission", type_t::STRING, 1023},
	{"fileType", "fileType", type_t::STRING, 1023},
	{"flexDate1", "flexDate1", type_t::TIMESTAMP, 0},
	{"flexDate1Label", "flexDate1Label", type_t::STRING, 128},
	{"flexString1", "flexString1", type_t::STRING, 1023},
	{"flexString1Label", "flexString1Label", type_t::STRING, 128},
	{"flexString2", "flexString2", type_t::STRING, 1023},
	{"flexString2Label", "flexString2Label", type_t::STRING, 128},
	{"fname", "filename", type_t::STRING, 128},
	{"frameworkName", "Framework Name", type_t::STRING, 256},
	{"fsize", "fileSize", type_t::INTEGER, 0},
	{"in", "bytesIn", type_t::INTEGER, 0},
	{"msg", "message", type_t::STRING, 1023},
	{"oldFileCreateTime", "oldFileCreateTime", type_t::TIMESTAMP, 0},
	{"oldFileHash", "oldFileHash", type_t::STRING, 255},
	{"oldFileId", "oldFileId", type_t::STRING, 1023},
	{"oldFileModificationTime", "oldFileModificationTime", type_t::TIMESTAMP, 0},
	{"oldFileName", "oldFileName", type_t::STRING, 1023},
	{"oldFilePath", "oldFilePath", type_t::STRING, 1023},
	{"oldFilePermission", "oldFilePermission", type_t::STRING, 1023},
	{"oldFileSize", "oldFileSize", type_t::INTEGER, 0},
	{"oldFileType", "oldFileType", type_t::STRING, 1023},
	{"out", "bytesOut", type_t::INTEGER, 0},
	{"outcome", "eventOutcome", type_t::STRING, 63},
	{"proto", "transportProtocol", type_t::STRING, 31},
	{"rawEvent", "rawEvent", type_t::STRING, 4000},
	{"reason", "Reason", type_t::STRING, 1023},
	{"reportedDuration", "Reported Duration", type_t::INTEGER, 0},
	{"reportedResourceGroupName", "Reported ResourceGroup Name", type_t::STRING, 128},
	{"reportedResourceID", "Reported Resource ID", type_t::STRING, 256},
	{"reportedResourceName", "Reported Resource Name", type_t::STRING, 64},
	{"reportedResourceType", "Reported Resource Type", type_t::STRING, 64},
	{"request", "requestUrl", type_t::STRING, 1023},
	{"requestClientApplication", "requestClientApplication", type_t::STRING, 1023},
	{"requestContext", "requestContext", type_t::STRING, 2048},
	{"requestCookies", "requestCookies", type_t::STRING, 1023},
	{"requestMethod", "requestMethod", type_t::STRING, 1023},
	{"rt", "deviceReceiptTime", type_t::TIMESTAMP, 0},
	{"sTranslatedZoneKey", "Source Translated Zone Key", type_t::INTEGER, 0},
	{"sZoneKey", "Source Zone Key", type_t::INTEGER, 0},
	{"shost", "sourceHostName", type_t::STRING, 1023},
	{"slat", "sourceGeoLatitude", type_t::DOUBLE, 0},
	{"slong", "sourceGeoLongitude", type_t::DOUBLE, 0},
	{"smac", "sourceMacAddress", type_t::MAC, 0},
	{"sntdom", "sourceNtDomain", type_t::STRING, 255},
	{"sourceDnsDomain", "sourceDnsDomain", type_t::STRING, 255},
	{"sourceServiceName", "sourceServiceName", type_t::STRING, 1023},
	{"sourceTranslatedAddress", "sourceTranslatedAddress", type_t::IPV4, 0},
	{"sourceTranslatedPort", "sourceTranslatedPort", type_t::INTEGER, 0},
	{"sourceTranslatedZoneExternalID", "sourceTranslatedZoneExternalID", type_t::STRING, 200},
	{"sourceTranslatedZoneURI", "sourceTranslatedZoneURI", type_t::STRING, 2048},
	{"sourceZoneExternalID", "sourceZoneExternalID", type_t::STRING, 200},
	{"sourceZoneURI", "sourceZoneURI", type_t::STRING, 2048},
	{"spid", "sourceProcessId", type_t::INTEGER, 0},
	{"spriv", "sourceUserPrivileges", type_t::STRING, 1023},
	{"sproc", "sourceProcessName", type_t::STRING, 1023},
	{"spt", "sourcePort", type_t::INTEGER, 0},
	{"src", "sourceAddress", type_t::IPV4, 0},
	{"start", "startTime", type_t::TIMESTAMP, 0},
	{"suid", "sourceUserId", type_t::STRING, 1023},
	{"suser", "sourceUserName", type_t::STRING, 1023},
	{"threatActor", "Threat actor", type_t::STRING, 40},
	{"threatAttackID", "Threat Attack ID", type_t::STRING, 32},
	{"type", "type", type_t::INTEGER, 0}
	};

	/**
	 * @brief Указатель таблицы ключей, ПО ПОЛНОМУ ИМЕНИ упорядоченный
	 *
	 * @details Второго списка записей не заводится: держатся одни лишь указатели на записи
	 *          таблицы выше, оттого правка вида либо предела правится в одном месте
	 */
	constexpr uint16_t BY_NAME[] = {
	5, 8, 55, 57, 58, 80, 83, 119, 136, 137, 139, 140, 141, 138, 148, 149, 172, 171, 10, 1, 11, 12, 13,
	2, 15, 17, 3, 4, 6, 16, 18, 7, 9, 14, 41, 121, 132, 54, 56, 95, 59, 87, 88, 86, 90, 94, 91, 93, 60,
	61, 62, 63, 97, 98, 92, 64, 65, 0, 99, 66, 67, 68, 69, 27, 28, 29, 30, 31, 32, 33, 34, 19, 20, 21,
	22, 23, 24, 25, 35, 36, 37, 38, 39, 40, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 70, 71, 26,
	72, 73, 100, 74, 89, 75, 76, 101, 77, 147, 96, 78, 79, 81, 82, 84, 85, 102, 103, 133, 104, 105, 106,
	107, 108, 109, 110, 120, 111, 118, 112, 113, 114, 115, 116, 117, 122, 123, 124, 125, 126, 127, 128,
	129, 130, 131, 135, 143, 144, 145, 146, 142, 167, 155, 151, 152, 150, 153, 154, 166, 163, 165, 156,
	157, 158, 159, 160, 169, 170, 164, 161, 162, 168, 134, 173
	};

	/**
	 * @brief Количество ключей расширения в таблице
	 */
	constexpr size_t COUNT = (sizeof(ENTRIES) / sizeof(ENTRIES[0]));
}

/**
 * @brief Метод розыска записи словаря по ключу расширения
 *
 * @param key ключ расширения, в записи стоящий
 * @return    запись словаря либо ничто, если ключ словарю неизвестен
 */
const entry_t * awh::codec::cef::dictionary::find(const string_view key) noexcept {
	// Начало отрезка розыска в таблице
	size_t begin = 0;
	// Конец отрезка розыска в таблице
	size_t end = COUNT;
	/**
	 * Выполняем сужение отрезка розыска, пока он не исчерпан
	 */
	while(begin < end){
		// Получаем середину отрезка розыска
		const size_t middle = (begin + ((end - begin) / 2));
		// Получаем ключ записи, в середине отрезка стоящей
		const string_view current = ENTRIES[middle].key;
		// Если искомый ключ середины отрезка больше
		if(current < key)
			// Сдвигаем начало отрезка за середину его
			begin = (middle + 1);
		// Если искомый ключ середины отрезка меньше
		else if(key < current)
			// Сдвигаем конец отрезка к середине его
			end = middle;
		// Если искомый ключ середине отрезка равен
		else return &ENTRIES[middle];
	}
	// Выводим отсутствие записи словаря
	return nullptr;
}

/**
 * @brief Метод розыска записи словаря по полному имени ключа
 *
 * @param name полное имя ключа расширения
 * @return     запись словаря либо ничто, если имя словарю неизвестно
 */
const entry_t * awh::codec::cef::dictionary::search(const string_view name) noexcept {
	// Начало отрезка розыска в указателе
	size_t begin = 0;
	// Конец отрезка розыска в указателе
	size_t end = COUNT;
	/**
	 * Выполняем сужение отрезка розыска, пока он не исчерпан
	 */
	while(begin < end){
		// Получаем середину отрезка розыска
		const size_t middle = (begin + ((end - begin) / 2));
		// Получаем полное имя записи, в середине отрезка стоящей
		const string_view current = ENTRIES[BY_NAME[middle]].name;
		// Если искомое имя середины отрезка больше
		if(current < name)
			// Сдвигаем начало отрезка за середину его
			begin = (middle + 1);
		// Если искомое имя середины отрезка меньше
		else if(name < current)
			// Сдвигаем конец отрезка к середине его
			end = middle;
		// Если искомое имя середине отрезка равно
		else return &ENTRIES[BY_NAME[middle]];
	}
	// Выводим отсутствие записи словаря
	return nullptr;
}

/**
 * @brief Метод получения количества ключей расширения в словаре
 *
 * @return количество ключей расширения в словаре
 */
size_t awh::codec::cef::dictionary::size() noexcept {
	// Выводим количество ключей расширения в словаре
	return COUNT;
}

/**
 * @brief Метод получения записи словаря по её порядковому номеру
 *
 * @param index порядковый номер записи словаря
 * @return      запись словаря либо ничто, если номер за таблицу выходит
 */
const entry_t * awh::codec::cef::dictionary::at(const size_t index) noexcept {
	// Выводим запись словаря по её порядковому номеру
	return (index < COUNT ? &ENTRIES[index] : nullptr);
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
