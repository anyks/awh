/**
 * @file toml-document.cpp
 * @date 2026-08-12
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
 * @brief Пример работы с деревом настроек TOML — чтение значений по составному имени,
 *        обход таблиц и наборов таблиц, правка записей на месте и обратная запись дерева
 *        с сохранением примечаний и порядка записей
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
#include <codec/toml/document.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Разбираемый текст настроек
 *
 */
static const char * TEXT =
 "# настройки службы\n"
 "title = \"пример\"\n"
 "\n"
 "[server]\n"
 "host = 'локальный' # хозяин\n"
 "port = 8080\n"
 "mask = 0xFF\n"
 "hosts = [\"первый\", \"второй\"]\n"
 "\n"
 "[[products]]\n"
 "name = \"гвоздь\"\n"
 "\n"
 "[[products]]\n"
 "name = \"молоток\"\n";

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
	// Создаём объект дерева настроек
	codec::toml::document_t document;
	/**
	 * Если разбор текста настроек не удался
	 */
	if(!document.parse(TEXT)){
		// Выводим сведения об обнаруженной ошибке разбора
		cout << "ошибка: " << codec::toml::message(document.error())
		     << " в " << document.errorLocation().line << ":" << document.errorLocation().column << endl;
		// Выводим код выхода из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Выводим содержимое пары верхнего уровня
	cout << "title = " << document.text({"title"}) << endl;
	// Выводим содержимое пары объявленной таблицы
	cout << "server.host = " << document.text({"server", "host"}) << endl;
	// Целое число значения пары
	int64_t port = 0;
	/**
	 * Если чтение целого числа удалось
	 */
	if(document.value(port, {"server", "port"}))
		// Выводим прочитанное целое число
		cout << "server.port = " << port << endl;
	/**
	 * Выполняем перебор всех дочерних имён объявленной таблицы
	 */
	for(auto & key : document.keys({"server"}))
		// Выводим очередное дочернее имя таблицы
		cout << "  ключ server." << key << endl;
	/**
	 * Выполняем перебор всех значений перечня
	 */
	for(size_t i = 0; i < document.length({"server", "hosts"}); i++){
		// Прочитанное значение перечня
		codec::toml::content_t value;
		/**
		 * Если чтение очередного значения перечня удалось
		 */
		if(document.item({"server", "hosts"}, i, value))
			// Выводим прочитанное значение перечня
			cout << "  hosts[" << i << "] = " << value.text << endl;
	}
	/**
	 * Выполняем перебор всех таблиц набора таблиц
	 *
	 * @note Имя у таблиц набора общее, и обратиться к ним можно лишь порядковым
	 *       номером частью составного имени
	 */
	for(size_t i = 0; i < document.count({"products"}); i++)
		// Выводим содержимое пары очередной таблицы набора
		cout << "  products[" << i << "].name = " << document.text({"products", to_string(i), "name"}) << endl;
	/**
	 * Выполняем установку значения объявленной пары
	 *
	 * @note Значение заменяется в собственной записи пары, отчего ни порядок, ни
	 *       примечания, ни пустые строки не страдают
	 */
	document.set({"server", "port"}, static_cast <int64_t> (9090));
	// Выполняем заведение отсутствующей пары объявленной таблицы
	document.set({"server", "backlog"}, static_cast <int64_t> (128));
	// Выполняем объявление отсутствующей таблицы
	document.create({"client"});
	// Выполняем заведение пары объявленной таблицы
	document.set({"client", "retries"}, static_cast <int64_t> (3));
	// Выполняем удаление объявленной пары
	document.erase({"server", "mask"});
	// Выводим обозначение перезаписи дерева настроек
	cout << endl << "== перезапись ==" << endl;
	// Выводим перезапись дерева настроек
	cout << document.text();
	// Выводим код успешного выхода из приложения
	return EXIT_SUCCESS;
}
