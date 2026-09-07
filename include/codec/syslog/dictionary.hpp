/**
 * @file dictionary.hpp
 * @date 2026-09-07
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
 * @brief Заголовочный файл словаря источников сообщений и степеней их важности
 *
 * \~english
 * @brief Header file of the dictionary of the sources of the messages and of the degrees of their importance
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_SYSLOG_DICTIONARY__
#define __AWH_CODEC_SYSLOG_DICTIONARY__

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
 * Подавляем системные макросы, занявшие имена членов перечислений ниже
 */
#include "../../sys/macro/suppress.hpp"

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
		 * @brief Пространство имён контейнера SysLog
		 *
		 *
		 * \~english
		 * @brief SysLog container namespace
		 *
		 * \~
		 */
		namespace syslog {
			/**
			 * \~russian
			 * @brief Запись словаря источников сообщений и степеней их важности
			 *
			 * @details Запись связывает числовой код, приоритетом объявляемый, с
			 * кратким именем, службами журналов употребляемым, и с человеческим
			 * названием
			 *
			 * \~english
			 * @brief Record of the dictionary of the sources of the messages and of the degrees of their importance
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Entry {
				// Числовой код, приоритетом объявляемый
				uint8_t code;
				// Краткое имя, службами журналов употребляемое
				string_view name;
				// Человеческое название
				string_view title;
			} entry_t;

			/**
			 * \~russian
			 * @brief Пространство имён словаря источников сообщений
			 *
			 * @details Словарь неизменен и общ у всех потребителей: сведений о состоянии
			 * он не держит, и обращение к нему безопасно из любого потока
			 *
			 * \~english
			 * @brief Namespace of the dictionary of the sources of the messages
			 *
			 * \~
			 */
			namespace facilities {
				/**
				 * \~russian
				 * @brief Метод розыска источника сообщения по числовому коду
				 *
				 * @param code числовой код источника сообщения
				 * @return     запись словаря либо nullptr, если кода в словаре нет
				 *
				 * \~english
				 * @brief Method of the search for a source of a message by a numeric code
				 * @param code numeric code of the source of the message
				 * @return     record of the dictionary or nullptr if the code is not in the dictionary
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ const entry_t * at(const uint8_t code) noexcept;
				/**
				 * \~russian
				 * @brief Метод розыска источника сообщения по краткому имени
				 *
				 * @details Розыск ведётся без разбора величины букв: службы журналов
				 * пишут имена и строчными, и прописными
				 *
				 * @param name краткое имя источника сообщения
				 * @return     запись словаря либо nullptr, если имени в словаре нет
				 *
				 * \~english
				 * @brief Method of the search for a source of a message by a short name
				 * @param name short name of the source of the message
				 * @return     record of the dictionary or nullptr if the name is not in the dictionary
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ const entry_t * find(const string_view name) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения количества записей словаря
				 *
				 * @return количество записей словаря
				 *
				 * \~english
				 * @brief Method of getting the number of the records of the dictionary
				 * @return number of the records of the dictionary
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ size_t size() noexcept;
			}

			/**
			 * \~russian
			 * @brief Пространство имён словаря степеней важности сообщений
			 *
			 * @details Словарь неизменен и общ у всех потребителей: сведений о состоянии
			 * он не держит, и обращение к нему безопасно из любого потока
			 *
			 * \~english
			 * @brief Namespace of the dictionary of the degrees of the importance of the messages
			 *
			 * \~
			 */
			namespace severities {
				/**
				 * \~russian
				 * @brief Метод розыска степени важности сообщения по числовому коду
				 *
				 * @param code числовой код степени важности сообщения
				 * @return     запись словаря либо nullptr, если кода в словаре нет
				 *
				 * \~english
				 * @brief Method of the search for a degree of the importance of a message by a numeric code
				 * @param code numeric code of the degree of the importance of the message
				 * @return     record of the dictionary or nullptr if the code is not in the dictionary
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ const entry_t * at(const uint8_t code) noexcept;
				/**
				 * \~russian
				 * @brief Метод розыска степени важности сообщения по краткому имени
				 *
				 * @details Розыск ведётся без разбора величины букв и принимает имена,
				 * службами журналов принятые наравне с основными: `error` наравне с
				 * `err`, `warn` наравне с `warning`, `panic` наравне с `emerg`
				 *
				 * @param name краткое имя степени важности сообщения
				 * @return     запись словаря либо nullptr, если имени в словаре нет
				 *
				 * \~english
				 * @brief Method of the search for a degree of the importance of a message by a short name
				 * @param name short name of the degree of the importance of the message
				 * @return     record of the dictionary or nullptr if the name is not in the dictionary
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ const entry_t * find(const string_view name) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения количества записей словаря
				 *
				 * @return количество записей словаря
				 *
				 * \~english
				 * @brief Method of getting the number of the records of the dictionary
				 * @return number of the records of the dictionary
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ size_t size() noexcept;
			}
		}
	}
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include "../../sys/macro/restore.hpp"

#endif // __AWH_CODEC_SYSLOG_DICTIONARY__
