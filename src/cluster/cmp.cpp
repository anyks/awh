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
 * Определяем размер заголовка
 */
static const size_t HEADER_SIZE = sizeof(awh::cmp::header_t);

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
			// Если число достигло предела
			if(this->_count == numeric_limits <decltype(this->_count)>::max())
				// Выполняем сброс счётчика
				this->_count = 0;
			// Заголовоки блока данных
			header_t header(this->_count++);
			// Устанавливаем идентификатор сообщения
			header.mid = mid;
			// Если размер данных больше размера чанка
			if((HEADER_SIZE + size) > this->_chunkSize){
				// Смещение в буфере бинарных и размер заголовка
				size_t offset = 0;
				// Выполняем формирование буфера до тех пор пока все не добавим
				while((size - offset) > 0){
					// Если данные не помещаются в буфере
					if((HEADER_SIZE + (size - offset)) > this->_chunkSize){
						// Устанавливаем режим отравки буфера данных
						header.mode = mode_t::CONTINE;
						// Формируем актуальный размер данных буфера
						header.bytes = static_cast <uint16_t> (this->_chunkSize - HEADER_SIZE);
					// Если данные помещаются в буфере
					} else {
						// Устанавливаем режим отравки буфера данных
						header.mode = mode_t::END;
						// Формируем актуальный размер данных буфера
						header.bytes = static_cast <uint16_t> (size - offset);
					}
					// Выполняем добавление блока заголовка
					this->_buffer.push(&header, HEADER_SIZE);
					// Выполняем добавление полезную нагрузку
					this->_buffer.push(reinterpret_cast <const char *> (buffer) + offset, static_cast <size_t> (header.bytes));
					// Увеличиваем смещение в буфере
					offset += static_cast <size_t> (header.bytes);
				}
			// Если размер данных помещается в буфер
			} else {
				// Устанавливаем режим отравки буфера данных
				header.mode = mode_t::END;
				// Формируем актуальный размер данных буфера
				header.bytes = static_cast <uint16_t> (size);
				// Выполняем добавление блока заголовка
				this->_buffer.push(&header, HEADER_SIZE);
				// Выполняем добавление полезную нагрузку
				this->_buffer.push(reinterpret_cast <const char *> (buffer), static_cast <size_t> (header.bytes));
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
	// Выводим количество записей в протоколе
	return this->_queue.size();
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
		// Выполняем удаление всех временных данных
		this->_cache.clear();
		// Выполняем очистку очереди данных
		this->_queue.reset();
		// Выполняем очистку буфера данных
		this->_buffer.clear();
		// Выполняем очистку объекта заголовка
		this->_header = header_t();
		// Очищаем выделенную память для временных данных
		map <uint32_t, unique_ptr <buffer_t>> ().swap(this->_cache);
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
 * get Метод получения записи протокола
 * @return объект данных записи
 */
awh::cmp::Decoder::record_t awh::cmp::Decoder::get() const noexcept {
	// Результат работы функции
	record_t result;
	// Устанавливаем размер данных
	result.size = this->_queue.size(queue_t::pos_t::FRONT);
	// Устанавливаем адрес заднных
	result.data = reinterpret_cast <const char *> (this->_queue.get(queue_t::pos_t::FRONT));
	// Извлекаем идентификатор сообщения
	result.mid = static_cast <uint8_t> (result.data[0]);
	// Извлекаем данные идентификатора процесса
	::memcpy(&result.pid, result.data + sizeof(result.mid), sizeof(result.pid));
	// Уменьшаем общий размер сообщения
	result.size -= (sizeof(result.mid) + sizeof(result.pid));
	// Выполняем смещение в буфере данных
	result.data += (sizeof(result.mid) + sizeof(result.pid));
	// Выводим результат
	return result;
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
		// Если список записей не пустой
		if(!this->_queue.empty())
			// Выполняем удаление первой записи
			this->_queue.pop();
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
			// Если заголовок пустой
			if(this->_header.mode == mode_t::NONE){
				// Если данных достаточно для извлечения заголовка
				if(size >= HEADER_SIZE){
					// Выполняем получение режима работы буфера данных
					::memcpy(&this->_header, reinterpret_cast <const char *> (buffer), HEADER_SIZE);
					// Если общий размер блока слишком большой
					if(this->_header.crc != HEADER_CRC){
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
					// Выполняем смещение в буфере данных
					} else result = HEADER_SIZE;
				}
			}
			// Если заголовок не пустой
			if(this->_header.mode != mode_t::NONE){
				// Если данных достаточно для извлечения полезной нагрузки
				if(size >= (result + static_cast <size_t> (this->_header.bytes))){
					// Выполняем поиск указанной записи во временном объекте
					auto i = this->_cache.find(static_cast <uint32_t> (this->_header.id));
					// Если запись найдена в временном блоке данных
					if(i != this->_cache.end()){
						// Если размер полезной нагрузки установлен
						if(this->_header.bytes > 0)
							// Выполняем копирование данных полезной нагрузки
							i->second->push(reinterpret_cast <const uint8_t *> (buffer) + result, static_cast <size_t> (this->_header.bytes));
						// Если запись мы получили последнюю
						if(this->_header.mode == mode_t::END){
							// Устанавливаем данные идентификатора сообщения
							this->_arbitrary.at(0).first = &this->_header.mid;
							// Устанавливаем размер идентификатора сообщения
							this->_arbitrary.at(0).second = sizeof(this->_header.mid);
							// Устанавливаем данные идентификатора процесса
							this->_arbitrary.at(1).first = &this->_header.pid;
							// Устанавливаем размер идентификатора процесса
							this->_arbitrary.at(1).second = sizeof(this->_header.pid);
							// Устанавливаем данные сообщения
							this->_arbitrary.at(2).first = i->second->get();
							// Устанавливаем размер сообщения
							this->_arbitrary.at(2).second = i->second->size();
							// Выполняем перемещение данных в очередь
							this->_queue.push(
								this->_arbitrary,
								this->_arbitrary.at(0).second +
								this->_arbitrary.at(1).second +
								this->_arbitrary.at(2).second
							);
							// Выполняем удаление данных из временного контейнера
							this->_cache.erase(i);
						}
					// Если запись не найдена во временном блоке данных
					} else {
						// Если запись мы получили последнюю
						if(this->_header.mode == mode_t::END){
							// Устанавливаем данные идентификатора сообщения
							this->_arbitrary.at(0).first = &this->_header.mid;
							// Устанавливаем размер идентификатора сообщения
							this->_arbitrary.at(0).second = sizeof(this->_header.mid);
							// Устанавливаем данные идентификатора процесса
							this->_arbitrary.at(1).first = &this->_header.pid;
							// Устанавливаем размер идентификатора процесса
							this->_arbitrary.at(1).second = sizeof(this->_header.pid);
							// Устанавливаем данные сообщения
							this->_arbitrary.at(2).first = (reinterpret_cast <const uint8_t *> (buffer) + result);
							// Устанавливаем размер сообщения
							this->_arbitrary.at(2).second = static_cast <size_t> (this->_header.bytes);
							// Выполняем перемещение данных в очередь
							this->_queue.push(
								this->_arbitrary,
								this->_arbitrary.at(0).second +
								this->_arbitrary.at(1).second +
								this->_arbitrary.at(2).second
							);
						// Если мы получили одну из записей
						} else {
							// Выполняем добавление записи во временный объект
							auto ret = this->_cache.emplace(static_cast <uint32_t> (this->_header.id), make_unique <buffer_t> (this->_log));
							// Выделяем достаточно данных для формирования объекта
							ret.first->second->reserve(static_cast <size_t> (this->_header.bytes));
							// Если размер полезной нагрузки установлен
							if(this->_header.bytes > 0)
								// Выполняем копирование данных полезной нагрузки
								ret.first->second->push(reinterpret_cast <const uint8_t *> (buffer) + result, static_cast <size_t> (this->_header.bytes));
						}
					}
					// Выполняем увеличение смещения
					result += static_cast <size_t> (this->_header.bytes);
					// Выполняем сброс объекта заголовка
					this->_header = header_t();
					// Если мы извлекли не все данные из буфера
					if((size > result) && ((size - result) >= HEADER_SIZE))
						// Выполняем извлечение слещующей порции данных
						result += this->prepare(reinterpret_cast <const char *> (buffer) + result, size - result);
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
