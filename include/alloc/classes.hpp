/**
 * @file classes.hpp
 * @date 2026-08-20
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
 * @brief Заголовочный файл классов размеров — разбиение мелких запросов на разряды,
 *        внутри которых блоки выдаются одинакового размера
 *
 * @section classes_decisions Намеренные решения
 *
 * @details <b>Разряды растут геометрически, а не равномерно.</b> Равномерный шаг даёт
 *          либо непомерное число разрядов, либо непомерную потерю на округлении у
 *          мелких запросов: шаг в 64 байта на запросе в 129 байт теряет треть. Шаг же,
 *          равный восьмой доле размера, держит потерю ниже 12,5 % при семи десятках
 *          разрядов.
 *
 *          <b>Выравнивание шестнадцать байт у всех разрядов.</b> Язык требует, чтобы
 *          выданная память годилась под любой вид с основным выравниванием, а таковое
 *          на всех наших системах равно шестнадцати. Восьмибайтовое выравнивание
 *          мелких разрядов сберегло бы немного памяти ценою нарушения этого правила.
 *
 *          <b>Таблицы строятся при заведении, а не пишутся числами.</b> Записанные
 *          числами разряды расходятся с расчётом при первой же правке размера страницы
 *          или порога мелкого запроса, и расхождение это ничем не выявляется. Здесь
 *          таблица строится тем же расчётом, что и проверяется.
 *
 *          <b>Число страниц на разряд подбирается по потере в хвосте.</b> Область,
 *          нарезаемая на блоки, редко делится на них нацело, и остаток пропадает.
 *          Оттого для каждого разряда берётся наименьшее число страниц, при каком
 *          остаток не превышает восьмой доли области.
 *
 * \~english
 * @brief Header file of size classes — splitting small requests into classes within
 *        which blocks are issued of the same size
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_CLASSES__
#define __AWH_ALLOC_CLASSES__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "pages.hpp"
#include "../sys/global.hpp"

/**
 * @brief Пространство имён фреймворка
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён распределителя памяти
	 *
	 */
	namespace alloc {
		/**
		 * \~russian
		 * @brief Класс разрядов размеров
		 *
		 * \~english
		 * @brief Size classes class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Classes {
			public:
				// Выравнивание выдаваемых блоков в байтах
				static constexpr size_t ALIGN = 16;
				// Наибольший размер, обслуживаемый разрядами, в байтах
				static constexpr size_t MAXIMUM = (32u * 1024u);
				// Наибольшее число разрядов
				static constexpr size_t LIMIT = 96;
				// Порог, до которого поиск разряда идёт мелкой таблицей
				static constexpr size_t SMALL = 1024;
				// Шаг мелкой таблицы поиска в байтах
				static constexpr size_t STEP_SMALL = ALIGN;
				// Шаг крупной таблицы поиска в байтах
				static constexpr size_t STEP_LARGE = 128;
			private:
				// Число заведённых разрядов
				size_t _count;
				// Размер блока каждого разряда в байтах
				size_t _size[LIMIT];
				// Число страниц кучи на область каждого разряда
				size_t _pages[LIMIT];
				// Число блоков в области каждого разряда
				size_t _blocks[LIMIT];
				/**
				 * Таблицы поиска разряда
				 *
				 * Длина берётся с запасом в два места, а не в одно: место ищется делением
				 * с округлением вверх, и при размере, равном порогу, оно выходит на
				 * единицу больше частного. Взяв длину ровно по частному, поиск читал бы
				 * за концом массива - и читал бы мусор, а не отказывал
				 */
				// Таблица поиска разряда для мелких запросов
				uint8_t _small[(SMALL / STEP_SMALL) + 2];
				// Таблица поиска разряда для крупных запросов
				uint8_t _large[(MAXIMUM / STEP_LARGE) + 2];
			public:
				/**
				 * \~russian
				 * @brief Метод построения таблиц разрядов
				 *
				 * @return число заведённых разрядов
				 *
				 * \~english
				 * @brief Method of building the class tables
				 *
				 * @return number of classes created
				 *
				 */
				size_t init() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения разряда по требуемому размеру
				 *
				 * @param size требуемый размер в байтах
				 * @return     номер разряда, либо LIMIT если размер разрядами не обслуживается
				 *
				 * \~english
				 * @brief Method of determining the class by the required size
				 *
				 */
				size_t index(const size_t size) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения размера блока разряда
				 *
				 * @param index номер разряда
				 * @return      размер блока в байтах
				 *
				 * \~english
				 * @brief Method of getting the class block size
				 *
				 */
				size_t size(const size_t index) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения числа страниц на область разряда
				 *
				 * @param index номер разряда
				 * @return      число страниц кучи
				 *
				 * \~english
				 * @brief Method of getting the number of pages per class region
				 *
				 */
				size_t pages(const size_t index) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения числа блоков в области разряда
				 *
				 * @param index номер разряда
				 * @return      число блоков
				 *
				 * \~english
				 * @brief Method of getting the number of blocks per class region
				 *
				 */
				size_t blocks(const size_t index) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения числа заведённых разрядов
				 *
				 * @return число разрядов
				 *
				 * \~english
				 * @brief Method of getting the number of classes created
				 *
				 */
				size_t count() const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Classes() noexcept;
		} classes_t;
	};
};

#endif // __AWH_ALLOC_CLASSES__
