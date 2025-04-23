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
 * empty Метод проверки на пустоту контейнера
 * @return результат проверки
 */
bool awh::cmp::Encoder::empty() const noexcept {
	// Выводим результат проверки
	return this->_queue.empty();
}
/**
 * size Метод получения количества подготовленных буферов
 * @return количество подготовленных буферов
 */
size_t awh::cmp::Encoder::size() const noexcept {
	// Выводим количество записей в протоколе
	return this->_queue.size();
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
		this->_queue.clear();
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
awh::queue_t::buffer_t awh::cmp::Encoder::get() const noexcept {
	// Результат работы функции
	queue_t::buffer_t result;
	// Устанавливаем размер данных
	result.second = this->_queue.size(queue_t::pos_t::FRONT);
	// Устанавливаем адрес заднных
	result.first = reinterpret_cast <const char *> (this->_queue.get(queue_t::pos_t::FRONT));
	// Выводим результат
	return result;
}
/**
 * pop Метод удаления первой записи протокола
 */
void awh::cmp::Encoder::pop() noexcept {
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
void awh::cmp::Encoder::push(const void * buffer, const size_t size) noexcept {
	// Если данные для добавления переданы
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard <mutex> lock(this->_mtx);
			// Заголовоки блока данных
			header_t header(this->_count++, size);
			// Получаем размер заголовка
			const size_t headerSize = sizeof(header);
			// Если размер данных больше размера чанка
			if((headerSize + size) > this->_chunkSize){
				// Смещение в буфере бинарных и размер заголовка
				size_t offset = 0;
				// Выполняем формирование буфера до тех пор пока все не добавим
				while((size - offset) > 0){
					// Если данные не помещаются в буфере
					if((headerSize + (size - offset)) > this->_chunkSize){
						// Формируем актуальный размер данных буфера
						header.bytes = static_cast <uint16_t> (this->_chunkSize - headerSize);
						// Устанавливаем режим отравки буфера данных
						header.mode = (offset == 0 ? mode_t::BEGIN : mode_t::CONTINE);
						// Добавляем в буфер новую запись
						this->_queue.push(vector <queue_t::buffer_t> ({
							{&header, headerSize},
							{reinterpret_cast <const char *> (buffer) + offset, static_cast <size_t> (header.bytes)}
						}), static_cast <size_t> (header.bytes) + headerSize);
					// Если данные помещаются в буфере
					} else {
						// Устанавливаем режим отравки буфера данных
						header.mode = mode_t::END;
						// Формируем актуальный размер данных буфера
						header.bytes = static_cast <uint16_t> (size - offset);
						// Добавляем в буфер новую запись
						this->_queue.push(vector <queue_t::buffer_t> ({
							{&header, headerSize},
							{reinterpret_cast <const char *> (buffer) + offset, static_cast <size_t> (header.bytes)}
						}), static_cast <size_t> (header.bytes) + headerSize);
					}
					// Увеличиваем смещение в буфере
					offset += static_cast <size_t> (header.bytes);
				}
			// Если размер данных помещается в буфер
			} else {
				// Устанавливаем режим отравки буфера данных
				header.mode = mode_t::END;
				// Формируем актуальный размер данных буфера
				header.bytes = static_cast <uint16_t> (size);
				// Добавляем в буфер новую запись
				this->_queue.push(vector <queue_t::buffer_t> ({
					{&header, headerSize},
					{reinterpret_cast <const char *> (buffer), static_cast <size_t> (header.bytes)}
				}), static_cast <size_t> (header.bytes) + headerSize);
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
		this->_temp.clear();
		// Выполняем очистку очереди данных
		this->_queue.clear();
		// Выполняем очистку буфера данных
		this->_buffer.clear();
		// Очищаем выделенную память для временных данных
		map <uint32_t, unique_ptr <buffer_t>> ().swap(this->_temp);
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
awh::queue_t::buffer_t awh::cmp::Decoder::get() const noexcept {
	// Результат работы функции
	queue_t::buffer_t result;
	// Устанавливаем размер данных
	result.second = this->_queue.size(queue_t::pos_t::FRONT);
	// Устанавливаем адрес заднных
	result.first = reinterpret_cast <const char *> (this->_queue.get(queue_t::pos_t::FRONT));
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
			// Получаем размер заголовка
			const size_t headerSize = sizeof(header_t);
			// Если данных достаточно для извлечения заголовка
			if(size >= headerSize){
				// Создаём объект заголовка
				header_t header{};
				// Выполняем получение режима работы буфера данных
				::memcpy(&header, reinterpret_cast <const char *> (buffer), headerSize);
				// Если общий размер блока слишком большой
				if((header.size == 0) || (static_cast <size_t> (header.bytes) > (this->_chunkSize - headerSize))){
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
				// Продолжаем дальнейшую работу
				} else {
					// Если данных достаточно для извлечения полезной нагрузки
					if(size >= (headerSize + static_cast <size_t> (header.bytes))){
						// Выполняем смещение в буфере данных
						result += headerSize;
						// Выполняем поиск указанной записи во временном объекте
						auto i = this->_temp.find(static_cast <uint32_t> (header.id));
						// Если запись найдена в временном блоке данных
						if(i != this->_temp.end()){
							// Если размер полезной нагрузки установлен
							if(header.bytes > 0)
								// Выполняем копирование данных полезной нагрузки
								i->second->push(reinterpret_cast <const uint8_t *> (buffer) + result, static_cast <size_t> (header.bytes));
							// Если запись мы получили последнюю
							if(header.mode == mode_t::END){
								// Выполняем перемещение данных в очередь
								this->_queue.push(i->second->get(), i->second->size());
								// Выполняем удаление данных из временного контейнера
								this->_temp.erase(i);
							}
						// Если запись не найдена во временном блоке данных
						} else {
							// Если запись мы получили последнюю
							if(header.mode == mode_t::END)
								// Выполняем перемещение данных в очередь
								this->_queue.push(reinterpret_cast <const uint8_t *> (buffer) + result, static_cast <size_t> (header.size));
							// Если мы получили одну из записей
							else {
								// Выполняем добавление записи во временный объект
								auto ret = this->_temp.emplace(static_cast <uint32_t> (header.id), unique_ptr <buffer_t> (new buffer_t(this->_log)));
								// Выделяем достаточно данных для формирования объекта
								ret.first->second->reserve(static_cast <size_t> (header.size));
								// Если размер полезной нагрузки установлен
								if(header.bytes > 0)
									// Выполняем копирование данных полезной нагрузки
									ret.first->second->push(reinterpret_cast <const uint8_t *> (buffer) + result, static_cast <size_t> (header.bytes));
							}
						}
						// Выполняем увеличение смещения
						result += static_cast <size_t> (header.bytes);
						// Если мы извлекли не все данные из буфера
						if((size > result) && ((size - result) >= headerSize))
							// Выполняем извлечение слещующей порции данных
							result += this->prepare(reinterpret_cast <const char *> (buffer) + result, size - result);
					}
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
