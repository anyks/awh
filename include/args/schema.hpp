/**
 * @file schema.hpp
 * @date 2026-09-03
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
 * @brief Заголовочный файл описания ожидаемых параметров запуска
 *
 * \~english
 * @brief Header file of the description of the expected parameters of the launch
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_ARGS_SCHEMA__
#define __AWH_ARGS_SCHEMA__

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "common.hpp"
#include "../sys/fmk.hpp"

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
	 * @brief Пространство имён параметров запуска приложения
	 *
	 *
	 * \~english
	 * @brief Namespace of the parameters of the launch of an application
	 *
	 * \~
	 */
	namespace args {
		/**
		 * \~russian
		 * @brief Описание ожидаемых параметров запуска
		 *
		 * @details Описание необязательно: без него разбор принимает всякий поданный
		 * параметр. С ним же разбор становится строгим - имя, описанию неизвестное,
		 * отвечается отказом, - и появляется справка о применении, собранная из
		 * самого описания, а не написанная вторым разом руками
		 *
		 * @par Намеренные решения
		 *
		 * Перечисленное ниже не является пробелом реализации: это очерченные границы
		 * задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Справка собирается из описания, а не пишется отдельно.** Написанная
		 * отдельно, она расходится с делом при первой же правке, и расхождение это
		 * молчаливо: приложение работает, а справка лжёт
		 *
		 * @li **Короткое имя есть один знак, и это не украшение.** Склейка «-abc»
		 * разбирается тремя признаками лишь тогда, когда все три знака описанию
		 * известны короткими именами; иначе запись эта неотличима от длинного имени
		 * под одним тире, каковое запись «-name VALUE» дозволяет
		 *
		 * @li **Строгость - настройка, а не поведение.** Приложению, принимающему
		 * настройки сверх описанных, отказ на неизвестное имя мешал бы; оттого
		 * описание само по себе строгости не вводит, её взводит настройка
		 *
		 * @li **Потребность значения различает три состояния, а не два.** Признак
		 * значения не принимает вовсе, ключ требует непременно, а третьему виду
		 * значение необязательно - «--log» взводит признак, «--log=файл» задаёт
		 * назначение. Сведение к двум состояниям вынуждало бы заводить два имени
		 * под одно понятие
		 *
		 * \~english
		 * @brief Description of the expected parameters of the launch
		 * @details The description is optional: without it the parsing accepts every submitted
		 * parameter. With it the parsing becomes strict — a name unknown to the description
		 * is answered by a refusal — and there appears a help of the usage, assembled from
		 * the description itself rather than written a second time by hand
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Schema {
			public:
				/**
				 * \~russian
				 * @brief Потребность параметра в значении
				 *
				 *
				 * \~english
				 * @brief Need of a parameter in a value
				 *
				 * \~
				 */
				enum class value_t : uint8_t {
					NONE     = 0x00, // Значения не принимает вовсе: взведённый признак
					REQUIRED = 0x01, // Значение потребно непременно
					OPTIONAL = 0x02  // Значение необязательно: без него взведённый признак
				};
			public:
				/**
				 * \~russian
				 * @brief Описание одного ожидаемого параметра
				 *
				 *
				 * \~english
				 * @brief Description of one expected parameter
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Param {
					// Потребность параметра в значении
					value_t value;
					// Признак обязательности параметра
					bool required;
					// Признак дозволенности повторной подачи параметра
					bool multiple;
					// Короткое имя параметра одним знаком, нуль при его отсутствии
					char letter;
					// Длинное имя параметра с разделителем звеньев
					string name;
					// Значение параметра по умолчанию
					string fallback;
					// Признак наличия значения по умолчанию
					bool preset;
					// Описание назначения параметра для справки
					string description;
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
					Param() noexcept :
					 value(value_t::REQUIRED), required(false), multiple(false),
					 letter(0), name{""}, fallback{""}, preset(false), description{""} {}
				} param_t;
			private:
				// Описания ожидаемых параметров в порядке их заведения
				vector <param_t> _params;
			private:
				// Розыск описания по длинному имени параметра
				map <string, size_t> _names;
				// Розыск описания по короткому имени параметра
				map <char, size_t> _letters;
			private:
				// Название приложения для справки
				string _application;
				// Описание назначения приложения для справки
				string _description;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки описания ожидаемых параметров
				 *
				 *
				 * \~english
				 * @brief Method of the clearing of the description of the expected parameters
				 *
				 * \~
				 */
				void clear() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения описания ожидаемого параметра
				 *
				 * @details Имя, заведённое повторно, описание прежнее заменяет
				 *
				 * @param param описание ожидаемого параметра
				 * @return      результат заведения
				 *
				 * \~english
				 * @brief Method of the creation of a description of an expected parameter
				 * @param param description of the expected parameter
				 * @return result of the creation
				 *
				 * \~
				 */
				[[nodiscard]] bool add(const param_t & param) noexcept;
				/**
				 * \~russian
				 * @brief Метод заведения описания ожидаемого параметра
				 *
				 * @param name        длинное имя параметра с разделителем звеньев
				 * @param letter      короткое имя параметра одним знаком, нуль при отсутствии
				 * @param value       потребность параметра в значении
				 * @param description описание назначения параметра для справки
				 * @return            результат заведения
				 *
				 * \~english
				 * @brief Method of the creation of a description of an expected parameter
				 * @param name long name of the parameter with the separator of the links
				 * @param letter short name of the parameter by one character, zero at its absence
				 * @param value need of the parameter in a value
				 * @param description description of the purpose of the parameter for the help
				 * @return result of the creation
				 *
				 * \~
				 */
				[[nodiscard]] bool add(const string_view name, const char letter, const value_t value, const string_view description = "") noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения описания параметра по длинному имени
				 *
				 * @param name длинное имя параметра
				 * @return     описание параметра либо nullptr при его отсутствии
				 *
				 * \~english
				 * @brief Method of the extraction of a description of a parameter by a long name
				 * @param name long name of the parameter
				 * @return description of the parameter or nullptr at its absence
				 *
				 * \~
				 */
				const param_t * get(const string_view name) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения описания параметра по короткому имени
				 *
				 * @param letter короткое имя параметра одним знаком
				 * @return       описание параметра либо nullptr при его отсутствии
				 *
				 * \~english
				 * @brief Method of the extraction of a description of a parameter by a short name
				 * @param letter short name of the parameter by one character
				 * @return description of the parameter or nullptr at its absence
				 *
				 * \~
				 */
				const param_t * get(const char letter) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора склейки коротких имён
				 *
				 * @details Запись «-abc» разбирается тремя признаками лишь тогда, когда ВСЕ
				 * знаки её описанию известны короткими именами и ни один из них значения
				 * не требует. Иначе запись эта неотличима от длинного имени под одним тире
				 *
				 * @param cluster склейка коротких имён без ведущего тире
				 * @param result  контейнер разобранных длинных имён
				 * @return        результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a cluster of the short names
				 * @param cluster cluster of the short names without the leading dash
				 * @param result container of the parsed long names
				 * @return result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool cluster(const string_view cluster, vector <string> & result) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения описаний всех ожидаемых параметров
				 *
				 * @return описания в порядке их заведения
				 *
				 * \~english
				 * @brief Method of the extraction of the descriptions of all the expected parameters
				 * @return descriptions in the order of their creation
				 *
				 * \~
				 */
				const vector <param_t> & params() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки пустоты описания
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of the check of the emptiness of the description
				 * @return result of the check
				 *
				 * \~
				 */
				[[nodiscard]] bool empty() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод сборки справки о применении
				 *
				 * @details Справка собирается из самого описания, а не пишется отдельно:
				 * написанная отдельно, она расходится с делом при первой же правке
				 *
				 * @return собранный текст справки
				 *
				 * \~english
				 * @brief Method of the assembling of the help of the usage
				 * @return assembled text of the help
				 *
				 * \~
				 */
				string usage() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки названия приложения для справки
				 *
				 * @param application название приложения
				 * @param description описание назначения приложения
				 *
				 * \~english
				 * @brief Method of the setting of the name of the application for the help
				 * @param application name of the application
				 * @param description description of the purpose of the application
				 *
				 * \~
				 */
				void application(const string_view application, const string_view description = "") noexcept;
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
				 * @param fmk object of the framework
				 * @param log object for working with the logs
				 *
				 * \~
				 */
				Schema(const fmk_t * fmk, const log_t * log) noexcept :
				 _application{""}, _description{""}, _fmk(fmk), _log(log) {}
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
				~Schema() noexcept {}
		} schema_t;
	}
}

#endif // __AWH_ARGS_SCHEMA__
