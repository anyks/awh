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
 * Стандартный заголовочный файл
 */
#include <array>

/**
 * Подключаем заголовочный файл модуля
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
	 *          проверочным испытанием, а не оставлен на усмотрение правящего таблицу.
	 *
	 */
	constexpr entry_t ENTRIES[] = {
		{type_t::STRING, 63, "act", "deviceAction"},
		{type_t::STRING, 255, "agentDnsDomain", "agentDnsDomain"},
		{type_t::STRING, 255, "agentNtDomain", "agentNtDomain"},
		{type_t::ADDRESS, 0, "agentTranslatedAddress", "agentTranslatedAddress"},
		{type_t::STRING, 200, "agentTranslatedZoneExternalID", "agentTranslatedZoneExternalID"},
		{type_t::INTEGER, 0, "agentTranslatedZoneKey", "Agent Translated Zone Key"},
		{type_t::STRING, 2048, "agentTranslatedZoneURI", "agentTranslatedZoneURI"},
		{type_t::STRING, 200, "agentZoneExternalID", "agentZoneExternalID"},
		{type_t::INTEGER, 0, "agentZoneKey", "Agent Zone Key"},
		{type_t::STRING, 2048, "agentZoneURI", "agentZoneURI"},
		{type_t::ADDRESS, 0, "agt", "agentAddress"},
		{type_t::STRING, 1023, "ahost", "agentHostName"},
		{type_t::STRING, 40, "aid", "agentId"},
		{type_t::MAC, 0, "amac", "agentMacAddress"},
		{type_t::STRING, 31, "app", "applicationProtocol"},
		{type_t::TIMESTAMP, 0, "art", "agentReceiptTime"},
		{type_t::STRING, 63, "at", "agentType"},
		{type_t::STRING, 255, "atz", "agentTimeZone"},
		{type_t::STRING, 31, "av", "agentVersion"},
		{type_t::IPV6, 0, "c6a1", "deviceCustomIPv6Address1"},
		{type_t::STRING, 1023, "c6a1Label", "deviceCustomIPv6Address1Label"},
		{type_t::IPV6, 0, "c6a2", "deviceCustomIPv6Address2"},
		{type_t::STRING, 1023, "c6a2Label", "deviceCustomIPv6Address2Label"},
		{type_t::IPV6, 0, "c6a3", "deviceCustomIPv6Address3"},
		{type_t::STRING, 1023, "c6a3Label", "deviceCustomIPv6Address3Label"},
		{type_t::IPV6, 0, "c6a4", "deviceCustomIPv6Address4"},
		{type_t::STRING, 1023, "c6a4Label", "deviceCustomIPv6Address4Label"},
		{type_t::STRING, 1023, "cat", "deviceEventCategory"},
		{type_t::DOUBLE, 0, "cfp1", "deviceCustomFloatingPoint1"},
		{type_t::STRING, 1023, "cfp1Label", "deviceCustomFloatingPoint1Label"},
		{type_t::DOUBLE, 0, "cfp2", "deviceCustomFloatingPoint2"},
		{type_t::STRING, 1023, "cfp2Label", "deviceCustomFloatingPoint2Label"},
		{type_t::DOUBLE, 0, "cfp3", "deviceCustomFloatingPoint3"},
		{type_t::STRING, 1023, "cfp3Label", "deviceCustomFloatingPoint3Label"},
		{type_t::DOUBLE, 0, "cfp4", "deviceCustomFloatingPoint4"},
		{type_t::STRING, 1023, "cfp4Label", "deviceCustomFloatingPoint4Label"},
		{type_t::INTEGER, 0, "cn1", "deviceCustomNumber1"},
		{type_t::STRING, 1023, "cn1Label", "deviceCustomNumber1Label"},
		{type_t::INTEGER, 0, "cn2", "deviceCustomNumber2"},
		{type_t::STRING, 1023, "cn2Label", "deviceCustomNumber2Label"},
		{type_t::INTEGER, 0, "cn3", "deviceCustomNumber3"},
		{type_t::STRING, 1023, "cn3Label", "deviceCustomNumber3Label"},
		{type_t::INTEGER, 0, "cnt", "baseEventCount"},
		{type_t::STRING, 4000, "cs1", "deviceCustomString1"},
		{type_t::STRING, 1023, "cs1Label", "deviceCustomString1Label"},
		{type_t::STRING, 4000, "cs2", "deviceCustomString2"},
		{type_t::STRING, 1023, "cs2Label", "deviceCustomString2Label"},
		{type_t::STRING, 4000, "cs3", "deviceCustomString3"},
		{type_t::STRING, 1023, "cs3Label", "deviceCustomString3Label"},
		{type_t::STRING, 4000, "cs4", "deviceCustomString4"},
		{type_t::STRING, 1023, "cs4Label", "deviceCustomString4Label"},
		{type_t::STRING, 4000, "cs5", "deviceCustomString5"},
		{type_t::STRING, 1023, "cs5Label", "deviceCustomString5Label"},
		{type_t::STRING, 4000, "cs6", "deviceCustomString6"},
		{type_t::STRING, 1023, "cs6Label", "deviceCustomString6Label"},
		{type_t::STRING, 200, "customerExternalID", "customerExternalID"},
		{type_t::INTEGER, 0, "customerKey", "Customer Key"},
		{type_t::STRING, 2048, "customerURI", "customerURI"},
		{type_t::INTEGER, 0, "dTranslatedZoneKey", "Destination Translated Zone Key"},
		{type_t::INTEGER, 0, "dZoneKey", "Destination Zone Key"},
		{type_t::STRING, 255, "destinationDnsDomain", "destinationDnsDomain"},
		{type_t::STRING, 1023, "destinationServiceName", "destinationServiceName"},
		{type_t::INTEGER, 0, "destinationTranslatedPort", "destinationTranslatedPort"},
		{type_t::STRING, 200, "destinationTranslatedZoneExternalID", "destinationTranslatedZoneExternalID"},
		{type_t::STRING, 2048, "destinationTranslatedZoneURI", "destinationTranslatedZoneURI"},
		{type_t::STRING, 200, "destinationZoneExternalID", "destinationZoneExternalID"},
		{type_t::STRING, 2048, "destinationZoneURI", "destinationZoneURI"},
		{type_t::TIMESTAMP, 0, "deviceCustomDate1", "deviceCustomDate1"},
		{type_t::STRING, 1023, "deviceCustomDate1Label", "deviceCustomDate1Label"},
		{type_t::TIMESTAMP, 0, "deviceCustomDate2", "deviceCustomDate2"},
		{type_t::STRING, 1023, "deviceCustomDate2Label", "deviceCustomDate2Label"},
		{type_t::INTEGER, 0, "deviceDirection", "deviceDirection"},
		{type_t::STRING, 255, "deviceDnsDomain", "deviceDnsDomain"},
		{type_t::STRING, 255, "deviceExternalId", "deviceExternalId"},
		{type_t::STRING, 1023, "deviceFacility", "deviceFacility"},
		{type_t::STRING, 128, "deviceInboundInterface", "deviceInboundInterface"},
		{type_t::STRING, 255, "deviceNtDomain", "deviceNtDomain"},
		{type_t::STRING, 128, "deviceOutboundInterface", "deviceOutboundInterface"},
		{type_t::STRING, 1023, "deviceProcessName", "deviceProcessName"},
		{type_t::IPV4, 0, "deviceTranslatedAddress", "deviceTranslatedAddress"},
		{type_t::STRING, 200, "deviceTranslatedZoneExternalID", "deviceTranslatedZoneExternalID"},
		{type_t::INTEGER, 0, "deviceTranslatedZoneKey", "Device Translated Zone Key"},
		{type_t::STRING, 2048, "deviceTranslatedZoneURI", "deviceTranslatedZoneURI"},
		{type_t::STRING, 200, "deviceZoneExternalID", "deviceZoneExternalID"},
		{type_t::INTEGER, 0, "deviceZoneKey", "Device Zone Key"},
		{type_t::STRING, 2048, "deviceZoneURI", "deviceZoneURI"},
		{type_t::STRING, 128, "deviceeventId", "deviceeventId"},
		{type_t::STRING, 1023, "dhost", "destinationHostName"},
		{type_t::DOUBLE, 0, "dlat", "destinationGeoLatitude"},
		{type_t::DOUBLE, 0, "dlong", "destinationGeoLongitude"},
		{type_t::MAC, 0, "dmac", "deviceMacAddress"},
		{type_t::STRING, 255, "dntdom", "destinationNtDomain"},
		{type_t::INTEGER, 0, "dpid", "destinationProcessId"},
		{type_t::STRING, 1023, "dpriv", "destinationUserPrivileges"},
		{type_t::STRING, 1023, "dproc", "destinationProcessName"},
		{type_t::INTEGER, 0, "dpt", "destinationPort"},
		{type_t::IPV4, 0, "dst", "destinationAddress"},
		{type_t::STRING, 255, "dtz", "deviceTimeZone"},
		{type_t::STRING, 1023, "duid", "destinationUserId"},
		{type_t::STRING, 1023, "duser", "destinationUserName"},
		{type_t::IPV4, 0, "dvc", "deviceAddress"},
		{type_t::STRING, 100, "dvchost", "deviceHostName"},
		{type_t::INTEGER, 0, "dvcpid", "deviceProcessId"},
		{type_t::TIMESTAMP, 0, "end", "endTime"},
		{type_t::INTEGER, 0, "eventId", "eventId"},
		{type_t::STRING, 40, "externalId", "externalId"},
		{type_t::TIMESTAMP, 0, "fileCreateTime", "fileCreateTime"},
		{type_t::STRING, 255, "fileHash", "fileHash"},
		{type_t::STRING, 1023, "fileId", "fileId"},
		{type_t::TIMESTAMP, 0, "fileModificationTime", "fileModificationTime"},
		{type_t::STRING, 1023, "filePath", "filePath"},
		{type_t::STRING, 1023, "filePermission", "filePermission"},
		{type_t::STRING, 1023, "fileType", "fileType"},
		{type_t::TIMESTAMP, 0, "flexDate1", "flexDate1"},
		{type_t::STRING, 128, "flexDate1Label", "flexDate1Label"},
		{type_t::STRING, 1023, "flexString1", "flexString1"},
		{type_t::STRING, 128, "flexString1Label", "flexString1Label"},
		{type_t::STRING, 1023, "flexString2", "flexString2"},
		{type_t::STRING, 128, "flexString2Label", "flexString2Label"},
		{type_t::STRING, 128, "fname", "filename"},
		{type_t::STRING, 256, "frameworkName", "Framework Name"},
		{type_t::INTEGER, 0, "fsize", "fileSize"},
		{type_t::INTEGER, 0, "in", "bytesIn"},
		{type_t::STRING, 1023, "msg", "message"},
		{type_t::TIMESTAMP, 0, "oldFileCreateTime", "oldFileCreateTime"},
		{type_t::STRING, 255, "oldFileHash", "oldFileHash"},
		{type_t::STRING, 1023, "oldFileId", "oldFileId"},
		{type_t::TIMESTAMP, 0, "oldFileModificationTime", "oldFileModificationTime"},
		{type_t::STRING, 1023, "oldFileName", "oldFileName"},
		{type_t::STRING, 1023, "oldFilePath", "oldFilePath"},
		{type_t::STRING, 1023, "oldFilePermission", "oldFilePermission"},
		{type_t::INTEGER, 0, "oldFileSize", "oldFileSize"},
		{type_t::STRING, 1023, "oldFileType", "oldFileType"},
		{type_t::INTEGER, 0, "out", "bytesOut"},
		{type_t::STRING, 63, "outcome", "eventOutcome"},
		{type_t::STRING, 31, "proto", "transportProtocol"},
		{type_t::STRING, 4000, "rawEvent", "rawEvent"},
		{type_t::STRING, 1023, "reason", "Reason"},
		{type_t::INTEGER, 0, "reportedDuration", "Reported Duration"},
		{type_t::STRING, 128, "reportedResourceGroupName", "Reported ResourceGroup Name"},
		{type_t::STRING, 256, "reportedResourceID", "Reported Resource ID"},
		{type_t::STRING, 64, "reportedResourceName", "Reported Resource Name"},
		{type_t::STRING, 64, "reportedResourceType", "Reported Resource Type"},
		{type_t::STRING, 1023, "request", "requestUrl"},
		{type_t::STRING, 1023, "requestClientApplication", "requestClientApplication"},
		{type_t::STRING, 2048, "requestContext", "requestContext"},
		{type_t::STRING, 1023, "requestCookies", "requestCookies"},
		{type_t::STRING, 1023, "requestMethod", "requestMethod"},
		{type_t::TIMESTAMP, 0, "rt", "deviceReceiptTime"},
		{type_t::INTEGER, 0, "sTranslatedZoneKey", "Source Translated Zone Key"},
		{type_t::INTEGER, 0, "sZoneKey", "Source Zone Key"},
		{type_t::STRING, 1023, "shost", "sourceHostName"},
		{type_t::DOUBLE, 0, "slat", "sourceGeoLatitude"},
		{type_t::DOUBLE, 0, "slong", "sourceGeoLongitude"},
		{type_t::MAC, 0, "smac", "sourceMacAddress"},
		{type_t::STRING, 255, "sntdom", "sourceNtDomain"},
		{type_t::STRING, 255, "sourceDnsDomain", "sourceDnsDomain"},
		{type_t::STRING, 1023, "sourceServiceName", "sourceServiceName"},
		{type_t::IPV4, 0, "sourceTranslatedAddress", "sourceTranslatedAddress"},
		{type_t::INTEGER, 0, "sourceTranslatedPort", "sourceTranslatedPort"},
		{type_t::STRING, 200, "sourceTranslatedZoneExternalID", "sourceTranslatedZoneExternalID"},
		{type_t::STRING, 2048, "sourceTranslatedZoneURI", "sourceTranslatedZoneURI"},
		{type_t::STRING, 200, "sourceZoneExternalID", "sourceZoneExternalID"},
		{type_t::STRING, 2048, "sourceZoneURI", "sourceZoneURI"},
		{type_t::INTEGER, 0, "spid", "sourceProcessId"},
		{type_t::STRING, 1023, "spriv", "sourceUserPrivileges"},
		{type_t::STRING, 1023, "sproc", "sourceProcessName"},
		{type_t::INTEGER, 0, "spt", "sourcePort"},
		{type_t::IPV4, 0, "src", "sourceAddress"},
		{type_t::TIMESTAMP, 0, "start", "startTime"},
		{type_t::STRING, 1023, "suid", "sourceUserId"},
		{type_t::STRING, 1023, "suser", "sourceUserName"},
		{type_t::STRING, 40, "threatActor", "Threat actor"},
		{type_t::STRING, 32, "threatAttackID", "Threat Attack ID"},
		{type_t::INTEGER, 0, "type", "type"}
	};

	/**
	 * @brief Количество записей таблицы ключей расширения
	 *
	 */
	constexpr size_t COUNT = (sizeof(ENTRIES) / sizeof(ENTRIES[0]));

	/**
	 * @brief Указатель таблицы ключей, ПО ПОЛНОМУ ИМЕНИ упорядоченный
	 *
	 * @details Второго списка записей не заводится: держатся одни лишь номера записей
	 *          таблицы выше, оттого правка вида либо предела правится в одном месте.
	 *
	 * @warning Строится он ВЫЧИСЛЕНИЕМ при сборке, а не выписывается руками: прежде
	 *          номера стояли списком, и всякая вставка записи в середину таблицы сдвигала
	 *          их все разом - указатель начинал вести к чужим записям, а розыск по
	 *          полному имени отвечал отсутствием. Так и вышло при добавлении «c6a2Label»
	 *          14.09.2026: правка в одну строку сломала розыск по 60 именам сразу, и
	 *          поймала это лишь проверка замкнутости. Вычисление делает такую беду
	 *          невозможной по устройству
	 *
	 * @note Сортировка вставками взята намеренно: она коротка и при 175 записях
	 *       вычисляется собирателем мгновенно, а порядок её устойчив - записи с равными
	 *       именами сохраняют порядок таблицы
	 *
	 */
	constexpr auto BY_NAME = []() noexcept {
		// Результирующий указатель таблицы ключей
		array <uint16_t, COUNT> result{};
		/**
		 * Выполняем заполнение указателя номерами записей таблицы
		 */
		for(size_t i = 0; i < COUNT; i++)
			// Ставим очередной номер записи таблицы
			result[i] = static_cast <uint16_t> (i);
		/**
		 * Выполняем упорядочение указателя по полному имени записи
		 */
		for(size_t i = 1; i < COUNT; i++){
			// Номер записи, в упорядоченную часть вставляемый
			const uint16_t current = result[i];
			// Место вставки номера записи в упорядоченную часть
			size_t j = i;
			/**
			 * Выполняем сдвиг номеров записей, вставляемому предшествующих
			 */
			while((j > 0) && (ENTRIES[current].name < ENTRIES[result[j - 1]].name)){
				// Сдвигаем номер записи на место следующего
				result[j] = result[j - 1];
				// Переходим к предыдущему месту упорядоченной части
				j--;
			}
			// Ставим номер записи на найденное ему место
			result[j] = current;
		}
		// Выводим результирующий указатель таблицы ключей
		return result;
	}();

	/**
	 * @brief Функция поверки упорядоченности таблицы и указателя, СОБИРАТЕЛЕМ творимая
	 *
	 * @details Обе упорядоченности - таблицы по ключу и указателя по полному имени -
	 *          суть условие двоичного розыска: нарушь любую, и розыск обратится в
	 *          молчаливую выдачу отсутствия. Поверка при сборке ловит это раньше всякого
	 *          прогона, а проверочные испытания стерегут то же самое со стороны выдачи.
	 *
	 */
	constexpr bool ordered() noexcept {
		/**
		 * Выполняем перебор всех записей таблицы ключей
		 */
		for(size_t i = 1; i < COUNT; i++){
			// Если порядок таблицы по ключу расширения нарушен
			if(!(ENTRIES[i - 1].key < ENTRIES[i].key))
				// Выводим признак нарушения порядка
				return false;
			// Если порядок указателя по полному имени нарушен
			if(!(ENTRIES[BY_NAME[i - 1]].name < ENTRIES[BY_NAME[i]].name))
				// Выводим признак нарушения порядка
				return false;
		}
		// Выводим признак целости обоих порядков
		return true;
	}
	// Поверяем оба порядка при сборке модуля
	static_assert(ordered(), "The order of the extension key table or its pointer is corrupted");
};

