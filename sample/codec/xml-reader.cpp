/**
 * @file reader.cpp
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
 * @brief Пример потокового чтения текста разметки XML — выдача событий разбора по мере поступления
 *        кусков исходного текста, разрешение пространств имён и обработка ошибок построения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <codec/xml/reader.hpp>

/**
 * @brief Пространство имён образца
 *
 */
namespace {
	/**
	 * @brief Функция получения объекта для работы с логами
	 *
	 * @details Кодек связку берёт конструктором, а построения образца стоят и вне
	 *          main(): объект заводится статикою местною, дабы всякое построение
	 *          образца писало сообщения в один и тот же журнал
	 *
	 * @return объект для работы с логами
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект фреймворка
		static awh::fmk_t fmk;
		// Объект для работы с логами
		static awh::log_t log(&fmk);
		// Выводим объект для работы с логами
		return &log;
	}
}

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция вывода одного события разбора
 *
 * @param reader объект потокового чтения текста разметки
 *
 */
static void event(const codec::xml::reader_t & reader) noexcept {
	/**
	 * Определяем вид полученного события разбора
	 */
	switch(static_cast <uint8_t> (reader.event())){
		/**
		 * Если получено объявление разметки
		 */
		case static_cast <uint8_t> (codec::xml::event_t::DECLARATION):
			// Выводим объявленное издание разметки
			cout << "Объявление разметки: издание " << reader.text() << endl << flush;
		break;
		/**
		 * Если получено начало узла разметки
		 */
		case static_cast <uint8_t> (codec::xml::event_t::ELEMENT_OPEN): {
			// Выводим отступ по глубине вложенности узла
			cout << string(reader.depth() * 2, ' ') << "<" << reader.name().local;
			/**
			 * Если узел принадлежит пространству имён
			 */
			if(!reader.name().uri.empty())
				// Выводим обозначение пространства имён узла
				cout << " {" << reader.name().uri << "}";
			/**
			 * Выполняем перебор всех атрибутов узла
			 */
			for(const codec::xml::attribute_t & attribute : reader.attributes())
				// Выводим имя и значение очередного атрибута
				cout << " " << attribute.name.local << "=\"" << attribute.value << "\"";
			// Выводим конец открывающей метки узла
			cout << ">" << endl << flush;
		} break;
		/**
		 * Если получен конец узла разметки
		 */
		case static_cast <uint8_t> (codec::xml::event_t::ELEMENT_CLOSE):
			// Выводим закрывающую метку узла
			cout << string(reader.depth() * 2, ' ') << "</" << reader.name().local << ">" << endl << flush;
		break;
		/**
		 * Если получено текстовое содержимое узла
		 */
		case static_cast <uint8_t> (codec::xml::event_t::TEXT):
			// Выводим содержимое узла
			cout << string((reader.depth() + 1) * 2, ' ') << "«" << reader.text() << "»" << endl << flush;
		break;
		/**
		 * Если получен раздел дословного текста
		 */
		case static_cast <uint8_t> (codec::xml::event_t::CDATA):
			// Выводим содержимое раздела дословного текста
			cout << string((reader.depth() + 1) * 2, ' ') << "[дословно] " << reader.text() << endl << flush;
		break;
		/**
		 * Если получено примечание
		 */
		case static_cast <uint8_t> (codec::xml::event_t::COMMENT):
			// Выводим содержимое примечания
			cout << string(reader.depth() * 2, ' ') << "<!-- " << reader.text() << " -->" << endl << flush;
		break;
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
	// Ответ по договору SOAP, каким его отдаёт устройство по договору UPnP
	const string data =
		"<?xml version=\"1.0\"?>"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
		"<s:Body>"
		"<u:GetExternalIPAddressResponse xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewExternalIPAddress>203.0.113.7</NewExternalIPAddress>"
		"</u:GetExternalIPAddressResponse>"
		"</s:Body>"
		"</s:Envelope>";
	// Печатаем заголовок разбора текста, поданного целиком
	cout << " ======== Разбор текста целиком ======== " << endl << flush;
	/**
	 * Выполняем разбор текста разметки, поданного целиком
	 */
	{
		// Создаём объект потокового чтения текста разметки
		codec::xml::reader_t reader(::logger());
		/**
		 * Если передать текст разметки не удалось
		 */
		if(!reader.feed(data))
			// Выводим сообщение об ошибке приведения исходного текста
			cout << "Ошибка: " << codec::xml::message(reader.error()) << endl << flush;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next())
			// Выполняем вывод очередного события разбора
			::event(reader);
		/**
		 * Если разбор прекращён ошибкой
		 */
		if(reader.state() == codec::xml::state_t::FAILED)
			// Выводим сообщение об ошибке разбора с местом её обнаружения
			cout << "Ошибка: " << codec::xml::message(reader.error())
				 << " (строка " << reader.errorLocation().line
				 << ", столбец " << reader.errorLocation().column << ")" << endl << flush;
	}
	// Выводим пустую строку
	cout << endl << flush;
	// Печатаем заголовок разбора текста, поданного кусками
	cout << " ======== Разбор текста кусками по 16 октетов ======== " << endl << flush;
	/**
	 * Выполняем разбор текста разметки, поданного кусками
	 *
	 * @note Ровно так текст и приходит из сети: разбор ведётся по мере поступления
	 *       ответа, не дожидаясь его целиком. Разбиение на куски для разбора незаметно -
	 *       итог совпадает с разбором текста, поданного целиком
	 */
	{
		// Настройки разбора текста разметки
		codec::xml::reader_t::settings_t settings;
		/**
		 * Выполняем склеивание подряд идущих кусков содержимого в одно событие
		 *
		 * @note Без склеивания содержимое, разорванное границей куска, выдаётся
		 *       несколькими событиями: так оно доступно раньше, но и обрабатывать
		 *       его приходится по частям
		 */
		settings.mergeText = true;
		// Создаём объект потокового чтения текста разметки
		codec::xml::reader_t reader(::logger(), settings);
		/**
		 * Выполняем подачу текста разметки кусками
		 */
		for(size_t offset = 0; offset < data.size(); offset += 16){
			// Получаем размер очередного куска текста разметки
			const size_t size = ((offset + 16) > data.size() ? (data.size() - offset) : 16);
			/**
			 * Если передать очередной кусок текста не удалось
			 */
			if(!reader.feed(data.data() + offset, size, ((offset + size) >= data.size()))){
				// Выводим сообщение об ошибке приведения исходного текста
				cout << "Ошибка: " << codec::xml::message(reader.error()) << endl << flush;
				// Выходим из подачи текста разметки
				break;
			}
			/**
			 * Выполняем перебор всех событий, полученных из очередного куска
			 */
			while(reader.next())
				// Выполняем вывод очередного события разбора
				::event(reader);
			/**
			 * Если разбор прекращён ошибкой
			 */
			if(reader.state() == codec::xml::state_t::FAILED){
				// Выводим сообщение об ошибке разбора
				cout << "Ошибка: " << codec::xml::message(reader.error()) << endl << flush;
				// Выходим из подачи текста разметки
				break;
			}
		}
	}
	// Выводим пустую строку
	cout << endl << flush;
	// Печатаем заголовок разбора неправильно построенного текста
	cout << " ======== Разбор неправильно построенного текста ======== " << endl << flush;
	/**
	 * Выполняем разбор неправильно построенного текста разметки
	 *
	 * @note Разбор не выбрасывает исключений: признаком отказа служит состояние
	 *       вместе с кодом ошибки и местом её обнаружения в исходном тексте
	 */
	{
		// Неправильно построенный текст разметки
		const string broken = "<a>\n  <b>содержимое</c>\n</a>";
		// Создаём объект потокового чтения текста разметки
		codec::xml::reader_t reader(::logger());
		// Выполняем передачу неправильно построенного текста разметки
		reader.feed(broken);
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next());
		// Выводим сообщение об ошибке разбора с местом её обнаружения
		cout << "Ошибка: " << codec::xml::message(reader.error())
			 << " (строка " << reader.errorLocation().line
			 << ", столбец " << reader.errorLocation().column
			 << ", смещение " << reader.errorLocation().offset << ")" << endl << flush;
	}
	// Выводим результат работы приложения
	return EXIT_SUCCESS;
}
