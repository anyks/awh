/**
 * @file profile.hpp
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
 * @brief Заголовочный файл учёта мест выдачи памяти — запоминание стека вызовов у
 *        части выданных блоков и перебор того, что прикладной код удерживает
 *
 * @section profile_decisions Намеренные решения
 *
 * @details <b>Речь об УДЕРЖИВАЕМОМ, а не об утечках.</b> Распределитель не знает
 *          намерений прикладного кода: блок, живущий до конца процесса, может быть и
 *          утечкой, и намеренным кэшем. Оттого модуль отвечает на вопрос «что и откуда
 *          выдано и до сих пор не освобождено», а называть это утечкой волен лишь тот,
 *          кто писал код.
 *
 *          <b>Учёт выборочный, и по умолчанию выключен вовсе.</b> Место выдачи стоит
 *          съёма стека при выдаче и поиска по таблице при КАЖДОМ освобождении - в том
 *          числе у блоков, под учёт не попавших. Плата эта оправдана при разборе, а не
 *          в бою, и потому берётся лишь по требованию.
 *
 *          <b>Стек хранится в самой записи, а не ссылкой.</b> Ссылка означала бы
 *          второе хранилище со своим сроком жизни и своим замком. Запись же с местом
 *          под стек внутри выдаётся и возвращается одним движением, а стоит она сотни
 *          байт на блок - при выборке одного блока из тысячи это ничто.
 *
 *          <b>Перебор идёт с замком, взятым на всё время.</b> Перебирать живые блоки,
 *          отпуская замок между ними, значило бы отдавать наружу записи, какие сосед
 *          волен снести. Перебор этот идёт при разборе, а не в бою, и цена замка тут
 *          неважна.
 *
 * \~english
 * @brief Header file of allocation site accounting — remembering the call stack of a
 *        fraction of allocated blocks and enumerating what the application holds
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_ALLOC_PROFILE__
#define __AWH_ALLOC_PROFILE__

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <cstddef>
#include <cstdint>

/**
 * Наши модули
 */
#include "spin.hpp"
#include "trace.hpp"
#include "source.hpp"
#include "../sys/global.hpp"

/**
 * Если компилятор принадлежит к Visual Studio
 */
#if defined(_MSC_VER)
	/**
	 * Принудительная подстановка средствами Visual Studio
	 */
	#define AWH_PROFILE_INLINE __forceinline
/**
 * Если компилятор принадлежит к семейству GCC или Clang
 */
#else
	/**
	 * Принудительная подстановка средствами GCC и Clang
	 */
	#define AWH_PROFILE_INLINE inline __attribute__((always_inline))