/**
 * @brief Функция получения количества ключей расширения в словаре
 *
 * @return количество ключей расширения в словаре
 *
 */
size_t awh::codec::cef::dictionary::size() noexcept {
	// Выводим количество ключей расширения в словаре
	return COUNT;
}
/**
 * @brief Функция получения записи словаря по её порядковому номеру
 *
 * @param index порядковый номер записи словаря
 * @return      запись словаря либо ничто, если номер за таблицу выходит
 *
 */
const awh::codec::cef::entry_t * awh::codec::cef::dictionary::at(const size_t index) noexcept {
	// Выводим запись словаря по её порядковому номеру
	return (index < COUNT ? &ENTRIES[index] : nullptr);
}
/**
 * @brief Функция розыска записи словаря по ключу расширения
 *
 * @param key ключ расширения, в записи стоящий
 * @return    запись словаря либо ничто, если ключ словарю неизвестен
 *
 */
const awh::codec::cef::entry_t * awh::codec::cef::dictionary::find(const string_view key) noexcept {
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
 * @brief Функция розыска записи словаря по полному имени ключа
 *
 * @param name полное имя ключа расширения
 * @return     запись словаря либо ничто, если имя словарю неизвестно
 *
 */
const awh::codec::cef::entry_t * awh::codec::cef::dictionary::search(const string_view name) noexcept {
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
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
