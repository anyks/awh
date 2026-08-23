/**
 * @file grok.hpp
 * @date 2026-08-04
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
 * @brief Заголовочный файл открытого интерфейса модуля Grok —
 *        реестр именованных шаблонов, разворот ссылок вида «%{NAME:поле:вид}»
 *        в регулярное выражение и извлечение именованных полей из текста
 *
 * @section grok_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Ссылки разворачиваются текстом, а не вызовом подшаблона.</b>
 *          Развернуть «%{IP}» можно было бы и рекурсивным вызовом «(?&IP)»,
 *          сохранив текст выражения коротким. Разворот текстом выбран по
 *          замерам: он открывает сборщику выражения всё дерево целиком, отчего
 *          работают и предварительный отбор начальных байтов, и порождение
 *          машинного кода, - а вызов подшаблона запирает их за границей вызова.
 *          Плата - размер развёрнутого текста, и она принята.
 *
 *          <b>Поле пишется группой безымянной, а название ведётся своей
 *          таблицей.</b> Прямая запись «%{IP:src-ip}» в «(?<src-ip>...)»
 *          выглядит короче, но названия полей Grok шире названий групп
 *          регулярного выражения: в них встречаются дефис и точка, а внутри
 *          одного шаблона название вправе повторяться. Поэтому соответствие
 *          «номер группы - название поля» ведётся набором fields собранного
 *          шаблона, а выражение получает группу безымянную.
 *
 *          <b>Выражение собирается с режимом DUPNAMES.</b> Тексты шаблонов
 *          набора несут именованные группы и впрямую, и одно название
 *          встречается в ветвях, объединяемых шаблоном вышестоящим. Для Grok
 *          это допустимо, поэтому повторное объявление названия отказом не
 *          сопровождается.
 *
 *          <b>Кэш собранных шаблонов очищается при всякой правке реестра.</b>
 *          Развёрнутый текст шаблона зависит от всего реестра целиком, а не от
 *          одного текста шаблона: замена шаблона «WORD» меняет разворот всякой
 *          ссылки, к нему ведущей, - и отследить эту зависимость дешевле
 *          очисткой кэша, чем ведением связей. Правка реестра дело редкое,
 *          сборка - частое, поэтому обмен выгоден.
 *
 *          <b>Вид значения поля запоминается, но извлечение приведения не
 *          выполняет.</b> Захват выдаётся текстом всегда: приведение к числу
 *          нужно выводу в JSON и не нужно выводу в поток, поэтому вид оставлен
 *          сведением, а применяет его лишь вывод в JSON.
 *
 *          <b>Вывод в JSON приводит значение по виду поля, а не по виду
 *          записи.</b> Прежняя надстройка Grok опознавала числа, разглядывая
 *          сам захват: запись, числу подобная, выводилась числом независимо от
 *          объявленного вида. Способ этот ошибается на записях, числами не
 *          являющихся: номер версии, код ответа с ведущим нулём, значение поля
 *          «id» выводились числом и теряли вид исходный. Ныне числом выводится
 *          лишь поле, вид какому объявлен ссылкой, - а захват, объявленному
 *          виду не отвечающий, выводится текстом, дабы вывод оставался
 *          правильным JSON.
 *
 *          <b>Набор шаблонов читается из текста, а не из файла.</b> Классический
 *          Grok держит наборы файлами вида «НАЗВАНИЕ выражение», и чтение их
 *          напрашивалось бы прямо здесь. Однако файловый ввод-вывод AWH
 *          требует объектов фреймворка и журнала, каких у Grok нет и заводить
 *          какие ради одного чтения означало бы утяжелить всякого потребителя.
 *          Поэтому метод read принимает текст, а чтение файла либо обход
 *          каталога возложены на вызывающую сторону: объектом «awh::fs_t»
 *          либо любым иным способом.
 *
 *          <b>Сжатие записи выполняют обработчики потребителя.</b> Решение
 *          то же, что и у хранилища собранных выражений, и по той же причине:
 *          сторонних библиотек сжатия модуль не подключает.
 *
 *          Обработчики эти Grok и хранилищу передаёт, и сам применяет: запись
 *          складывается из части, шаблоны описывающей, и записи хранилища,
 *          к ней приложенной, - и хранилище сжимает лишь свою часть. Часть же
 *          Grok несёт развёрнутые тексты выражений, каковые сжимаются лучше
 *          всего прочего: на встроенном наборе она весит 218 Кбайт из 13,4 Мбайт
 *          записи несжатой - долю ничтожную, - но сжимается до 16,4 Кбайт, тогда
 *          как часть хранилища сжимается лишь до 890 Кбайт. Оставленная несжатой,
 *          она составила бы пятую долю записи сжатой, а сжатие её делает запись
 *          легче на восемнадцатую часть.
 *
 *          Метод сжатия пишется в заголовок части наравне с длинами её до сжатия
 *          и после: восстановление опирается на запись, а не на настройку
 *          восстанавливающего. Обработчик, записи потребный, при этом обязан
 *          быть установлен - отсутствие его отказ и вызывает, - ибо самих
 *          средств сжатия модуль не несёт.
 *
 *          <b>Запись несёт развёрнутый текст и таблицу полей, а не одни лишь
 *          выражения.</b> Разворот ссылок зависит от всего реестра целиком,
 *          поэтому восстановление по одному тексту шаблона потребовало бы
 *          того же реестра, в том же состоянии. Запись же несёт итог разворота,
 *          отчего восстановленный шаблон от реестра не зависит вовсе и годен
 *          объекту с реестром иным либо пустым.
 *
 *          <b>Восстановленные шаблоны размещаются в кэше слабой ссылкой.</b>
 *          Сильная ссылка удерживала бы весь восстановленный набор до очистки
 *          реестра, а набор этот бывает в миллион шаблонов. Поэтому владение
 *          остаётся у вызывающей стороны, а кэш лишь выдаёт готовое, пока она
 *          набор удерживает.
 *
 *          <b>Повторное название поля в выводе JSON выигрывает последним.</b>
 *          Объект JSON повторов ключа не несёт, и выбор стоял между потерей
 *          значения и переменной формой вывода - то текстом, то набором.
 *          Форма постоянная для потребителя важнее, а способ, потерь не
 *          несущий, остаётся: извлечение набором значений.
 *
 * \~english
 * @brief Header file of the public interface of the Grok module —
 *        the registry of named patterns, the expansion of references of the «%{NAME:field:kind}» form
 *        into a regular expression and the extraction of named fields from a text
 * @section grok_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>The references are expanded as text rather than by a subpattern call.</b>
 *          «%{IP}» could have been expanded by a recursive «(?&IP)» call as well,
 *          keeping the text of the expression short. Expansion as text was chosen by
 *          measurement: it opens the whole tree to the expression builder, which is why
 *          both the preliminary selection of starting bytes and the generation of
 *          machine code work — whereas a subpattern call locks them behind the boundary of the call.
 *          The price is the size of the expanded text, and it has been accepted.
 *          <b>A field is written as an unnamed group, while its name is kept in a
 *          table of its own.</b> Writing «%{IP:src-ip}» directly as «(?<src-ip>...)»
 *          looks shorter, but the field names of Grok are wider than the group names
 *          of a regular expression: they contain a hyphen and a dot, and within
 *          one pattern a name is entitled to repeat. Therefore the correspondence
 *          «group number — field name» is kept by the fields set of the built
 *          pattern, while the expression receives an unnamed group.
 *          <b>The expression is built with the DUPNAMES mode.</b> The pattern texts of the
 *          set carry named groups directly as well, and one name
 *          occurs in the branches joined by a higher-level pattern. For Grok
 *          that is admissible, therefore a repeated declaration of a name is not
 *          accompanied by a refusal.
 *          <b>The cache of built patterns is cleared on every change of the registry.</b>
 *          The expanded text of a pattern depends on the whole registry rather than on
 *          the text of one pattern: replacing the «WORD» pattern changes the expansion of every
 *          reference leading to it — and tracking that dependency is cheaper by
 *          clearing the cache than by keeping the links. Changing the registry is a rare business,
 *          building a frequent one, therefore the trade is profitable.
 *          <b>The kind of a field value is remembered, but the extraction performs no
 *          conversion.</b> The capture is always yielded as text: conversion to a number
 *          is needed by the output to JSON and is not needed by the output to a stream, therefore the kind is left as
 *          information, and only the output to JSON applies it.
 *          <b>The output to JSON converts a value by the kind of the field rather than by the kind of the
 *          record.</b> The former Grok superstructure recognised numbers by examining
 *          the capture itself: a record resembling a number was output as a number regardless of
 *          the declared kind. That way errs on records that are not
 *          numbers: a version number, a response code with a leading zero, the value of an
 *          «id» field were output as numbers and lost their original form. Nowadays only a field
 *          whose kind is declared by a reference is output as a number — while a capture not matching the declared
 *          kind is output as text, so that the output remains
 *          valid JSON.
 *          <b>A pattern set is read from a text rather than from a file.</b> The classic
 *          Grok keeps its sets as files of the «NAME expression» form, and reading them
 *          would suggest itself right here. However the file input-output of AWH
 *          requires framework and log objects, which Grok does not have and introducing
 *          which for the sake of one read would mean weighing down every consumer.
 *          Therefore the read method takes a text, while reading a file or walking
 *          a directory is laid on the calling side: by an «awh::fs_t» object
 *          or by any other means.
 *          <b>Compression of the record is performed by the handlers of the consumer.</b> The decision
 *          is the same as for the storage of built expressions, and for the same reason:
 *          the module includes no third-party compression libraries.
 *          Those handlers Grok both passes to the storage and applies itself: the record
 *          consists of the part describing the patterns and the storage record
 *          attached to it — and the storage compresses only its own part. The Grok part,
 *          on the other hand, carries the expanded texts of the expressions, which compress better
 *          than anything else: on the built-in set it weighs 218 KB out of 13.4 MB
 *          of the uncompressed record — a negligible share — but compresses down to 16.4 KB, whereas
 *          the storage part compresses only down to 890 KB. Left uncompressed,
 *          it would make up a fifth of the compressed record, while compressing it makes the record
 *          lighter by an eighteenth.
 *          The compression method is written into the header of the part on a par with its lengths before
 *          and after compression: the restoration rests on the record rather than on the setting of the
 *          restoring side. The handler needed by the record must at the same time
 *          be set — its absence is what causes a refusal — for the module carries no
 *          compression means of its own.
 *          <b>The record carries the expanded text and the field table rather than the
 *          expressions alone.</b> The expansion of the references depends on the whole registry,
 *          therefore restoration from the text of one pattern would require
 *          the same registry, in the same state. The record, on the other hand, carries the outcome of the expansion,
 *          which is why a restored pattern does not depend on the registry at all and is fit for
 *          an object with a different registry or with none.
 *          <b>The restored patterns are placed in the cache by a weak reference.</b>
 *          A strong reference would hold the whole restored set until the registry
 *          is cleared, and that set is sometimes a million patterns. Therefore the ownership
 *          stays with the calling side, and the cache only hands out what is ready while it
 *          holds the set.
 *          <b>A repeated field name in the JSON output wins by the last one.</b>
 *          A JSON object carries no repeated key, and the choice was between losing
 *          a value and a variable form of the output — now a text, now an array.
 *          A constant form is more important to the consumer, and a way that carries no
 *          losses remains: extraction as an array of values.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_GROK_INTERFACE__
#define __AWH_GROK_INTERFACE__

/**
 * Стандартные заголовочные файлы
 */
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <utility>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "table.hpp"
#include "common.hpp"
#include "../storage.hpp"

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
	 * @brief Класс разбора текста по шаблонам Grok
	 *
	 * @details Класс ведёт реестр именованных шаблонов, разворачивает ссылки
	 *          вида «%{NAME}», «%{NAME:поле}» и «%{NAME:поле:вид}» в текст
	 *          регулярного выражения и извлекает из текста именованные поля.
	 *          Собранный шаблон отделён от объекта сборки, разделяется
	 *          несколькими объектами и потоками исполнения одновременно и
	 *          освобождается вместе с последней ссылкой на него.
	 *
	 * \~english
	 * @brief Class of parsing a text by Grok patterns
	 * @details The class keeps the registry of named patterns, expands the references
	 *          of the «%{NAME}», «%{NAME:field}» and «%{NAME:field:kind}» form into the text
	 *          of a regular expression and extracts the named fields from a text.
	 *          A built pattern is separated from the building object, is shared
	 *          by several objects and threads of execution at once and
	 *          is released together with the last reference to it.
	 *
	 * \~
	 */
	typedef class __AWH_SHARED_EXPORT__ Grok {
		public:
			/**
			 * \~russian
			 * @brief Код ошибки разбора шаблона
			 *
			 * \~english
			 * @brief Parse error code of a pattern
			 *
			 * \~
			 */
			using error_t = awh::grok::error_t;
			/**
			 * \~russian
			 * @brief Вид значения поля шаблона
			 *
			 * \~english
			 * @brief Kind of a field value of a pattern
			 *
			 * \~
			 */
			using kind_t = awh::grok::kind_t;
			/**
			 * \~russian
			 * @brief Описание поля шаблона
			 *
			 * \~english
			 * @brief Description of a field of a pattern
			 *
			 * \~
			 */
			using field_t = awh::grok::field_t;
			/**
			 * \~russian
			 * @brief Режим сборки регулярного выражения
			 *
			 * \~english
			 * @brief Build mode of a regular expression
			 *
			 * \~
			 */
			using flag_t = awh::regex::flag_t;
			/**
			 * \~russian
			 * @brief Извлечённое значение поля шаблона
			 *
			 * \~english
			 * @brief Extracted value of a field of a pattern
			 *
			 * \~
			 */
			using value_t = awh::grok::value_t;
			/**
			 * \~russian
			 * @brief Собранный шаблон Grok
			 *
			 * \~english
			 * @brief Built Grok pattern
			 *
			 * \~
			 */
			using exp_t = shared_ptr <const awh::grok::expression_t>;
			/**
			 * \~russian
			 * @brief Обработчик сжатия либо разжатия записи собранных шаблонов
			 *
			 * \~english
			 * @brief Handler of compression or decompression of the record of built patterns
			 *
			 * \~
			 */
			using packer_t = awh::regex::storage_t::packer_t;
		private:
			/**
			 * \~russian
			 * @brief Ключ кэша собранных шаблонов Grok
			 *
			 * \~english
			 * @brief Key of the cache of built Grok patterns
			 *
			 * \~
			 */
			using key_t = pair <uint32_t, string>;
		private:
			/**
			 * \~russian
			 * @brief Хэш-функция ключа кэша собранных шаблонов Grok
			 *
			 * \~english
			 * @brief Hash function of the key of the cache of built Grok patterns
			 *
			 * \~
			 */
			struct __AWH_SHARED_EXPORT__ Hash {
				/**
				 * \~russian
				 * @brief Оператор вычисления хэша ключа кэша
				 *
				 * @param key ключ кэша собранных шаблонов Grok
				 * @return    вычисленное значение хэша ключа
				 *
				 * \~english
				 * @brief Operator computing the hash of a cache key
				 * @param key key of the cache of built Grok patterns
				 * @return    computed hash value of the key
				 *
				 * \~
				 */
				size_t operator () (const key_t & key) const noexcept;
			};
		private:
			// Объект работы с регулярными выражениями
			awh::RegularExpression _regexp;
		private:
			// Объект хранилища собранных регулярных выражений
			awh::regex::storage_t _storage;
		private:
			/**
			 * \~russian
			 * Метод сжатия части записи, шаблоны описывающей
			 *
			 * @details Запись складывается из части, шаблоны описывающей, и
			 *          записи хранилища собранных выражений, к ней приложенной.
			 *          Хранилище сжимает свою часть само, а часть эта сжимается
			 *          здесь теми же обработчиками: без того она осталась бы
			 *          несжатой посреди сжатого, а весит она развёрнутые тексты
			 *          выражений - пятую долю записи сжатой.
			 *
			 * \~english
			 * Compression method of the part of the record describing the patterns
			 * @details The record consists of the part describing the patterns and of the
			 *          record of the storage of built expressions attached to it.
			 *          The storage compresses its own part itself, while this part is compressed
			 *          here by the same handlers: without that it would stay
			 *          uncompressed in the middle of the compressed, and it weighs the expanded texts
			 *          of the expressions — a fifth of the compressed record.
			 *
			 * \~
			 */
			compressor::method_t _method;
			// Обработчик сжатия части записи, шаблоны описывающей
			packer_t _pack;
			// Обработчик разбора сжатой части записи, шаблоны описывающей
			packer_t _unpack;
		private:
			// Код ошибки разбора шаблона
			mutable error_t _error;
		private:
			// Объект журнала событий
			const log_t * _log;
		private:
			// Признак согласования доступа к реестру шаблонов
			bool _threadSafety;
		private:
			// Объект согласования доступа к реестру шаблонов
			/**
			 * \~russian
			 * Имя типа уточняется пространством имён намеренно
			 *
			 * @warning У Solaris в общем пространстве имён своё имя mutex - оно приходит
			 *          из sys/t_lock.h, - и голое обращение там становится двусмысленным.
			 *          Уточнение принято и у прочих заголовков набора: threadpool и signals
			 *          пишут его так же
			 *
			 * \~english
			 * The type name is qualified by the namespace deliberately
			 * @warning On Solaris the global namespace has a mutex name of its own — it comes
			 *          from sys/t_lock.h — and a bare reference becomes ambiguous there.
			 *          The qualification is adopted in the other headers of the set as well: threadpool and signals
			 *          write it the same way
			 *
			 * \~
			 */
			mutable std::mutex _mtx;
		private:
			// Реестр именованных шаблонов
			unordered_map <string, string> _patterns;
		private:
			// Кэш собранных шаблонов Grok
			mutable unordered_map <key_t, weak_ptr <const awh::grok::expression_t>, Hash> _cache;
		private:
			/**
			 * \~russian
			 * @brief Метод разворота ссылок текста шаблона
			 *
			 * @param body   текст шаблона со ссылками
			 * @param result развёрнутый текст регулярного выражения
			 * @param fields набор полей шаблона
			 * @param stack  набор шаблонов, разворот каких не завершён
			 * @param number номер очередной группы захвата
			 * @param depth  действующая глубина разворота
			 * @return       результат разворота ссылок текста шаблона
			 *
			 * \~english
			 * @brief Method of expanding the references of the text of a pattern
			 * @param body   text of the pattern with references
			 * @param result expanded text of the regular expression
			 * @param fields set of the fields of the pattern
			 * @param stack  set of the patterns whose expansion is not finished
			 * @param number number of the next capture group
			 * @param depth  current depth of the expansion
			 * @return       result of expanding the references of the text of the pattern
			 *
			 * \~
			 */
			bool expand(string_view body, string & result, vector <field_t> & fields, vector <string> & stack, uint32_t & number, const uint16_t depth) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод подсчёта групп захвата участка текста выражения
			 *
			 * @param text   участок текста регулярного выражения
			 * @param number номер очередной группы захвата
			 *
			 * \~english
			 * @brief Method of counting the capture groups of a stretch of the text of an expression
			 * @param text   stretch of the text of the regular expression
			 * @param number number of the next capture group
			 *
			 * \~
			 */
			void account(string_view text, uint32_t & number) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки реестра шаблонов
			 *
			 * @details Реестр очищается целиком, вместе со встроенным набором.
			 *
			 * \~english
			 * @brief Method of clearing the pattern registry
			 * @details The registry is cleared as a whole, together with the built-in set.
			 *
			 * \~
			 */
			void clear() noexcept;
			/**
			 * \~russian
			 * @brief Метод восстановления встроенного набора шаблонов
			 *
			 * @details Реестр очищается и наполняется встроенным набором заново.
			 *
			 * \~english
			 * @brief Method of restoring the built-in pattern set
			 * @details The registry is cleared and filled with the built-in set anew.
			 *
			 * \~
			 */
			void reset() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки наличия шаблона в реестре
			 *
			 * @param name название шаблона
			 * @return     результат проверки наличия шаблона в реестре
			 *
			 * \~english
			 * @brief Method of checking the presence of a pattern in the registry
			 * @param name name of the pattern
			 * @return     result of checking the presence of the pattern in the registry
			 *
			 * \~
			 */
			bool has(string_view name) const noexcept;
			/**
			 * \~russian
			 * @brief Метод удаления шаблона из реестра
			 *
			 * @param name название шаблона
			 * @return     результат удаления шаблона из реестра
			 *
			 * \~english
			 * @brief Method of removing a pattern from the registry
			 * @param name name of the pattern
			 * @return     result of removing the pattern from the registry
			 *
			 * \~
			 */
			bool erase(string_view name) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения текста шаблона из реестра
			 *
			 * @param name название шаблона
			 * @return     текст шаблона либо пустой текст при его отсутствии
			 *
			 * \~english
			 * @brief Method of getting the text of a pattern from the registry
			 * @param name name of the pattern
			 * @return     text of the pattern or an empty text if it is absent
			 *
			 * \~
			 */
			string pattern(string_view name) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления шаблона в реестр
			 *
			 * @param name название шаблона
			 * @param body текст шаблона, допускающий ссылки вида «%{NAME}»
			 * @return     результат добавления шаблона в реестр
			 *
			 * @details Шаблон с названием, реестру уже известным, заменяет
			 *          прежний: пользовательский набор кладётся поверх
			 *          встроенного.
			 *
			 * \~english
			 * @brief Method of adding a pattern to the registry
			 * @param name name of the pattern
			 * @param body text of the pattern, admitting references of the «%{NAME}» form
			 * @return     result of adding the pattern to the registry
			 * @details A pattern with a name already known to the registry replaces
			 *          the former one: the user set is laid over the
			 *          built-in one.
			 *
			 * \~
			 */
			bool pattern(string_view name, string_view body) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод наполнения реестра набором шаблонов из текста
			 *
			 * @param text текст набора шаблонов
			 * @return     количество шаблонов, принятых реестром
			 *
			 * @details Набор записывается построчно видом «НАЗВАНИЕ выражение»:
			 *          название отделено от текста шаблона пробельными символами.
			 *          Строки пустые и строки, начинающиеся знаком «#», пропускаются.
			 *          Шаблон с названием, реестру уже известным, заменяет прежний.
			 *
			 *          Чтение самого файла возложено на вызывающую сторону: Grok
			 *          файловым вводом-выводом не ведает, дабы не тянуть за собою
			 *          объекты фреймворка и журнала. Прочесть файл можно объектом
			 *          «awh::fs_t» либо любым иным способом, а сюда подать текст.
			 *
			 * \~english
			 * @brief Method of filling the registry with a pattern set from a text
			 * @param text text of the pattern set
			 * @return     number of the patterns accepted by the registry
			 * @details The set is written line by line in the «NAME expression» form:
			 *          the name is separated from the text of the pattern by whitespace characters.
			 *          Empty lines and lines starting with the «#» sign are skipped.
			 *          A pattern with a name already known to the registry replaces the former one.
			 *          Reading the file itself is laid on the calling side: Grok
			 *          knows nothing of file input-output, so as not to drag along
			 *          the framework and log objects. A file can be read by an
			 *          «awh::fs_t» object or by any other means, and the text supplied here.
			 *
			 * \~
			 */
			size_t read(string_view text) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения названий шаблонов реестра
			 *
			 * @return набор названий шаблонов реестра
			 *
			 * \~english
			 * @brief Method of getting the names of the patterns of the registry
			 * @return set of the names of the patterns of the registry
			 *
			 * \~
			 */
			vector <string> patterns() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод сборки шаблона Grok
			 *
			 * @param pattern текст шаблона со ссылками вида «%{NAME}»
			 * @param flags   набор режимов сборки регулярного выражения
			 * @return        собранный шаблон либо пустая ссылка при отказе
			 *
			 * \~english
			 * @brief Method of building a Grok pattern
			 * @param pattern text of the pattern with references of the «%{NAME}» form
			 * @param flags   set of build modes of the regular expression
			 * @return        built pattern or an empty reference on a failure
			 *
			 * \~
			 */
			exp_t build(string_view pattern, const uint32_t flags = 0) const noexcept;
			/**
			 * \~russian
			 * @brief Метод сборки шаблона Grok
			 *
			 * @param pattern текст шаблона со ссылками вида «%{NAME}»
			 * @param flags   набор режимов сборки регулярного выражения
			 * @return        собранный шаблон либо пустая ссылка при отказе
			 *
			 * \~english
			 * @brief Method of building a Grok pattern
			 * @param pattern text of the pattern with references of the «%{NAME}» form
			 * @param flags   set of build modes of the regular expression
			 * @return        built pattern or an empty reference on a failure
			 *
			 * \~
			 */
			exp_t build(string_view pattern, const vector <flag_t> & flags) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки сжатия записи собранных шаблонов
			 *
			 * @param method метод сжатия записи собранных шаблонов
			 * @param pack   обработчик сжатия записи
			 * @param unpack обработчик разжатия записи
			 *
			 * @details Сжатию подлежит запись собранных регулярных выражений -
			 *          доля наибольшая. Само сжатие выполняют обработчики,
			 *          вызывающей стороной установленные: Grok, как и хранилище,
			 *          сторонних библиотек сжатия не подключает.
			 *
			 * \~english
			 * @brief Method of setting the compression of the record of built patterns
			 * @param method compression method of the record of built patterns
			 * @param pack   compression handler of the record
			 * @param unpack decompression handler of the record
			 * @details Subject to compression is the record of built regular expressions —
			 *          the largest share. The compression itself is performed by the handlers
			 *          set by the calling side: Grok, like the storage,
			 *          includes no third-party compression libraries.
			 *
			 * \~
			 */
			void packer(const compressor::method_t method, packer_t pack, packer_t unpack) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки доверия порождённому коду записи
			 *
			 * @param mode признак доверия порождённому коду записи
			 *
			 * @details Признак снят по умолчанию: восстановление порождённый
			 *          машинный код записи не берёт, а порождает заново.
			 *          Установка его означает, что запись изготовлена самим
			 *          потребителем и подмене не подвергалась. Признак
			 *          передаётся хранилищу собранных выражений: порождённый
			 *          код несёт оно, а не часть записи, шаблоны описывающая.
			 *
			 * \~english
			 * @brief Method of setting the trust in the generated code of the record
			 * @param mode indication of trust in the generated code of the record
			 * @details The indication is cleared by default: restoration does not take the generated
			 *          machine code of the record but generates it anew.
			 *          Setting it means that the record was produced by the
			 *          consumer itself and was not subjected to substitution. The indication
			 *          is passed to the storage of built expressions: it is that which carries the generated
			 *          code, and not the part of the record describing the patterns.
			 *
			 * \~
			 */
			void trusted(const bool mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод записи собранных шаблонов Grok
			 *
			 * @param patterns набор текстов шаблонов со ссылками
			 * @param result   запись собранных шаблонов
			 * @param flags    набор режимов сборки регулярного выражения
			 * @return         результат записи собранных шаблонов
			 *
			 * @details Шаблоны собираются и записываются вместе с развёрнутым
			 *          текстом и таблицей полей, поэтому восстановление минует
			 *          и разворот ссылок, и разбор выражения, и компиляцию.
			 *
			 * \~english
			 * @brief Method of writing built Grok patterns
			 * @param patterns set of the texts of the patterns with references
			 * @param result   record of the built patterns
			 * @param flags    set of build modes of the regular expression
			 * @return         result of writing the built patterns
			 * @details The patterns are built and written together with the expanded
			 *          text and the field table, therefore restoration bypasses
			 *          both the expansion of the references and the parsing of the expression and the compilation.
			 *
			 * \~
			 */
			bool save(const vector <string> & patterns, string & result, const uint32_t flags = 0) const noexcept;
			/**
			 * \~russian
			 * @brief Метод записи собранных шаблонов Grok
			 *
			 * @param patterns набор текстов шаблонов со ссылками
			 * @param result   запись собранных шаблонов
			 * @param flags    набор режимов сборки регулярного выражения
			 * @return         результат записи собранных шаблонов
			 *
			 * \~english
			 * @brief Method of writing built Grok patterns
			 * @param patterns set of the texts of the patterns with references
			 * @param result   record of the built patterns
			 * @param flags    set of build modes of the regular expression
			 * @return         result of writing the built patterns
			 *
			 * \~
			 */
			bool save(const vector <string> & patterns, string & result, const vector <flag_t> & flags) const noexcept;
			/**
			 * \~russian
			 * @brief Метод восстановления собранных шаблонов Grok
			 *
			 * @param record запись собранных шаблонов
			 * @param result набор восстановленных шаблонов
			 * @return       результат восстановления собранных шаблонов
			 *
			 * @details Восстановленные шаблоны размещаются и в кэше, поэтому
			 *          сборка шаблона, записи принадлежащего, выдаётся кэшем,
			 *          пока вызывающая сторона удерживает его в наборе.
			 *
			 * \~english
			 * @brief Method of restoring built Grok patterns
			 * @param record record of the built patterns
			 * @param result set of the restored patterns
			 * @return       result of restoring the built patterns
			 * @details The restored patterns are placed in the cache as well, therefore
			 *          building a pattern belonging to the record is yielded by the cache
			 *          while the calling side holds it in the set.
			 *
			 * \~
			 */
			bool load(string_view record, vector <exp_t> & result) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки соответствия текста шаблону
			 *
			 * @param text текст для сопоставления
			 * @param exp  собранный шаблон
			 * @return     результат проверки соответствия текста шаблону
			 *
			 * \~english
			 * @brief Method of checking whether a text matches a pattern
			 * @param text text to match
			 * @param exp  built pattern
			 * @return     result of checking whether the text matches the pattern
			 *
			 * \~
			 */
			bool test(string_view text, const exp_t & exp) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения границ совпадения и захваченных групп
			 *
			 * @param text текст для сопоставления
			 * @param exp  собранный шаблон
			 * @return     набор границ совпадения и захваченных групп
			 *
			 * \~english
			 * @brief Method of getting the boundaries of a match and of the captured groups
			 * @param text text to match
			 * @param exp  built pattern
			 * @return     set of the boundaries of the match and of the captured groups
			 *
			 * \~
			 */
			vector <pair <size_t, size_t>> match(string_view text, const exp_t & exp) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения именованных полей из текста
			 *
			 * @param text   текст для сопоставления
			 * @param exp    собранный шаблон
			 * @param result набор извлечённых полей
			 * @return       результат извлечения именованных полей из текста
			 *
			 * @details Поле, захват какого не выполнен, в набор не добавляется:
			 *          отличить пустой захват от невыполненного иначе нельзя.
			 *
			 * \~english
			 * @brief Method of extracting the named fields from a text
			 * @param text   text to match
			 * @param exp    built pattern
			 * @param result set of the extracted fields
			 * @return       result of extracting the named fields from the text
			 * @details A field whose capture was not performed is not added to the set:
			 *          an empty capture cannot be told from an unperformed one otherwise.
			 *
			 * \~
			 */
			bool exec(string_view text, const exp_t & exp, unordered_map <string, string> & result) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения значений полей из текста
			 *
			 * @param text   текст для сопоставления
			 * @param exp    собранный шаблон
			 * @param result набор извлечённых значений полей
			 * @return       результат извлечения значений полей из текста
			 *
			 * @details Значения выдаются в порядке объявления полей шаблона и
			 *          вместе с видом каждого. Способ этот отличается от выдачи
			 *          набором соответствий тем, что сохраняет и порядок, и
			 *          повторы: название поля внутри шаблона вправе повторяться,
			 *          и набор соответствий такие значения теряет.
			 *
			 *          Поле, захват какого не выполнен, в набор не добавляется.
			 *
			 * \~english
			 * @brief Method of extracting the field values from a text
			 * @param text   text to match
			 * @param exp    built pattern
			 * @param result set of the extracted field values
			 * @return       result of extracting the field values from the text
			 * @details The values are yielded in the order of declaration of the fields of the pattern and
			 *          together with the kind of each. That way differs from yielding
			 *          a set of correspondences in that it keeps both the order and the
			 *          repetitions: a field name inside a pattern is entitled to repeat,
			 *          and a set of correspondences loses such values.
			 *          A field whose capture was not performed is not added to the set.
			 *
			 * \~
			 */
			bool exec(string_view text, const exp_t & exp, vector <value_t> & result) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод вывода извлечённых полей записью JSON
			 *
			 * @param text   текст для сопоставления
			 * @param exp    собранный шаблон
			 * @param result запись JSON извлечённых полей
			 * @param pretty признак вывода записи с отступами
			 * @return       результат вывода извлечённых полей записью JSON
			 *
			 * \~english
			 * @brief Method of outputting the extracted fields as a JSON record
			 * @param text   text to match
			 * @param exp    built pattern
			 * @param result JSON record of the extracted fields
			 * @param pretty indication of outputting the record with indentation
			 * @return       result of outputting the extracted fields as a JSON record
			 *
			 * \~
			 */
			bool json(string_view text, const exp_t & exp, grok::json_t & result) const noexcept;
			/**
			 * \~russian
			 * @brief Метод вывода набора значений полей записью JSON
			 *
			 * @param values набор значений полей
			 * @param pretty признак вывода записи с отступами
			 * @return       запись JSON набора значений полей
			 *
			 * \~english
			 * @brief Method of outputting a set of field values as a JSON record
			 * @param values set of the field values
			 * @param pretty indication of outputting the record with indentation
			 * @return       JSON record of the set of field values
			 *
			 * \~
			 */
			grok::json_t json(const vector <value_t> & values) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения набора полей собранного шаблона
			 *
			 * @param exp собранный шаблон
			 * @return    набор полей собранного шаблона
			 *
			 * \~english
			 * @brief Method of getting the set of fields of a built pattern
			 * @param exp built pattern
			 * @return    set of the fields of the built pattern
			 *
			 * \~
			 */
			const vector <field_t> & fields(const exp_t & exp) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения кода ошибки разбора шаблона
			 *
			 * @return код ошибки разбора шаблона
			 *
			 * \~english
			 * @brief Method of getting the parse error code of a pattern
			 * @return parse error code of the pattern
			 *
			 * \~
			 */
			error_t error() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки согласования доступа к реестру шаблонов
			 *
			 * @param mode режим согласования доступа к реестру шаблонов
			 *
			 * @details Согласование ведётся для реестра шаблонов, а не для
			 *          сопоставления: собранный шаблон после сборки не
			 *          изменяется и разделяется потоками без согласования.
			 *
			 * \~english
			 * @brief Method of setting the coordination of the access to the pattern registry
			 * @param mode mode of coordinating the access to the pattern registry
			 * @details The coordination is kept for the pattern registry rather than for
			 *          the matching: a built pattern is not changed after building
			 *          and is shared by the threads without coordination.
			 *
			 * \~
			 */
			void threadSafety(const bool mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 *
			 * @details Журналом сообщается отказ сборки шаблона: имя неизвестное,
			 *          ссылка круговая либо вложенность чрезмерная. Ошибки самого
			 *          регулярного выражения сообщает фасад выражений, каким
			 *          объект журнала и передаётся.
			 *
			 * \~english
			 * @brief Constructor
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Grok(const log_t * log = nullptr) noexcept;
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
			~Grok() noexcept {}
	} grok_t;
}

#endif // __AWH_GROK_INTERFACE__
