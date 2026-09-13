/**
 * @file lexer.hpp
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
 * @brief Заголовочный файл разборщика параметров запуска и текстовых потоков
 *
 * \~english
 * @brief Header file of the parser of the parameters of the launch and of the text streams
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_ARGS_LEXER__
#define __AWH_ARGS_LEXER__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <functional>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "common.hpp"
#include <sys/macro/global.hpp>

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
		 * @brief Разборщик параметров запуска и текстовых потоков
		 *
		 * @details Выдаёт лексемы одну за другой отзывом, своих копий не держа.
		 * Оба входа - набор запуска и текстовый поток - разбирает один и тот же код:
		 * текст сперва режется на слова, а дальше разбор идёт словом за словом
		 *
		 * @par Намеренные решения
		 *
		 * @li **Слово, начатое тире, значением следующим не берётся - кроме числа.**
		 * Запись «--count -5» подаёт значение «-5», а запись «--verbose --debug» -
		 * два параметра без значений. Иначе всякий признак съедал бы следующий за
		 * ним параметр, и набор менял бы смысл от перестановки
		 *
		 * @li **Разбор отказом не прерывается.** Отказавшая лексема пропускается, а
		 * код отказа с положением уходит отзыву: набор запуска разбирается целиком,
		 * чтобы приложение показало сразу все огрехи набора, а не первый из них
		 *
		 * \~english
		 * @brief Parser of the parameters of the launch and of the text streams
		 * @details Issues the lexemes one after another by a callback, making no copies of the submitted.
		 * Both inputs — the set of the launch and a text stream — are parsed by one and the same code:
		 * the text is at first cut into the words, and further the parsing goes word by word
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Lexer {
			public:
				/**
				 * \~russian
				 * @brief Настройки разбора параметров
				 *
				 *
				 * \~english
				 * @brief Settings of the parsing of the parameters
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Settings {
					// Признак признания записи «--» концом именованных параметров
					bool terminus;
					// Признак взятия слова, начатого тире и видом схожего с числом, значением
					bool negative;
					// Предельная длина имени параметра в октетах
					size_t maxKey;
					// Предельная длина значения параметра в октетах
					size_t maxValue;
					// Предельное число лексем у одного разбора
					size_t maxTokens;
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
					 terminus(true), negative(true),
					 maxKey(1024), maxValue(1048576), maxTokens(65536) {}
				} settings_t;
				/**
				 * \~russian
				 * @brief Отзыв выдачи разобранной лексемы
				 *
				 * @details Возврат ложью останавливает разбор
				 *
				 * \~english
				 * @brief Callback of the issuance of a parsed lexeme
				 * @details A return by a false stops the parsing
				 *
				 * \~
				 */
				typedef function <bool (const lexeme_t &)> callback_t;
				/**
				 * \~russian
				 * @brief Отзыв извещения об отказе разбора
				 *
				 * @details Возврат ложью останавливает разбор
				 *
				 * \~english
				 * @brief Callback of the notification of a refusal of the parsing
				 * @details A return by a false stops the parsing
				 *
				 * \~
				 */
				typedef function <bool (const error_t, const location_t &)> failure_t;
			private:
				// Настройки разбора параметров
				settings_t _settings;
			private:
				/**
				 * Объект работы с логами
				 *
				 * @warning Поле это разборщиком НЕ ЧИТАЕТСЯ, и это намеренно, а не
				 *          недосмотр: об отказах разбора он извещает ОТЗЫВОМ, а не
				 *          журналом - решает о них вызывающий, он же и оглашает.
				 *          Хранится же поле оттого, что пара `(fmk_t *, log_t *)`
				 *          есть общий вид конструкторов всего дерева, и снимать её
				 *          ради тишины собирателя значило бы разводить виды
				 *          конструкторов по классам
				 *
				 * @note Собиратель clang зовёт это `-Wunused-private-field`; ключ
				 *       этот в сборке проекта не включён. Понадобится разборщику
				 *       журнал - поле уже на месте
				 */
			private:
				/**
				 * \~russian
				 * @brief Метод определения схожести слова с числом
				 *
				 * @param word слово для проверки
				 * @return     результат проверки
				 *
				 * \~english
				 * @brief Method of the determination of the resemblance of a word to a number
				 * @param word word for the check
				 * @return result of the check
				 *
				 * \~
				 */
				[[nodiscard]] bool numeric(const string_view word) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разреза текста на слова
				 *
				 * @details Кавычки одинарные и двойные объединяют слово целиком, а обратная
				 * косая снимает особое значение у следующего за нею знака
				 *
				 * @param text     текст для разреза
				 * @param result   контейнер собранных слов
				 * @param failure  отзыв извещения об отказе разбора
				 * @return         результат разреза
				 *
				 * \~english
				 * @brief Method of the cutting of a text into the words
				 * @param text text for the cutting
				 * @param result container of the assembled words
				 * @param failure callback of the notification of a refusal of the parsing
				 * @return result of the cutting
				 *
				 * \~
				 */
				[[nodiscard]] bool split(const string_view text, vector <string> & result, const failure_t & failure = nullptr) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод разбора набора доводов запуска
				 *
				 * @param items    набор доводов запуска
				 * @param callback отзыв выдачи разобранной лексемы
				 * @param failure  отзыв извещения об отказе разбора
				 * @return         результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a set of the arguments of the launch
				 * @param items set of the arguments of the launch
				 * @param callback callback of the issuance of a parsed lexeme
				 * @param failure callback of the notification of a refusal of the parsing
				 * @return result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool parse(const vector <string> & items, const callback_t & callback, const failure_t & failure = nullptr) const noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора текстового потока
				 *
				 * @param text     текст для разбора
				 * @param callback отзыв выдачи разобранной лексемы
				 * @param failure  отзыв извещения об отказе разбора
				 * @return         результат разбора
				 *
				 * \~english
				 * @brief Method of the parsing of a text stream
				 * @param text text for the parsing
				 * @param callback callback of the issuance of a parsed lexeme
				 * @param failure callback of the notification of a refusal of the parsing
				 * @return result of the parsing
				 *
				 * \~
				 */
				[[nodiscard]] bool parse(const string_view text, const callback_t & callback, const failure_t & failure = nullptr) const noexcept;
			public:
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
				const settings_t & settings() const noexcept;
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
				void settings(const settings_t & settings) noexcept;
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
				explicit Lexer() noexcept;
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
				~Lexer() noexcept {}
		} lexer_t;
	}
}

#endif // __AWH_ARGS_LEXER__
