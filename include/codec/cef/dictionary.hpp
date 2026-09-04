/**
 * @file dictionary.hpp
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
 * \~russian
 * @brief Заголовочный файл словаря расширений контейнера CEF
 *
 * \~english
 * @brief Header file of the dictionary of the extensions of the CEF container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_CEF_DICTIONARY__
#define __AWH_CODEC_CEF_DICTIONARY__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён контейнеров данных
	 *
	 *
	 * \~english
	 * @brief Data containers namespace
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Пространство имён контейнера CEF
		 *
		 *
		 * \~english
		 * @brief CEF container namespace
		 *
		 * \~
		 */
		namespace cef {
			/**
			 * \~russian
			 * @brief Запись словаря расширений
			 *
			 * @details Запись связывает ключ, в записи CEF стоящий, с полным его именем,
			 * с видом значения и с пределом длины, описанием ArcSight заданным
			 *
			 * @note Виды словаря и полные имена держатся ВИДАМИ последовательностей
			 * знаков, а не строками: таблица укладывается в постоянную память двоичного
			 * файла целиком и не стоит ни выделения памяти, ни разбора при запуске
			 *
			 * \~english
			 * @brief Record of the dictionary of the extensions
			 * @details The record links a key standing in a CEF record with its full name,
			 * with the kind of the value and with the limit of the length given by the ArcSight specification
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Entry {
				// Ключ расширения, в записи стоящий
				string_view key;
				// Полное имя ключа расширения
				string_view name;
				// Вид значения ключа расширения
				type_t type;
				// Предел длины значения в байтах, нулём отсутствующий
				uint32_t limit;
			} entry_t;

			/**
			 * \~russian
			 * @brief Пространство имён словаря расширений
			 *
			 * @details Словарь неизменен и общ у всех потребителей: сведений о состоянии
			 * он не несёт вовсе, оттого заведён пространством имён, а не объектом
			 *
			 * @par Намеренные решения
			 *
			 * @li **Словарь неизменяем.** Расширить его своими ключами нельзя, и это
			 * не пробел: ключ, словарю неизвестный, принимается разбором при
			 * настройке сличения слабее строгой, а вид его выдаётся знаками. Иначе
			 * два потребителя одного словаря видели бы разные его составы
			 *
			 * @li **Розыск ведётся двоичным поиском по упорядоченной таблице.**
			 * Отображения с подсчётом отпечатков не заводится: таблица неизменна,
			 * укладывается в постоянную память целиком и не стоит ни выделения
			 * памяти при запуске, ни разбора её
			 *
			 * \~english
			 * @brief Namespace of the dictionary of the extensions
			 * @details The dictionary is immutable and common to all the consumers: it carries no information
			 * about a state at all, whereby it is made a namespace rather than an object
			 *
			 * \~
			 */
			namespace dictionary {
				/**
				 * \~russian
				 * @brief Метод розыска записи словаря по ключу расширения
				 *
				 * @param key ключ расширения, в записи стоящий
				 * @return    запись словаря либо ничто, если ключ словарю неизвестен
				 *
				 * \~english
				 * @brief Method of the search for a record of the dictionary by a key of an extension
				 * @param key key of an extension standing in a record
				 * @return    record of the dictionary or nothing if the key is unknown to the dictionary
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ const entry_t * find(const string_view key) noexcept;
				/**
				 * \~russian
				 * @brief Метод розыска записи словаря по полному имени ключа
				 *
				 * @details Ход этот и есть второй ход к сведённому именованию: обход
				 * дерева выдаёт ключи сырыми, а полное имя разыскивается им
				 *
				 * @param name полное имя ключа расширения
				 * @return     запись словаря либо ничто, если имя словарю неизвестно
				 *
				 * \~english
				 * @brief Method of the search for a record of the dictionary by the full name of a key
				 * @param name full name of a key of an extension
				 * @return     record of the dictionary or nothing if the name is unknown to the dictionary
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ const entry_t * search(const string_view name) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения количества ключей расширения в словаре
				 *
				 * @return количество ключей расширения в словаре
				 *
				 * \~english
				 * @brief Method of getting the number of the keys of the extensions in the dictionary
				 * @return number of the keys of the extensions in the dictionary
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ size_t size() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения записи словаря по её порядковому номеру
				 *
				 * @details Обход словаря целиком нужен справке о применении и проверке
				 * упорядоченности таблицы, розыском же пользоваться надлежит ходами
				 * `find` и `search`
				 *
				 * @param index порядковый номер записи словаря
				 * @return      запись словаря либо ничто, если номер за таблицу выходит
				 *
				 * \~english
				 * @brief Method of getting a record of the dictionary by its ordinal number
				 * @param index ordinal number of a record of the dictionary
				 * @return      record of the dictionary or nothing if the number goes beyond the table
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ const entry_t * at(const size_t index) noexcept;
			}
		}
	}
}

#endif // __AWH_CODEC_CEF_DICTIONARY__
