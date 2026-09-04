/**
 * @file args.hpp
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
 * @brief Заголовочный файл модуля параметров запуска приложения
 *
 * \~english
 * @brief Header file of the module of the parameters of the launch of an application
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_ARGS__
#define __AWH_ARGS__

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <vector>
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "lexer.hpp"
#include "common.hpp"
#include "schema.hpp"
#include "../sys/fs.hpp"
#include "../sys/fmk.hpp"
#include "../codec/bridge.hpp"
#include "../codec/abc/value.hpp"

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
		 * @brief Параметры запуска приложения
		 *
		 * @details Собирает настройки приложения из набора запуска, переменных окружения
		 * и текстовых потоков в единое дерево значений. Осью хранения служит владеющее
		 * значение контейнера ABC: система видов его вмещает виды всех текстовых кодеков
		 * разом, оттого выдача настроек любым из них не требует второго дерева
		 *
		 * @par Намеренные решения
		 *
		 * Перечисленное ниже не является пробелом реализации: это очерченные границы
		 * задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **Старшинство источников задано порядком членов перечня.** Значение из
		 * набора запуска перекрывает переменную окружения, та - файл настроек, а тот -
		 * значение по умолчанию. Обратное же перекрытие не совершается вовсе: значение
		 * младшего источника поверх старшего не ложится, в каком бы порядке источники
		 * ни подавались. Иначе разбор файла настроек, поданный после набора запуска,
		 * молча отменял бы поданное из набора
		 *
		 * @li **Источник хранится у каждого значения.** Без него ответ на вопрос
		 * «откуда взялось это значение» неотличим от догадки, а отладка настроек слепа
		 *
		 * @li **Вид значения выводится из записи, и это настройка.** Записи «true»,
		 * «17» и «1.5», поданные набором запуска, ложатся логическим значением и
		 * числами, а не последовательностями знаков. Довод тому - совпадение с файлом
		 * настроек: без вывода вида один и тот же ключ приходил бы числом из файла и
		 * знаками из набора, и потребителю пришлось бы знать источник. Обратный уклад
		 * отведён настройкой тем, кому вывод вида мешает
		 *
		 * @li **Разделитель пути в имени параметра - точка, а внутри оси - косая.**
		 * Запись «--net.port=80» ложится по пути «net/port» оси хранения. Точка
		 * выбрана привычною записи настроек, а перевод её в косую делается разбором,
		 * а не потребителем
		 *
		 * \~english
		 * @brief Parameters of the launch of an application
		 * @details Assembles the settings of an application from the set of the launch, from the variables of the environment
		 * and from the text streams into a single tree of the values. The owning value of the container ABC serves as the axis
		 * of the storage: its system of the kinds encompasses the kinds of all the text codecs
		 * at once, whereby the issuance of the settings by any of them requires no second tree
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Args {
			public:
				/**
				 * \~russian
				 * @brief Настройки сбора параметров запуска
				 *
				 *
				 * \~english
				 * @brief Settings of the assembling of the parameters of the launch
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Settings {
					// Признак вывода вида значения из его записи
					bool typed;
					// Признак укладки повторно поданного параметра массивом
					bool multiple;
					// Признак отказа на параметр, описанию ожидаемых неизвестный
					bool strict;
					// Разделитель звеньев пути в имени параметра
					char delimiter;
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
					Settings() noexcept : typed(true), multiple(true), strict(false), delimiter('.') {}
				} settings_t;
			private:
				// Настройки сбора параметров запуска
				settings_t _settings;
			private:
				// Описание ожидаемых параметров запуска
				schema_t _schema;
			private:
				// Начало имён переменных окружения, приложению отведённых
				string _prefix;
			private:
				// Разборщик параметров запуска и текстовых потоков
				lexer_t _lexer;
				// Мост между контейнером ABC и текстовыми кодеками
				codec::bridge_t _bridge;
				// Объект работы с файловой системой
				const fs_t _fs;
			private:
				// Дерево собранных значений настроек
				codec::abc::value_t _root;
			private:
				// Источники собранных значений настроек, ключом путь звеньями
				map <string, source_t> _origins;
				// Позиционные доводы в порядке их встречи в наборе запуска
				vector <string> _operands;
			private:
				// Отказы, случившиеся при последнем разборе
				vector <pair <error_t, location_t>> _errors;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			private:
				/**
				 * \~russian
				 * @brief Метод перевода имени параметра в путь оси хранения
				 *
				 * @param key имя параметра с разделителем звеньев
				 * @return    путь звеньями оси хранения
				 *
				 * \~english
				 * @brief Method of the conversion of a name of a parameter into a path of the axis of the storage
				 * @param key name of a parameter with the separator of the links
				 * @return path by the links of the axis of the storage
				 *
				 * \~
				 */
				string route(const string_view key) const noexcept;
				/**
				 * \~russian
				 * @brief Метод выведения значения из его записи
				 *
				 * @details Вид выводится лишь при взведённой настройке; иначе запись
				 * ложится последовательностью знаков как есть
				 *
				 * @param text запись значения
				 * @return     выведенное значение оси хранения
				 *
				 * \~english
				 * @brief Method of the inference of a value from its record
				 * @param text record of a value
				 * @return inferred value of the axis of the storage
				 *
				 * \~
				 */
				codec::abc::value_t derive(const string_view text) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод укладки значения в дерево настроек
				 *
				 * @details Значение младшего источника поверх старшего не ложится
				 *
				 * @param path   путь звеньями оси хранения
				 * @param value  значение для укладки
				 * @param source источник значения
				 * @return       результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a value into the tree of the settings
				 * @param path path by the links of the axis of the storage
				 * @param value value for the laying
				 * @param source source of the value
				 * @return result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool lay(const string & path, codec::abc::value_t && value, const source_t source) noexcept;
				/**
				 * \~russian
				 * @brief Метод слияния дерева значений с деревом настроек
				 *
				 * @details Отображения сливаются вглубь, а вместимые и одиночные значения
				 * ложатся целиком: сливать их поэлементно значило бы смешивать перечень,
				 * поданный файлом настроек, с перечнем, поданным набором запуска
				 *
				 * @param value  сливаемое дерево значений
				 * @param path   путь звеньями оси хранения, уже пройденный слиянием
				 * @param source источник сливаемых значений
				 * @return       результат слияния
				 *
				 * \~english
				 * @brief Method of the merging of a tree of the values with the tree of the settings
				 * @param value merged tree of the values
				 * @param path path by the links of the axis of the storage already passed by the merging
				 * @param source source of the merged values
				 * @return result of the merging
				 *
				 * \~
				 */
				[[nodiscard]] bool merge(const codec::abc::value_t & value, const string & path, const source_t source) noexcept;
				/**
				 * \~russian
				 * @brief Метод укладки разобранной лексемы по описанию ожидаемых
				 *
				 * @details Без описания лексема ложится как есть. С описанием же имя
				 * сличается с ожидаемыми, склейка коротких имён разбирается, а потребность
				 * значения проверяется
				 *
				 * @param lexeme разобранная лексема
				 * @param source источник поданного значения
				 * @return       результат укладки
				 *
				 * \~english
				 * @brief Method of the laying of a parsed lexeme by the description of the expected
				 * @param lexeme parsed lexeme
				 * @param source source of the submitted value
				 * @return result of the laying
				 *
				 * \~
				 */
				[[nodiscard]] bool apply(const lexeme_t & lexeme, const source_t source) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки собранных параметров запуска
				 *
				 *
				 * \~english
				 * @brief Method of the clearing of the assembled parameters of the launch
				 *
				 * \~
				 */
				void clear() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора набора доводов запуска
				 *
				 * @param count число доводов набора запуска
				 * @param items набор доводов запуска
				 * @return      результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a set of the arguments of the launch
				 * @param count number of the arguments of the set of the launch
				 * @param items set of the arguments of the launch
				 * @return result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool parse(const int32_t count, const char * items[]) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора набора доводов запуска широкими знаками
				 *
				 * @details Отведён MS Windows, где набор запуска приходит широкими знаками
				 *
				 * @param count число доводов набора запуска
				 * @param items набор доводов запуска
				 * @return      результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a set of the arguments of the launch by the wide characters
				 * @param count number of the arguments of the set of the launch
				 * @param items set of the arguments of the launch
				 * @return result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool parse(const int32_t count, const wchar_t * items[]) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора набора доводов запуска
				 *
				 * @param items набор доводов запуска
				 * @return      результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a set of the arguments of the launch
				 * @param items set of the arguments of the launch
				 * @return result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool parse(const vector <string> & items) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора текстового потока
				 *
				 * @details Отведён вводу с клавиатуры и подаче настроек одною строкою
				 *
				 * @param text текст для разбора
				 * @return     результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a text stream
				 * @param text text for the parsing
				 * @return result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool text(const string_view text) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод сбора переменных окружения
				 *
				 * @details Собираются переменные, имя которых начато отведённым приложению
				 * началом. Имя «PREFIX_NET_PORT» ложится по пути «net/port» оси хранения
				 *
				 * @return результат сбора
				 *
				 * \~english
				 * @brief Method of the assembling of the variables of the environment
				 * @return result of the assembling
				 *
				 * \~
				 */
				[[nodiscard]] bool env() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки значения по умолчанию
				 *
				 * @param key   имя параметра с разделителем звеньев
				 * @param value запись значения по умолчанию
				 * @return      результат установки
				 *
				 * \~english
				 * @brief Method of the setting of a value by default
				 * @param key name of a parameter with the separator of the links
				 * @param value record of the value by default
				 * @return result of the setting
				 *
				 * \~
				 */
				[[nodiscard]] bool fallback(const string_view key, const string_view value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения описания ожидаемых параметров запуска
				 *
				 * @return описание ожидаемых параметров запуска
				 *
				 * \~english
				 * @brief Method of the extraction of the description of the expected parameters of the launch
				 * @return description of the expected parameters of the launch
				 *
				 * \~
				 */
				schema_t & schema() noexcept;
				/**
				 * \~russian
				 * @brief Метод сборки справки о применении
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
				/**
				 * \~russian
				 * @brief Метод проверки собранного по описанию ожидаемых
				 *
				 * @details Проверяется подача обязательных параметров, а значения по
				 * умолчанию, описанием заданные, укладываются недостающим
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of the check of the assembled by the description of the expected
				 * @return result of the check
				 *
				 * \~
				 */
				[[nodiscard]] bool verify() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора записи настроек кодеком
				 *
				 * @details Разобранное сливается с деревом настроек источником файла:
				 * значения набора запуска, текстового потока и окружения его перекрывают
				 *
				 * @param text запись настроек для разбора
				 * @return     результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a record of the settings by a codec
				 * @param text record of the settings for the parsing
				 * @return result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool config(const string_view text, const codec::Bridge::format_t format) noexcept;
				/**
				 * \~russian
				 * @brief Метод чтения файла настроек
				 *
				 * @param filename путь к файлу настроек
				 * @return         результат чтения
				 *
				 * \~english
				 * @brief Method of the reading of a file of the settings
				 * @param filename path to the file of the settings
				 * @return result of the reading
				 *
				 * \~
				 */
				[[nodiscard]] bool filename(const string & filename, const codec::Bridge::format_t format) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выдачи дерева настроек записью кодека
				 *
				 * @param result собранная запись настроек
				 * @return       результат выдачи
				 *
				 * \~english
				 * @brief Method of the issuance of the tree of the settings by a record of a codec
				 * @param result assembled record of the settings
				 * @return result of the issuance
				 *
				 * \~
				 */
				[[nodiscard]] bool dump(string & result, const codec::Bridge::format_t format) noexcept;
				/**
				 * \~russian
				 * @brief Метод записи дерева настроек в файл
				 *
				 * @param filename путь к файлу настроек
				 * @return         результат записи
				 *
				 * \~english
				 * @brief Method of the writing of the tree of the settings into a file
				 * @param filename path to the file of the settings
				 * @return result of the writing
				 *
				 * \~
				 */
				[[nodiscard]] bool save(const string & filename, const codec::Bridge::format_t format) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения моста между контейнером ABC и текстовыми кодеками
				 *
				 * @details Выдан наружу настройкам перевода: правило обращения с видами,
				 * записи кодека неведомыми, и вид оформления задаются им
				 *
				 * @return мост между контейнером ABC и текстовыми кодеками
				 *
				 * \~english
				 * @brief Method of the extraction of the bridge between the container ABC and the text codecs
				 * @return bridge between the container ABC and the text codecs
				 *
				 * \~
				 */
				codec::bridge_t & bridge() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки наличия параметра
				 *
				 * @param key имя параметра с разделителем звеньев
				 * @return    результат проверки
				 *
				 * \~english
				 * @brief Method of the check of the presence of a parameter
				 * @param key name of a parameter with the separator of the links
				 * @return result of the check
				 *
				 * \~
				 */
				[[nodiscard]] bool has(const string_view key) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения источника значения параметра
				 *
				 * @param key имя параметра с разделителем звеньев
				 * @return    источник значения параметра
				 *
				 * \~english
				 * @brief Method of the extraction of the source of a value of a parameter
				 * @param key name of a parameter with the separator of the links
				 * @return source of the value of the parameter
				 *
				 * \~
				 */
				source_t source(const string_view key) const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения числа значений вместимого параметра
				 *
				 * @param key имя параметра с разделителем звеньев
				 * @return    число значений вместимого
				 *
				 * \~english
				 * @brief Method of the extraction of the number of the values of a container parameter
				 * @param key name of a parameter with the separator of the links
				 * @return number of the values of the container
				 *
				 * \~
				 */
				size_t size(const string_view key) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Шаблон метода извлечения значения параметра
				 *
				 * @tparam T тип извлекаемого значения
				 * @param key имя параметра с разделителем звеньев
				 * @return    извлечённое значение параметра
				 *
				 * \~english
				 * @brief Template of the method of the extraction of a value of a parameter
				 * @tparam T type of the extracted value
				 * @param key name of a parameter with the separator of the links
				 * @return extracted value of the parameter
				 *
				 * \~
				 */
				template <typename T>
				T get(const string_view key) const noexcept;
				/**
				 * \~russian
				 * @brief Шаблон метода извлечения значений вместимого параметра
				 *
				 * @tparam T тип извлекаемых значений
				 * @param key имя параметра с разделителем звеньев
				 * @return    извлечённые значения вместимого
				 *
				 * \~english
				 * @brief Template of the method of the extraction of the values of a container parameter
				 * @tparam T type of the extracted values
				 * @param key name of a parameter with the separator of the links
				 * @return extracted values of the container
				 *
				 * \~
				 */
				template <typename T>
				vector <T> arr(const string_view key) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения позиционных доводов набора запуска
				 *
				 * @return позиционные доводы в порядке их встречи
				 *
				 * \~english
				 * @brief Method of the extraction of the positional arguments of the set of the launch
				 * @return positional arguments in the order of their occurrence
				 *
				 * \~
				 */
				const vector <string> & operands() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения дерева собранных значений настроек
				 *
				 * @return дерево собранных значений настроек
				 *
				 * \~english
				 * @brief Method of the extraction of the tree of the assembled values of the settings
				 * @return tree of the assembled values of the settings
				 *
				 * \~
				 */
				const codec::abc::value_t & root() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения отказов последнего разбора
				 *
				 * @return отказы, случившиеся при последнем разборе
				 *
				 * \~english
				 * @brief Method of the extraction of the refusals of the last parsing
				 * @return refusals that have occurred at the last parsing
				 *
				 * \~
				 */
				const vector <pair <error_t, location_t>> & errors() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения начала имён переменных окружения
				 *
				 * @return начало имён переменных окружения
				 *
				 * \~english
				 * @brief Method of the extraction of the beginning of the names of the variables of the environment
				 * @return beginning of the names of the variables of the environment
				 *
				 * \~
				 */
				const string & prefix() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки начала имён переменных окружения
				 *
				 * @param prefix начало имён переменных окружения
				 *
				 * \~english
				 * @brief Method of the setting of the beginning of the names of the variables of the environment
				 * @param prefix beginning of the names of the variables of the environment
				 *
				 * \~
				 */
				void prefix(const string_view prefix) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения настроек сбора параметров запуска
				 *
				 * @return настройки сбора параметров запуска
				 *
				 * \~english
				 * @brief Method of the extraction of the settings of the assembling of the parameters of the launch
				 * @return settings of the assembling of the parameters of the launch
				 *
				 * \~
				 */
				const settings_t & settings() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки настроек сбора параметров запуска
				 *
				 * @param settings настройки сбора параметров запуска
				 *
				 * \~english
				 * @brief Method of the setting of the settings of the assembling of the parameters of the launch
				 * @param settings settings of the assembling of the parameters of the launch
				 *
				 * \~
				 */
				void settings(const settings_t & settings) noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения настроек разбора параметров
				 *
				 * @return настройки разбора параметров
				 *
				 * \~english
				 * @brief Method of the extraction of the settings of the parsing of the parameters
				 * @return settings of the parsing of the parameters
				 *
				 * \~
				 */
				const lexer_t::settings_t & lexing() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки настроек разбора параметров
				 *
				 * @param settings настройки разбора параметров
				 *
				 * \~english
				 * @brief Method of the setting of the settings of the parsing of the parameters
				 * @param settings settings of the parsing of the parameters
				 *
				 * \~
				 */
				void lexing(const lexer_t::settings_t & settings) noexcept;
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
				Args(const fmk_t * fmk, const log_t * log) noexcept;
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
				~Args() noexcept {}
		} args_t;
	}
}

#endif // __AWH_ARGS__
