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
 * @brief Автоматические тесты словаря расширений контейнера CEF — упорядоченности таблиц,
 *        замкнутости розыска по тождеству записей, видов значений и непригодности ключей,
 *        пробел несущих
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/cef/cef.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений AWH
 */
#include <sys/macro/suppress.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Проверка упорядоченности таблиц словаря расширений
 *
 * @details Упорядоченность обязательна: розыск ведётся двоичным поиском, и нарушение
 *          порядка обратило бы его в молчаливую выдачу отсутствия. Проверка эта и
 *          закрепляет порядок, дабы правящий таблицу узнал о нарушении сразу
 *
 */
TEST(CodecCefDictionary, Ordering) {
	/**
	 * Выполняем перебор всех записей словаря расширений
	 */
	for(size_t i = 1; i < cef::dictionary::size(); i++){
		// Выполняем проверку упорядоченности таблицы по ключу расширения
		EXPECT_LT(cef::dictionary::at(i - 1)->key, cef::dictionary::at(i)->key)
			<< "нарушен порядок ключей у записи " << i;
	}
}

/**
 * @brief Проверка замкнутости розыска по тождеству записей словаря
 *
 * @details Замкнутость доказывается ТОЖДЕСТВОМ записей, а не совпадением количеств:
 *          розыск, выдающий чужую запись с тем же именем, совпадению количеств не
 *          противоречит вовсе
 *
 */
TEST(CodecCefDictionary, Closure) {
	/**
	 * Выполняем перебор всех записей словаря расширений
	 */
	for(size_t i = 0; i < cef::dictionary::size(); i++){
		// Получаем очередную запись словаря расширений
		const cef::entry_t * entry = cef::dictionary::at(i);
		// Выполняем проверку розыска записи по ключу расширения
		EXPECT_EQ(cef::dictionary::find(entry->key), entry) << "ключ: " << entry->key;
		// Выполняем проверку розыска записи по полному имени ключа
		EXPECT_EQ(cef::dictionary::search(entry->name), entry) << "полное имя: " << entry->name;
	}
}

/**
 * @brief Проверка непригодности ключей, пробел несущих
 *
 * @details Ключ с пробелом в записи CEF недостижим по устройству формата: пробел
 *          разделяет пары расширения, и совпасть такой ключ не может никогда. Словарь
 *          старого модуля нёс 41 такую запись - следы переноса строк из таблицы
 *          описания, - и все они были мёртвыми. Проверка эта стоит на страже того,
 *          чтобы они не вернулись
 *
 */
TEST(CodecCefDictionary, NoSpacedKeys) {
	/**
	 * Выполняем перебор всех записей словаря расширений
	 */
	for(size_t i = 0; i < cef::dictionary::size(); i++){
		// Получаем очередную запись словаря расширений
		const cef::entry_t * entry = cef::dictionary::at(i);
		// Выполняем проверку отсутствия пробела в ключе расширения
		EXPECT_EQ(entry->key.find(' '), string_view::npos) << "ключ с пробелом: " << entry->key;
		// Выполняем проверку непустоты ключа расширения
		EXPECT_FALSE(entry->key.empty());
		// Выполняем проверку непустоты полного имени ключа
		EXPECT_FALSE(entry->name.empty());
	}
}

/**
 * @brief Проверка видов значений, словарём заданных
 *
 */
TEST(CodecCefDictionary, Types) {
	// Выполняем проверку вида адреса устройства сети
	ASSERT_NE(cef::dictionary::find("dmac"), nullptr);
	// Выполняем проверку вида значения ключа адреса устройства сети
	EXPECT_EQ(cef::dictionary::find("dmac")->type, cef::type_t::MAC);
	// Выполняем проверку вида значения ключа метки времени
	ASSERT_NE(cef::dictionary::find("rt"), nullptr);
	// Выполняем проверку вида значения ключа метки времени
	EXPECT_EQ(cef::dictionary::find("rt")->type, cef::type_t::TIMESTAMP);
	// Выполняем проверку полного имени ключа адреса устройства сети
	EXPECT_EQ(cef::dictionary::find("dmac")->name, "deviceMacAddress");
	// Выполняем проверку розыска ключа по полному имени
	ASSERT_NE(cef::dictionary::search("deviceCustomNumber1"), nullptr);
	// Выполняем проверку ключа, полным именем разысканного
	EXPECT_EQ(cef::dictionary::search("deviceCustomNumber1")->key, "cn1");
}

/**
 * @brief Проверка отсутствия ключа, словарю неизвестного
 *
 */
TEST(CodecCefDictionary, Unknown) {
	// Выполняем проверку отсутствия ключа, словарю неизвестного
	EXPECT_EQ(cef::dictionary::find("ad.prog-id"), nullptr);
	// Выполняем проверку отсутствия полного имени, словарю неизвестного
	EXPECT_EQ(cef::dictionary::search("несуществующее имя"), nullptr);
	// Выполняем проверку отсутствия пустого ключа
	EXPECT_EQ(cef::dictionary::find(""), nullptr);
	// Выполняем проверку выхода за таблицу словаря
	EXPECT_EQ(cef::dictionary::at(cef::dictionary::size()), nullptr);
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
