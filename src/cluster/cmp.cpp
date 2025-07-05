/**
 * @file: cmp.cpp
 * @date: 2024-11-03
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

/**
 * Подключаем заголовочный файл
 */
#include <cluster/cmp.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Контрольная сумма заголовка запроса
 */
static constexpr uint8_t HEADER_SIGN[3] = {'A','W','H'};
/**
 * Определяем размер заголовка
 */
static constexpr size_t HEADER_SIZE = sizeof(awh::cmp::header_t);

/**
 * Header Конструктор
 */
awh::cmp::Header::Header() noexcept :
 pid(::getpid()), mid(0), size(0),
 cipher(hash_t::cipher_t::NONE), method(hash_t::method_t::NONE) {
	// Заполняем контрольную сумму
	::memcpy(this->sign, HEADER_SIGN, sizeof(HEADER_SIGN));
}
/**
 * work Метод формирования новой записи
 * @param buffer буфер данных для добавления
 * @param size   размер буфера данных
 */
void awh::cmp::Encoder::work(const void * buffer, const size_t size) noexcept {
	// Если данные для добавления переданы
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Формируем актуальный размер данных буфера
			this->_header.size = size;
			// Выполняем добавление блока заголовка
			this->_buffer.push(&this->_header, HEADER_SIZE);
			// Если размер данных больше размера чанка
			if((HEADER_SIZE + size) > this->_chunkSize){
				// Смещение в буфере бинарных и размер одного чанка
				size_t offset = 0, length = 0;
				// Выполняем формирование буфера до тех пор пока все не добавим
				while((size - offset) > 0){
					// Если данные не помещаются в буфере
					if((HEADER_SIZE + (size - offset)) > this->_chunkSize){
						// Получаем размер одного чанка
						length = static_cast <size_t> (this->_chunkSize - HEADER_SIZE);
						// Выполняем добавление полезную нагрузку
						this->_buffer.push(reinterpret_cast <const char *> (buffer) + offset, length);
						// Увеличиваем смещение в буфере
						offset += length;
					// Если данные помещаются в буфере
					} else {
						// Получаем размер одного чанка
						length = static_cast <size_t> (size - offset);
						// Выполняем добавление полезную нагрузку
						this->_buffer.push(reinterpret_cast <const char *> (buffer) + offset, length);
						// Увеличиваем смещение в буфере
						offset += length;
					}
				}
			// Выполняем добавление полезную нагрузку
			} else this->_buffer.push(reinterpret_cast <const char *> (buffer), this->_header.size);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, "Non-existent data was sent to the encoder");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, "Non-existent data was sent to the encoder");
		#endif
	}
}
/**
 * empty Метод проверки на пустоту контейнера
 * @return результат проверки
 */
bool awh::cmp::Encoder::empty() const noexcept {
	// Выводим результат проверки
	return this->_buffer.empty();
}
/**
 * size Метод получения количества подготовленных буферов
 * @return количество подготовленных буферов
 */
size_t awh::cmp::Encoder::size() const noexcept {
	// Выводим количество записей в протоколе
	return this->_buffer.size();
}
/**
 * clear Метод очистки данных
 */
void awh::cmp::Encoder::clear() noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем очистку очереди данных
		this->_buffer.clear();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * data Метод получения бинарных данных буфера
 * @return бинарные данные буфера
 */
const void * awh::cmp::Encoder::data() const noexcept {
	// Выводим бинарные данные буфера
	return this->_buffer.get();
}
/**
 * erase Метод удаления количества первых байт буфера
 * @param size размер данных для удаления
 */
void awh::cmp::Encoder::erase(const size_t size) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем удаление данных в буфере
		this->_buffer.erase(size);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * chunkSize Метод извлечения размера установленного чанка
 * @return размер установленного чанка
 */
size_t awh::cmp::Encoder::chunkSize() const noexcept {
	// Выводим установленный размер чанка
	return this->_chunkSize;
}
/**
 * chunkSize Метод установки максимального размера одного блока
 * @param size размер блока данных
 */
void awh::cmp::Encoder::chunkSize(const size_t size) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем установку размера чанка
		this->_chunkSize = (size > 0 ? size : CHUNK_SIZE);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * salt Метод установки соли шифрования
 * @param salt соль для шифрования
 */
