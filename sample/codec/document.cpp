/**
 * @file: document.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример работы с деревом разметки XML — обход дерева узлами, поиск по имени с учётом
 *        пространства имён и обращение к атрибутам на описании устройства по договору UPnP
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/xml/document.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Обозначение пространства имён описания устройства по договору UPnP
 *
 * @note Пространство имён является частью имени узла: поиск без него узла из
 *       пространства имён не найдёт. Так и задумано - иначе ответ чужой службы
 *       с тем же местным именем прошёл бы за свой
 *
 */
static constexpr const char * DEVICE_NAMESPACE = "urn:schemas-upnp-org:device-1-0";

/**
 * @brief Функция проверки наличия вложенных узлов разметки
 *
 * @note Дерево хранит и содержимое узлов, и примечания, и разделы дословного текста,
 *       поэтому наличие вложенного узла ещё не означает наличия вложенной разметки
 *
 * @param node проверяемый узел дерева разметки
 * @return     признак наличия вложенных узлов разметки
 *
 */
static bool branch(const codec::xml::node_t & node) noexcept {
	/**
	 * Выполняем перебор всех непосредственно вложенных узлов
	 */
	for(codec::xml::node_t child = node.first(); child.valid(); child = child.next()){
		/**
		 * Если вложенный узел является узлом разметки
		 */
		if(child.kind() == codec::xml::kind_t::ELEMENT)
			// Выводим признак наличия вложенных узлов разметки
			return true;
	}
	// Выводим признак отсутствия вложенных узлов разметки
	return false;
}
/**
 * @brief Функция обхода дерева разметки вглубь
 *
 * @param node  узел, с которого начинается обход
 * @param depth текущая глубина вложенности узла
 *
 */
