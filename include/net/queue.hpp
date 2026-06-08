/**
 * @file: queue.hpp
 * @date: 2026-02-07
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_NETWORK_QUEUE__
#define __AWH_NETWORK_QUEUE__

/**
 * Стандартные модули
 */
#include <cstddef>
#include <cstring>

/**
 * Наши модуля
 */
#include "../sys/log.hpp"

/**
 * Если размер буфера в байтах не определён
 */
#ifndef AWH_NETWORK_QUEUE_BUFFER_SIZE
	/**
	 * Устанавливаем размер буфера для сетевой очереди (по умолчанию 64 КБ)
	 */
	#define AWH_NETWORK_QUEUE_BUFFER_SIZE 0x10000
#endif

/**
 * Устанавливаем выравнивание на границу кэш-линии для предотвращения false sharing
 */
#define __AWH_NETWORK_QUEUE_CACHELINE_ALIGN__ alignas(64)

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс для работы с сетевыми очередями
	 *
	 */
	typedef class __AWH_NETWORK_QUEUE_CACHELINE_ALIGN__ __AWH_SHARED_EXPORT__ Network_Queue {
		public:
			/**
			 * @brief Типы сетевых очередей
			 *
			 */
			enum class type_t : uint8_t {
				TCP = 0x01, // Тип очереди для потоков данных (например, TCP)
				UDP = 0x02  // Тип очереди для границ сообщений (например, UDP)
			};
		private:
			// Тип очереди
			type_t _type;
		private:
			// Позиция чтения (начало первой записи)
			size_t _read;
			// Позиция записи (конец последней записи)
			size_t _write;
			// Кэшированный размер полезных данных (без метаданных)
			size_t _total;
			// Количество записей в очереди
			size_t _count;
		private:
			// Буфер для хранения данных очереди
			uint8_t _buffer[AWH_NETWORK_QUEUE_BUFFER_SIZE];
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект для работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Метод сдвига всех данных к началу буфера при фрагментации
			 *
			 */
			void compact() noexcept;
		private:
			/**
			 * @brief Метод быстрого получения размера записи (без проверок - вызывается только для валидных позиций)
			 *
			 * @return размер данных в очереди
			 */
			size_t recordSize(const size_t pos) const noexcept;
			/**
			 * @brief Метод установки размера записи (прямой доступ)
			 *
			 * @param pos  позиция записи для обновления размера
			 * @param size новый размер данных в очереди
			 */
			void recordSize(const size_t pos, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод очистки очереди от всех данных
			 *
			 */
			void clear() noexcept;
		public:
			/**
			 * @brief Метод получения типа сетевой очереди
			 *
			 * @return тип сетевой очереди
			 */
			type_t type() const noexcept;
			/**
			 * @brief Метод Установки типа сетевой очереди
			 *
			 */
			void type(const type_t type) noexcept;
		public:
			/**
			 * @brief Метод получения общего размера полезных данных в очереди (без учёта метаданных)
			 *
			 * @return размер данных в очереди
			 */
			size_t size() const noexcept;
		public:
			/**
			 * @brief Метод получения количества записей в очереди
			 *
			 * @return количество записей в очереди
			 */
			size_t count() const noexcept;
		public:
			/**
			 * @brief Метод определения доступного пространства для новых данных (в байтах полезной нагрузки)
			 *
			 * @return доступное пространство для новых данных в очереди
			 */
			size_t available() const noexcept;
		public:
			/**
			 * @brief Метод проверки на пустоту очереди
			 *
			 * @return результат проверки на пустоту очереди
			 */
			bool empty() const noexcept;
		public:
			/**
			 * @brief Метод удаления верхней записи из очереди
			 *
			 * @param size размер данных для удаления из очереди
			 * @return     результат удаления верхней записи из очереди (true при успехе, false если очередь пуста)
			 */
			bool pop(const size_t size = 0) noexcept;
		public:
			/**
			 * @brief Метод добавления данных в очередь
			 *
			 * @param data данные для добавления в очередь
			 * @param size размер данных для добавления в очередь
			 * @return     количество данных, успешно добавленных в очередь (0 при неудаче, когда недостаточно места)
			 */
			size_t push(const void * data, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения данных из очереди (без удаления - для чтения)
			 *
			 * @param data данные для получения из очереди (устанавливается указатель на данные в очереди)
			 * @param size размер данных для получения из очереди
			 * @return     результат (true при успехе, false если очередь пуста)
			 */
			bool front(const void ** data, size_t & size) const noexcept;
		public:
			/**
			 * @brief Конструктор инициализации сетевой очереди
			 *
			 * @param fmk объект фреймворка для доступа к его функциям
			 * @param log объект для работы с логами
			 */
			Network_Queue(const fmk_t * fmk, const log_t * log) noexcept;
			 /**
			  * @brief Деструктор сетевой очереди
			  *
			  */
			~Network_Queue() noexcept = default;
	} net_queue_t;
};

#endif // __AWH_NETWORK_QUEUE__