void awh::cmp::Encoder::salt(const string & salt) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем установку соли шифрования
	this->_hash.salt(salt);
}
/**
 * password Метод установки пароля шифрования
 * @param password пароль шифрования
 */
void awh::cmp::Encoder::password(const string & password) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем установку пароля шифрвоания
	this->_hash.password(password);
}
/**
 * cipher Метод установки размера шифрования
 * @param cipher размер шифрования
 */
void awh::cmp::Encoder::cipher(const hash_t::cipher_t cipher) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем размера шифрования
	this->_cipher = cipher;
}
/**
 * method Метод установки метода компрессии
 * @param method метод компрессии для установки
 */
void awh::cmp::Encoder::method(const hash_t::method_t method) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем установку метода компрессии
	this->_method = method;
	// Если метод установлен актуальный
	if(this->_method != hash_t::method_t::NONE)
		// Выставляем уровень компрессии
		this->_hash.level(hash_t::level_t::SPEED);
}
/**
 * push Метод добавления новой записи в протокол
 * @param mid    идентификатор сообщения
 * @param buffer буфер данных для добавления
 * @param size   размер буфера данных
 */
void awh::cmp::Encoder::push(const uint8_t mid, const void * buffer, const size_t size) noexcept {
	// Если данные для добавления переданы
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard <mutex> lock(this->_mtx);
			// Устанавливаем идентификатор сообщения
			this->_header.mid = mid;
			// Снимаем флаг зашифрованных данных
			this->_header.cipher = hash_t::cipher_t::NONE;
			// Снимаем флаг компрессии данных
			this->_header.method = hash_t::method_t::NONE;
			// Если компрессия не активированна
			if((this->_method == hash_t::method_t::NONE) || (size <= CHUNK_SIZE)){
				// Если шифрование не активированно
				if(this->_cipher == hash_t::cipher_t::NONE)
					// Выполняем добавление сообщения
					this->work(buffer, size);
				// Если шифрование активированно
				else {
					// Результирующий объект буфер данных
					vector <char> result;
					// Выполняем шифрование данных
					this->_hash.encode(reinterpret_cast <const char *> (buffer), size, this->_cipher, result);
					// Если шифрование выполнено удачно
					if(!result.empty()){
						// Устанавливаем флаг зашифрованных данных
						this->_header.cipher = this->_cipher;
						// Выполняем добавление сообщения
						this->work(result.data(), result.size());
					// Если шифрование не выполнено
					} else this->work(buffer, size);
				}
			// Если компрессия активированна
			} else {
				// Результирующий объект буфер данных
				vector <char> result;
				// Выполняем компрессию данных
				this->_hash.compress(reinterpret_cast <const char *> (buffer), size, this->_method, result);
				// Если компрессия выполнена удачно
				if(!result.empty()){
					// Устанавливаем флаг компрессии данных
					this->_header.method = this->_method;
					// Если шифрование не активированно
					if(this->_cipher == hash_t::cipher_t::NONE)
						// Выполняем добавление сообщения
						this->work(result.data(), result.size());
					// Если шифрование активированно
					else {
						// Результирующий объект буфер данных
						vector <char> buffer;
						// Выполняем шифрование данных
						this->_hash.encode(result.data(), result.size(), this->_cipher, buffer);
						// Если шифрование выполнено удачно
						if(!buffer.empty()){
							// Устанавливаем флаг зашифрованных данных
							this->_header.cipher = this->_cipher;
							// Выполняем добавление сообщения
							this->work(buffer.data(), buffer.size());
						// Если шифрование не выполнено
						} else this->work(result.data(), result.size());
					}
				// Если компрессия не выполнена
				} else {
					// Если шифрование не активированно
					if(this->_cipher == hash_t::cipher_t::NONE)
						// Выполняем добавление сообщения
						this->work(buffer, size);
					// Если шифрование активированно
					else {
						// Результирующий объект буфер данных
						vector <char> result;
						// Выполняем шифрование данных
						this->_hash.encode(reinterpret_cast <const char *> (buffer), size, this->_cipher, result);
						// Если шифрование выполнено удачно
						if(!result.empty()){
							// Устанавливаем флаг зашифрованных данных
							this->_header.cipher = this->_cipher;
							// Выполняем добавление сообщения
							this->work(result.data(), result.size());
						// Если шифрование не выполнено
						} else this->work(buffer, size);
					}
				}	
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, "Non-existent data was sent to the encoder");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, "Non-existent data was sent to the encoder");
		#endif
	}
}
/**
 * Оператор проверки на доступность данных в контейнере
 * @return результат проверки
 */
