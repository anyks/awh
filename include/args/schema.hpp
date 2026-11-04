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
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочный файл проекта
 */
#include "common.hpp"

/**
 * \~russian
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * OPTIONAL у MS Windows.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 *
 * @warning Препроцессор области видимости не разбирает и заменяет имя даже внутри
 *          «enum class»: объявление обращается в негодное, и сборка встаёт. Замерено
 *          16.09.2026 щупом, подавшим те же макросы ключами собирателя
 *
 * \~english
 * We suppress the system macros, having taken the names of the members of the enumerations below:
 * OPTIONAL at MS Windows.
 * The names are removed only for the time of the declarations - a return is at the end of a file
 *
 * @warning A preprocessor does not distinguish a scope and replaces a name even inside an
 *          `enum class`: a declaration turns into a worthless one, and a build stops. It was measured
 *          on 2026-09-16 by a probe, having submitted the same macros by the keys of a builder
 *
 * \~
 */
#include "../sys/macro/suppress.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
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
		 *          параметр. С ним же имя сличается с ожидаемыми, склейка коротких имён
		 *          разбирается, потребность значения и обязательность параметра проверяются, а
		 *          справка о применении собирается из самого описания, а не пишется вторым
		 *          разом руками.
		 *
		 * @warning Строгость описание САМО ПО СЕБЕ не взводит: отказ на имя, описанию
		 *          неизвестное, стоит отдельною настройкой `strict`, и умолчанием она
		 *          СНЯТА - приложению, принимающему настройки сверх описанных, отказ
		 *          на всякое незнакомое имя мешал бы. Прежде здесь стояло обратное,
		 *          будто с описанием разбор становится строгим сам собою; утверждение
		 *          это расходилось с телом при умолчании настроек, то есть у всякого,
		 *          кто её не трогал. Замерено 09.09.2026 аудитом, закреплено проверкой
		 *          `ArgsSchema.DescriptionAloneDoesNotImplyStrictness`
		 *
		 * @par Намеренные решения
		 *
		 *      Перечисленное ниже не является пробелом реализации: это очерченные границы
		 *      задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Справка собирается из описания, а не пишется отдельно.** Написанная
		 *     отдельно, она расходится с делом при первой же правке, и расхождение это
		 *     молчаливо: приложение работает, а справка лжёт
		 *
		 * @li **Короткое имя есть знак буквенно-цифровой.** Указание 3 договора POSIX
		 *     о наречии программ зовёт коротким именем один знак буквенно-цифровой, и
		 *     заведение всякого иного отвечается отказом: знак «=» разбор наречия отрезает
		 *     значением и до имени не доносит, «-» сливается с двойным тире и одиночным
		 *     доводом, пробел и перевод строки оболочка до программы не доводит вовсе.
		 *     Прежде описание такие имена принимало и выводило их справкой, обещая наречие,
		 *     подать которое нельзя. Замерено 15.09.2026 аудитом, закреплено проверкой
		 *     `ArgsSchema.AShortNameMustBeTypeable`
		 *
		 * @li **Склейка коротких имён есть один знак, и это не украшение.** Склейка «-abc»
		 *     разбирается тремя признаками лишь тогда, когда все три знака описанию
		 *     известны короткими именами; иначе запись эта неотличима от длинного имени
		 *     под одним тире, каковое запись «-name VALUE» дозволяет
		 *
		 * @li **Строгость - настройка, а не поведение.** Приложению, принимающему
		 *     настройки сверх описанных, отказ на неизвестное имя мешал бы; оттого
		 *     описание само по себе строгости не вводит, её взводит настройка
		 *
		 * @li **Потребность значения различает три состояния, а не два.** Признак
		 *     значения не принимает вовсе, ключ требует непременно, а третьему виду
		 *     значение необязательно - «--log» взводит признак, «--log=файл» задаёт
		 *     назначение. Сведение к двум состояниям вынуждало бы заводить два имени
		 *     под одно понятие
		 *
		 * \~english
		 * @brief Description of the expected parameters of the launch
		 *
		 * @details The description is optional: without it the parsing accepts every submitted
		 *          parameter. With it the name is checked against the expected ones, a cluster of the
		 *          short names is parsed, the need of a value and the obligation of a parameter are
		 *          verified, and a help of the usage is assembled from the description itself rather
		 *          than written a second time by hand.
		 *
		 * @warning A strictness is not armed by a description ITSELF: a refusal on a name
		 *         unknown to a description stands by a separate setting `strict`, and by a default
		 *         it is REMOVED - to an application, accepting the settings beyond the described ones,
		 *         a refusal on every unfamiliar name would hinder. Before, there stood the reverse
		 *         here, as if a parsing with a description became strict by itself; this assertion
		 *         diverged with the body at a default of the settings, that is at everybody,
		 *         who did not touch it. It was measured on 2026-09-09 by an audit, fixed by a check
		 *         `ArgsSchema.DescriptionAloneDoesNotImplyStrictness`
		 *
		 * @par Deliberate decisions
		 *
		 *      Listed below is not a gap of the implementation: these are the outlined bounds
		 *      of the task, and each of the decisions is fixed by a verification test
		 *
		 * @li **A help is assembled from a description, and is not written separately.** Written
		 *     separately, it diverges with the deed at a first same edit, and this divergence is
		 *     silent: an application works, and a help lies
		 *
		 * @li **A short name is a letter-or-digit sign.** The indication 3 of a POSIX contract
		 *     about a syntax of the programs calls a short name by one letter-or-digit sign, and
		 *     a starting of any other is answered by a refusal: a sign `=` cuts a parsing of a syntax
		 *     by a value and does not carry it up to a name, `-` is glued with a double dash and a single
		 *     argument, a space and a line feed a shell does not carry up to a program at all.
		 *     Before, a description accepted such names and issued them by a help, promising a syntax,
		 *     which is impossible to submit. It was measured on 2026-09-15 by an audit, fixed by a check
		 *     `ArgsSchema.AShortNameMustBeTypeable`
		 *
		 * @li **A gluing of the short names is one sign, and this is not an ornament.** A gluing `-abc`
		 *     is parsed by three signs only then, when all three signs are known to a description
		 *     as short names; otherwise this record is indistinguishable from a long name
		 *     under one dash, which a record `-name VALUE` allows
		 *
		 * @li **A strictness is a setting, and not a behaviour.** To an application, accepting
		 *     the settings beyond the described ones, a refusal on an unknown name would hinder; therefore
		 *     a description by itself does not introduce a strictness, a setting turns it on
		 *
		 * @li **A necessity of a value distinguishes three states, and not two.** A sign
		 *     does not accept a value at all, a key requires it certainly, and to a third kind
		 *     a value is not obligatory - `--log` turns on a sign, `--log=file` sets
		 *     an appointment. A reducing to two states would oblige to start two names
		 *     under one notion
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Schema {
			public:
				/**
				 * \~russian
				 * @brief Потребность параметра в значении
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
				 * \~english
				 * @brief Description of one expected parameter
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Param {
					// Признак наличия значения по умолчанию
					bool preset;
					// Короткое имя параметра одним знаком, нуль при его отсутствии
					char letter;
					// Признак обязательности параметра
					bool required;
					// Признак дозволенности повторной подачи параметра
					bool multiple;
					// Потребность параметра в значении
					value_t value;
					// Длинное имя параметра с разделителем звеньев
					string name;
					// Значение параметра по умолчанию
					string fallback;
					// Описание назначения параметра для справки
					string description;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Param() noexcept;
				} param_t;
			private:
				// Название приложения для справки
				string _application;
				// Описание назначения приложения для справки
				string _description;
			private:
				// Описания ожидаемых параметров в порядке их заведения
				vector <param_t> _params;
			private:
				// Розыск описания по длинному имени параметра
				std::map <string, size_t> _names;
				// Розыск описания по короткому имени параметра
				std::map <char, size_t> _letters;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки описания ожидаемых параметров
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
				 * @details Имя, заведённое повторно, описание прежнее заменяет.
				 *
				 * @note Замена отвечается ОТКАЗОМ, если короткое имя поданного описания занято
				 *       описанием ИНЫМ: иначе розыск по знаку вёл бы к чужому параметру, а
				 *       прежнее короткое имя заменяемого пропадало бы вовсе
				 *
				 * @note Заведение отвечается ОТКАЗОМ и тогда, когда короткое имя не является
				 *       знаком буквенно-цифровым: подать такое имя наречием нельзя
				 *
				 * @param param описание ожидаемого параметра
				 * @return      результат заведения
				 *
				 * \~english
				 * @brief Method of the creation of a description of an expected parameter
				 *
				 *
				 * @details A name, started repeatedly, replaces a former description.
				 *
				 * @note A replacing is answered by a REFUSAL, if a short name of a submitted description is occupied
				 *       by ANOTHER description: otherwise a search by a sign would lead to an alien parameter, and
				 *       a former short name of a replaced one would be lost at all
				 *
				 * @note A starting is answered by a REFUSAL and then, when a short name is not
				 *       a letter-or-digit sign: such a name is impossible to submit by a syntax
				 *
				 * @param param description of the expected parameter
				 * @return      result of the creation
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
				 *
				 * @param name        long name of the parameter with the separator of the links
				 * @param letter      short name of the parameter by one character, zero at its absence
				 * @param value       need of the parameter in a value
				 * @param description description of the purpose of the parameter for the help
				 * @return            result of the creation
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
				 *
				 * @param name long name of the parameter
				 * @return     description of the parameter or nullptr at its absence
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
				 *
				 * @param letter short name of the parameter by one character
				 * @return       description of the parameter or nullptr at its absence
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
				 *          знаки её описанию известны короткими именами и ни один из них значения
				 *          не требует. Иначе запись эта неотличима от длинного имени под одним тире.
				 *
				 * @param cluster склейка коротких имён без ведущего тире
				 * @param result  контейнер разобранных длинных имён
				 * @return        результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a cluster of the short names
				 *
				 *
				 * @details A record `-abc` is parsed by three signs only then, when ALL
				 *          signs of it are known to a description as short names and none of them
				 *          requires a value. Otherwise this record is indistinguishable from a long name under one dash.
				 *
				 * @param cluster cluster of the short names without the leading dash
				 * @param result  container of the parsed long names
				 * @return        result of the parsing
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
				 *
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
				 *
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
				 *          написанная отдельно, она расходится с делом при первой же правке.
				 *
				 * @warning Признак повтора спрашивается ДВАЖДЫ - у описания параметра и
				 *          у настроек сбора, - ибо настройка сбора описание старше
				 *          по решению от 15.09.2026. Прежде справка знала лишь
				 *          описание и обещала «(repeatable)» тому параметру, чьи повторы
				 *          сбор массивом не собирал вовсе: потребитель, справку
				 *          прочитавший, подавал повтор и терял прежнее значение.
				 *          Замерено 16.09.2026 аудитом
				 *
				 * @param multiple признак дозволенности повтора настройками сбора
				 * @return         собранный текст справки
				 *
				 * \~english
				 * @brief Method of the assembling of the help of the usage
				 *
				 * @details A help is assembled from a description itself, and is not written separately:
				 *          written separately, it diverges with the deed at a first same edit.
				 *
				 * @warning A sign of a repeat is asked TWICE - at a description of a parameter and
				 *          at a settings of a gathering, - for a setting of a gathering is senior to a description
				 *          by a decision of 2026-09-15. Before, a help knew only a
				 *          description and promised `(repeatable)` to that parameter, whose repeats
				 *          a gathering by an array did not gather at all: a consumer,
				 *          having read a help, submitted a repeat and lost a former value.
				 *          It was measured on 2026-09-16 by an audit
				 *
				 * @param multiple sign of the permission of a repetition by the settings of the collection
				 * @return         assembled text of the help
				 *
				 * \~
				 */
				string usage(const bool multiple = true) const noexcept;
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
				 *
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
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Schema() noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Schema() noexcept = default;
		} schema_t;
	}
}

/**
 * Возвращаем подавленные системные макросы
 */
#include "../sys/macro/restore.hpp"
#include "../sys/macro/global.hpp"
