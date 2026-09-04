/**
 * @file reader.hpp
 * @date 2026-08-09
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
 * @brief Заголовочный файл потокового чтения текста настроек INI — класс Reader, выдающий события разбора
 *        по мере поступления кусков исходного текста, с выбором наречия записи, снятием кавычек,
 *        разбором управляющих последовательностей и склеиванием строк продолжения
 *
 * \~english
 * @brief Header file of the streaming reading of an INI settings text — the Reader class, which issues the parsing events
 *        as the chunks of the source text arrive, with a choice of the dialect of the notation, with the removal of the quotes,
 *        with the parsing of the escape sequences and with the gluing of the continuation lines
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_INI_READER__
#define __AWH_CODEC_INI_READER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"
#include "encoding.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
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
		 * @brief Пространство имён контейнера INI
		 *
		 *
		 * \~english
		 * @brief INI container namespace
		 *
		 * \~
		 */
		namespace ini {
			/**
			 * \~russian
			 * @brief Состояние чтения текста настроек
			 *
			 * \~english
			 * @brief State of the reading of a settings text
			 *
			 * \~
			 */
			enum class state_t : uint8_t {
				READY    = 0x00, // Событие получено и доступно для чтения
				HUNGRY   = 0x01, // Для продолжения разбора требуется следующий кусок текста
				FINISHED = 0x02, // Текст разобран до конца
				FAILED   = 0x03  // Разбор прекращён ошибкой
			};

			/**
			 * \~russian
			 * @brief Класс потокового чтения текста настроек
			 *
			 * @details Разбор ведётся по кускам исходного текста и выдаёт события по мере
			 * их обнаружения, не удерживая текст целиком. Разрыв куска допустим в любом
			 * месте, в том числе посреди имени, значения либо управляющей последовательности:
			 * разобранное сохраняется, а недостающее дочитывается следующим куском
			 *
			 * Разбор не выбрасывает исключений: признаком отказа служит состояние
			 * @c state_t::FAILED вместе с кодом ошибки и местом отказа в исходном тексте
			 *
			 * @par Порядок работы
			 *
			 * @warning Все выдаваемые последовательности знаков ссылаются на память,
			 * принадлежащую разбору, и остаются пригодными **лишь до следующего обращения**
			 * к @c next() либо @c feed(). Содержимое, нужное дольше, следует скопировать
			 * @note Подстановка обращений к другим значениям чтением не выполняется и
			 * настройками его не задаётся. Подставить обращение можно лишь тогда, когда
			 * весь текст настроек уже разобран: значение, на которое ссылаются, вправе
			 * стоять ниже по тексту. Работа эта возложена на дерево настроек, где текст
			 * собран целиком
			 *
			 *  @code{.cpp}
			 *  reader_t reader(log, reader_t::settings_t::git());
			 *
			 *  while(reader.feed(chunk, size, last)){
			 *    while(reader.next()){
			 *      switch(static_cast <uint8_t> (reader.event())){
			 *        case static_cast <uint8_t> (event_t::SECTION):
			 *          // Обработка объявления раздела: reader.section()
			 *        break;
			 *        case static_cast <uint8_t> (event_t::PROPERTY):
			 *          // Обработка свойства: reader.key(), reader.text()
			 *        break;
			 *      }
			 *    }
			 *    if(reader.state() == state_t::FAILED)
			 *      break;
			 *  }
			 *  @endcode
			 *
			 * @warning КОПИЯ ЧТЕНИЯ УНОСИТ ВИДЫ, В ИСХОДНИК УКАЗЫВАЮЩИЕ. Свёртки событий
			 * хранятся готовыми видами в строки самого чтения, и заводитель копии, неявный,
			 * переносит их как есть - виды копии продолжают указывать в память ИСХОДНОГО
			 * объекта. Покуда исходник жив, копия читается верно; переживи копия исходник -
			 * обращение к её видам есть обращение к освобождённому. Надзор за памятью валит
			 * это `stack-use-after-scope`, замерено щупом
			 *
			 * @note Довод принесён Василием от кодеков JSON и XML, где виды хранятся
			 * УКАЗАНИЯМИ в своё хранилище и копия перебазирует их сама собою. Здесь устройство
			 * иное, и потому ручательство обратное
			 *
			 * @warning Правка возможна тремя путями - запретить копирование, завести
			 * перебазирующий заводитель копии либо перейти на указания вместо видов, - и всякий
			 * из них меняет договор либо устройство. Решение вынесено владельцу
			 *
			 * \~english
			 * @brief Class of the streaming reading of a settings text
			 * @details The parsing is conducted by the chunks of the source text and issues the events as
			 * they are detected without holding the text in full. A break of a chunk is admissible in any
			 * place, including in the middle of a name, of a value or of an escape sequence:
			 * what has been parsed is preserved, while what is missing is read up by the next chunk
			 * The parsing does not throw exceptions: the @c state_t::FAILED state together with the error code
			 * and the place of the refusal in the source text serves as the sign of a refusal
			 * @par Order of the work
			 * @warning All the issued sequences of characters refer to the memory
			 * belonging to the parsing and remain valid **only until the next call**
			 * to @c next() or @c feed(). The content needed for longer should be copied
			 * @note The substitution of the references to the other values is not performed by the reading and
			 * is not given by its settings. A reference can be substituted only when
			 * the whole settings text has already been parsed: the value being referred to has the right
			 * to stand lower in the text. That work is laid upon the settings tree, where the text
			 * is assembled in full
			 *
			 *  @code{.cpp}
			 *  reader_t reader(log, reader_t::settings_t::git());
			 *
			 *  while(reader.feed(chunk, size, last)){
			 *    while(reader.next()){
			 *      switch(static_cast <uint8_t> (reader.event())){
			 *        case static_cast <uint8_t> (event_t::SECTION):
			 *          // The handling of the declaration of a section: reader.section()
			 *        break;
			 *        case static_cast <uint8_t> (event_t::PROPERTY):
			 *          // The handling of a property: reader.key(), reader.text()
			 *        break;
			 *      }
			 *    }
			 *    if(reader.state() == state_t::FAILED)
			 *      break;
			 *  }
			 *  @endcode
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Reader {
				private:
					/**
					 * \~russian
					 * Объект для работы с логами
					 *
					 * \~english
					 * Object for working with logs
					 *
					 * \~
					 */
					const log_t * _log;
				public:
					/**
					 * \~russian
					 * @brief Настройки разбора текста настроек
					 *
					 * @details Задают наречие записи и пределы, ограничивающие расход памяти
					 * на подставном тексте. Готовые наборы настроек для сложившихся наречий
					 * выдают методы @c windows(), @c python(), @c systemd(), @c git() и
					 * @c strict()
					 *
					 * \~english
					 * @brief Settings of the parsing of a settings text
					 * @details They give the dialect of the notation and the limits restricting the expenditure of the memory
					 * on a planted text. The ready sets of the settings for the established dialects
					 * are issued by the @c windows(), @c python(), @c systemd(), @c git() and
					 * @c strict() methods
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Признаваемые знаки начала примечания
						marker_t comments;
						// Признаваемые знаки разделителя имени и значения
						separator_t separators;
						/**
						 * \~russian
						 * Обращение с повторным объявлением свойства в разделе
						 *
						 * @note Потокового чтения касается лишь значение @c ERROR: события
						 * выдаются по мере разбора, и какое из объявлений считать
						 * действующим, решает потребитель - выдать одно, отбросив прочие,
						 * чтение не вправе, не удерживая текст целиком. Значения
						 * @c FIRST, @c LAST и @c MERGE потому исполняет дерево настроек,
						 * решая, какое объявление выдаёт @c Document::get()
						 *
						 * \~english
						 * Treatment of a repeated declaration of a property in a section
						 * @note Only the @c ERROR value concerns the streaming reading: the events
						 * are issued as the parsing goes on, and which of the declarations to consider
						 * the effective one is decided by the consumer — the reading has no right to issue one, discarding the rest,
						 * without holding the text in full. The @c FIRST, @c LAST and @c MERGE values
						 * are therefore executed by the settings tree, which decides which declaration
						 * @c Document::get() issues
						 *
						 * \~
						 */
						duplicate_t duplicates;
						// Обращение с кавычками вокруг значения свойства
						quote_t quotes;
						// Построение имени подраздела
						subsection_t subsections;
						// Знак, отделяющий имя подраздела при построении разделителем
						char delimiter;
						/**
						 * \~russian
						 * Флаг признания примечания в конце строки свойства
						 *
						 * @warning Наречия MS Windows и systemd такого примечания не
						 * признают: точка с запятой остаётся частью значения. Включение
						 * этого флага молча отрезает у них часть значения - скажем, у пути
						 * с точкой с запятой либо у пароля
						 *
						 * \~english
						 * Flag of the recognition of a comment at the end of a property line
						 * @warning The dialects of MS Windows and of systemd do not recognize such a comment:
						 * the semicolon remains a part of the value. The enabling of
						 * this flag silently cuts off a part of the value from them — say, from a path
						 * with a semicolon or from a password
						 *
						 * \~
						 */
						bool inlineComments;
						/**
						 * \~russian
						 * Флаг требования пробельного знака перед началом примечания
						 *
						 * @details Без пробела знак примечания частью значения остаётся: запись
						 * «key = a#b» даёт значение «a#b» целиком. С ним - обрывает значение
						 * всякий раз, где бы ни стоял
						 *
						 * @warning Требование это - защита пути и пароля, где точка с запятой и
						 * решётка стоят посреди значения. Снимать его следует лишь там, где
						 * наречие того требует: наречие Git примечание режет без пробела
						 *
						 * \~english
						 * Flag of the requirement of a whitespace character before the beginning of a comment
						 * @details Without a space the character of a comment remains a part of the value: the record
						 * «key = a#b» gives the value «a#b» in full. With it the value is cut off
						 * every time, wherever the character stands
						 * @warning This requirement is a protection of a path and of a password, where a semicolon and
						 * a hash stand in the middle of a value. It should be lifted only there, where
						 * the dialect requires it: the Git dialect cuts a comment without a space
						 *
						 * \~
						 */
						bool spacedComments;
						/**
						 * \~russian
						 * Флаг поиска закрывающей скобки объявления раздела до последней в строке
						 *
						 * @details Умолчанием берётся первая: запись «[x[]]» такому разбору есть
						 * имя «x[» с лишним хвостом. Разбор языка Python берёт последнюю, и
						 * имя выходит «x[]»
						 *
						 * \~english
						 * Flag of the search for the closing bracket of a section declaration up to the last one in the line
						 * @details By default the first one is taken: the record «[x[]]» is for such a parsing
						 * the name «x[» with a superfluous tail. The parsing of the Python language takes the last one,
						 * and the name comes out as «x[]»
						 *
						 * \~
						 */
						bool greedySections;
						/**
						 * \~russian
						 * Флаг отбрасывания пробельной обвязки имени раздела
						 *
						 * @details Умолчанием обвязка отбрасывается: запись «[ раздел ]» даёт имя
						 * «раздел». Разбор языка Python её сохраняет, и имя выходит с пробелами
						 *
						 * @warning Сохранение обвязки значит, что «[a]» и «[ a ]» суть разные
						 * разделы: сличение имён ведётся знак в знак
						 *
						 * \~english
						 * Flag of the discarding of the whitespace trimming of the name of a section
						 * @details By default the trimming is discarded: the record «[ section ]» gives the name
						 * «section». The parsing of the Python language preserves it, and the name comes out with the spaces
						 * @warning The preservation of the trimming means that «[a]» and «[ a ]» are different
						 * sections: the comparison of the names is conducted character by character
						 *
						 * \~
						 */
						bool trimSections;
						/**
						 * \~russian
						 * Флаг поверки имён строгою грамматикой наречия
						 *
						 * @details Умолчанием имя раздела и имя свойства грамматикой не
						 * поверяются вовсе: наречия INI расходятся в них слишком широко.
						 * Наречие Git же строит имя раздела из букв, цифр, черты и точки, а
						 * имя свойства - из буквы первым знаком и далее из букв, цифр и
						 * черты. Записи «[a b]», «[a_b]», «k_2 = v» и «1k = v» средство
						 * «git config» отвергает, а чтение без поверки принимало их все
						 *
						 * @warning Имя подраздела грамматике этой не подлежит: в кавычках
						 * оно вольно, и «[a "любое имя"]» средство принимает
						 *
						 * \~english
						 * Flag of the verification of the names by the strict grammar of the dialect
						 * @details By default the name of a section and the name of a property are not verified
						 * by a grammar at all: the dialects of INI diverge in them too widely.
						 * The Git dialect however builds the name of a section out of the letters, the digits, the dash
						 * and the dot, while the name of a property — out of a letter as the first character and further
						 * out of the letters, the digits and the dash. The records «[a b]», «[a_b]», «k_2 = v» and «1k = v»
						 * the «git config» tool rejects while the reading without the verification accepted all of them
						 * @warning The name of a subsection is not subject to this grammar: within the quotes
						 * it is free, and «[a "any name"]» the tool accepts
						 *
						 * \~
						 */
						bool strictNames;
						// Флаг разбора управляющих последовательностей в значении свойства
						bool escapes;
						// Флаг склеивания строк, продолженных знаком обратной косой черты
						bool continuations;
						// Флаг склеивания строк, продолженных отступом по образцу configparser
						bool indents;
						// Флаг признания свойства, записанного без разделителя и значения
						bool valueless;
						// Флаг признания записи «имя[]» добавлением к перечню значений
						bool arrays;
						// Флаг учёта регистра имён разделов и свойств при сличении
						bool sensitive;
						/**
						 * \~russian
						 * Флаг учёта регистра имён разделов при сличении
						 *
						 * @note Заведён ради наречия configparser: имена свойств оно приводит
						 * к нижнему регистру, а имена разделов сличает как записаны. Флаг
						 * этот действует поверх общего: учёт регистра разделов включает
						 * любой из двух
						 *
						 * \~english
						 * Flag of taking the case of the names of the sections into account at the comparison
						 * @note Established for the sake of the configparser dialect: it brings the names of the properties
						 * to the lower case while it compares the names of the sections as they are written. This flag
						 * acts on top of the common one: the taking into account of the case of the sections is enabled by
						 * either of the two
						 *
						 * \~
						 */
						bool sensitiveSections;
						// Флаг отбрасывания пробельной обвязки значения свойства
						bool trim;
						// Флаг признания свойств, объявленных до первого раздела
						bool global;
						// Флаг выдачи примечаний отдельным событием
						bool emitComments;
						// Флаг выдачи пустых строк отдельным событием
						bool emitBlanks;
						/**
						 * \~russian
						 * Наибольшая допустимая длина логической строки в байтах
						 *
						 * @note Значение в ноль значит «без предела»: длина не поверяется вовсе
						 *
						 * \~english
						 * Largest admissible length of a logical line in bytes
						 * @note A value of zero means «without a limit»: the length is not verified at all
						 *
						 * \~
						 */
						uint32_t maxLine;
						/**
						 * \~russian
						 * Наибольшая допустимая длина имени раздела или свойства в байтах
						 *
						 * @note Значение в ноль значит «без предела»: длина не поверяется вовсе.
						 * Уклад этот один у разбора, у правки дерева и у записи
						 *
						 * \~english
						 * Largest admissible length of the name of a section or of a property in bytes
						 * @note A value of zero means «without a limit»: the length is not verified at all.
						 * This convention is one and the same for the parsing, for the editing of the tree and for the writing
						 *
						 * \~
						 */
						uint32_t maxName;
						/**
						 * \~russian
						 * Наибольшая допустимая глубина вложенности подразделов
						 *
						 * @note Граница ВКЛЮЧАЮЩАЯ, и считается по числу знаков-разделителей в имени
						 * раздела: при значении в единицу «[a.b]» принимается, а «[a.b.c]» отвергается.
						 * Ноль значит «без предела» - уклад этот один у всех кодеков рамки
						 *
						 * @warning Граница прежде была ИСКЛЮЧАЮЩЕЙ, а ноль ЗАПРЕЩАЛ подразделы вовсе, и
						 *          оба уклада расходились с прочими кодеками рамки. Хуже того, они
						 *          расходились между собою: при исключающей границе единица значила ровно
						 *          то же, что ноль, - «подразделов нельзя», - и запросить «допускаю один
						 *          уровень» было нечем вовсе. Запрет получил своё выражение - признак
						 *          `nesting`
						 *
						 * \~english
						 * Largest admissible depth of the nesting of the subsections
						 * @note The boundary is an inclusive one and is counted by the number of the separator characters
						 * in the name of the section: at a value of one «[a.b]» is accepted while «[a.b.c]» is rejected.
						 * Zero means «without a limit» — this convention is one and the same for all the codecs of the framework
						 *
						 * \~
						 */
						uint32_t maxDepth;
						/**
						 * \~russian
						 * Признак дозволения подразделов
						 *
						 * @details Снятый признак отвергает всякое имя раздела со знаком-разделителем,
						 * оставляя разделы простые. Прежде мысль эта выражалась нулём у `maxDepth`, а ноль
						 * ныне значит «без предела» у всех кодеков рамки
						 *
						 * @note Признак этот от настройки `subsections` отличен: та решает, ВЫДЕЛЯЮТСЯ ли
						 *       подразделы вовсе, а этот - дозволены ли они, коль скоро выделяются. При
						 *       `subsection_t::NONE` имя берётся целиком и отказа не выходит вовсе
						 *
						 * \~english
						 * Sign of the permission of the subsections
						 * @details A cleared sign rejects every name of a section carrying a separator character
						 *
						 * \~
						 */
						bool nesting;
						/**
						 * \~russian
						 * Наибольшее допустимое количество строк продолжения у одной записи
						 *
						 * @warning Уклад нуля здесь ИНОЙ, нежели у `maxLine` и `maxName`: нуль
						 * значит не «без предела», а запрет продолжений вовсе - первая же
						 * склеенная строка отвергается. Уклад этот один с укладом `maxDepth`,
						 * где нуль запрещает подразделы: пределы, считающие вложенность,
						 * нулём запрещают её, а пределы длины нулём снимаются
						 *
						 * \~english
						 * Largest admissible number of the lines of a continuation of one record
						 * @warning The convention of zero here is DIFFERENT from that of `maxLine` and `maxName`: zero
						 * means not «without a limit» but a prohibition of the continuations altogether - the very first
						 * glued line is rejected. This convention is one and the same with the convention of `maxDepth`,
						 * where zero prohibits the subsections: the limits that count the nesting prohibit it by zero,
						 * while the limits of the length are lifted by zero
						 *
						 * \~
						 */
						uint32_t maxContinuation;
						// Кодировка, навязанная извне вопреки метке порядка байтов
						encoding_t encoding;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @details Умолчание берёт общее пересечение сложившихся наречий:
						 * признаются оба знака примечания, разделителем служит знак
						 * равенства, кавычки снимаются, примечание в конце строки значения
						 * не признаётся, подразделы не выделяются
						 *
						 * \~english
						 * @brief Constructor
						 * @details The default takes the common intersection of the established dialects:
						 * both comment characters are recognized, the equals sign serves as the
						 * separator, the quotes are removed, a comment at the end of a value line
						 * is not recognized, the subsections are not singled out
						 *
						 * \~
						 */
						Settings() noexcept;
						/**
						 * \~russian
						 * @brief Метод получения настроек наречия MS Windows
						 *
						 * @details Примечание начинает лишь точка с запятой и лишь в начале
						 * строки, разделителем служит знак равенства, кавычки частью
						 * значения остаются, повторное свойство берётся первым
						 *
						 * @return настройки разбора наречия MS Windows
						 *
						 * \~english
						 * @brief Method of getting the settings of the MS Windows dialect
						 * @details A comment is begun only by a semicolon and only at the beginning of a
						 * line, the equals sign serves as the separator, the quotes remain a part of the
						 * value, a repeated property is taken as the first one
						 * @return settings of the parsing of the MS Windows dialect
						 *
						 * \~
						 */
						static Settings windows() noexcept;
						/**
						 * \~russian
						 * @brief Метод получения настроек наречия configparser языка Python
						 *
						 * @details Разделителем служит знак равенства либо двоеточие,
						 * продолжение строки задаётся отступом, повторное свойство берётся
						 * последним
						 *
						 * @return настройки разбора наречия configparser
						 *
						 * \~english
						 * @brief Method of getting the settings of the configparser dialect of the Python language
						 * @details The equals sign or the colon serves as the separator,
						 * a continuation of a line is given by an indent, a repeated property is taken as
						 * the last one
						 * @return settings of the parsing of the configparser dialect
						 *
						 * \~
						 */
						static Settings python() noexcept;
						/**
						 * \~russian
						 * @brief Метод получения настроек наречия описания служб systemd
						 *
						 * @details Признаются оба знака примечания в начале строки,
						 * продолжение строки задаётся обратной косой чертой, повторное
						 * свойство добавляется к перечню значений
						 *
						 * @return настройки разбора наречия systemd
						 *
						 * \~english
						 * @brief Method of getting the settings of the dialect of the systemd unit files
						 * @details Both comment characters are recognized at the beginning of a line,
						 * a continuation of a line is given by a backslash, a repeated
						 * property is added to the list of the values
						 * @return settings of the parsing of the systemd dialect
						 *
						 * \~
						 */
						static Settings systemd() noexcept;
						/**
						 * \~russian
						 * @brief Метод получения настроек наречия настроек Git
						 *
						 * @details Подраздел заключается в кавычки и учитывает регистр,
						 * значение допускает управляющие последовательности, свойство без
						 * значения признаётся за истину, повторное свойство добавляется к
						 * перечню значений
						 *
						 * @return настройки разбора наречия Git
						 *
						 * \~english
						 * @brief Method of getting the settings of the dialect of the Git settings
						 * @details A subsection is enclosed in quotes and takes the case into account,
						 * a value admits the escape sequences, a property without
						 * a value is recognized as truth, a repeated property is added to
						 * the list of the values
						 * @return settings of the parsing of the Git dialect
						 *
						 * \~
						 */
						static Settings git() noexcept;
						/**
						 * \~russian
						 * @brief Метод получения строгих настроек разбора
						 *
						 * @details Всякое расхождение прекращает разбор ошибкой: повторное
						 * свойство, свойство до первого раздела, отсутствие разделителя.
						 * Применимо там, где текст настроек собирается своими средствами и
						 * молчаливое разночтение недопустимо
						 *
						 * @return строгие настройки разбора
						 *
						 * \~english
						 * @brief Method of getting the strict settings of the parsing
						 * @details Every divergence terminates the parsing with an error: a repeated
						 * property, a property before the first section, an absence of a separator.
						 * Applicable where the settings text is assembled by one's own means and
						 * a silent discrepancy is inadmissible
						 * @return strict settings of the parsing
						 *
						 * \~
						 */
						static Settings strict() noexcept;
					} settings_t;
				private:
					// Объект приведения исходного текста к кодировке UTF-8
					decoder_t _decoder;
				private:
					// Признак получения последнего куска исходного текста
					bool _final;
				private:
					// Признак того, что первый раздел текста настроек уже объявлен
					bool _sectioned;
				private:
					// Состояние чтения текста настроек
					state_t _state;
				private:
					// Вид текущего события разбора
					event_t _event;
				private:
					// Код ошибки последней операции разбора
					error_t _error;
				private:
					/**
					 * \~russian
					 * Отложенный код ошибки приведения исходного текста к кодировке UTF-8
					 *
					 * @note Приведение переносит в хранилище всё, что успело разобрать, и
					 *       лишь затем отвечает отказом. Отказ выдаётся не сразу, а по
					 *       исчерпании приведённого начала текста: иначе разбор одного и
					 *       того же текста целиком и кусками расходился бы событиями
					 *
					 * \~english
					 * Postponed error code of the conversion of the source text to the UTF-8 encoding
					 * @note The conversion transfers into the storage everything it has managed to parse, and
					 *       only then answers with a refusal. The refusal is issued not at once but upon
					 *       the exhaustion of the converted beginning of the text: otherwise the parsing of one and
					 *       the same text in full and by chunks would diverge in the events
					 *
					 * \~
					 */
					error_t _decoding;
				private:
					// Положение обнаруженной ошибки в исходном тексте
					location_t _errorLocation;
				private:
					// Положение начала текущего события в исходном тексте
					location_t _location;
				private:
					// Приведённый к кодировке UTF-8 исходный текст
					string _buffer;
				private:
					// Положение разбираемого знака в приведённом тексте
					size_t _offset;
				private:
					/**
					 * \~russian
					 * Положение начала разбираемой логической строки в приведённом тексте
					 *
					 * @note Держится ради указания места ошибки: положение в строке
					 * считается от её начала, а разбор к этому времени ушёл вперёд
					 *
					 * @warning У строки, собранной из продолжений, положение это точным
					 * быть перестаёт: знаки её в исходном тексте непрерывным отрезком не
					 * лежат. Указание места ошибки в такой строке следует читать как
					 * указание на строку, а не на знак в ней
					 *
					 * \~english
					 * Position of the beginning of the logical line being parsed in the converted text
					 * @note It is kept for the sake of indicating the place of an error: the position in a line
					 * is counted from its beginning, while the parsing has gone forward by that time
					 * @warning For a line assembled from the continuations this position ceases to be
					 * an exact one: its characters do not lie in the source text as a continuous
					 * segment. The indication of the place of an error in such a line should be read as
					 * an indication of the line rather than of a character in it
					 *
					 * \~
					 */
					size_t _start;
				private:
					// Смещение начала приведённого текста от начала исходного в байтах
					uint64_t _base;
				private:
					// Номер разбираемой строки исходного текста, считая с единицы
					uint32_t _line;
				private:
					// Имя текущего раздела текста настроек
					name_t _name;
				private:
					// Хранилище имени текущего раздела
					string _section;
				private:
					// Хранилище имени текущего подраздела
					string _subsection;
				private:
					// Свойство текущего события разбора
					property_t _property;
				private:
					// Примечание текущего события разбора
					comment_t _comment;
				private:
					/**
					 * \~russian
					 * Хранилище значения свойства текущего события
					 *
					 * @note Заполняется лишь тогда, когда значение отличается от записанного
					 * в тексте: сняты кавычки, разобрана управляющая последовательность. В
					 * прочих случаях значение выдаётся указателем прямо в приведённый текст,
					 * и хранилище это не трогается вовсе
					 *
					 * \~english
					 * Storage of the value of the property of the current event
					 * @note Filled in only when the value differs from what has been written
					 * in the text: the quotes have been removed, an escape sequence has been parsed. In
					 * the other cases the value is issued as a pointer right into the converted text,
					 * and this storage is not touched at all
					 *
					 * \~
					 */
					string _value;
				private:
					// Значение свойства текущего события разбора
					string_view _view;
				private:
					// Хранилище содержимого примечания текущего события
					string _content;
				private:
					/**
					 * \~russian
					 * Перечень уже объявленных разделов текста настроек
					 *
					 * @note Заполняется лишь тогда, когда повторное объявление признано
					 * ошибкой: в прочих случаях удерживать имена незачем, а на большом
					 * тексте настроек перечень этот памяти стоит
					 *
					 * \~english
					 * List of the already declared sections of the settings text
					 * @note Filled in only when a repeated declaration is recognized as
					 * an error: in the other cases there is no point in holding the names, while on a large
					 * settings text this list costs memory
					 *
					 * \~
					 */
					unordered_set <string> _sections;
				private:
					/**
					 * \~russian
					 * Перечень уже объявленных свойств текущего раздела
					 *
					 * @note Заполняется по тому же правилу, что и перечень разделов, и
					 * очищается при переходе к следующему разделу
					 *
					 * \~english
					 * List of the already declared properties of the current section
					 * @note Filled in by the same rule as the list of the sections, and
					 * cleared at the transition to the next section
					 *
					 * \~
					 */
					unordered_set <string> _keys;
				private:
					/**
					 * \~russian
					 * Собранная логическая строка с разрешёнными продолжениями
					 *
					 * @note Держится отдельно от приведённого текста намеренно: строки
					 * продолжения склеиваются, и получившаяся строка в исходном тексте
					 * непрерывным отрезком не лежит
					 *
					 * \~english
					 * Assembled logical line with the continuations resolved
					 * @note It is kept separately from the converted text deliberately: the continuation
					 * lines are glued together, and the resulting line does not lie in the source text
					 * as a continuous segment
					 *
					 * \~
					 */
					string _logical;
				private:
					/**
					 * \~russian
					 * @brief Отрезок собранной логической строки, взятый из исходного текста
					 *
					 * @details Отрезки записываются при сборке логической строки и служат
					 * обратному отображению: место знака собранной строки восстанавливается
					 * по ним в место того же знака в исходном тексте
					 *
					 * @note Отображение это записывается при сборке, а не выводится
					 * повторным разбором правил склейки: два списка правил разошлись бы при
					 * первой же правке одного из них
					 *
					 * \~english
					 * @brief Segment of the assembled logical line taken from the source text
					 * @details The segments are recorded at the assembly of the logical line and serve the
					 * reverse mapping: the place of a character of the assembled line is restored by them
					 * into the place of the same character in the source text
					 * @note This mapping is recorded at the assembly rather than derived by a repeated parsing
					 * of the gluing rules: two lists of the rules would diverge at the first editing of one of them
					 *
					 * \~
					 */
					typedef struct Piece {
						/**
						 * \~russian
						 * Положение начала отрезка в собранной логической строке
						 * \~english
						 * Position of the beginning of the segment in the assembled logical line
						 * \~
						 */
						size_t logical;
						/**
						 * \~russian
						 * Положение начала отрезка в приведённом исходном тексте
						 * \~english
						 * Position of the beginning of the segment in the converted source text
						 * \~
						 */
						size_t source;
						/**
						 * \~russian
						 * Длина отрезка в байтах
						 * \~english
						 * Length of the segment in bytes
						 * \~
						 */
						size_t length;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param logical положение начала отрезка в собранной логической строке
						 * @param source  положение начала отрезка в приведённом исходном тексте
						 * @param length  длина отрезка в байтах
						 *
						 * \~english
						 * @brief Constructor
						 * @param logical position of the beginning of the segment in the assembled logical line
						 * @param source  position of the beginning of the segment in the converted source text
						 * @param length  length of the segment in bytes
						 *
						 * \~
						 */
						Piece(const size_t logical, const size_t source, const size_t length) noexcept :
						 logical(logical), source(source), length(length) {}
					} piece_t;
				private:
					/**
					 * \~russian
					 * Отрезки собранной логической строки в исходном тексте
					 *
					 * @note Перечень пуст у строки без продолжений: такая строка лежит в
					 * исходном тексте непрерывным отрезком, и отображение ей тождественно
					 *
					 * \~english
					 * Segments of the assembled logical line in the source text
					 * @note The list is empty for a line without the continuations: such a line lies in the
					 * source text as a continuous segment, and the mapping is an identity for it
					 *
					 * \~
					 */
					vector <piece_t> _pieces;
				private:
					/**
					 * \~russian
					 * Собранная логическая строка для разбора
					 *
					 * @details Ссылается либо на приведённый текст напрямую, либо на
					 * хранилище собранной логической строки
					 *
					 * @note Строка без продолжений в приведённом тексте лежит
					 * непрерывным отрезком, и копировать её в отдельное хранилище
					 * незачем. Копия эта обходилась в проход по всему тексту сверх
					 * самого разбора, а строки с продолжениями в файле настроек -
					 * редкость
					 *
					 * \~english
					 * Assembled logical line for the parsing
					 * @details Refers either to the converted text directly or to
					 * the storage of the assembled logical line
					 * @note A line without continuations lies in the converted text
					 * as a continuous segment, and there is no point in copying it into a separate storage.
					 * That copy used to cost a pass over the whole text on top of
					 * the parsing itself, while the lines with continuations in a settings file are
					 * a rarity
					 *
					 * \~
					 */
					string_view _current;
				private:
					/**
					 * \~russian
					 * Признак того, что примечание конца строки ждёт своей выдачи
					 *
					 * @note Строка свойства и строка объявления раздела несут за собою
					 * примечание, а событие выдаётся одно. Примечание такое удерживается
					 * и выдаётся следующим событием: иначе оно терялось бы, а вместе с
					 * ним и возможность переписать файл настроек, не обеднив его
					 *
					 * \~english
					 * Flag of a line-ending comment waiting for its issuance
					 * @note A property line and a section declaration line carry a comment
					 * after them, while a single event is issued. Such a comment is held
					 * and issued by the next event: otherwise it would be lost, and along with
					 * it the possibility of rewriting the settings file without impoverishing it
					 *
					 * \~
					 */
					bool _pending;
				private:
					// Удержанное примечание конца строки
					comment_t _tail;
				private:
					// Настройки разбора текста настроек
					settings_t _settings;
				private:
					/**
					 * \~russian
					 * @brief Метод приведения имени к виду для сличения
					 *
					 * @details При разборе без учёта регистра имя приводится к нижнему
					 * регистру по правилам US-ASCII: правила местности к именам разделов и
					 * свойств отношения не имеют
					 *
					 * @param name приводимое имя раздела или свойства
					 * @return     имя, приведённое к виду для сличения
					 *
					 * \~english
					 * @brief Method of bringing a name to the form for the comparison
					 * @details At a parsing without taking the case into account the name is brought to the lower
					 * case by the rules of US-ASCII: the rules of the locale have no relation to the names of the sections and
					 * of the properties
					 * @param name name of the section or of the property being brought
					 * @return     name brought to the form for the comparison
					 *
					 * \~
					 */
					string fold(const string_view name, const bool section = false) const noexcept;
					/**
					 * \~russian
					 * @brief Метод запоминания ошибки разбора
					 *
					 * @param error  код ошибки разбора
					 * @param offset положение ошибки в приведённом тексте
					 * @param line   номер строки, где обнаружена ошибка
					 * @param column положение в строке, где обнаружена ошибка
					 * @return       признак отказа для передачи вызывающему
					 *
					 * \~english
					 * @brief Method of remembering a parsing error
					 * @param error  error code of the parsing
					 * @param offset position of the error in the converted text
					 * @param line   number of the line where the error has been detected
					 * @param column position in the line where the error has been detected
					 * @return       flag of a refusal for passing to the caller
					 *
					 * \~
					 */
					bool fail(const error_t error, const size_t offset, const uint32_t line, const uint32_t column) noexcept;
					/**
					 * \~russian
					 * @brief Метод поиска конца логической строки
					 *
					 * @details Логической строкой считается строка вместе со всеми своими
					 * продолжениями. Разбор её не начинается, пока она не поступила целиком:
					 * иначе продолжение, разорванное границей куска, разбиралось бы
					 * отдельной записью
					 *
					 * @param begin  положение начала логической строки в приведённом тексте
					 * @param length длина логической строки без знака её конца
					 * @param next   положение начала следующей строки в приведённом тексте
					 * @return       признак того, что логическая строка поступила целиком
					 *
					 * \~english
					 * @brief Method of searching for the end of a logical line
					 * @details A logical line is a line together with all its
					 * continuations. Its parsing does not begin until it has arrived in full:
					 * otherwise a continuation torn by the boundary of a chunk would be parsed as
					 * a separate record
					 * @param begin  position of the beginning of the logical line in the converted text
					 * @param length length of the logical line without its ending character
					 * @param next   position of the beginning of the next line in the converted text
					 * @return       flag of the logical line having arrived in full
					 *
					 * \~
					 */
					bool measure(const size_t begin, size_t & length, size_t & next) noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки логической строки с разрешением продолжений
					 *
					 * @details Строки продолжения склеиваются в одну: продолженные обратной
					 * косой чертой - без разделителя, а продолженные отступом - через знак
					 * перевода строки по образцу configparser
					 *
					 * @note Копирование выполняется лишь при наличии продолжений: строка
					 * без них лежит в приведённом тексте непрерывным отрезком, и на неё
					 * выдаётся ссылка
					 *
					 * @param begin  положение начала логической строки в приведённом тексте
					 * @param length длина логической строки в приведённом тексте
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of assembling a logical line with the resolution of the continuations
					 * @details The continuation lines are glued into one: the ones continued by a backslash —
					 * without a separator, while the ones continued by an indent — through a line feed
					 * character after the pattern of configparser
					 * @note The copying is performed only in the presence of the continuations: a line
					 * without them lies in the converted text as a continuous segment, and a reference to it
					 * is issued
					 * @param begin  position of the beginning of the logical line in the converted text
					 * @param length length of the logical line in the converted text
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool join(const size_t begin, const size_t length) noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места знака в исходном тексте
					 *
					 * @details Положение в строке считается в знаках Юникода, а не в байтах:
					 * строка настроек вправе нести имена и значения на любом языке, и указание
					 * места ошибки в байтах читающему ничего не сообщает
					 *
					 * @note Строка и столбец считаются вместе, а не порознь: логическая строка
					 * вправе собираться из нескольких физических, и столбец, посчитанный от её
					 * начала сквозь переносы, указывал бы на место, которого в строке нет вовсе
					 *
					 * @param offset положение знака в приведённом тексте
					 * @param line   номер строки, на которой знак записан
					 * @param column положение знака в строке, считая с единицы
					 *
					 * \~english
					 * @brief Method of getting the place of a character in the source text
					 * @details The position in a line is counted in Unicode characters rather than in bytes: a
					 * settings line has the right to carry the names and the values in any language, and an
					 * indication of the place of an error in bytes tells the reader nothing
					 * @note The line and the column are counted together rather than separately: a logical line
					 * has the right to be assembled out of several physical ones, and a column counted from its
					 * beginning through the line breaks would point at a place that does not exist in the line
					 * @param offset position of the character in the converted text
					 * @param line   number of the line the character is written on
					 * @param column position of the character in the line, counting from one
					 *
					 * \~
					 */
					void place(const size_t offset, uint32_t & line, uint32_t & column) const noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора объявления раздела
					 *
					 * @param line   разбираемая логическая строка без знака её конца
					 * @param offset положение начала логической строки в приведённом тексте
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing a section declaration
					 * @param line   logical line being parsed without its ending character
					 * @param offset position of the beginning of the logical line in the converted text
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool header(const string_view line, const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора свойства со значением
					 *
					 * @param line   разбираемая логическая строка без знака её конца
					 * @param offset положение начала логической строки в приведённом тексте
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of parsing a property with a value
					 * @param line   logical line being parsed without its ending character
					 * @param offset position of the beginning of the logical line in the converted text
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool assign(const string_view line, const size_t offset) noexcept;
					/**
					 * \~russian
					 * @brief Метод приведения значения свойства к окончательному виду
					 *
					 * @details Снимает кавычки, разбирает управляющие последовательности и
					 * отбрасывает примечание в конце строки - в объёме, разрешённом
					 * настройками разбора
					 *
					 * @param text     разбираемое значение свойства
					 * @param position положение начала значения в приведённом тексте
					 * @return         результат выполнения операции
					 *
					 * \~english
					 * @brief Method of bringing the value of a property to its final form
					 * @details Removes the quotes, parses the escape sequences and
					 * discards a comment at the end of the line — in the volume permitted by
					 * the settings of the parsing
					 * @param text     value of the property being parsed
					 * @param position position of the beginning of the value in the converted text
					 * @return         result of performing the operation
					 *
					 * \~
					 */
					bool extract(const string_view text, const size_t position) noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки того, что значение свойства приведения не требует
					 *
					 * @details Значение, в котором нет ни кавычек, ни управляющих
					 * последовательностей, ни примечания в конце строки, лежит в приведённом
					 * тексте непрерывным отрезком и выдаётся указателем на него без переноса
					 * в отдельное хранилище
					 *
					 * @param value проверяемое значение свойства
					 * @return      признак того, что значение приведения не требует
					 *
					 * \~english
					 * @brief Method of checking that the value of a property does not require a conversion
					 * @details A value in which there are neither quotes, nor escape
					 * sequences, nor a comment at the end of the line lies in the converted
					 * text as a continuous segment and is issued as a pointer to it without a transfer
					 * into a separate storage
					 * @param value value of the property being checked
					 * @return      flag of the value not requiring a conversion
					 *
					 * \~
					 */
					bool plain(const string_view value) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текущих настроек разбора
					 *
					 * @return текущие настройки разбора текста настроек
					 *
					 * \~english
					 * @brief Method of getting the current settings of the parsing
					 * @return current settings of the parsing of a settings text
					 *
					 * \~
					 */
					const settings_t & settings() const noexcept;
					/**
					 * \~russian
					 * @brief Метод установки настроек разбора
					 *
					 * @note Смена настроек, поданная после первого куска исходного текста, не
					 * исполняется: чтобы читать иными настройками, чтение сбрасывают методом
					 * @c reset()
					 *
					 * @warning Настройки применяются к разбору целиком и смены посреди
					 * текста не допускают: устанавливать их следует до первого куска
					 *
					 * @param settings настройки разбора текста настроек
					 *
					 * @return признак того, что настройки приняты
					 *
					 * @warning Смена настроек посреди разбора НЕ ИСПОЛНЯЕТСЯ и отвечает ложью:
					 * применилась бы она к остатку текста, но не к разобранному началу, и один
					 * файл читался бы двумя наречиями сразу. Чтобы задать иные настройки, чтение
					 * сбрасывают
					 *
					 * @note Прежде тело было `void` и смену теряло МОЛЧА - потребитель узнать о
					 * том не мог ничем. Сведено с чтениями TOML и YAML, где отказ отвечался
					 * ложью изначально, решением владельца о единообразии кодеков
					 *
					 * \~english
					 * @brief Method of setting the settings of the parsing
					 * @note A change of the settings submitted after the first chunk of the source text is not
					 * executed: in order to read with other settings, the reading is reset by the
					 * @c reset() method
					 * @warning The settings are applied to the parsing as a whole and do not admit a change in the middle of
					 * a text: they should be set before the first chunk
					 * @param settings settings of the parsing of a settings text
					 *
					 * \~
					 */
					bool settings(const settings_t & settings) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сброса разбора в исходное состояние
					 *
					 * @details Освобождает накопленное и подготавливает чтение к разбору
					 * нового текста, сохраняя установленные настройки
					 *
					 * \~english
					 * @brief Method of resetting the parsing into the initial state
					 * @details Releases what has been accumulated and prepares the reading for the parsing of
					 * a new text, preserving the settings that have been set
					 *
					 * \~
					 */
					void reset() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод передачи очередного куска исходного текста
					 *
					 * @details Куски передаются в порядке их следования в тексте и делятся
					 * произвольно: разрыв допустим в любом месте, в том числе посреди имени
					 * либо значения
					 *
					 * @note Признак последнего куска обязателен: без него не отличить конец
					 * текста от его обрыва, и последняя строка без знака конца осталась бы
					 * неразобранной
					 *
					 * @note Отказ приведения исходного текста к кодировке UTF-8 выдаётся не
					 * настоящим методом, а по исчерпании уже приведённого начала текста:
					 * события, разобранные до испорченного знака, выдаются, и подача текста
					 * целиком выдаёт то же самое, что и подача его кусками. Оборванная
					 * испорченным знаком строка событием не выдаётся
					 *
					 * @param buffer буфер очередного куска исходного текста
					 * @param size   размер буфера очередного куска исходного текста
					 * @param end    признак того, что кусок является последним
					 * @return       результат выполнения операции
					 *
					 * \~english
					 * @brief Method of passing the next chunk of the source text
					 * @details The chunks are passed in the order of their succession in the text and are divided
					 * arbitrarily: a break is admissible in any place, including in the middle of a name
					 * or of a value
					 * @note The flag of the last chunk is obligatory: without it the end of a text cannot be distinguished
					 * from its cut-off, and the last line without an ending character would remain
					 * unparsed
					 * @note A refusal of the conversion of the source text to the UTF-8 encoding is issued not by
					 * the present method but upon the exhaustion of the already converted beginning of the text:
					 * the events parsed before the spoiled character are issued, and a feeding of the text
					 * in full issues the same as a feeding of it by chunks. A line cut off
					 * by a spoiled character is not issued as an event
					 * @param buffer buffer of the next chunk of the source text
					 * @param size   size of the buffer of the next chunk of the source text
					 * @param end    flag of the chunk being the last one
					 * @return       result of performing the operation
					 *
					 * \~
					 */
					bool feed(const void * buffer, const size_t size, const bool end) noexcept;
					/**
					 * \~russian
					 * @brief Метод передачи исходного текста целиком
					 *
					 * @details Разбирает переданный текст как единственный и последний кусок
					 *
					 * @note Переданный текст переживать вызов не обязан: он приводится к
					 * кодировке UTF-8 в собственное хранилище разбора, и выдаваемые
					 * последовательности знаков ссылаются на него, а не на переданное
					 *
					 * @param text исходный текст настроек целиком
					 * @return     результат выполнения операции
					 *
					 * \~english
					 * @brief Method of passing the source text in full
					 * @details Parses the passed text as the single and last chunk
					 * @note The passed text is not obliged to outlive the call: it is converted to
					 * the UTF-8 encoding into the own storage of the parsing, and the issued
					 * sequences of characters refer to it rather than to what has been passed
					 * @param text source settings text in full
					 * @return     result of performing the operation
					 *
					 * \~
					 */
					bool feed(const string_view text) noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод перехода к следующему событию разбора
					 *
					 * @details Отрицательный итог означает, что событий больше нет: разбор
					 * либо исчерпал переданное и ждёт следующего куска, либо дошёл до конца
					 * текста, либо прекращён ошибкой. Что именно произошло, сообщает
					 * состояние чтения
					 *
					 * @return признак наличия очередного события разбора
					 *
					 * \~english
					 * @brief Method of moving to the next parsing event
					 * @details A negative result means that there are no more events: the parsing
					 * has either exhausted what has been passed and is waiting for the next chunk, or has reached the end of the
					 * text, or has been terminated by an error. What exactly has happened is reported by the
					 * state of the reading
					 * @return flag of the presence of the next parsing event
					 *
					 * \~
					 */
					bool next() noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения текущего состояния чтения
					 *
					 * @return текущее состояние чтения текста настроек
					 *
					 * \~english
					 * @brief Method of getting the current state of the reading
					 * @return current state of the reading of a settings text
					 *
					 * \~
					 */
					state_t state() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения вида текущего события разбора
					 *
					 * @return вид текущего события разбора
					 *
					 *
					 * \~english
					 * @brief Method of getting the kind of the current parsing event
					 * @return kind of the current parsing event
					 *
					 * \~
					 */
					event_t event() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки разбора
					 *
					 * @return код ошибки последней операции разбора
					 *
					 *
					 * \~english
					 * @brief Method of getting the error code of the parsing
					 * @return error code of the last operation of the parsing
					 *
					 * \~
					 */
					error_t error() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места обнаружения ошибки
					 *
					 * @return положение обнаруженной ошибки в исходном тексте
					 *
					 *
					 * \~english
					 * @brief Method of getting the place of the detection of an error
					 * @return position of the detected error in the source text
					 *
					 * \~
					 */
					const location_t & errorLocation() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения имени текущего раздела
					 *
					 * @details Имя устанавливается событием объявления раздела и держится
					 * до объявления следующего. До первого объявления имя пусто: свойства,
					 * записанные раньше, принадлежат разделу без имени
					 *
					 * @return имя текущего раздела текста настроек
					 *
					 * @warning Вид живёт ЛИШЬ до следующего события того же рода. Ссылается он не
					 * на буфер разбора, а на удержанную строку чтения, и оттого ОСТАЁТСЯ ЧИТАЕМЫМ
					 * после - но отдаёт содержимое ЧУЖОЙ записи, той, что пришла следом. Надзор за
					 * памятью такого не ловит вовсе: память жива, лгут одни лишь данные. Замерено
					 * щупом на трёхстах записях с несхожими именами - вид отдал «ИНОЕ299» вместо
					 * своего содержимого
					 *
					 * @note Имена в щупе взяты НЕСХОЖИЕ намеренно: при общем начале подмена
					 * неразличима - вид той же длины читает те же первые байты, и первый заход
					 * щупа ложно отчитался о сохранности
					 *
					 * @warning Беда эта хуже освобождённой памяти: там отказ явен, а здесь ответ
					 * правдоподобен. Держать надлежит копию, а не вид
					 *
					 * \~english
					 * @brief Method of getting the name of the current section
					 * @details The name is set by the event of a section declaration and is kept
					 * until the next declaration. Before the first declaration the name is empty: the properties
					 * written earlier belong to the section without a name
					 * @return name of the current section of the settings text
					 *
					 * \~
					 */
					const name_t & section() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения свойства текущего события
					 *
					 * @details Пригодно лишь для события свойства; для прочих событий поля
					 * свойства пусты
					 *
					 * @return свойство текущего события разбора
					 *
					 * @warning Вид живёт ЛИШЬ до следующего обращения к @c next() либо @c feed().
					 * Ссылается он на буфер разбора, и подача его перевыделяет: вид, удержанный
					 * прежде, указывает на память ОСВОБОЖДЁННУЮ. Надзор за памятью валит это
					 * настоящим обращением к освобождённому - замерено щупом на трёхстах записях.
					 * Держать надлежит копию, а не вид
					 *
					 * \~english
					 * @brief Method of getting the property of the current event
					 * @details Suitable only for a property event; for the other events the fields
					 * of the property are empty
					 * @return property of the current parsing event
					 *
					 * \~
					 */
					const property_t & property() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения примечания текущего события
					 *
					 * @details Пригодно лишь для события примечания; для прочих событий
					 * поля примечания пусты
					 *
					 * @return примечание текущего события разбора
					 *
					 * @warning Вид живёт ЛИШЬ до следующего события того же рода. Ссылается он не
					 * на буфер разбора, а на удержанную строку чтения, и оттого ОСТАЁТСЯ ЧИТАЕМЫМ
					 * после - но отдаёт содержимое ЧУЖОЙ записи, той, что пришла следом. Надзор за
					 * памятью такого не ловит вовсе: память жива, лгут одни лишь данные. Замерено
					 * щупом на трёхстах записях с несхожими именами - вид отдал «ИНОЕ299» вместо
					 * своего содержимого
					 *
					 * @note Имена в щупе взяты НЕСХОЖИЕ намеренно: при общем начале подмена
					 * неразличима - вид той же длины читает те же первые байты, и первый заход
					 * щупа ложно отчитался о сохранности
					 *
					 * @warning Беда эта хуже освобождённой памяти: там отказ явен, а здесь ответ
					 * правдоподобен. Держать надлежит копию, а не вид
					 *
					 * \~english
					 * @brief Method of getting the comment of the current event
					 * @details Suitable only for a comment event; for the other events
					 * the fields of the comment are empty
					 * @return comment of the current parsing event
					 *
					 * \~
					 */
					const comment_t & comment() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения имени свойства текущего события
					 *
					 * @return имя свойства текущего события разбора
					 *
					 * @warning Вид живёт ЛИШЬ до следующего обращения к @c next() либо @c feed().
					 * Ссылается он на буфер разбора, и подача его перевыделяет: вид, удержанный
					 * прежде, указывает на память ОСВОБОЖДЁННУЮ. Надзор за памятью валит это
					 * настоящим обращением к освобождённому - замерено щупом на трёхстах записях.
					 * Держать надлежит копию, а не вид
					 *
					 * \~english
					 * @brief Method of getting the name of the property of the current event
					 * @return name of the property of the current parsing event
					 *
					 * \~
					 */
					string_view key() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения содержимого текущего события
					 *
					 * @details Для свойства - его значение, для примечания - его содержимое.
					 * Для прочих событий содержимое пусто
					 *
					 * @warning Возвращаемая последовательность знаков остаётся пригодной лишь
					 * до следующего обращения к @c next() либо @c feed()
					 *
					 * @return содержимое текущего события разбора
					 *
					 * \~english
					 * @brief Method of getting the content of the current event
					 * @details For a property — its value, for a comment — its content.
					 * For the other events the content is empty
					 * @warning The returned sequence of characters remains valid only
					 * until the next call to @c next() or @c feed()
					 * @return content of the current parsing event
					 *
					 * \~
					 */
					string_view text() const noexcept;
					/**
					 * \~russian
					 * @brief Метод получения места текущего события в исходном тексте
					 *
					 * @return положение начала текущего события в исходном тексте
					 *
					 * \~english
					 * @brief Method of getting the place of the current event in the source text
					 * @return position of the beginning of the current event in the source text
					 *
					 * \~
					 */
					const location_t & location() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Шаблон типа числа результата разбора
					 *
					 * @tparam T тип числа результата разбора
					 *
					 *
					 * \~english
					 * @brief Template of the number type of the parsing result
					 * @tparam T number type of the parsing result
					 *
					 * \~
					 */
					template <typename T>
					/**
					 * \~russian
					 * @brief Метод получения значения текущего свойства числом
					 *
					 * @details Разбор ведётся по правилам местности «C» с отбрасыванием
					 * пробельной обвязки и с проверкой выхода за пределы запрошенного типа
					 *
					 * @param result ссылка на результат разбора
					 * @param forms  признаваемая запись логического значения
					 * @return       признак успешного разбора
					 *
					 * \~english
					 * @brief Method of getting the value of the current property as a number
					 * @details The parsing is conducted by the rules of the «C» locale with the discarding of the
					 * whitespace padding and with a check of going beyond the limits of the requested type
					 * @param result reference to the result of the parsing
					 * @param forms  recognized notation of a logical value
					 * @return       flag of a successful parsing
					 *
					 * \~
					 */
					bool value(T & result, const boolean_t forms = boolean_t::EXTENDED) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения определённой кодировки исходного текста
					 *
					 * @return определённая кодировка исходного текста
					 *
					 *
					 * \~english
					 * @brief Method of getting the determined encoding of the source text
					 * @return determined encoding of the source text
					 *
					 * \~
					 */
					encoding_t encoding() const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log объект для работы с логами
					 *
					 * \~english
					 * @brief Constructor
					 * @param log object for working with logs
					 *
					 * \~
					 */
					Reader(const log_t * log) noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param log      объект для работы с логами
					 * @param settings настройки разбора текста настроек
					 *
					 * \~english
					 * @brief Constructor
					 * @param log      object for working with logs
					 * @param settings settings of the parsing of a settings text
					 *
					 * \~
					 */
					Reader(const log_t * log, const settings_t & settings) noexcept;
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
					~Reader() noexcept;
			} reader_t;
		};
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"

#endif // __AWH_CODEC_INI_READER__
