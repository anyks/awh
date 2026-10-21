/**
 * @file pages.inc
 * @date 2026-10-21
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
 * @brief Внутренние тела встраиваемых методов страничной кучи
 *
 * @details Файл подключается реализациями модуля и в поставляемый набор не входит:
 *          под include/ собраны только интерфейсные заголовки. Тело метода обязано
 *          быть видно в каждой единице трансляции, зовущей метод, - иначе
 *          всегда-встраивание не работает, - а общее тело в одном месте избавляет
 *          от копий по числу зовущих файлов.
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Подключаем заголовочный файл модуля
 */
#include <alloc/pages.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Пространство имён фреймворка
 */
namespace awh {
	/**
	 * Пространство имён распределителя памяти
	 */
	namespace alloc {
		/**
		 * @brief Метод поиска куска, которому принадлежит адрес
		 *
		 * @note Подставляется намеренно: подсказка отвечает почти всегда, и вызов
		 *       ради сличения двух границ стоил бы дороже самого сличения. Поиск
		 *       же по таблице - холодный хвост, и он остался в файле кода
		 *
		 * @param addr разбираемый адрес
		 * @param hint место хранения подсказки поиска
		 * @return     найденный кусок либо nullptr
		 *
		 */
		AWH_PAGES_INLINE pages_t::chunk_t * pages_t::lookup(const void * addr, void ** hint) const noexcept {
			/**
			 * Сперва пробуем подсказку
			 *
			 * Поток освобождает блоки из тех же немногих кусков, что и выдавал, и
			 * сличение границ последнего найденного куска отвечает почти всегда
			 */
			if(hint != nullptr){
				// Получаем кусок из подсказки
				chunk_t * chunk = reinterpret_cast <chunk_t *> (* hint);
				// Если подсказка задана и адрес лежит внутри её куска
				if((chunk != nullptr) && (reinterpret_cast <uintptr_t> (addr) >= reinterpret_cast <uintptr_t> (chunk->base)) &&
				   (reinterpret_cast <uintptr_t> (addr) < (reinterpret_cast <uintptr_t> (chunk->base) + chunk->size)))
					// Выводим кусок из подсказки
					return chunk;
			}
			// Ищем кусок по таблице
			chunk_t * chunk = this->discover(addr);
			// Если кусок найден и подсказку требуется запомнить
			if((chunk != nullptr) && (hint != nullptr))
				// Запоминаем найденный кусок подсказкой
				(* hint) = chunk;
			// Выводим найденный кусок
			return chunk;
		}
		/**
		 * @brief Метод получения метки разряда по адресу
		 *
		 * @details Быстрый путь освобождения: отвечает меткой разряда одним
		 *          чтением, не трогая ни указателей страниц, ни учётной записи
		 *          области
		 *
		 * @note Нуль означает «ответить меткой нельзя»: адрес не наш, либо
		 *       страница разряду не отдана. Тогда разбор идёт полным путём -
		 *       через `describe`
		 *
		 * @param addr разбираемый адрес
		 * @param hint место хранения подсказки поиска
		 * @return     метка разряда, увеличенная на единицу, либо нуль
		 *
		 */
		AWH_PAGES_INLINE uint8_t pages_t::classify(const void * addr, [[maybe_unused]] void ** hint) const noexcept {
			// Определяем номер страницы адреса
			const uintptr_t page = (reinterpret_cast <uintptr_t> (addr) >> SHIFT);
			// Получаем корень радикс-карты
			const root_t * root = this->_root.load(std::memory_order_acquire);
			// Если корень не заведён
			if(root == nullptr)
				// Ответить меткой нельзя
				return 0;
			// Получаем средний ярус радикс-карты
			const mid_t * mid = root[page >> (LEAF_BITS * 2)].load(std::memory_order_acquire);
			// Если среднего яруса нет
			if(mid == nullptr)
				// Ответить меткой нельзя
				return 0;
			// Получаем лист радикс-карты
			const leaf_t * leaf = mid[(page >> LEAF_BITS) & LEAF_MASK].load(std::memory_order_acquire);
			// Если листа нет
			if(leaf == nullptr)
				// Ответить меткой нельзя
				return 0;
			// Выводим метку разряда страницы
			return leaf[page & LEAF_MASK].load(std::memory_order_acquire);
		}
		/**
		 * @brief Метод описания области, которой принадлежит адрес
		 *
		 * @details Адрес сперва ищется в куске по подсказке, затем страница куска
		 *          отдаёт запись области. Описание отвечает лишь по выданной наружу
		 *          области: у освобождённой или никому не отданной страницы описания
		 *          нет.
		 *
		 * @param addr  разбираемый адрес
		 * @param begin место записи адреса начала области
		 * @param pages место записи размера области в страницах
		 * @param tag   место записи метки владельца области
		 * @param hint  место хранения подсказки поиска
		 * @return      признак принадлежности адреса выданной области
		 *
		 */
		AWH_PAGES_INLINE bool pages_t::describe(const void * addr, void ** begin, size_t * pages, uint32_t * tag, void ** hint) const noexcept {
			// Ищем кусок, которому принадлежит адрес
			const chunk_t * chunk = this->lookup(addr, hint);
			// Если куска за адресом не нашлось
			if(chunk == nullptr)
				// Адрес куче не принадлежит
				return false;
			// Определяем номер страницы, которой принадлежит адрес
			const size_t page = static_cast <size_t> ((reinterpret_cast <uintptr_t> (addr) - reinterpret_cast <uintptr_t> (chunk->base)) / PAGE);
			// Получаем область, которой принадлежит страница
			const span_t * span = chunk->index[page].load(std::memory_order_acquire);
			// Если области у страницы нет либо область наружу не выдана
			if((span == nullptr) || span->released)
				// Описывать нечего
				return false;
			// Если требуется адрес начала области
			if(begin != nullptr)
				// Записываем адрес начала области
				(* begin) = span->base;
			// Если требуется размер области
			if(pages != nullptr)
				// Записываем размер области
				(* pages) = span->pages;
			// Если требуется метка владельца
			if(tag != nullptr)
				// Записываем метку владельца
				(* tag) = span->tag;
			// Отвечаем успехом
			return true;
		}
	};
};
