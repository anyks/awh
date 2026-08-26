/**
 * @file trace.hpp
 * @date 2026-08-21
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
 * @brief Заголовочный файл съёма стека вызовов — запоминание места выдачи памяти и
 *        обращение снятых адресов в имена
 *
 * @section trace_decisions Намеренные решения
 *
 * @details <b>Съём идёт раскруткой, а не `backtrace` из execinfo.h.</b> Заголовка того
 *          нет у musl вовсе, у FreeBSD и NetBSD он требует отдельной библиотеки
 *          (`-lexecinfo`), а у glibc `backtrace` при первом обращении подгружает
 *          libgcc_s через `dlopen` - то есть ВЫДЕЛЯЕТ ПАМЯТЬ, а мы стоим внутри
 *          выдачи памяти. Раскрутка же (`_Unwind_Backtrace`) есть у всех собирателей,
 *          какими мы собираемся, и связывается при загрузке.
 *
 *          <b>У MS Windows съём свой.</b> Раскрутки того же вида там нет, а
 *          `RtlCaptureStackBackTrace` снимает стек без памяти и без замков - это родной
 *          путь системы, и подражать ему чужим незачем.
 *
 *          <b>Прогрев обязателен.</b> Первое обращение к раскрутке строит указатели
 *          разделов раскрутки и у части систем выделяет память. Оттого съём
 *          прогревается ОДНАЖДЫ при заведении - вне всякого выделения, - и дальше
 *          памяти не просит. Проверяется это не рассуждением, а наблюдением: щуп
 *          считает обращения к нашему же malloc вокруг съёма.
 *
 *          <b>Возвратность разрывается признаком потока, а не запретом.</b> Раскрутка
 *          вольна обратиться за памятью, и обращение это пришло бы обратно в съём.
 *          Признак «мы внутри съёма» хранится ключом системы, а НЕ поток-локальным
 *          местом: ленивое заведение того само зовёт выделение памяти (проверено срывом
 *          стека на macOS внутри `_tlv_get_addr`).
 *
 *          <b>Имена добываются отдельно от съёма.</b> Обращение адреса в имя (`dladdr`,
 *          перебор образов у MS Windows) стоит дорого и памяти просит. Съём же обязан
 *          быть дёшев: он идёт на пути выдачи. Оттого снятое хранится адресами, а имена
 *          добываются потом - при докладе.
 *
 * \~english
 * @brief Header file of call stack capture — remembering the allocation site and
 *        resolving captured addresses into names
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_TRACE__
#define __AWH_ALLOC_TRACE__

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
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
		 * @brief Сведения об адресе стека вызовов
		 *
		 * @note Строки указывают на память образа и на память средств системы: годны
		 *       они до следующего обращения к разбору В ТОМ ЖЕ ПОТОКЕ, а копировать их
		 *       обязан тот, кому они нужны дольше. Оговорка о потоке не лишняя: у
		 *       MS Windows название образа хранится за нами, и буфер под него свой у
		 *       каждого потока - разбор в чужом потоке строку не трогает
		 *
		 * \~english
		 * @brief Information about a call stack address
		 *
		 */
		typedef struct Symbol {
			// Название образа, которому принадлежит адрес
			const char * image;
			// Название функции, которой принадлежит адрес
			const char * name;
			// Адрес начала функции
			const void * begin;
			// Смещение разбираемого адреса от начала функции
			ptrdiff_t offset;
			/**
			 * @brief Конструктор
			 *
			 */
			Symbol() noexcept :
			 image(nullptr), name(nullptr), begin(nullptr), offset(0) {}
		} symbol_t;
		/**
		 * \~russian
		 * @brief Класс съёма стека вызовов
		 *
		 * \~english
		 * @brief Call stack capture class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Trace {
			public:
				// Наибольшая снимаемая глубина стека вызовов
				static constexpr size_t DEPTH = 32;
			private:
				/**
				 * Признак прогретого съёма
				 *
				 * Вид НЕДЕЛИМЫЙ: пишет его настройка съёма, а читает сам съём - и читает
				 * на всяком потоке, идущем путём разбора мест выдачи. Обычное поле давало
				 * бы состязание по стандарту
				 */
				// Признак прогретого съёма
				std::atomic <bool> _warmed;
				// Признак заведённого ключа признака съёма
				/**
				 * Признак заведённого ключа - НЕДЕЛИМЫЙ
				 *
				 * Заведение съёма зовётся из настроек, а те приложение вправе менять из
				 * любого потока. Два потока, вошедшие в заведение разом, оба видели
				 * признак снятым и оба заводили ключ: второй затирал первый, тот утекал,
				 * а поток, отметившийся внутри съёма ПЕРВЫМ ключом, читался вторым как
				 * не отметившийся - и вложенный съём становился возможен, ровно тот, ради
				 * запрета которого отметка и заведена
				 */
				std::atomic <bool> _keyed;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения съёма стека вызовов
				 *
				 * @note Звать вне выделения памяти: заведение прогревает раскрутку, а та
				 *       при первом обращении памяти просить вправе
				 *
				 * @return признак заведения съёма
				 *
				 * \~english
				 * @brief Method of initializing the call stack capture
				 *
				 */
				bool init() noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия съёма стека вызовов
				 *
				 * \~english
				 * @brief Method of shutting down the call stack capture
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод съёма стека вызовов
				 *
				 * @note Памяти метод не просит вовсе: снятое кладётся в переданный массив,
				 *       а раскрутка прогрета при заведении
				 *
				 * @param frames массив под адреса стека вызовов
				 * @param depth  длина массива в местах
				 * @param skip   число ближних уровней, какие пропустить
				 * @return       число снятых адресов
				 *
				 * \~english
				 * @brief Method of capturing the call stack
				 *
				 */
				size_t capture(const void ** frames, const size_t depth, const size_t skip) noexcept;
				/**
				 * \~russian
				 * @brief Метод обращения адреса стека в имя
				 *
				 * @note Метод дорог и памяти просит: звать его на пути выдачи нельзя, он
				 *       для доклада
				 *
				 * @param frame  разбираемый адрес
				 * @param symbol сведения о разобранном адресе
				 * @return       признак разбора адреса
				 *
				 * \~english
				 * @brief Method of resolving a stack address into a name
				 *
				 */
				bool resolve(const void * frame, symbol_t & symbol) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения готовности съёма
				 *
				 * @return признак готовности съёма
				 *
				 * \~english
				 * @brief Method of determining whether the capture is ready
				 *
				 */
				bool ready() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения названия способа съёма
				 *
				 * @return название способа съёма
				 *
				 * \~english
				 * @brief Method of getting the capture method name
				 *
				 */
				const char * name() const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Trace() noexcept : _warmed(false), _keyed(false) {}
		} trace_t;
	};
};

#endif // __AWH_ALLOC_TRACE__
