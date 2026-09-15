/**
 * @file writer.hpp
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
 * @brief Заголовочный файл записи событий в сообщение системного журнала
 *
 * \~english
 * @brief Header file of the writing of the events into a system log message
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

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
#include "../../sys/chrono.hpp"
#include "../abc/value.hpp"

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
			 * @brief Класс записи событий в сообщение системного журнала
			 *
			 * @details Обходит дерево контейнера ABC и собирает запись: приставку
			 * приоритета, заголовок по правилам выбранного описания, блоки
			 * структурированных данных и текст сообщения. Второго представления по
			 * дороге не заводится вовсе
			 *
			 * @par Намеренные решения
			 *
			 * Перечисленное ниже не является пробелом реализации: это очерченные границы
			 * задачи, и каждое из решений закреплено проверочным испытанием
			 *
			 * @li **Описание выбирается по СОСТАВУ дерева, а не по достатку полей.**
			 * Настройка `AUTO` берёт RFC 5424 тогда, когда дерево несёт номер описания,
			 * опознаватель сообщения либо структурированные данные, - то есть то, чего
			 * RFC 3164 не знает вовсе. Гадать по правдоподобию даты нельзя: обе записи
			 * несут дату, и запись, читанная одним описанием, обязана писаться им же
			 *
			 * @li **Отсутствующее поле пишется знаком «-», а не пропускается.** У RFC
			 * 5424 поля заголовка позиционны, и пропуск поля сдвинул бы все следующие:
			 * запись разбиралась бы, но означала иное. У RFC 3164 позиций нет, и там
			 * пустое поле опускается вместе со своим разделителем
			 *
			 * @li **Приставка приоритета НЕ домысливается.** Дерево, приоритета не
			 * несущее, собрано из записи, приставки не имевшей: дописать её значило бы
			 * объявить источник с важностью, каких отправитель не объявлял. Чтение
			 * записи без приставки отказом не отвечает, и запись отвечает ему тем же.
			 * Правило это действует и ПОЛОВИНЕ приставки: дерево, несущее лишь источник
			 * либо лишь важность, отвечается отказом, а не дополняется нулём - нуль
			 * источника есть ядро системы, а нуль важности есть «система непригодна к
			 * работе», и домыслить их значило бы поднять тревогу от имени отправителя
			 *
			 * @note Закреплено проверкой `HalfDeclaredPriorityIsRefused`
			 *
			 * @li **Дата НЕ переписывается вовсе.** Содержимое поля есть то, что
			 * положил отправитель либо потребитель, и менять его кодек не вправе:
			 * перепись была бы домысливанием - тем же, каким была бы приписка приставки
			 * приоритета. Довод особенно весом у даты: вид её несёт зону отправителя.
			 * Приведение даты к нужному виду есть дело потребителя, и ход `timestamp`
			 * события ему для того и дан
			 *
			 * @li **Поле, описанием ОБЯЗАТЕЛЬНОЕ, отсутствием отвечается отказом.** У
			 * RFC 3164 это дата и имя узла, а опознаватель работы без названия
			 * приложения невыразим вовсе: знака отсутствия описание это не знает, и
			 * подставить на место поля нечего. Собрать запись без них значило бы отдать
			 * потребителю текст, какой сам кодек прочтёт иначе
			 *
			 * @li **Знак «-» есть отсутствие значения ЛИШЬ у RFC 5424.** У RFC 3164 он
			 * законное имя узла и законная метка приложения, и толковать его отсутствием
			 * там нельзя
			 *
			 * @li **Отмена знаков ставится ЛИШЬ в значениях структурированных данных.**
			 * Описание знает ровно три отменяемых знака - кавычку, закрывающую скобку и
			 * саму обратную косую, - и лишь внутри значения. Ни заголовок, ни текст
			 * сообщения отмены не знают вовсе, и ставить её там значило бы портить
			 * содержимое
			 *
			 * @li **Метка порядка байтов ставится лишь перед текстом, ASCII не
			 * являющимся.** Описание требует её признаком того, что текст записан в
			 * UTF-8; ставить её перед текстом ASCII значило бы наращивать три октета на
			 * всякую запись без нужды. Настройкой она отключается целиком
			 *
			 * @li **Дословного совпадения записи перевод не обещает - обещает
			 * значение.** Дата выдаётся видом, описанию отвечающим, а не тем, каким
			 * стояла в исходной записи. Обратимость закрепляется сличением ДЕРЕВЬЕВ, а
			 * не текстов
			 *
			 * \~english
			 * @brief Class of the writing of the events into a system log message
			 * @details Traverses the tree of the ABC container and assembles a record: the prefix of the
			 * priority, the header by the rules of the chosen description, the blocks of the structured
			 * data and the text of the message
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
						// Описание, каким надлежит писать записи
						standard_t standard;
						// Обращение с вложенным значением, записи системного журнала неведомым
						nested_t nested;
						// Признак записи приставки приоритета перед заголовком
						bool prefix;
						// Признак постановки метки порядка байтов перед текстом сообщения
						bool bom;
						// Признак постановки отмены знаков в значениях структурированных данных
						bool escape;
						// Признак записи знака конца строки за записью
						bool terminate;
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
						 standard(standard_t::AUTO), nested(nested_t::STRICT), prefix(true),
						 bom(true), escape(true), terminate(true) {}
					} settings_t;
				private:
					// Настройки записи событий
					settings_t _settings;
				private:
					// Код ошибки последней операции записи
					error_t _error;
				private:
					// Описание, каким собрана последняя запись
					standard_t _standard;
				private:
					// Объект работы с датой и временем
					mutable chrono_t _chrono;
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
					 * @brief Метод определения описания записи по составу дерева
					 *
					 * @details Признаком служит то, чего RFC 3164 не знает вовсе: номер
					 * описания, опознаватель сообщения и структурированные данные
					 *
					 * @param value дерево контейнера ABC
					 * @return      определённое описание записи
					 *
					 * \~english
					 * @brief Method of the determination of the description of a record by the composition of a tree
					 * @param value tree of the ABC container
					 * @return      determined description of the record
					 *
					 * \~
					 */
					standard_t detect(const abc::value_t & value) const noexcept;
					/**
					 * \~russian
					 * @brief Метод постановки отмены знаков в значении структурированных данных
					 *
					 * @details Отменяются ровно три знака, описанием названные: кавычка,
					 * закрывающая скобка и сама обратная косая
					 *
					 * @param text   значение, отмены знаков требующее
					 * @param result значение с поставленной отменой знаков
					 *
					 * \~english
					 * @brief Method of the placing of the escaping of the characters in a value of the structured data
					 * @param text   value requiring the escaping of the characters
					 * @param result value with the escaping of the characters placed
					 *
					 * \~
					 */
					void escape(const string_view text, string & result) const noexcept;
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
				private:
					/**
					 * \~russian
					 * @brief Метод получения поля заголовка последовательностью знаков
					 *
					 * @param header поля заголовка, деревом объявленные
					 * @param name   имя разыскиваемого поля заголовка
					 * @param result значение поля последовательностью знаков
					 * @return       признак успешности получения поля
					 *
					 * \~english
					 * @brief Method of getting a field of the header as a sequence of characters
					 * @param header fields of the header declared by the tree
					 * @param name   name of the searched field of the header
					 * @param result value of the field as a sequence of characters
					 * @return       flag of the success of getting of the field
					 *
					 * \~
					 */
					bool extract(const abc::value_t & header, const string_view name, string & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод приведения даты сообщения к виду выбранного описания
					 *
					 * @details Дата в дереве стоит тем видом, каким стояла в исходной
					 * записи, а описания вида требуют разного: RFC 3164 - вида BSD, RFC
					 * 5424 - вида RFC 3339. Дата, ни одному из известных видов не
					 * отвечающая, ставится в запись как есть
					 *
					 * @param text     дата сообщения, деревом объявленная
					 * @param standard описание, каким собирается запись
					 * @param result   дата видом выбранного описания
					 *
					 * \~english
					 * @brief Method of the bringing of the date of a message to the appearance of the chosen description
					 * @param text     date of the message declared by the tree
					 * @param standard description the record is assembled by
					 * @param result   date in the appearance of the chosen description
					 *
					 * \~
					 */
					void timestamp(const string_view text, const standard_t standard, string & result) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки блоков структурированных данных
					 *
					 * @param value  блоки структурированных данных, деревом объявленные
					 * @param result собираемая запись
					 * @return       признак успешности сборки блоков
					 *
					 * \~english
					 * @brief Method of the assembly of the blocks of the structured data
					 * @param value  blocks of the structured data declared by the tree
					 * @param result assembled record
					 * @return       flag of the success of the assembly of the blocks
					 *
					 * \~
					 */
					bool structured(const abc::value_t & value, string & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки текста сообщения
					 *
					 * @param value  дерево контейнера ABC
					 * @param result собираемая запись
					 * @return       признак успешности сборки текста сообщения
					 *
					 * \~english
					 * @brief Method of the assembly of the text of a message
					 * @param value  tree of the ABC container
					 * @param result assembled record
					 * @return       flag of the success of the assembly of the text of the message
					 *
					 * \~
					 */
					bool payload(const abc::value_t & value, string & result) noexcept;
				private:
					/**
					 * \~russian
					 * @brief Метод сборки записи описания RFC 3164
					 *
					 * @param value  дерево контейнера ABC
					 * @param result собираемая запись
					 * @return       признак успешности сборки записи
					 *
					 * \~english
					 * @brief Method of the assembly of a record of the RFC 3164 description
					 * @param value  tree of the ABC container
					 * @param result assembled record
					 * @return       flag of the success of the assembly of the record
					 *
					 * \~
					 */
					bool legacy(const abc::value_t & value, string & result) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки записи описания RFC 5424
					 *
					 * @param value  дерево контейнера ABC
					 * @param result собираемая запись
					 * @return       признак успешности сборки записи
					 *
					 * \~english
					 * @brief Method of the assembly of a record of the RFC 5424 description
					 * @param value  tree of the ABC container
					 * @param result assembled record
					 * @return       flag of the success of the assembly of the record
					 *
					 * \~
					 */
					bool modern(const abc::value_t & value, string & result) noexcept;
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
					/**
					 * \~russian
					 * @brief Метод получения описания, каким собрана последняя запись
					 *
					 * @details При самоопределении выдаётся ОПОЗНАННОЕ описание, а не
					 * настройка `AUTO`, - тем же правилом, каким отвечает чтение
					 *
					 * @return описание, каким собрана последняя запись
					 *
					 * \~english
					 * @brief Method of getting the description the last record is assembled by
					 * @return description the last record is assembled by
					 *
					 * \~
					 */
					standard_t standard() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сборки записи системного журнала из дерева контейнера ABC
					 *
					 * @details Дерево ожидается тем же по устройству, какое собирает
					 * разбор: приоритет полем «priority», поля заголовка отображением
					 * «header», блоки данных отображением «structures», текст сообщения
					 * полем «message»
					 *
					 * @param value  дерево контейнера ABC
					 * @param result собранная запись системного журнала
					 * @return       признак успешности сборки записи
					 *
					 * \~english
					 * @brief Method of the assembly of a system log record from a tree of the ABC container
					 * @param value  tree of the ABC container
					 * @param result assembled system log record
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
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					Writer() noexcept;
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

/**
 * Возвращаем имена, системными макросами занятые
 */
#include "../../sys/macro/restore.hpp"
#include <sys/macro/global.hpp>
