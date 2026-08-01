/**
 * @file: writer.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример записи текста разметки XML — сборка запроса по договору SOAP последовательными
 *        указаниями, плотная и удобная для чтения запись и отклонение неправильных построений
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
#include <codec/xml/writer.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Обозначение пространства имён договора SOAP
 *
 */
static constexpr const char * SOAP_NAMESPACE = "http://schemas.xmlsoap.org/soap/envelope/";

/**
 * @brief Обозначение пространства имён службы соединения по договору UPnP
 *
 */
static constexpr const char * SERVICE_NAMESPACE = "urn:schemas-upnp-org:service:WANIPConnection:1";

/**
 * @brief Функция сборки запроса по договору SOAP
 *
 * @param format вид записи собираемого текста разметки
 * @return       собранный текст разметки
 *
 */
static string request(const codec::xml::format_t format) noexcept {
	// Настройки записи текста разметки
	codec::xml::writer_t::settings_t settings;
	/**
	 * Запоминаем вид записи собираемого текста разметки
	 *
	 * @note Отступы меняют содержимое документа: между узлами появляется пробельное
	 *       содержимое, которого в плотной записи нет. Там, где текст подписывается
	 *       либо сличается побайтно, годится только плотная запись
	 */
	settings.format = format;
	// Создаём объект записи текста разметки
	codec::xml::writer_t writer(settings);
	// Выполняем запись объявления разметки
	writer.declaration();
	// Выполняем открытие конверта запроса
	writer.open("Envelope", SOAP_NAMESPACE);
	// Выполняем запись правил записи содержимого конверта
	writer.attribute("encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/", SOAP_NAMESPACE);
	// Выполняем открытие тела запроса
	writer.open("Body", SOAP_NAMESPACE);
	// Выполняем открытие вызываемого действия службы
	writer.open("AddPortMapping", SERVICE_NAMESPACE);
	// Выполняем запись внешнего адреса перенаправления
	writer.element("NewRemoteHost", "");
	// Выполняем запись внешнего порта перенаправления
	writer.element("NewExternalPort", "8080");
	// Выполняем запись договора перенаправления
	writer.element("NewProtocol", "TCP");
	// Выполняем запись внутреннего порта перенаправления
	writer.element("NewInternalPort", "8080");
	// Выполняем запись внутреннего адреса перенаправления
	writer.element("NewInternalClient", "192.168.1.42");
	// Выполняем запись признака включения перенаправления
	writer.element("NewEnabled", "1");
	// Выполняем запись описания перенаправления
	writer.element("NewPortMappingDescription", "Сервер «AWH» <рабочий> & запасной");
	// Выполняем запись срока действия перенаправления
	writer.element("NewLeaseDuration", "0");
	// Выполняем закрытие вызываемого действия службы
	writer.close();
	// Выполняем закрытие тела запроса
	writer.close();
	// Выполняем закрытие конверта запроса
	writer.close();
	/**
	 * Если собранный текст разметки не завершён
	 */
	if(!writer.complete()){
		// Выводим сообщение об ошибке записи
		cout << "Ошибка записи: " << codec::xml::message(writer.error()) << endl << flush;
		// Выводим пустой текст разметки
		return string();
	}
	// Выводим собранный текст разметки
	return writer.text();
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
	// Печатаем заголовок плотной записи текста разметки
	cout << " ======== Плотная запись ======== " << endl << flush;
	// Выводим собранный запрос в плотной записи
	cout << ::request(codec::xml::format_t::COMPACT) << endl << endl << flush;
	// Печатаем заголовок записи с отступами
	cout << " ======== Запись с отступами ======== " << endl << flush;
	// Выводим собранный запрос в записи с отступами
	cout << ::request(codec::xml::format_t::PRETTY) << endl << endl << flush;
	// Печатаем заголовок отклонения неправильных построений
	cout << " ======== Отклонение неправильных построений ======== " << endl << flush;
	/**
	 * Выполняем проверку отклонения повторного имени атрибута
	 *
	 * @note Запись не позволяет собрать неправильно построенный текст: указание,
	 *       нарушающее строение, отвергается, а не записывается
	 */
	{
		// Создаём объект записи текста разметки
		codec::xml::writer_t writer;
		// Выполняем открытие узла разметки
		writer.open("mapping");
		// Выполняем запись атрибута узла
		writer.attribute("port", "8080");
		/**
		 * Если повторное имя атрибута отвергнуто
		 */
		if(!writer.attribute("port", "8081"))
			// Выводим сообщение об отклонённом указании
			cout << "Повторное имя атрибута: " << codec::xml::message(writer.error()) << endl << flush;
	}
	/**
	 * Выполняем проверку отклонения закрытия неоткрытого узла
	 */
	{
		// Создаём объект записи текста разметки
		codec::xml::writer_t writer;
		// Выполняем открытие узла разметки
		writer.open("mapping");
		// Выполняем закрытие узла разметки
		writer.close();
		/**
		 * Если закрытие неоткрытого узла отвергнуто
		 */
		if(!writer.close())
			// Выводим сообщение об отклонённом указании
			cout << "Закрытие неоткрытого узла: " << codec::xml::message(writer.error()) << endl << flush;
	}
	/**
	 * Выполняем проверку отклонения ошибочно построенного имени узла
	 */
	{
		// Создаём объект записи текста разметки
		codec::xml::writer_t writer;
		/**
		 * Если ошибочно построенное имя узла отвергнуто
		 */
		if(!writer.open("1mapping"))
			// Выводим сообщение об отклонённом указании
			cout << "Имя узла с цифры: " << codec::xml::message(writer.error()) << endl << flush;
	}
	/**
	 * Выполняем проверку отклонения недопустимого знака в примечании
	 */
	{
		// Создаём объект записи текста разметки
		codec::xml::writer_t writer;
		// Выполняем открытие узла разметки
		writer.open("mapping");
		/**
		 * Если недопустимый знак в примечании отвергнут
		 */
		if(!writer.comment(string("до\x01после")))
			// Выводим сообщение об отклонённом указании
			cout << "Управляющий знак в примечании: " << codec::xml::message(writer.error()) << endl << flush;
	}
	// Выводим результат работы приложения
	return EXIT_SUCCESS;
}