awh::cmp::Encoder::operator bool() const noexcept {
	// Выводим результат проверки
	return !this->empty();
}
/**
 * Оператор получения количества записей
 * @return количество записей в протоколе
 */
awh::cmp::Encoder::operator size_t() const noexcept {
	// Выводим количество записей в протоколе
	return this->size();
}
/**
 * Оператор получения бинарных данных буфера
 * @return бинарные данные буфера
 */
awh::cmp::Encoder::operator const void * () const noexcept {
	// Выводим бинарные данные буфера
	return this->data();
}
/**
 * Оператор [=] установки максимального размера одного блока
 * @param size размер блока данных
 * @return     текущий объект протокола
 */
awh::cmp::Encoder & awh::cmp::Encoder::operator = (const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->chunkSize(size);
	// Выводим текущий объект
	return (* this);
}
/**
 * pop Метод удаления первой записи протокола
 */
void awh::cmp::Decoder::pop() noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Если очередь записей не пустая
		if(!this->_queue.empty())
			// Выполняем удаление первой записи
			this->_queue.pop();
		// Если очередь пустая
		if(this->_queue.empty())
			// Выполняем очистку выделенной памяти
			this->_queue.reset();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * clear Метод очистки данных
 */
void awh::cmp::Decoder::clear() noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем сброс идентификатора процесса
		this->_pid = 0;
		// Выполняем очистку очереди данных
		this->_queue.reset();
		// Выполняем очистку буфера данных
		this->_buffer.clear();
		// Выполняем очистку объекта заголовка
		this->_header = header_t();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * pid Метод извлечения идентификатора процесса от которого пришло сообщение
 * @return идентификатор процесса
 */
pid_t awh::cmp::Decoder::pid() const noexcept {
	// Выводим идентификатор полученного сообщения
	return this->_pid;
}
/**
 * empty Метод проверки на пустоту контейнера
 * @return результат проверки
 */
bool awh::cmp::Decoder::empty() const noexcept {
	// Выводим результат проверки
	return this->_queue.empty();
}
/**
 * size Метод получения количества подготовленных буферов
 * @return количество подготовленных буферов
 */
size_t awh::cmp::Decoder::size() const noexcept {
	// Выводим количество записей в очереди
	return this->_queue.size();
}
/**
 * get Метод получения сообщения
 * @return объект данных сообщения
 */
awh::cmp::Decoder::message_t awh::cmp::Decoder::get() const noexcept {
	// Результат работы функции
	message_t result;
	// Если очередь не пустая
	if(!this->_queue.empty()){
		// Устанавливаем размер данных
		result.size = this->_queue.size(queue_t::pos_t::FRONT);
		// Устанавливаем адрес заднных
		result.buffer = reinterpret_cast <const char *> (this->_queue.get(queue_t::pos_t::FRONT));
		// Извлекаем идентификатор сообщения
		result.mid = static_cast <uint8_t> (result.buffer[0]);
		// Уменьшаем общий размер сообщения
		result.size -= sizeof(result.mid);
		// Выполняем смещение в буфере данных
		result.buffer += sizeof(result.mid);
	}
	// Выводим результат
	return result;
}
/**
 * push Метод добавления новой записи в протокол
 * @param buffer буфер данных для добавления
 * @param size   размер буфера данных
 */
