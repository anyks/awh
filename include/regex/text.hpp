/**
 * @file: text.hpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл операций над текстом сопоставления — извлечение кодового
 *        значения символа, проверка привязок к позиции в тексте и проверка принадлежности
 *        символа классу символов, общие для всех способов исполнения регулярного выражения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_TEXT__
#define __AWH_REGEX_TEXT__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "common.hpp"

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
	 * @brief Пространство имён модуля регулярных выражений
	 *
	 */
	namespace regex {
		/**
		 * @brief Функция проверки установки режима компиляции
		 *
		 * @param flags набор режимов компиляции регулярного выражения
		 * @param value проверяемый режим компиляции регулярного выражения
		 * @return      результат проверки установки режима компиляции
		 *
		 */
		__AWH_SHARED_EXPORT__ bool hasFlag(const uint32_t flags, const flag_t value) noexcept;
		/**
		 * @brief Функция приведения кодового значения символа к нижнему регистру
		 *
		 * @details Приведение выполняется для символов набора ASCII. Приведение символов
		 *          за пределами набора ASCII требует таблиц свойств Юникода.
		 *
		 * @param code кодовое значение приводимого символа
		 * @return     приведённое кодовое значение символа
		 *
		 */
		__AWH_SHARED_EXPORT__ uint32_t fold(const uint32_t code) noexcept;
		/**
		 * @brief Функция проверки принадлежности символа символам слова
		 *
		 * @param code кодовое значение проверяемого символа
		 * @return     результат проверки принадлежности символа символам слова
		 *
		 */
		__AWH_SHARED_EXPORT__ bool isWord(const uint32_t code) noexcept;
		/**
		 * @brief Функция извлечения кодового значения символа в позиции текста
		 *
		 * @details В режиме «UTF» функция разбирает последовательность UTF-8 целиком,
		 *          иначе возвращает кодовое значение одиночного байта. Некорректная
		 *          последовательность разбирается как одиночный байт.
		 *
		 * @param text  текст для сопоставления
		 * @param pos   позиция символа в тексте
		 * @param flags набор режимов компиляции инструкции
		 * @param width длина символа в байтах
		 * @return      кодовое значение символа в позиции текста
		 *
		 */
		__AWH_SHARED_EXPORT__ uint32_t decode(string_view text, const size_t pos, const uint32_t flags, size_t & width) noexcept;
		/**
		 * @brief Функция извлечения длины символа, предшествующего позиции текста
		 *
		 * @details В режиме «UTF» функция отступает к началу последовательности UTF-8,
		 *          иначе отступает на один байт. Функция применяется исполнением,
		 *          продвигающимся по тексту в обратном направлении.
		 *
		 * @param text  текст для сопоставления
		 * @param pos   позиция, предшествующий которой символ измеряется
		 * @param flags набор режимов компиляции инструкции
		 * @return      длина предшествующего позиции символа в байтах
		 *
		 */
		__AWH_SHARED_EXPORT__ size_t behind(string_view text, const size_t pos, const uint32_t flags) noexcept;
		/**
		 * @brief Функция проверки привязки к позиции в тексте
		 *
		 * @param text  текст для сопоставления
		 * @param start позиция начала попытки сопоставления
		 * @param type  тип проверяемой привязки к позиции в тексте
		 * @param flags набор режимов компиляции инструкции
		 * @param pos   проверяемая позиция в тексте
		 * @return      результат проверки привязки к позиции в тексте
		 *
		 */
		__AWH_SHARED_EXPORT__ bool assertion(string_view text, const size_t start, const anchor_t type, const uint32_t flags, const size_t pos) noexcept;
		/**
		 * @brief Функция проверки принадлежности символа классу символов
		 *
		 * @param value класс символов регулярного выражения
		 * @param code  кодовое значение проверяемого символа
		 * @param flags набор режимов компиляции инструкции
		 * @return      результат проверки принадлежности символа классу
		 *
		 */
		__AWH_SHARED_EXPORT__ bool belongs(const class_t & value, const uint32_t code, const uint32_t flags) noexcept;
	};
};

#endif // __AWH_REGEX_TEXT__