static void walk(const codec::xml::node_t & node, const uint32_t depth) noexcept {
	/**
	 * Выполняем перебор всех непосредственно вложенных узлов
	 */
	for(codec::xml::node_t child = node.first(); child.valid(); child = child.next()){
		/**
		 * Если вложенный узел узлом разметки не является
		 *
		 * @note Дерево хранит и содержимое узлов, и примечания, и разделы дословного
		 *       текста: для обхода строения документа нужны только узлы разметки
		 */
		if(child.kind() != codec::xml::kind_t::ELEMENT)
			// Выполняем переход к следующему вложенному узлу
			continue;
		// Выводим отступ по глубине вложенности узла и имя узла
		cout << string(depth * 2, ' ') << child.name().local;
		/**
		 * Выполняем перебор всех атрибутов узла
		 */
		for(const codec::xml::attribute_t & attribute : child.attributes())
			// Выводим имя и значение очередного атрибута
			cout << " " << attribute.name.local << "=\"" << attribute.value << "\"";
		/**
		 * Если узел вложенных узлов разметки не содержит
		 */
		if(!::branch(child) && !child.text().empty())
			// Выводим содержимое узла
			cout << " -> " << child.text();
		// Выводим конец записи узла
		cout << endl << flush;
		// Выполняем обход вложенных узлов
		::walk(child, (depth + 1));
	}
}
/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Описание устройства, каким его отдаёт устройство по договору UPnP
	const string data =
		"<?xml version=\"1.0\"?>"
		"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
		"<specVersion><major>1</major><minor>0</minor></specVersion>"
		"<device>"
		"<deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>"
		"<friendlyName>Маршрутизатор ANYKS</friendlyName>"
		"<manufacturer>ANYKS</manufacturer>"
		"<UDN>uuid:12345678-1234-1234-1234-123456789012</UDN>"
		"<serviceList>"
		"<service>"
		"<serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>"
		"<serviceId>urn:upnp-org:serviceId:WANIPConn1</serviceId>"
		"<controlURL>/ctl/IPConn</controlURL>"
		"<eventSubURL>/evt/IPConn</eventSubURL>"
		"</service>"
		"<service>"
		"<serviceType>urn:schemas-upnp-org:service:Layer3Forwarding:1</serviceType>"
		"<serviceId>urn:upnp-org:serviceId:L3Forwarding1</serviceId>"
		"<controlURL>/ctl/L3F</controlURL>"
		"<eventSubURL>/evt/L3F</eventSubURL>"
		"</service>"
		"</serviceList>"
		"</device>"
		"</root>";
	// Создаём объект дерева разметки
	codec::xml::document_t document;
	/**
	 * Если разбор описания устройства выполнить не удалось
	 */
	if(!document.parse(data)){
		// Выводим сообщение об ошибке разбора с местом её обнаружения
		cout << "Ошибка: " << codec::xml::message(document.error())
			 << " (строка " << document.errorLocation().line
			 << ", столбец " << document.errorLocation().column << ")" << endl << flush;
		// Выводим результат работы приложения
		return EXIT_FAILURE;
	}
	// Печатаем заголовок обхода дерева разметки
	cout << " ======== Обход дерева разметки ======== " << endl << flush;
	// Выполняем обход дерева разметки от корневого узла
	::walk(document.element(), 0);
	// Выводим количество узлов собранного дерева
	cout << "Узлов в дереве: " << document.size() << endl << endl << flush;
	// Печатаем заголовок выборки по имени
	cout << " ======== Выборка узлов по имени ======== " << endl << flush;
	/**
	 * Выполняем поиск узла вглубь дерева
	 *
	 * @note Поиск вглубь обходит всё поддерево и выдаёт первое совпадение: путь до
	 *       узла знать не требуется
	 */
	cout << "Название устройства: "
		 << document.element().find("friendlyName", DEVICE_NAMESPACE).text() << endl << flush;
	// Выполняем поиск узла вглубь дерева
	cout << "Обозначение устройства: "
		 << document.element().find("UDN", DEVICE_NAMESPACE).text() << endl << flush;
	/**
	 * Выполняем спуск по уровням дерева
	 *
	 * @note Поиск среди непосредственно вложенных узлов вглубь не спускается: так
	 *       узел ищется там, где он и обязан быть по договору
	 */
	const codec::xml::node_t list = document.element()
		.child("device", DEVICE_NAMESPACE)
		.child("serviceList", DEVICE_NAMESPACE);
	// Выводим количество служб устройства
	cout << "Служб устройства: " << list.children("service", DEVICE_NAMESPACE).size() << endl << flush;
	/**
	 * Выполняем перебор всех служб устройства
	 */
	for(const codec::xml::node_t & service : list.children("service", DEVICE_NAMESPACE)){
		// Выводим обозначение вида очередной службы
		cout << "  Служба: " << service.child("serviceType", DEVICE_NAMESPACE).text() << endl << flush;
		// Выводим адрес управления очередной службой
		cout << "    Управление: " << service.child("controlURL", DEVICE_NAMESPACE).text() << endl << flush;
		// Выводим адрес подписки на события очередной службы
		cout << "    Подписка:   " << service.child("eventSubURL", DEVICE_NAMESPACE).text() << endl << flush;
	}
	// Выводим пустую строку
	cout << endl << flush;
	// Печатаем заголовок обращения к непригодному узлу
	cout << " ======== Обращение к отсутствующему узлу ======== " << endl << flush;
	/**
	 * Выполняем поиск узла, которого в дереве нет
	 *
	 * @note Обход от непригодного узла снова даёт непригодный узел, а его содержимое
	 *       и атрибуты пусты: цепочку обращений можно записывать без проверки на
	 *       каждом шаге, проверив лишь итог
	 */
	const codec::xml::node_t missing = document.element()
		.find("presentationURL", DEVICE_NAMESPACE)
		.child("nothing")
		.next();
	// Выводим признак пригодности отсутствующего узла
	cout << "Узел обнаружен: " << (missing.valid() ? "да" : "нет") << endl << flush;
	// Выводим содержимое отсутствующего узла
	cout << "Содержимое пусто: " << (missing.text().empty() ? "да" : "нет") << endl << flush;
	/**
	 * Выполняем поиск узла без учёта пространства имён
	 *
	 * @note Пространство имён является частью имени: тот же узел без обозначения
	 *       пространства имён не находится
	 */
	cout << "Поиск без пространства имён: "
		 << (document.element().find("friendlyName").valid() ? "найден" : "не найден") << endl << flush;
	// Выводим результат работы приложения
	return EXIT_SUCCESS;
}
