/**
 * @file bridge.hpp
 * @date 2026-09-02
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
 * @brief Заголовочный файл моста между контейнером ABC и текстовыми кодеками
 *
 * \~english
 * @brief Header file of the bridge between the container ABC and the text codecs
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_BRIDGE__
#define __AWH_CODEC_BRIDGE__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../sys/log.hpp"
#include "../sys/global.hpp"
#include "abc/value.hpp"
#include "json/json.hpp"
#include "yaml/yaml.hpp"
#include "xml/xml.hpp"
#include "toml/toml.hpp"
#include "ini/ini.hpp"

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
		 * @brief Мост между контейнером ABC и текстовыми кодеками
		 *
		 * @details Переводит дерево значений контейнера ABC в запись любого из текстовых
		 * кодеков и обратно. Осью перевода служит именно ABC: система видов его вмещает
		 * виды всех текстовых кодеков разом, оттого перевод между двумя текстовыми
		 * кодеками идёт через одно промежуточное дерево, а не через пятнадцать пар
		 *
		 * @par Намеренные решения
		 *
		 * Перечисленное ниже не является пробелом реализации: это очерченные границы
		 * задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Сужение выдаётся отказом, а не молчаливой порчей.** Виды ABC, записи
		 * кодека неведомые, обращаются по правилу, настройками заданному: либо в
		 * последовательность знаков, либо в отказ с кодом. Молчаливая же порча
		 * оставляла бы потребителя с записью, которая разбирается, но означает иное
		 *
		 * @li **Перевод ведётся обходом дерева, а не разбором записи.** Дерево ABC
		 * подаётся писателю кодека событиями, и второго представления по дороге не
		 * заводится вовсе
		 *
		 * @li **Дословного совпадения записи перевод не обещает - обещает значение.**
		 * Запись «1e2» выдаётся как «100.0», «1.50» как «1.5», лишнее экранирование
		 * снимается, а суррогатная пара обращается самим знаком. Это записанный
		 * договор кодеков, а не изъян моста: разбор хранит число, а не знаки его.
		 * Обратимость закреплена сличением ДЕРЕВЬЕВ, а не текстов
		 *
		 * @li **Число сверх родных видов выдаётся последовательностью знаков.** Вид
		 * `EXTENDED` записи JSON укладывается в ABC записью числа знаками, ибо
		 * перевод числа в родной вид терял бы разряды. Запись числа берётся ходом
		 * `raw()`, а НЕ `text()`: последний выдаёт содержимое лишь у строковых узлов.
		 * Причина знаков лежит не у ABC - ход `Value::decimal` вид этот собрать
		 * позволяет, - а у самого JSON: величину числа он держит записью знаков, а
		 * не октетами, и накормить `decimal` мосту нечем, не разбирая десятичную
		 * запись произвольной длины самому
		 *
		 * \~english
		 * @brief Bridge between the container ABC and the text codecs
		 * @details Translates a tree of the values of the container ABC into a record of any of the text
		 * codecs and back. Precisely ABC serves as the axis of the translation: its system of the kinds encompasses
		 * the kinds of all the text codecs at once, whereby a translation between two text
		 * codecs goes through one intermediate tree rather than through fifteen pairs
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Bridge {
			public:
				/**
				 * \~russian
				 * @brief Коды отказов перевода
				 *
				 *
				 * \~english
				 * @brief Error codes of the translation
				 *
				 * \~
				 */
				enum class error_t : uint8_t {
					NONE        = 0x00, // Отказа не произошло
					PARSING     = 0x01, // Разбор записи кодеком окончился отказом
					WRITING     = 0x02, // Запись кодеком окончилась отказом
					UNSUPPORTED = 0x03, // Записи кодека вид значения неведом
					DEEP_TREE   = 0x04  // Глубина дерева превысила предел перевода
				};
				/**
				 * \~russian
				 * @brief Вид записи, с которым мост переводит дерево
				 *
				 * @details Кодек CSV в перечень не входит намеренно: содержимое его
				 *          таблично, а не древовидно, и настройками оно не бывает
				 *
				 * \~english
				 * @brief Kind of a record with which the bridge translates a tree
				 * @details The codec CSV is deliberately not in the list: its content is
				 *          tabular, not tree-shaped, and it is never settings
				 *
				 * \~
				 */
				enum class format_t : uint8_t {
					NONE = 0x00, // Вид записи не задан
					ABC  = 0x01, // Запись контейнера ABC как она есть
					JSON = 0x02, // Запись JSON
					XML  = 0x03, // Запись XML
					YAML = 0x04, // Запись YAML
					TOML = 0x05, // Запись TOML
					INI  = 0x06  // Запись INI
				};
				/**
				 * \~russian
				 * @brief Правила обращения с видами, записи кодека неведомыми
				 *
				 *
				 * \~english
				 * @brief Rules of the treatment of the kinds unknown to the record of a codec
				 *
				 * \~
				 */
				enum class narrow_t : uint8_t {
					STRICT = 0x00, // Отвечать отказом с кодом UNSUPPORTED
					TEXT   = 0x01, // Обращать в последовательность знаков
					SKIP   = 0x02  // Пропускать значение вовсе
				};
				/**
				 * \~russian
				 * @brief Настройки перевода
				 *
				 *
				 * \~english
				 * @brief Settings of the translation
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Settings {
					// Правило обращения с видами, записи кодека неведомыми
					narrow_t narrow;
					// Вид оформления собираемой записи
					json::format_t format;
					// Предельная глубина дерева перевода
					uint32_t depth;
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
					Settings() noexcept : narrow(narrow_t::TEXT), format(json::format_t::PRETTY), depth(512) {}
				} settings_t;
			private:
				// Настройки перевода
				settings_t _settings;
			private:
				// Код отказа последнего перевода
				error_t _error;
			private:
				// Объект работы с логами
				const log_t * _log;
			private:
				/**
				 * \~russian
				 * @brief Метод подачи значения ABC писателю JSON
				 *
				 * @param value  значение контейнера ABC
				 * @param writer писатель записи JSON
				 * @param depth  глубина обхода дерева
				 * @return       результат подачи
				 *
				 * \~english
				 * @brief Method of the submission of a value of ABC to the writer of JSON
				 * @param value value of the container ABC
				 * @param writer writer of a record of JSON
				 * @param depth depth of the traversal of the tree
				 * @return result of the submission
				 *
				 * \~
				 */
				[[nodiscard]] bool feed(const abc::value_t & value, json::writer_t & writer, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки значения JSON в значение ABC
				 *
				 * @param value  значение документа JSON
				 * @param result укладываемое значение контейнера ABC
				 * @param depth  глубина обхода дерева
				 * @return       результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value of JSON into a value of ABC
				 * @param value value of a document of JSON
				 * @param result laid value of the container ABC
				 * @param depth depth of the traversal of the tree
				 * @return result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool absorb(const json::Document::value_t & value, abc::value_t & result, const uint32_t depth) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись JSON
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись JSON
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of JSON
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of JSON
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encodeJSON(const abc::value_t & value, string & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи JSON в дерево ABC
				 *
				 * @param text   запись JSON для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of JSON into a tree of ABC
				 * @param text record of JSON for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeJSON(const string_view text, abc::value_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия отменяющей записи со звена пути
				 *
				 * @details Звенья пути выдаются кодеками с отменяющими записями RFC 6901:
				 *          имя `a/b` записывается звеном `a~1b`, имя `a~b` - звеном `a~0b`.
				 *          Ход возвращает звену исходное имя
				 *
				 * @param link звено пути с отменяющими записями
				 * @return     исходное имя потомка
				 *
				 * \~english
				 * @brief Method of the removal of the escaping from a link of a path
				 * @param link link of a path with the escaping
				 * @return original name of the child
				 *
				 * \~
				 */
				[[nodiscard]] string unescape(const string & link) const noexcept;
				/**
				 * \~russian
				 * @brief Метод наложения отменяющей записи на имя потомка
				 *
				 * @details Ход обратный снятию: имя `a/b` обращается в звено `a~1b`,
				 *          имя `a~b` - в звено `a~0b`. Без него имя, косую черту
				 *          несущее, разошлось бы у кодека на два звена пути
				 *
				 * @param name исходное имя потомка
				 * @return     звено пути с отменяющими записями
				 *
				 * \~english
				 * @brief Method of the applying of the escaping to a name of a child
				 * @param name original name of the child
				 * @return link of a path with the escaping
				 *
				 * \~
				 */
				[[nodiscard]] string escape(const string & name) const noexcept;
				/**
				 * \~russian
				 * @brief Метод подачи значения ABC документу YAML
				 *
				 * @param value    значение контейнера ABC
								 * @param result   собираемое значение записи YAML
				 * @param depth    глубина обхода дерева
				 * @return         результат подачи
				 *
				 * \~english
				 * @brief Method of the submission of a value of ABC to the document of YAML
				 * @param value value of the container ABC
				 * @param document assembled document of a record of YAML
				 * @param path path to the value being submitted
				 * @param depth depth of the traversal of the tree
				 * @return result of the submission
				 *
				 * \~
				 */
				[[nodiscard]] bool feedYAML(const abc::value_t & value, yaml::Value & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись YAML
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись YAML
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of YAML
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of YAML
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encodeYAML(const abc::value_t & value, string & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод подачи значения ABC значению TOML
				 *
				 * @param value  значение контейнера ABC
				 * @param result собираемое значение записи TOML
				 * @param depth  глубина обхода дерева
				 * @return       результат подачи
				 *
				 * \~english
				 * @brief Method of the submission of a value of ABC to a value of TOML
				 * @param value value of the container ABC
				 * @param result assembled value of a record of TOML
				 * @param depth depth of the traversal of the tree
				 * @return result of the submission
				 *
				 * \~
				 */
				[[nodiscard]] bool feedTOML(const abc::value_t & value, toml::Value & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись TOML
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись TOML
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of TOML
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of TOML
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encodeTOML(const abc::value_t & value, string & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки значения YAML в значение ABC
				 *
				 * @param document документ записи YAML
				 * @param path     путь к укладываемому значению
				 * @param result   укладываемое значение контейнера ABC
				 * @param depth    глубина обхода дерева
				 * @return         результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value of YAML into a value of ABC
				 * @param document document of a record of YAML
				 * @param path path to the value being laid
				 * @param result laid value of the container ABC
				 * @param depth depth of the traversal of the tree
				 * @return result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool absorbYAML(const yaml::document_t & document, const string & path, abc::value_t & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи YAML в дерево ABC
				 *
				 * @param text   запись YAML для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of YAML into a tree of ABC
				 * @param text record of YAML for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeYAML(const string_view text, abc::value_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки значения XML в значение ABC
				 *
				 * @details Своей системы видов у разметки нет: узел считается простым
				 *          значением тогда, когда потомков-узлов у него не нашлось, и
				 *          укладывается тогда собранным текстом. Признак этот берётся
				 *          перечнем звеньев, а не числом потомков: примечания и разделы
				 *          дословного текста узлами разметки не являются, и счёт лжёт
				 *
				 * @param document документ записи XML
				 * @param path     путь к укладываемому значению
				 * @param result   укладываемое значение контейнера ABC
				 * @param depth    глубина обхода дерева
				 * @return         результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value of XML into a value of ABC
				 * @param document document of a record of XML
				 * @param path path to the value being laid
				 * @param result laid value of the container ABC
				 * @param depth depth of the traversal of the tree
				 * @return result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool absorbXML(const xml::document_t & document, const string & path, abc::value_t & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи XML в дерево ABC
				 *
				 * @param text   запись XML для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of XML into a tree of ABC
				 * @param text record of XML for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeXML(const string_view text, abc::value_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки значения TOML в значение ABC
				 *
				 * @param document документ записи TOML
				 * @param path     путь к укладываемому значению
				 * @param result   укладываемое значение контейнера ABC
				 * @param depth    глубина обхода дерева
				 * @return         результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value of TOML into a value of ABC
				 * @param document document of a record of TOML
				 * @param path path to the value being laid
				 * @param result laid value of the container ABC
				 * @param depth depth of the traversal of the tree
				 * @return result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool absorbTOML(const toml::document_t & document, const string & path, abc::value_t & result, const uint32_t depth) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи TOML в дерево ABC
				 *
				 * @param text   запись TOML для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of TOML into a tree of ABC
				 * @param text record of TOML for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeTOML(const string_view text, abc::value_t & result) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи INI в дерево ABC
				 *
				 * @details Обход ведётся РОДНЫМ ходом кодека - перечнем разделов и
				 *          перечнем свойств раздела, - а не общим видом `keys(путь)`:
				 *          общего вида у этого кодека покуда нет, ибо имя занято прежним
				 *          плоским ходом с иным обещанием
				 *
				 * @param text   запись INI для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of INI into a tree of ABC
				 * @param text record of INI for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decodeINI(const string_view text, abc::value_t & result) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перевода дерева ABC в запись JSON
				 *
				 * @param value  дерево значений контейнера ABC
				 * @param result собранная запись JSON
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a tree of ABC into a record of JSON
				 * @param value tree of the values of the container ABC
				 * @param result assembled record of JSON
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool encode(const abc::value_t & value, string & result, const format_t format) noexcept;
				/**
				 * \~russian
				 * @brief Метод перевода записи JSON в дерево ABC
				 *
				 * @param text   запись JSON для перевода
				 * @param result собранное дерево значений контейнера ABC
				 * @return       результат перевода
				 *
				 * \~english
				 * @brief Method of the translation of a record of JSON into a tree of ABC
				 * @param text record of JSON for the translation
				 * @param result assembled tree of the values of the container ABC
				 * @return result of the translation
				 *
				 * \~
				 */
				[[nodiscard]] bool decode(const string_view text, abc::value_t & result, const format_t format) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения кода отказа последнего перевода
				 *
				 * @return код отказа последнего перевода
				 *
				 * \~english
				 * @brief Method of the extraction of the error code of the last translation
				 * @return error code of the last translation
				 *
				 * \~
				 */
				error_t error() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения настроек перевода
				 *
				 * @return настройки перевода
				 *
				 * \~english
				 * @brief Method of the extraction of the settings of the translation
				 * @return settings of the translation
				 *
				 * \~
				 */
				const settings_t & settings() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки настроек перевода
				 *
				 * @param settings настройки перевода
				 *
				 * \~english
				 * @brief Method of the setting of the settings of the translation
				 * @param settings settings of the translation
				 *
				 * \~
				 */
				void settings(const settings_t & settings) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param log объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param log object for working with the logs
				 *
				 * \~
				 */
				explicit Bridge(const log_t * log) noexcept : _error(error_t::NONE), _log(log) {}
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
				~Bridge() noexcept {}
		} bridge_t;
	}
}

#endif // __AWH_CODEC_BRIDGE__
