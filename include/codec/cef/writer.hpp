/**
 * @file writer.hpp
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
 * @brief Заголовочный файл записи событий в запись CEF
 *
 * \~english
 * @brief Header file of the writing of the events into a CEF record
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_CEF_WRITER__
#define __AWH_CODEC_CEF_WRITER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
#include "../abc/value.hpp"

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
			 * @brief Класс записи событий в запись CEF
			 *
			 * @details Обходит дерево контейнера ABC и собирает запись CEF: приставку
			 * syslog, заголовок из семи полей и расширение из пар «ключ=значение».
			 * Второго представления по дороге не заводится вовсе
			 *
			 * @par Намеренные решения
			 *
			 * Перечисленное ниже не является пробелом реализации: это очерченные границы
			 * задачи, и каждое из решений закреплено проверочным испытанием
			 *
			 * @li **Отмена знаков ставится порознь по областям.** В заголовке отменяются
			 * прямая черта и обратная косая, а знак равенства НЕ отменяется; в
			 * расширении отменяется знак равенства и обратная косая, а прямая черта НЕ
			 * отменяется. Правило это взято прямо из описания ArcSight, и общего
			 * правила на обе области у него нет вовсе
			 *
			 * @li **Перевод строки в значении расширения записывается «\n».** Настоящий
			 * перевод строки означал бы конец записи, и запись, им разорванная,
			 * разбиралась бы двумя, из которых вторая слова «CEF:» не несёт
			 *
			 * @li **Вложенное значение обращается по правилу настройки.** Дерева
			 * произвольной глубины запись CEF не несёт: исход выбирает тот, кто пишет,
			 * а не кодек
			 *
			 * @li **Дословного совпадения записи перевод не обещает - обещает значение.**
			 * Число «1e2» выдаётся как «100», лишняя отмена снимается, порядок пар
			 * расширения берётся порядком дерева. Обратимость закрепляется сличением
			 * ДЕРЕВЬЕВ, а не текстов
			 *
			 * \~english
			 * @brief Class of the writing of the events into a CEF record
			 * @details Traverses the tree of the ABC container and assembles a CEF record: the syslog
			 * prefix, the header of seven fields and the extension of the pairs «key=value»
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Writer {
				public:
					/**
					 * \~russian
					 * @brief Настройки записи событий
					 *
					 *
					 * \~english
					 * @brief Settings of the writing of the events
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Обращение с вложенным значением, записи CEF неведомым
						nested_t nested;
						// Признак записи приставки syslog перед словом «CEF:»
						bool syslog;
						// Признак записи знака конца строки за записью
						bool terminate;
						// Номер редакции записи, словом «CEF:» объявляемый
						uint32_t version;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						Settings() noexcept :
						 nested(nested_t::STRICT), syslog(true), terminate(true), version(0) {}
					} settings_t;
				private:
					// Настройки записи событий
					settings_t _settings;
				private:
					// Код ошибки последней операции записи
					error_t _error;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект для работы с логами
					const log_t * _log;
				private:
					/**
					 * \~russian
					 * @brief Метод прекращения записи ошибкой
					 *
					 * @param error код ошибки записи
					 * @param name  имя поля, на котором запись прекращена
					 * @return      признак успешности записи
					 *
					 * \~english
					 * @brief Method of the termination of the writing by an error
					 * @param error error code of the writing
					 * @param name  name of the field the writing is terminated on
					 * @return      flag of the success of the writing
					 *
					 * \~
					 */
					bool fail(const error_t error, const string_view name) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод постановки отмены знаков в значении
					 *
					 * @details Правила отмены у областей записи РАЗНЫЕ, и область
					 * передаётся доводом именно поэтому
					 *
					 * @param text   значение, отмены знаков требующее
					 * @param area   область записи, в которую значение ставится
					 * @param result значение с поставленной отменой знаков
					 *
					 * \~english
					 * @brief Method of the placing of the escaping of the characters in a value
					 * @details The rules of the escaping of the areas of a record are DIFFERENT
					 * @param text   value requiring the escaping of the characters
					 * @param area   area of the record the value is placed into
					 * @param result value with the escaping of the characters placed
					 *
					 * \~
					 */
					void escape(const string_view text, const area_t area, string & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод обращения значения дерева в последовательность знаков
					 *
					 * @param value  значение дерева контейнера ABC
					 * @param result значение последовательностью знаков
					 * @return       признак успешности обращения значения
					 *
					 * \~english
					 * @brief Method of the conversion of a value of a tree into a sequence of characters
					 * @param value  value of a tree of the ABC container
					 * @param result value as a sequence of characters
					 * @return       flag of the success of the conversion of the value
					 *
					 * \~
					 */
					bool stringify(const abc::value_t & value, string & result) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения настроек записи событий
					 *
					 * @return настройки записи событий
					 *
					 * \~english
					 * @brief Method of getting the settings of the writing of the events
					 * @return settings of the writing of the events
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек записи событий
					 *
					 * @param settings настройки записи событий
					 *
					 * \~english
					 * @brief Method of setting the settings of the writing of the events
					 * @param settings settings of the writing of the events
					 *
					 * \~
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки записи
					 *
					 * @return код ошибки последней операции записи
					 *
					 * \~english
					 * @brief Method of getting the error code of the writing
					 * @return error code of the last operation of the writing
					 *
					 * \~
					 */
					error_t error() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сборки записи CEF из дерева контейнера ABC
					 *
					 * @details Дерево ожидается тем же по устройству, какое собирает
					 * разбор: приставка полем «syslog», поля заголовка отображением
					 * «header», пары расширения отображением «extension»
					 *
					 * @param value  дерево контейнера ABC
					 * @param result собранная запись CEF
					 * @return       признак успешности сборки записи
					 *
					 * \~english
					 * @brief Method of the assembly of a CEF record from a tree of the ABC container
					 * @details The tree is expected to be of the same construction as the one assembled by
					 * the parsing
					 * @param value  tree of the ABC container
					 * @param result assembled CEF record
					 * @return       flag of the success of the assembly of the record
					 *
					 * \~
					 */
					bool write(const abc::value_t & value, string & result) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 * @param fmk framework object
					 * @param log object for working with logs
					 *
					 * \~
					 */
					Writer(const fmk_t * fmk, const log_t * log) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					~Writer() noexcept {}
			} writer_t;
		}
	}
}

#endif // __AWH_CODEC_CEF_WRITER__
