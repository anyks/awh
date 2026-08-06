/**
 * @file: common.hpp
 * @date: 2026-08-04
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл общих типов модуля Grok —
 *        коды ошибок разбора шаблона, виды значений полей, описание поля
 *        и собранный шаблон
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_GROK_COMMON__
#define __AWH_GROK_COMMON__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../regex.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений, применяемых ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../sys/macro_push.hpp"

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
		 * @brief Описание поля шаблона Grok
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
			Field() noexcept : number(0), kind(kind_t::TEXT) {}
		} field_t;

		/**
		 * @brief Извлечённое значение поля шаблона Grok
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
			Value() noexcept : kind(kind_t::TEXT) {}
		} value_t;

		/**
		 * @brief Собранный шаблон Grok
		 *
		 * @details Собранный шаблон после сборки не изменяется и разделяется
		 *          потоками исполнения без согласования доступа.
		 *
		 */
		typedef struct __AWH_SHARED_EXPORT__ Expression {
			// Собранное регулярное выражение развёрнутого текста
			awh::RegularExpression::exp_t exp;
			// Набор полей шаблона в порядке объявления
			vector <field_t> fields;
			// Исходный текст шаблона со ссылками
			string pattern;
			// Развёрнутый текст регулярного выражения
			string expression;
		} expression_t;
	}
}

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_GROK_COMMON__
