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
 * @copyright: Copyright © 2025
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
#include <limits>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/hash.hpp>
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
		 * Максимальный размер одного буфера данных
		 */
		static constexpr size_t CHUNK_SIZE = 0x1000;
		/**
		 * Header Структура работы с заголовком буфера данных
		 */
		typedef struct AWHSHARED_EXPORT Header {
			// Идентификатор процесса
			pid_t pid;
			// Идентификатор сообщения
			uint8_t mid;
			// Размер текущего чанка
			size_t size;
			// Контрольная сумма
			uint8_t sign[3];
			// Размер шифрования
			hash_t::cipher_t cipher;
			// Метод компрессии
			hash_t::method_t method;
			/**
			 * Header Конструктор
			 */
			Header() noexcept;
		} __attribute__((packed)) header_t;
		/**
		 * Encoder Класс для работы с протоколом передачи данных
		 */
		typedef class AWHSHARED_EXPORT Encoder {
			private:
				// Мютекс для блокировки потока
				mutex _mtx;
			private:
				// Размер одного блока данных
				size_t _chunkSize;
			private:
				// Объект работы с хэшированием
				hash_t _hash;
				// Заголовок полученного сообщения
				header_t _header;
				// Объект буфера данных
				buffer_t _buffer;
			private:
				// Размер шифрования
				hash_t::cipher_t _cipher;
				// Метод компрессии
				hash_t::method_t _method;
			private:
				// Объект работы с логами
				const log_t * _log;
			private:
				/**
				 * work Метод формирования новой записи
				 * @param buffer буфер данных для добавления
				 * @param size   размер буфера данных
				 */
				void work(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * empty Метод проверки на пустоту контейнера
				 * @return результат проверки
				 */
				bool empty() const noexcept;
			public:
				/**
				 * size Метод получения размера бинарных данных буфера
				 * @return размер бинарных данных буфера
				 */
				size_t size() const noexcept;
			public:
				/**
				 * clear Метод очистки данных
				 */
				void clear() noexcept;
			public:
				/**
				 * data Метод получения бинарных данных буфера
				 * @return бинарные данные буфера
				 */
				const void * data() const noexcept;
			public:
				/**
				 * erase Метод удаления количества первых байт буфера
				 * @param size размер данных для удаления
				 */
				void erase(const size_t size) noexcept;
			public:
				/**
				 * chunkSize Метод извлечения размера установленного чанка
				 * @return размер установленного чанка
				 */
				size_t chunkSize() const noexcept;
				/**
				 * chunkSize Метод установки максимального размера одного блока
				 * @param size размер блока данных
				 */
				void chunkSize(const size_t size) noexcept;
			public:
				/**
				 * salt Метод установки соли шифрования
				 * @param salt соль для шифрования
				 */
				void salt(const string & salt) noexcept;
				/**
				 * password Метод установки пароля шифрования
				 * @param password пароль шифрования
				 */
				void password(const string & password) noexcept;
			public:
				/**
				 * cipher Метод установки размера шифрования
				 * @param cipher размер шифрования
				 */
				void cipher(const hash_t::cipher_t cipher) noexcept;
				/**
				 * method Метод установки метода компрессии
				 * @param method метод компрессии для установки
				 */
				void method(const hash_t::method_t method) noexcept;
			public:
				/**
				 * push Метод добавления новой записи в протокол
				 * @param mid    идентификатор сообщения
				 * @param buffer буфер данных для добавления
				 * @param size   размер буфера данных
				 */
				void push(const uint8_t mid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * Оператор проверки на доступность данных в контейнере
				 * @return результат проверки
				 */
				operator bool() const noexcept;
				/**
				 * Оператор определения размера бинарных данных буфера
				 * @return размер бинарных данных буфера
				 */
				operator size_t() const noexcept;
				/**
				 * Оператор получения бинарных данных буфера
				 * @return бинарные данные буфера
				 */
				operator const void * () const noexcept;
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
				 _chunkSize(CHUNK_SIZE),
				 _hash(log), _buffer(log),
				 _cipher(hash_t::cipher_t::NONE),
				 _method(hash_t::method_t::NONE), _log(log) {}
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
				/**
				 * Message Структура сообщения
				 */
				typedef struct Message {
					uint8_t mid;         // Идентификатор сообщения
					size_t size;         // Размер извлекаемого сообщения
					const char * buffer; // данные сообщения
					/**
					 * Message Конструктор
					 */
					Message() noexcept :
					 mid(0), size(0), buffer(nullptr) {}
				} __attribute__((packed)) message_t;
			private:
				// Идентификатор процесса который прислал сообщение
				pid_t _pid;
			private:
				// Объект работы с хэшированием
				hash_t _hash;
				// Очередь полученных сообщений
				queue_t _queue;
				// Заголовок полученного сообщения
				header_t _header;
				// Объект буфера данных
				buffer_t _buffer;
			private:
				// Размер одного блока данных
				size_t _chunkSize;
			private:
				// Мютекс для блокировки потока
				mutable mutex _mtx;
			private:
				// Временный буфер для вставки в очередь
				vector <queue_t::buffer_t> _tmp;
			private:
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * pop Метод удаления первой записи протокола
				 */
				void pop() noexcept;
			public:
				/**
				 * clear Метод очистки данных
				 */
				void clear() noexcept;
			public:
				/**
				 * pid Метод извлечения идентификатора процесса от которого пришло сообщение
				 * @return идентификатор процесса
				 */
				pid_t pid() const noexcept;
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
				 * get Метод получения сообщения
				 * @return объект данных сообщения
				 */
				message_t get() const noexcept;
			public:
				/**
				 * push Метод добавления новой записи в протокол
				 * @param buffer буфер данных для добавления
				 * @param size   размер буфера данных
				 */
				void push(const void * buffer, const size_t size) noexcept;
			private:
				/**
				 * process Метод извлечения данных из полученного буфера
				 * @param buffer буфер данных для препарирования
				 * @param size   размер буфера данных для препарирования
				 * @return       количество обработанных байт
				 */
				size_t process(const void * buffer, const size_t size) noexcept;
				/**
				 * prepare Метод препарирования полученных данных
				 * @param buffer буфер данных для препарирования
				 * @param size   размер буфера данных для препарирования
				 * @return       количество обработанных байт
				 */
				size_t prepare(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * chunkSize Метод извлечения размера установленного чанка
				 * @return размер установленного чанка
				 */
				size_t chunkSize() const noexcept;
				/**
				 * chunkSize Метод установки максимального размера одного блока
				 * @param size размер блока данных
				 */
				void chunkSize(const size_t size) noexcept;
			public:
				/**
				 * salt Метод установки соли шифрования
				 * @param salt соль для шифрования
				 */
				void salt(const string & salt) noexcept;
				/**
				 * password Метод установки пароля шифрования
				 * @param password пароль шифрования
				 */
				void password(const string & password) noexcept;
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
				Decoder & operator = (const size_t size) noexcept;
			public:
				/**
				 * Decoder Конструктор
				 * @param log объект для работы с логами
				 */
				Decoder(const log_t * log) noexcept :
				 _pid(0), _hash(log), _queue(log), _buffer(log),
				 _chunkSize(CHUNK_SIZE), _tmp(2), _log(log) {}
				/**
				 * ~Encoder Деструктор
				 */
				~Decoder() noexcept {}
		} decoder_t;
	};
};

#endif // __AWH_CLUSTER_MESSAGE_PROTOCOL__