void awh::cmp::Decoder::push(const void * buffer, const size_t size) noexcept {
	// Если данные для добавления переданы
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard <mutex> lock(this->_mtx);
			// Если данные в бинарном буфере существуют
			if(!this->_buffer.empty()){
				// Добавляем полученные данные в бинарный буфер
				this->_buffer.push(reinterpret_cast <const char *> (buffer), size);
				// Запускаем препарирование данных
				const size_t result = this->prepare(static_cast <const char *> (this->_buffer), static_cast <size_t> (this->_buffer));
				// Если количество обработанных данных больше нуля
				if(result > 0)
					// Удаляем количество обработанных байт
					this->_buffer.erase(result);
			// Если данных во временном буфере ещё нет
			} else {
				// Запускаем препарирование данных напрямую
				const size_t result = this->prepare(buffer, size);
				// Если данных из буфера обработано меньше чем передано
				if((size - result) > 0)
					// Добавляем полученные данные в бинарный буфер
					this->_buffer.push(reinterpret_cast <const char *> (buffer) + result, size - result);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, "Non-existent data was sent to the decoder");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, "Non-existent data was sent to the decoder");
		#endif
	}
}
/**
 * process Метод извлечения данных из полученного буфера
 * @param buffer буфер данных для препарирования
 * @param size   размер буфера данных для препарирования
 * @return       количество обработанных байт
 */
size_t awh::cmp::Decoder::process(const void * buffer, const size_t size) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если данные для добавления переданы
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Если заголовок пустой
			if(this->_header.size == 0){
				// Если данных достаточно для извлечения заголовка
				if(size >= HEADER_SIZE){
					// Выполняем получение режима работы буфера данных
					::memcpy(&this->_header, reinterpret_cast <const char *> (buffer), HEADER_SIZE);
					// Если подись заголовка не соответствует заявленной
					if(::memcmp(this->_header.sign, HEADER_SIGN, sizeof(HEADER_SIGN)) != 0){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, "Data buffer has been corrupted");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Data buffer has been corrupted");
						#endif
						// Очищаем все данные декодера
						this->clear();
					// Если данные получены правильные
					} else {
						// Устанавливаем идентификатор процесса
						this->_pid = this->_header.pid;
						// Выполняем смещение в буфере данных
						result = HEADER_SIZE;
					}
				}
			}
			// Если заголовок не пустой
			if(this->_header.size > 0){
				// Если данных достаточно для извлечения полезной нагрузки
				if(size >= (result + this->_header.size)){
					// Устанавливаем данные идентификатора сообщения
					this->_tmp.at(0).first = &this->_header.mid;
					// Устанавливаем размер идентификатора сообщения
					this->_tmp.at(0).second = sizeof(this->_header.mid);
					// Если необходимо выполнить дешифрование данных
					if(this->_header.cipher != hash_t::cipher_t::NONE){
						// Результирующий дешифрованный буфер
						vector <char> data;
						// Выполняем дешифрование данных
						this->_hash.decode(reinterpret_cast <const char *> (buffer) + result, this->_header.size, this->_header.cipher, data);
						// Если дешифрование выполнено успешно
						if(!data.empty()){
							// Если необходимо выполнить декомпрессию данных
							if(this->_header.method != hash_t::method_t::NONE){
								// Результирующий декомпрессионный буфер
								vector <char> result;
								// Выполняем декомпрессию сообщения
								this->_hash.decompress(data.data(), data.size(), this->_header.method, result);
								// Если декомпрессия выполнена удачно
								if(!result.empty()){
									// Устанавливаем данные сообщения
									this->_tmp.at(1).first = result.data();
									// Устанавливаем размер сообщения
									this->_tmp.at(1).second = result.size();
									// Выполняем перемещение данных в очередь
									this->_queue.push(this->_tmp, this->_tmp.at(0).second + this->_tmp.at(1).second);
								// Возвращаем данные как они есть
								} else {
									// Устанавливаем данные сообщения
									this->_tmp.at(1).first = data.data();
									// Устанавливаем размер сообщения
									this->_tmp.at(1).second = data.size();
									// Выполняем перемещение данных в очередь
									this->_queue.push(this->_tmp, this->_tmp.at(0).second + this->_tmp.at(1).second);
								}
							// Если декомпрессию выполнять не нужно
							} else {
								// Устанавливаем данные сообщения
								this->_tmp.at(1).first = data.data();
								// Устанавливаем размер сообщения
								this->_tmp.at(1).second = data.size();
								// Выполняем перемещение данных в очередь
								this->_queue.push(this->_tmp, this->_tmp.at(0).second + this->_tmp.at(1).second);
							}
						// Выводим сообщение об ошибке
						} else this->_log->print("%s", log_t::flag_t::WARNING, "Message decryption failed");
					// Если дешифровании нет необходимости
					} else {
						// Если необходимо выполнить декомпрессию данных
						if(this->_header.method != hash_t::method_t::NONE){
							// Результирующий декомпрессионный буфер
							vector <char> data;
							// Выполняем декомпрессию сообщения
							this->_hash.decompress(reinterpret_cast <const char *> (buffer) + result, this->_header.size, this->_header.method, data);
							// Если декомпрессия выполнена удачно
							if(!data.empty()){
								// Устанавливаем данные сообщения
								this->_tmp.at(1).first = data.data();
								// Устанавливаем размер сообщения
								this->_tmp.at(1).second = data.size();
								// Выполняем перемещение данных в очередь
								this->_queue.push(this->_tmp, this->_tmp.at(0).second + this->_tmp.at(1).second);
							// Возвращаем данные как они есть
							} else {
								// Устанавливаем данные сообщения
								this->_tmp.at(1).first = reinterpret_cast <const uint8_t *> (buffer) + result;
								// Устанавливаем размер сообщения
								this->_tmp.at(1).second = this->_header.size;
								// Выполняем перемещение данных в очередь
								this->_queue.push(this->_tmp, this->_tmp.at(0).second + this->_tmp.at(1).second);
							}
						// Если декомпрессию выполнять не нужно
						} else {
							// Устанавливаем данные сообщения
							this->_tmp.at(1).first = reinterpret_cast <const uint8_t *> (buffer) + result;
							// Устанавливаем размер сообщения
							this->_tmp.at(1).second = this->_header.size;
							// Выполняем перемещение данных в очередь
							this->_queue.push(this->_tmp, this->_tmp.at(0).second + this->_tmp.at(1).second);
						}
					}
					// Выполняем увеличение смещения
					result += this->_header.size;
					// Выполняем сброс объекта заголовка
					this->_header = header_t();
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, "Non-existent data was sent to the decoder");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, "Non-existent data was sent to the decoder");
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * prepare Метод препарирования полученных данных
 * @param buffer буфер данных для препарирования
 * @param size   размер буфера данных для препарирования
 * @return       количество обработанных байт
 */
