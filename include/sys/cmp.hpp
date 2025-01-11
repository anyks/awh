/**
 * @file: cmp.hpp
 * @date: 2024-10-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2024
 */

#ifndef __AWH_CLUSTER_MESSAGE_PROTOCOL__
#define __AWH_CLUSTER_MESSAGE_PROTOCOL__

/**
 * Стандартные модули
 */
#include <map>
#include <mutex>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/queue.hpp>
#include <sys/buffer.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * cmp пространство имён Cluster Message Protocol (CMP)
	 */
	namespace cmp {
		/**
		 * Режим передачи буфера данных
		 */
		enum class mode_t : uint8_t {
			NONE    = 0x00, // Режим буфера данных не установлен
			END     = 0x01, // Режим буфера данных конец передачи
			BEGIN   = 0x02, // Режим буфера данных начало передачи
			CONTINE = 0x03  // Режим буфера данных продолжение передачи
		};
		/**
		 * Header Структура работы с заголовком буфера данных
		 */
		typedef struct Header {
			mode_t mode;    // Режим работы буфера данных
			uint32_t id;    // Идентификатор сообщения
			uint64_t size;  // Общий размер записи
			uint16_t bytes; // Размер текущего чанка
			/**
			 * Header Конструктор
			 * @param id   идентификатор записи
			 * @param size полный размер записи
			 */
			Header(const uint32_t id = 0, const uint64_t size = 0) noexcept :
			 mode(mode_t::NONE), id(id), size(size), bytes(0) {}
		} __attribute__((packed)) header_t;
		/**
		 * Устанавливаем максимальный размер одного буфера данных
		 */
		static constexpr size_t CHUNK_SIZE = 0x1000;
		/**
		 * Encoder Класс для работы с протоколом передачи данных
		 */
		typedef class AWHSHARED_EXPORT Encoder {
			private:
				// Мютекс для блокировки потока
				mutex _mtx;
			private:
				// Набор собранных данных
				queue_t _queue;
			private:
				// Количество записей
				uint32_t _count;
			private:
				// Размер одного блока данных
				size_t _chunkSize;
			private:
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * empty Метод проверки на пустоту контейнера
				 * @return результат проверки
				 */
				bool empty() const noexcept;
			public:
				/**
				 * size Метод получения количества подготовленных буферов
				 * @return количество подготовленных буферов
				 */
				size_t size() const noexcept;
			public:
				/**
				 * clear Метод очистки данных
				 */
				void clear() noexcept;
			public:
				/**
				 * get Метод получения записи протокола
				 * @return объект данных записи
				 */
				queue_t::buffer_t get() const noexcept;
			public:
				/**
				 * pop Метод удаления первой записи протокола
				 */
				void pop() noexcept;
			public:
				/**
				 * push Метод добавления новой записи в протокол
				 * @param buffer буфер данных для добавления
				 * @param size   размер буфера данных
				 */
				void push(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * chunkSize Метод установки максимального размера одного блока
				 * @param size размер блока данных
				 */
				void chunkSize(const size_t size = CHUNK_SIZE) noexcept;
			public:
				/**
				 * Оператор проверки на доступность данных в контейнере
				 * @return результат проверки
				 */
				operator bool() const noexcept;
				/**
				 * Оператор получения количества записей
				 * @return количество записей в протоколе
				 */
				operator size_t() const noexcept;
			public:
				/**
				 * Оператор [=] установки максимального размера одного блока
				 * @param size размер блока данных
				 * @return     текущий объект протокола
				 */
				Encoder & operator = (const size_t size) noexcept;
			public:
				/**
				 * Encoder Конструктор
				 * @param log объект для работы с логами
				 */
				Encoder(const log_t * log) noexcept :
				 _queue(log), _count(0), _chunkSize(CHUNK_SIZE), _log(log) {}
				/**
				 * ~Encoder Деструктор
				 */
				~Encoder() noexcept {}
		} encoder_t;
		/**
		 * Decoder Класс для работы с протоколом получения данных
		 */
		typedef class AWHSHARED_EXPORT Decoder {
			private:
				// Мютекс для блокировки потока
				mutex _mtx;
			private:
				// Набор собранных данных
				queue_t _queue;
			private:
				// Размер одного блока данных
				size_t _chunkSize;
			private:
				// Объект буфера данных
				awh::buffer_t _buffer;
			private:
				// Набор временных буферов данных
				map <uint32_t, unique_ptr <buffer_t>> _temp;
			private:
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * empty Метод проверки на пустоту контейнера
				 * @return результат проверки
				 */
				bool empty() const noexcept;
			public:
				/**
				 * size Метод получения количества подготовленных буферов
				 * @return количество подготовленных буферов
				 */
				size_t size() const noexcept;
			public:
				/**
				 * clear Метод очистки данных
				 */
				void clear() noexcept;
			public:
				/**
				 * get Метод получения записи протокола
				 * @return объект данных записи
				 */
				queue_t::buffer_t get() const noexcept;
			public:
				/**
				 * pop Метод удаления первой записи протокола
				 */
				void pop() noexcept;
			public:
				/**
				 * push Метод добавления новой записи в протокол
				 * @param buffer буфер данных для добавления
				 * @param size   размер буфера данных
				 */
				void push(const void * buffer, const size_t size) noexcept;
			private:
				/**
				 * prepare Метод препарирования полученных данных
				 * @param buffer буфер данных для препарирования
				 * @param size   размер буфера данных для препарирования
				 * @return       количество обработанных байт
				 */
				size_t prepare(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * chunkSize Метод установки максимального размера одного блока
				 * @param size размер блока данных
				 */
				void chunkSize(const size_t size = CHUNK_SIZE) noexcept;
			public:
				/**
				 * Оператор проверки на доступность данных в контейнере
				 * @return результат проверки
				 */
				operator bool() const noexcept;
				/**
				 * Оператор получения количества записей
				 * @return количество записей в протоколе
				 */
				operator size_t() const noexcept;
			public:
				/**
				 * Decoder Конструктор
				 * @param log объект для работы с логами
				 */
				Decoder(const log_t * log) noexcept :
				 _queue(log), _chunkSize(CHUNK_SIZE), _buffer(log), _log(log) {}
				/**
				 * ~Encoder Деструктор
				 */
				~Decoder() noexcept {}
		} decoder_t;
	};
};

#endif // __AWH_CLUSTER_MESSAGE_PROTOCOL__
