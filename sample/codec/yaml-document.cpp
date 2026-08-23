/**
 * @file yaml-document.cpp
 * @date 2026-08-23
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
 * @brief Пример работы с деревом документа YAML — чтение значений по составному имени,
 *        обход отображений и перечней, правка записей на месте и обратная запись дерева
 *        с сохранением примечаний, оформления и порядка записей
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
#include <codec/yaml/document.hpp>
#include <sys/log.hpp>

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
 * @brief Разбираемый текст документа
 *
 */
static const char * TEXT =
 "# настройки службы\n"
 "title: пример\n"
 "\n"
 "server:\n"
 "  host: локальный # хозяин\n"
 "  port: 8080\n"
 "  mask: 0xFF\n"
 "  hosts:\n"
 "    - первый\n"
 "    - второй\n"
 "  greeting: |\n"
 "    первая строка\n"
 "    вторая строка\n"
 "\n"
 "products:\n"
 "  - name: гвоздь\n"
 "  - name: молоток\n";

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Блокируем неиспользуемую переменную
	(void) argc;
	// Блокируем неиспользуемую переменную
	(void) argv;
	// Настройки дерева документа
	codec::yaml::document_t::settings_t settings;
	/**
	 * Просим удержания исходного текста
	 *
	 * @note Удержание стоит памяти в размер текста и оттого просится прямо. Взамен
	 *       перезаписью сохраняется всё, чего правка не касалась: примечания,
	 *       оформление, порядок записей и пустые строки
	 */
	settings.retain = true;
	// Создаём объект дерева документа
	codec::yaml::document_t document(::logger(), settings);
	/**
	 * Если разбор текста документа не удался
	 */
	if(!document.parse(TEXT)){
		// Выводим сведения об обнаруженной ошибке разбора
		cout << "ошибка: " << codec::yaml::message(document.error())
		     << " в " << document.location().line << ":" << document.location().column << endl;
		// Выводим код выхода из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Получаем ссылку на корень документа
	const codec::yaml::document_t::value_t root = document.root();
	// Выводим содержимое пары верхнего уровня
	cout << "title = " << root["title"].text() << endl;
	// Выводим содержимое пары вложенного отображения
	cout << "server.host = " << root["server"]["host"].text() << endl;
	// Целое число значения пары
	int64_t port = 0;
	/**
	 * Если чтение целого числа удалось
	 *
	 * @note Число берётся из записи, а не из текста её: запись `0xFF` вернётся
	 *       записью `0xFF`, а извлечение числа выдаст 255
	 */
	if(root.at("/server/port").value(port))
		// Выводим прочитанное целое число
		cout << "server.port = " << port << endl;
	/**
	 * Выполняем перебор всех дочерних записей вложенного отображения
	 *
	 * @note Обход ведётся ссылкою на первую дочернюю запись да переходом к соседу:
	 *       перечень имён отдельно деревом не держится
	 */
	for(codec::yaml::document_t::value_t item = root["server"].begin(); item.valid(); item = item.next())
		// Выводим имя очередной дочерней записи
		cout << "  ключ server." << item.name() << endl;
	// Получаем ссылку на перечень значений
	const codec::yaml::document_t::value_t hosts = root.at("/server/hosts");
	/**
	 * Выполняем перебор всех значений перечня
	 */
	for(size_t i = 0; i < hosts.size(); i++)
		// Выводим очередное значение перечня
		cout << "  hosts[" << i << "] = " << hosts[i].text() << endl;
	// Выводим содержимое блочного значения
	cout << "server.greeting = «" << root.at("/server/greeting").text() << "»" << endl;
	/**
	 * Выполняем перебор всех записей перечня отображений
	 */
	for(size_t i = 0; i < root["products"].size(); i++)
		// Выводим содержимое пары очередной записи перечня
		cout << "  products[" << i << "].name = " << root["products"][i]["name"].text() << endl;
	/**
	 * Выполняем установку значения объявленной пары
	 *
	 * @note Значение заменяется в собственной записи пары, отчего ни порядок, ни
	 *       примечания, ни пустые строки, ни оформление соседей не страдают
	 */
	document.set("/server/port", static_cast <int64_t> (9090));
	// Выполняем заведение отсутствующей пары вложенного отображения
	document.set("/server/backlog", static_cast <int64_t> (128));
	// Выполняем объявление отсутствующего отображения
	document.arrange("/client");
	// Выполняем заведение пары объявленного отображения
	document.set("/client/retries", static_cast <int64_t> (3));
	// Выполняем удаление объявленной пары
	document.erase("/server/mask");
	// Выводим обозначение перезаписи дерева документа
	cout << endl << "== перезапись ==" << endl;
	// Выводим перезапись дерева документа
	cout << document.dump();
	// Выводим код успешного выхода из приложения
	return EXIT_SUCCESS;
}
