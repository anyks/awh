/**
 * @file common.hpp
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
 * @brief Заголовочный файл общих типов модуля Grok —
 *        коды ошибок разбора шаблона, виды значений полей, описание поля
 *        и собранный шаблон
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
#include <vector>
#include <memory>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочный файл модуля
 */
#include "../regex.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * DELETE и ERROR у MS Windows, CS и PRIVATE у Sun Solaris, CS5 у termios.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён модуля Grok
	 *
	 */
	namespace grok {
		/**
		 * @brief Виды значений полей шаблона Grok
		 *
		 * @details Вид задаётся третьей частью ссылки: «%{INT:code:int}». Модуль
		 *          вид запоминает, но приведением значения не занимается: захват
		 *          выдаётся текстом, а истолкование его остаётся за потребителем,
		 *          - кодеку JSON вид нужен для выбора представления значения, а
		 *          выводу в поток не нужен вовсе.
		 *
		 */
		enum class kind_t : uint8_t {
			TEXT     = 0x00, // Значение поля выдаётся текстом
			INTEGER  = 0x01, // Значение поля прочитано числом целым
			FLOATING = 0x02  // Значение поля прочитано числом дробным
		};
		/**
		 * @brief Коды ошибок разбора шаблона Grok
		 *
		 */
		enum class error_t : uint8_t {
			NONE                = 0x00, // Ошибок не обнаружено
			PATTERN_EMPTY       = 0x01, // Текст шаблона пуст
			NAME_EMPTY          = 0x02, // Название шаблона пусто
			REFERENCE_UNCLOSED  = 0x03, // Скобка ссылки на шаблон не закрыта
			REFERENCE_EMPTY     = 0x04, // Ссылка на шаблон не несёт названия
			REFERENCE_UNKNOWN   = 0x05, // Ссылка на шаблон, реестру неизвестный
			REFERENCE_CIRCULAR  = 0x06, // Круговая ссылка шаблонов
			FIELD_EMPTY         = 0x07, // Название поля в ссылке пусто
			KIND_UNKNOWN        = 0x08, // Вид значения поля модулю неизвестен
			NESTING_TOO_DEEP    = 0x09, // Превышена допустимая глубина разворота
			PATTERN_TOO_LARGE   = 0x0A, // Развёрнутый текст превышает допустимый размер
			EXPRESSION          = 0x0B, // Сборка развёрнутого выражения не выполнена
			STORAGE             = 0x0C, // Запись собранных шаблонов не восстановлена
			STORAGE_EMPTY       = 0x0D, // Набор шаблонов записи пуст
			STORAGE_MAGIC       = 0x0E, // Опознание записи собранных шаблонов не совпадает
			STORAGE_VERSION     = 0x0F, // Версия записи собранных шаблонов не поддерживается
			STORAGE_TRUNCATED   = 0x10, // Запись собранных шаблонов оборвана до завершения
			STORAGE_METHOD      = 0x11, // Обработчик метода сжатия записи не установлен
			STORAGE_PACKING     = 0x12, // Сжатие либо разбор сжатого содержимого записи не выполнены
			STORAGE_CHECKSUM    = 0x13  // Контрольная сумма содержимого записи не совпадает
		};

		/**
		 * @brief Вид записи JSON
		 */
		enum class format_t : uint8_t {
			COMPACT = 0x00, // Запись без отступов
			PRETTY  = 0x01  // Запись с отступами
		};
		/**
		 * @brief Значение JSON извлечённых полей
		 *
		 * @details Собственный вывод записи JSON заведён потому, что кодека JSON
		 *          в AWH v4 нет. Запись ведётся объектом полей: название поля и
		 *          значение, текст экранируется, числа кладутся готовым токеном,
		 *          отчего вид числа сохраняется, а не угадывается повторно.
		 *
		 */
		class __AWH_SHARED_EXPORT__ Json {
			private:
				/**
				 * @brief Пункт записи JSON
				 */
				typedef struct Item {
					// Название поля
					string name;
					// Признак числа
					bool numeric;
					// Запись значения (для числа — готовый токен)
					string value;
				} item_t;
			private:
				// Пункты записи JSON
				vector <item_t> _items;
			private:
				// Название поля, которому заносится значение
				string _key;
			public:
				/**
				 * @brief Метод очистки записи
				 */
				void clear() noexcept;
				/**
				 * @brief Метод проверки пустоты записи
				 *
				 * @return результат проверки
				 */
				bool empty() const noexcept;
				/**
				 * @brief Метод открытия объекта записи
				 */
				void object() noexcept;
				/**
				 * @brief Метод занесения названия поля
				 *
				 * @param name название поля
				 */
				void key(string_view name) noexcept;
				/**
				 * @brief Метод занесения значения поля текстом
				 *
				 * @param value значение поля
				 */
				void value(string_view value) noexcept;
				/**
				 * @brief Метод занесения значения поля числом дробным
				 *
				 * @param value значение поля
				 */
				void value(const double value) noexcept;
				/**
				 * @brief Метод занесения значения поля числом целым со знаком
				 *
				 * @param value значение поля
				 */
				void value(const int64_t value) noexcept;
				/**
				 * @brief Метод занесения значения поля числом целым без знака
				 *
				 * @param value значение поля
				 */
				void value(const uint64_t value) noexcept;
				/**
				 * @brief Метод получения собранной записи
				 *
				 * @return собранная запись
				 */
				Json finish() const noexcept;
				/**
				 * @brief Метод вывода записи JSON текстом
				 *
				 * @param format вид записи
				 * @return       запись JSON текстом
				 */
				string dump(const format_t format = format_t::COMPACT) const noexcept;
		};
		using json_t = Json;

		/**
		 * @brief Структура описания поля шаблона Grok
		 *
		 */
		typedef struct __AWH_SHARED_EXPORT__ Field {
			// Номер группы захвата развёрнутого выражения
			uint32_t number;
			// Вид значения поля
			kind_t kind;
			// Название поля
			string name;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Field() noexcept;
		} field_t;
		/**
		 * @brief Структура извлечённого значения поля шаблона Grok
		 *
		 * @details Значение выдаётся текстом всегда: вид его нужен потребителю
		 *          для выбора представления, а не модулю для разбора.
		 *
		 */
		typedef struct __AWH_SHARED_EXPORT__ Value {
			// Вид значения поля
			kind_t kind;
			// Название поля
			string name;
			// Извлечённое значение поля
			string value;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Value() noexcept;
		} value_t;
		/**
		 * @brief Структура собранного шаблона Grok
		 *
		 * @details Собранный шаблон после сборки не изменяется и разделяется
		 *          потоками исполнения без согласования доступа.
		 *
		 */
		typedef struct __AWH_SHARED_EXPORT__ Expression {
			// Исходный текст шаблона со ссылками
			string pattern;
			// Развёрнутый текст регулярного выражения
			string expression;
			// Собранное регулярное выражение развёрнутого текста
			regular_t::exp_t exp;
			// Набор полей шаблона в порядке объявления
			vector <field_t> fields;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Expression() noexcept;
		} expression_t;
	};
};

/**
 * Возвращаем системные макросы потребителю библиотеки:
 * имена, подавленные в начале файла, снова принадлежат ему
 */
#include "../../sys/macro/restore.hpp"
