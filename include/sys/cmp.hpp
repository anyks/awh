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
#include <set>
#include <map>
#include <mutex>
#include <string>
#include <vector>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * ClusterMessageProtocol Класс для работы с протоколом передачи данных
	 */
	typedef class AWHSHARED_EXPORT ClusterMessageProtocol {
		public:
			// Устанавливаем максимальный размер одного буфера данных
			static constexpr size_t CHUNK_SIZE = 0x1000;
		public:
			/**
			 * Режим передачи буфера данных
			 */
			enum class mode_t : uint8_t {
				NONE    = 0x00, // Режим буфера данных не установлен
				END     = 0x01, // Режим буфера данных конец передачи
				BEGIN   = 0x02, // Режим буфера данных начало передачи
				CONTINE = 0x03  // Режим буфера данных продолжение передачи
			};
		public:
			/**
			 * Header Структура работы с заголовком буфера данных
			 */
			typedef struct Header {
				pid_t  pid;   // Идентификатор процесса
				mode_t mode;  // Режим работы буфера данных
				size_t size;  // Общий размер записи
				size_t bytes; // Размер текущего чанка
				size_t index; // Индекс текущей записи
				/**
				 * Header Конструктор
				 */
				Header() noexcept : pid(::getpid()), mode(mode_t::NONE), size(0), bytes(0), index(0) {}
			} __attribute__((packed)) header_t;
			/**
			 * Buffer Структура работы с буфером данных
			 */
			typedef class AWHSHARED_EXPORT Buffer {
				private:
					// Устанавливаем дружбу с родительским классом
					friend class ClusterMessageProtocol;
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
					 * @param index  индекс текущей записи
					 * @param mode   режим отправки буфера данных
					 * @param size   общий размер записи целиком
					 * @param buffer буфер данных единичного чанка
					 * @param bytes  размер буфера данных единичного чанка
					 */
					void push(const size_t index, const mode_t mode, const size_t size, const void * buffer, const size_t bytes) noexcept;
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
			// Индекс последней записи
			size_t _index;
		private:
			// Размер одного блока данных
			size_t _chunkSize;
		private:
			// Временный буфер данных
			std::multimap <size_t, std::unique_ptr <buffer_t>> _tmp;
			// Очередь данных буферов записи
			std::multimap <size_t, std::unique_ptr <buffer_t>> _data;
		private:
			// Создаём объект работы с логами
			const log_t * _log;
		public:
			/**
			 * front Метод получения первой записи протокола
			 * @return объект данных первой записи
			 */
			std::vector <char> front() const noexcept;
			/**
			 * back Метод получения последней записи протокола
			 * @return объект данных последней записи
			 */
			std::vector <char> back() const noexcept;
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
			 * erase Метод удаления записи протокола
			 * @param index индекс конкретной записи
			 */
			void erase(const size_t index) noexcept;
		public:
			/**
			 * items Метод получения списка записей
			 * @return список записей в протоколе
			 */
			std::set <size_t> items() const noexcept;
		public:
			/**
			 * pid Получение идентификатора процесса
			 * @param index индекс конкретной записи
			 * @return      идентификатор процесса
			 */
			pid_t pid(const size_t index) const noexcept;
		public:
			/**
			 * at Метод извлечения данных конкретной записи
			 * @param index индекс конкретной записи
			 * @return      буфер данных записи
			 */
			std::vector <char> at(const size_t index) const noexcept;
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
			 * append Метод добавления записи в бинарном виде
			 * @param buffer буфер данных в бинарном виде
			 * @param size   размер буфера данных в бинарном виде
			 */
			void append(const void * buffer, const size_t size) noexcept;
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
			 * Оператор извлечения данных конкретной записи
			 * @param index индекс конкретной записи
			 * @return      буфер данных записи
			 */
			std::vector <char> operator [] (const size_t index) const noexcept;
		public:
			/**
			 * Оператор [=] установки максимального размера одного блока
			 * @param size размер блока данных
			 * @return     текущий объект протокола
			 */
			ClusterMessageProtocol & operator = (const size_t size) noexcept;
		public:
			/**
			 * ClusterMessageProtocol Конструктор
			 * @param log объект для работы с логами
			 */
			ClusterMessageProtocol(const log_t * log) noexcept : _index(0), _chunkSize(CHUNK_SIZE), _log(log) {}
			/**
			 * ~ClusterMessageProtocol Деструктор
			 */
			~ClusterMessageProtocol() noexcept {}
	} cmp_t;
};

#endif // __AWH_CLUSTER_MESSAGE_PROTOCOL__