#endif

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
		 * @brief Сведения об удерживаемом прикладным кодом блоке
		 *
		 * @note Массив стека указывает на память самой записи: годен он лишь внутри
		 *       отклика перебора, а нужен дольше - копировать
		 *
		 * \~english
		 * @brief Information about a block held by the application
		 *
		 */
		typedef struct Holding {
			// Адрес удерживаемого блока
			const void * block;
			// Затребованный размер блока в байтах
			size_t size;
			// Отметка времени выдачи в миллисекундах
			uint64_t stamp;
			// Стек вызовов места выдачи
			const void * const * frames;
			// Глубина снятого стека вызовов
			size_t depth;
			/**
			 * @brief Конструктор
			 *
			 */
			Holding() noexcept :
			 block(nullptr), size(0), stamp(0), frames(nullptr), depth(0) {}
		} holding_t;
		/**
		 * \~russian
		 * @brief Класс учёта мест выдачи памяти
		 *
		 * \~english
		 * @brief Allocation site accounting class
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Profile {
			public:
				// Начальная длина таблицы учёта в местах
				static constexpr size_t TABLE = 1024;
				// Наибольшее число учитываемых блоков
				static constexpr size_t LIMIT = (1024u * 1024u);
			public:
				/**
				 * \~russian
				 * @brief Состояние учёта мест выдачи
				 *
				 * \~english
				 * @brief Allocation site accounting state
				 *
				 */
				typedef struct State {
					// Число учитываемых живых блоков
					size_t live;
					// Объём учитываемых живых блоков в байтах
					size_t bytes;
					// Число блоков, взятых под учёт за время работы
					size_t enrolled;
					// Число блоков, не взятых под учёт из-за нехватки места
					size_t dropped;
					/**
					 * @brief Конструктор
					 *
					 */
					State() noexcept : live(0), bytes(0), enrolled(0), dropped(0) {}
				} state_t;
				/**
				 * \~russian
				 * @brief Отклик перебора удерживаемых блоков
				 *
				 * @note Возврат лжи прекращает перебор: он идёт с взятым замком, и дать
				 *       ему уйти раньше времени важнее удобства
				 *
				 * \~english
				 * @brief Callback for enumerating held blocks
				 *
				 */
				typedef bool (* walker_t)(const holding_t & holding, void * context);
			private:
				/**
				 * @brief Учётная запись выданного блока
				 *
				 */
				typedef struct Record {
					// Адрес выданного блока
					const void * block;
					// Затребованный размер блока в байтах
					size_t size;
					// Отметка времени выдачи в миллисекундах
					uint64_t stamp;
					// Глубина снятого стека вызовов
					size_t depth;
					// Следующая запись в списке повторно используемых
					struct Record * spare;
					// Стек вызовов места выдачи
					const void * frames[Trace::DEPTH];
				} record_t;
			private:
				// Источник страниц
				source_t * _source;
				// Съём стека вызовов
				trace_t * _trace;
				// Замок учёта
				spin_t _lock;
				// Таблица учёта выданных блоков
				record_t ** _table;
				// Длина таблицы учёта в местах
				size_t _length;
				// Число записей, внесённых в таблицу
				size_t _enrolled;
				// Текущий кусок памяти под учётные записи
				uint8_t * _meta;
				// Остаток текущего куска памяти под учётные записи
				size_t _metaLeft;
				// Список повторно используемых учётных записей
				record_t * _spare;
				// Доля выборки: одна выдача из скольких
				std::atomic <size_t> _rate;
				// Счётчик выдач для выборки
				std::atomic <size_t> _counter;
				// Число учитываемых живых блоков
				std::atomic <size_t> _live;
				// Состояние учёта мест выдачи
				state_t _state;
			private:
				/**
				 * Метка снесённого места таблицы учёта
				 *
				 */
				static record_t * const _tomb;
			private:
				/**
				 * @brief Метод выдачи памяти под учётную запись
				 *
				 * @return адрес выданной памяти либо nullptr
				 *
				 */
				void * meta() noexcept;
				/**
				 * @brief Метод перестроения таблицы учёта
				 *
				 * @param length требуемая длина таблицы в местах
				 * @return       признак перестроения таблицы
				 *
				 */
				bool rehash(const size_t length) noexcept;
				/**
				 * @brief Метод внесения записи в таблицу учёта
				 *
				 * @param record вносимая запись
				 * @return       признак внесения записи
				 *
				 */
				bool insert(record_t * record) noexcept;
				/**
				 * @brief Метод поиска места записи в таблице учёта
				 *
				 * @param block разбираемый адрес блока
				 * @return      место записи в таблице либо длина таблицы
				 *
				 */
				size_t seek(const void * block) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения учёта мест выдачи
				 *
				 * @param source источник страниц
				 * @param trace  съём стека вызовов
				 * @return       признак заведения учёта
				 *
				 * \~english
				 * @brief Method of initializing the allocation site accounting
				 *
				 */
				bool init(source_t * source, trace_t * trace) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия учёта мест выдачи
				 *
				 * \~english
				 * @brief Method of shutting down the allocation site accounting
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод определения надобности учёта очередной выдаче
				 *
				 * @note Метод меняет счётчик выборки и оттого зовётся ровно однажды на
				 *       выдачу
				 *
				 * @return признак надобности учёта
				 *
				 * \~english
				 * @brief Method of determining whether the next allocation is accounted
				 *
				 */
				/**
				 * \~russian
				 * @brief Метод выборки выдачи под учёт места
				 *
				 * @note Холодный хвост `wanted`: сюда приходят лишь при включённом учёте
				 *
				 * @return признак того, что выборка взяла эту выдачу
				 *
				 * \~english
				 * @brief Method of sampling an allocation for tracking
				 *
				 */
				bool sampled() noexcept;
				AWH_PROFILE_INLINE bool wanted() noexcept {
					/**
					 * Отвечаем отказом, не заходя в файл кода
					 *
					 * Учёт мест выдачи выключен у подавляющего большинства приложений, а вопрос
					 * этот стоит на пути КАЖДОЙ выдачи: работы здесь одно нестрогое чтение,
					 * а вызов стоил дороже работы
					 */
					// Если учёт выключен
					if(this->_rate.load(std::memory_order_relaxed) == 0)
						// Учёт не нужен
						return false;
					// Уходим холодным путём: учёт включён, идёт выборка
					return this->sampled();
				}
				/**
				 * \~russian
				 * @brief Метод определения ведения учёта хоть каких-то блоков
				 *
				 * @note Нужен затем, чтобы освобождение не искало в таблице, когда учёта
				 *       нет вовсе: поиск этот стоит замка на каждом освобождении
				 *
				 * @return признак наличия учитываемых блоков
				 *
				 * \~english
				 * @brief Method of determining whether any blocks are accounted for
				 *
				 */
				AWH_PROFILE_INLINE bool tracking() const noexcept {
					// Выводим признак наличия учитываемых блоков
					return (this->_live.load(std::memory_order_relaxed) > 0);
				}
			public:
				/**
				 * \~russian
				 * @brief Метод взятия выданного блока под учёт
				 *
				 * @note Стек снимается здесь же: место выдачи известно лишь в этот миг
				 *
				 * @param block адрес выданного блока
				 * @param size  затребованный размер блока в байтах
				 * @param stamp отметка времени выдачи в миллисекундах
				 * @param skip  число ближних уровней стека, какие пропустить
				 * @return      признак взятия блока под учёт
				 *
				 * \~english
				 * @brief Method of taking an allocated block into account
				 *
				 */
				bool enroll(const void * block, const size_t size, const uint64_t stamp, const size_t skip) noexcept;
				/**
				 * \~russian
				 * @brief Метод снятия блока с учёта
				 *
				 * @param block адрес освобождаемого блока
				 * @return      признак снятия блока с учёта
				 *
				 * \~english
				 * @brief Method of removing a block from the accounting
				 *
				 */
				bool expel(const void * block) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перебора удерживаемых прикладным кодом блоков
				 *
				 * @param callback отклик перебора
				 * @param context  предмет, отдаваемый отклику
				 * @return         число перебранных блоков
				 *
				 * \~english
				 * @brief Method of enumerating the blocks held by the application
				 *
				 */
				size_t walk(walker_t callback, void * context) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод задания доли выборки
				 *
				 * @param rate одна выдача из скольких: нуль - учёт выключен
				 *
				 * \~english
				 * @brief Method of setting the sampling rate
				 *
				 */
				void rate(const size_t rate) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения состояния учёта мест выдачи
				 *
				 * @return состояние учёта мест выдачи
				 *
				 * \~english
				 * @brief Method of getting the allocation site accounting state
				 *
				 */
				state_t state() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод захвата замка перед ветвлением процесса
				 *
				 * \~english
				 * @brief Method of acquiring the lock before process forking
				 *
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод отпускания замка после ветвления процесса
				 *
				 * \~english
				 * @brief Method of releasing the lock after process forking
				 *
				 */
				void resume() noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения замка в порядок у потомка ветвления
				 *
				 * \~english
				 * @brief Method of resetting the lock in the forked child
				 *
				 */
				void adopt() noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				Profile() noexcept;
		} profile_t;
	};
};

#endif // __AWH_ALLOC_PROFILE__
