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
#include <queue>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/buffer.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * cmp пространство имён Cluster Message Protocol (CMP)
	 */
	namespace cmp {
		/**
		 * Encoder Класс для работы с протоколом передачи данных
		 */
		typedef class AWHSHARED_EXPORT Encoder {
			public:
				// Устанавливаем максимальный размер одного буфера данных
				static constexpr size_t CHUNK_SIZE = 0x1000;
			private:
				/**
				 * Режим передачи буфера данных
				 */
				enum class mode_t : uint8_t {
					NONE    = 0x00, // Режим буфера данных не установлен
					END     = 0x01, // Режим буфера данных конец передачи
					BEGIN   = 0x02, // Режим буфера данных начало передачи
					CONTINE = 0x03  // Режим буфера данных продолжение передачи
				};
			private:
				/**
				 * Header Структура работы с заголовком буфера данных
				 */
				typedef struct Header {
					pid_t pid;    // Идентификатор процесса
					uint64_t id;  // Идентификатор сообщения
					mode_t mode;  // Режим работы буфера данных
					size_t size;  // Общий размер записи
					size_t bytes; // Размер текущего чанка
					size_t index; // Номер одного чанка
					/**
					 * Header Конструктор
					 */
					Header() noexcept : pid(::getpid()), id(0), mode(mode_t::NONE), size(0), bytes(0), index(0) {}
				} __attribute__((packed)) header_t;
				/**
				 * Buffer Структура работы с буфером данных
				 */
				typedef class AWHSHARED_EXPORT Buffer {
					private:
						// Устанавливаем дружбу с родительским классом
						friend class Encoder;
					private:
						// Заголовок буфера данных
						header_t _header;
						// Данные полезной нагрузки
						std::unique_ptr <uint8_t []> _payload;
					public:
						/**
						 * data данные буфера в бинарном виде
						 * @return буфер в бинарном виде
						 */
						std::vector <char> data() const noexcept;
					public:
						/**
						 * Оператор извлечения бинарного буфера в бинарном виде
						 * @return бинарный буфер в бинарном виде
						 */
						operator std::vector <char> () const noexcept;
					public:
						/**
						 * push Метод добавления в буфер записи данных для отправки
						 * @param id     идентификатор сообщения
						 * @param index  индекс текущей записи
						 * @param mode   режим отправки буфера данных
						 * @param size   общий размер записи целиком
						 * @param buffer буфер данных единичного чанка
						 * @param bytes  размер буфера данных единичного чанка
						 */
						void push(const uint64_t id, const size_t index, const mode_t mode, const size_t size, const void * buffer, const size_t bytes) noexcept;
					public:
						/**
						 * Buffer Конструктор
						 */
						Buffer() noexcept : _payload(nullptr) {}
						/**
						 * ~Buffer Деструктор
						 */
						~Buffer() noexcept {}
				} buffer_t;
			private:
				// Мютекс для блокировки потока
				mutex _mtx;
			private:
				// Количество записей
				uint64_t _count;
			private:
				// Размер одного блока данных
				size_t _chunkSize;
			private:
				// Очередь данных буферов записи
				std::deque <std::unique_ptr <buffer_t>> _data;
			private:
				// Создаём объект работы с логами
				const log_t * _log;
			public:
				/**
				 * back Метод получения последней записи протокола
				 * @return объект данных последней записи
				 */
				std::vector <char> back() const noexcept;
				/**
				 * front Метод получения первой записи протокола
				 * @return объект данных первой записи
				 */
				std::vector <char> front() const noexcept;
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
				Encoder(const log_t * log) noexcept : _count(0), _chunkSize(CHUNK_SIZE), _log(log) {}
				/**
				 * ~Encoder Деструктор
				 */
				~Encoder() noexcept {}
		} encoder_t;
		/**
		 * Decoder Класс для работы с протоколом получения данных
		 */
		typedef class AWHSHARED_EXPORT Decoder {
			public:
				// Устанавливаем максимальный размер одного буфера данных
				static constexpr size_t CHUNK_SIZE = 0x1000;
			private:
				/**
				 * Режим передачи буфера данных
				 */
				enum class mode_t : uint8_t {
					NONE    = 0x00, // Режим буфера данных не установлен
					END     = 0x01, // Режим буфера данных конец передачи
					BEGIN   = 0x02, // Режим буфера данных начало передачи
					CONTINE = 0x03  // Режим буфера данных продолжение передачи
				};
			private:
				/**
				 * Buffer Структура работы с буферами данных
				 */
				typedef struct Buffer {
					// Общий размер записи
					size_t size;
					// Смещение в бинарном буфере
					size_t offset;
					// Данные полезной нагрузки
					std::unique_ptr <uint8_t []> payload;
					/**
					 * Buffer Конструктор
					 */
					Buffer() noexcept : size(0), offset(0), payload(nullptr) {}
				} buffer_t;
				/**
				 * Header Структура работы с заголовком буфера данных
				 */
				typedef struct Header {
					pid_t pid;    // Идентификатор процесса
					uint64_t id;  // Идентификатор сообщения
					mode_t mode;  // Режим работы буфера данных
					size_t size;  // Общий размер записи
					size_t bytes; // Размер текущего чанка
					size_t index; // Номер одного чанка
					/**
					 * Header Конструктор
					 */
					Header() noexcept : pid(0), id(0), mode(mode_t::NONE), size(0), bytes(0), index(0) {}
				} __attribute__((packed)) header_t;
			
			private:

				vector <char> _bb;
			
			private:
				// Мютекс для блокировки потока
				mutex _mtx;
			private:
				// Размер одного блока данных
				size_t _chunkSize;
			private:
				// Объект буфера данных
				awh::buffer_t _buffer;
			private:
				// Набор временных буферов данных
				std::map <uint64_t, std::unique_ptr <buffer_t>> _tmp;
				// Набор собранных данных
				std::queue <std::pair <size_t, std::unique_ptr <uint8_t []>>> _data;
			private:
				// Создаём объект работы с логами
				const log_t * _log;
			public:
				/**
				 * back Метод получения последней записи протокола
				 * @return объект данных последней записи
				 */
				std::vector <char> back() const noexcept;
				/**
				 * front Метод получения первой записи протокола
				 * @return объект данных первой записи
				 */
				std::vector <char> front() const noexcept;
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
				 _chunkSize(CHUNK_SIZE), _buffer(awh::buffer_t::mode_t::COPY), _log(log) {}
				/**
				 * ~Encoder Деструктор
				 */
				~Decoder() noexcept {}
		} decoder_t;
	};
};

#endif // __AWH_CLUSTER_MESSAGE_PROTOCOL__