size_t awh::cmp::Decoder::prepare(const void * buffer, const size_t size) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если данные для добавления переданы
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Количество извлечённых байт
			size_t bytes = 0;
			/**
			 * Нам приходится так делать, так-как рекурсивная функция забивает стэк
			 */
			do
				// Выполняем извлечение данных из полученного буфера
				result += bytes = this->process(reinterpret_cast <const uint8_t *> (buffer) + result, size - result);
			// Если в буфере есть ещё данные продолжаем дальше
			while((bytes > 0) && (size > result) && ((size - result) >= HEADER_SIZE));
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, "Non-existent data was sent to the decoder");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::WARNING, "Non-existent data was sent to the decoder");
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * chunkSize Метод извлечения размера установленного чанка
 * @return размер установленного чанка
 */
size_t awh::cmp::Decoder::chunkSize() const noexcept {
	// Выводим установленный размер чанка
	return this->_chunkSize;
}
/**
 * chunkSize Метод установки максимального размера одного блока
 * @param size размер блока данных
 */
void awh::cmp::Decoder::chunkSize(const size_t size) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем установку размера чанка
		this->_chunkSize = (size > 0 ? size : CHUNK_SIZE);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * salt Метод установки соли шифрования
 * @param salt соль для шифрования
 */
void awh::cmp::Decoder::salt(const string & salt) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем установку соли шифрования
	this->_hash.salt(salt);
}
/**
 * password Метод установки пароля шифрования
 * @param password пароль шифрования
 */
void awh::cmp::Decoder::password(const string & password) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Выполняем установку пароля шифрвоания
	this->_hash.password(password);
}
/**
 * Оператор проверки на доступность данных в контейнере
 * @return результат проверки
 */
awh::cmp::Decoder::operator bool() const noexcept {
	// Выводим результат проверки
	return !this->empty();
}
/**
 * Оператор получения количества записей
 * @return количество записей в протоколе
 */
awh::cmp::Decoder::operator size_t() const noexcept {
	// Выводим количество записей в протоколе
	return this->size();
}
/**
 * Оператор [=] установки максимального размера одного блока
 * @param size размер блока данных
 * @return     текущий объект протокола
 */
awh::cmp::Decoder & awh::cmp::Decoder::operator = (const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->chunkSize(size);
	// Выводим текущий объект
	return (* this);
}
