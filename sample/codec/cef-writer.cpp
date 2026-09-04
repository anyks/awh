/**
 * @file cef-writer.cpp
 * @date 2026-09-05
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
 * @brief Пример сборки записи CEF из дерева события — построение дерева заголовка и
 *        расширения своими руками, постановка отмены знаков порознь по областям и
 *        отказ сборки на дереве, записи CEF не поддающемся
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
#include <codec/cef/writer.hpp>
#include <sys/log.hpp>

/**
 * @brief Пространство имён образца
 *
 */
namespace {
	/**
	 * @brief Функция получения объекта фреймворка
	 *
	 * @details Кодек связку берёт конструктором, а построения образца стоят и вне
	 *          main(): объект заводится статикою местною, дабы всякое построение
	 *          образца работало с одним и тем же фреймворком
	 *
	 * @return объект фреймворка
	 *
	 */
	const awh::fmk_t * framework() noexcept {
		// Объект фреймворка
		static awh::fmk_t fmk;
		// Выводим объект фреймворка
		return &fmk;
	}
	/**
	 * @brief Функция получения объекта для работы с логами
	 *
	 * @return объект для работы с логами
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект для работы с логами
		static awh::log_t log(::framework());
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
	// Создаём объект записи событий
	codec::cef::writer_t writer(::framework(), ::logger());
	// Дерево собираемого события
	codec::abc::value_t root;
	/**
	 * Выполняем построение заголовка записи
	 *
	 * @note Заголовок полей несёт ровно семь, и порядок их описанием закреплён:
	 *       version, vendor, product, release, signature, name, severity. Недостача
	 *       любого из них есть отказ сборки, ибо запись без него нечитаема
	 */
	codec::abc::value_t header;
	// Устанавливаем редакцию описания записи
	header.insert(codec::abc::value_t(string("version")), codec::abc::value_t(static_cast <int64_t> (0)));
	// Устанавливаем поставщика изделия
	header.insert(codec::abc::value_t(string("vendor")), codec::abc::value_t(string("ANYKS")));
	// Устанавливаем изделие, событие породившее
	header.insert(codec::abc::value_t(string("product")), codec::abc::value_t(string("AWH")));
	// Устанавливаем редакцию изделия
	header.insert(codec::abc::value_t(string("release")), codec::abc::value_t(string("5.0.0")));
	/**
	 * Устанавливаем опознаватель события
	 *
	 * @note Черта внутри поля заголовка отменяется писателем сама: в заголовке она
	 *       поля и разделяет, оттого писать её надлежит отменённой
	 */
	header.insert(codec::abc::value_t(string("signature")), codec::abc::value_t(string("net|drop")));
	// Устанавливаем человеческое название события
	header.insert(codec::abc::value_t(string("name")), codec::abc::value_t(string("Connection dropped")));
	// Устанавливаем важность события
	header.insert(codec::abc::value_t(string("severity")), codec::abc::value_t(static_cast <int64_t> (7)));
	/**
	 * Выполняем построение расширения записи
	 *
	 * @note Знак равенства внутри значения отменяется писателем сам: в расширении он
	 *       имя от значения и разделяет. Черта же отмены в расширении не требует
	 */
	codec::abc::value_t extension;
	// Устанавливаем адрес источника события
	extension.insert(codec::abc::value_t(string("src")), codec::abc::value_t(string("192.168.59.39")));
	// Устанавливаем порт источника события
	extension.insert(codec::abc::value_t(string("spt")), codec::abc::value_t(static_cast <int64_t> (8082)));
	// Устанавливаем сообщение события со знаком равенства внутри
	extension.insert(codec::abc::value_t(string("msg")), codec::abc::value_t(string("reason=timeout")));
	// Устанавливаем заголовок записи деревом события
	root.insert(codec::abc::value_t(string("header")), header);
	// Устанавливаем расширение записи деревом события
	root.insert(codec::abc::value_t(string("extension")), extension);
	// Собираемая запись события
	string record;
	/**
	 * Если сборка записи отказом завершилась
	 */
	if(!writer.write(root, record)){
		// Выводим сведения об обнаруженной ошибке сборки
		cout << "ошибка: " << codec::cef::message(writer.error()) << endl;
		// Выводим код выхода из приложения с ошибкой
		return EXIT_FAILURE;
	}
	// Выводим собранную запись события
	cout << record << endl;
	/**
	 * Выполняем заведение пары расширения с пробелом внутри имени
	 *
	 * @note Имя такое записи CEF не поддаётся: пробел пары и разделяет, и запись с ним
	 *       прочлась бы иначе, нежели писалась. Писатель отвечает отказом, а не пишет
	 *       заведомо испорченное
	 */
	extension.insert(codec::abc::value_t(string("bad key")), codec::abc::value_t(string("значение")));
	// Устанавливаем расширение записи деревом события
	root.insert(codec::abc::value_t(string("extension")), extension);
	// Выводим обозначение сборки дерева, записи не поддающегося
	cout << endl << "== дерево, записи не поддающееся ==" << endl;
	/**
	 * Если сборка записи отказом завершилась
	 */
	if(!writer.write(root, record))
		// Выводим сведения об обнаруженной ошибке сборки
		cout << "отказ: " << codec::cef::message(writer.error()) << endl;
	// Выводим собранную запись события
	else cout << record << endl;
	// Выводим код успешного выхода из приложения
	return EXIT_SUCCESS;
}
