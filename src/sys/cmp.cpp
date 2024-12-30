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
 * @copyright: Copyright © 2024
 */

/**
 * Подключаем заголовочный файл
 */
#include <sys/cmp.hpp>

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
		const lock_guard <std::mutex> lock(this->_mtx);
		// Выполняем очистку очереди данных
		this->_queue.clear();
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
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
		const lock_guard <std::mutex> lock(this->_mtx);
		// Если список записей не пустой
		if(!this->_queue.empty())
			// Выполняем удаление первой записи
			this->_queue.pop();
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
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
			const lock_guard <std::mutex> lock(this->_mtx);
			// Заголовоки блока данных
			header_t header;
			// Выполняем установку общего размера записи
			header.size = size;
			// Устанавливаем идентификатор сообщения
			header.id = this->_count++;
			// Получаем размер заголовка
			const size_t headerSize = sizeof(header);
			// Если размер данных больше размера чанка
			if((headerSize + size) > this->_chunkSize){
				// Смещение в буфере бинарных и размер заголовка
				size_t offset = 0;
				// Выполняем формирование буфера до тех пор пока все не добавим
				while((size - offset) > 0){
					// Если данные не помещаются в буфере
					if((headerSize + (header.size - offset)) > this->_chunkSize){
						// Формируем актуальный размер данных буфера
						header.bytes = (this->_chunkSize - headerSize);
						// Устанавливаем режим отравки буфера данных
						header.mode = (offset == 0 ? mode_t::BEGIN : mode_t::CONTINE);
						// Добавляем в буфер новую запись
						this->_queue.push(vector <queue_t::buffer_t> ({
							{&header, headerSize},
							{reinterpret_cast <const char *> (buffer) + offset, static_cast <size_t> (header.bytes)}
						}), header.bytes + headerSize);
					// Если данные помещаются в буфере
					} else {
						// Устанавливаем режим отравки буфера данных
						header.mode = mode_t::END;
						// Формируем актуальный размер данных буфера
						header.bytes = (header.size - offset);
						// Добавляем в буфер новую запись
						this->_queue.push(vector <queue_t::buffer_t> ({
							{&header, headerSize},
							{reinterpret_cast <const char *> (buffer) + offset, static_cast <size_t> (header.bytes)}
						}), header.bytes + headerSize);
					}
					// Увеличиваем смещение в буфере
					offset += header.bytes;
				}
			// Если размер данных помещается в буфер
			} else {
				// Формируем актуальный размер данных буфера
				header.bytes = size;
				// Устанавливаем режим отравки буфера данных
				header.mode = mode_t::END;
				// Добавляем в буфер новую запись
				this->_queue.push(vector <queue_t::buffer_t> ({
					{&header, headerSize},
					{reinterpret_cast <const char *> (buffer), static_cast <size_t> (header.bytes)}
				}), header.bytes + headerSize);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("CMP Encoder: %s", log_t::flag_t::WARNING, "non-existent data was sent to the encoder");
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
		const lock_guard <std::mutex> lock(this->_mtx);
		// Выполняем установку размера чанка
		this->_chunkSize = (size > 0 ? size : CHUNK_SIZE);
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(size), log_t::flag_t::CRITICAL, error.what());
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
		const lock_guard <std::mutex> lock(this->_mtx);
		// Выполняем удаление всех временных данных
		this->_tmp.clear();
		// Выполняем очистку очереди данных
		this->_queue.clear();
		// Выполняем очистку буфера данных
		this->_buffer.clear();
		// Очищаем выделенную память для временных данных
		std::map <uint64_t, std::unique_ptr <buffer_t>> ().swap(this->_tmp);
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
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
		const lock_guard <std::mutex> lock(this->_mtx);
		// Если список записей не пустой
		if(!this->_queue.empty())
			// Выполняем удаление первой записи
			this->_queue.pop();
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
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
			const lock_guard <std::mutex> lock(this->_mtx);
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
		} catch(const std::exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("CMP Decoder: %s", log_t::flag_t::WARNING, "non-existent data was sent to the decoder");
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
			const size_t length = sizeof(header_t);
			// Если данных достаточно для извлечения заголовка
			if(size >= length){
				// Создаём объект заголовка
				header_t header{};
				// Выполняем получение режима работы буфера данных
				::memcpy(&header, reinterpret_cast <const char *> (buffer), sizeof(header));
				// Если общий размер блока слишком большой
				if(header.bytes > (this->_chunkSize - length)){
					// Выводим в лог сообщение
					this->_log->print("CMP Decoder: %s", log_t::flag_t::CRITICAL, "data buffer has been corrupted");
					// Очищаем все данные декодера
					this->clear();
				// Продолжаем дальнейшую работу
				} else {
					// Если данных достаточно для извлечения полезной нагрузки
					if(size >= (length + header.bytes)){
						// Выполняем смещение в буфере данных
						result += length;
						// Получаем индекс текущей записи
						const uint64_t id = header.id;
						// Выполняем поиск указанной записи во временном объекте
						auto i = this->_tmp.find(id);
						// Если запись найдена в временном блоке данных
						if(i != this->_tmp.end()){
							// Если размер полезной нагрузки установлен
							if(header.bytes > 0)
								// Выполняем копирование данных полезной нагрузки
								i->second->push(reinterpret_cast <const uint8_t *> (buffer) + result, header.bytes);
							// Если запись мы получили последнюю
							if(header.mode == mode_t::END){
								// Выполняем перемещение данных в очередь
								this->_queue.push(i->second->get(), i->second->size());
								// Выполняем удаление данных из временного контейнера
								this->_tmp.erase(i);
							}
						// Если запись не найдена во временном блоке данных
						} else {
							// Если запись мы получили последнюю
							if(header.mode == mode_t::END)
								// Выполняем перемещение данных в очередь
								this->_queue.push(reinterpret_cast <const uint8_t *> (buffer) + result, header.size);
							// Если мы получили одну из записей
							else {
								// Выполняем добавление записи во временный объект
								auto ret = this->_tmp.emplace(id, std::unique_ptr <buffer_t> (new buffer_t(this->_log)));
								// Выделяем достаточно данных для формирования объекта
								ret.first->second->reserve(header.size);
								// Если размер полезной нагрузки установлен
								if(header.bytes > 0)
									// Выполняем копирование данных полезной нагрузки
									ret.first->second->push(reinterpret_cast <const uint8_t *> (buffer) + result, header.bytes);
							}
						}
						// Выполняем увеличение смещения
						result += header.bytes;
						// Если мы извлекли не все данные из буфера
						if((size > result) && ((size - result) >= length))
							// Выполняем извлечение слещующей порции данных
							result += this->prepare(reinterpret_cast <const char *> (buffer) + result, size - result);
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, "Memory allocation error");
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
		} catch(const std::exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	// Выводим сообщение об ошибке
	} else this->_log->print("CMP Decoder: %s", log_t::flag_t::WARNING, "non-existent data was sent to the decoder");
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
		const lock_guard <std::mutex> lock(this->_mtx);
		// Выполняем установку размера чанка
		this->_chunkSize = (size > 0 ? size : CHUNK_SIZE);
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(size), log_t::flag_t::CRITICAL, error.what());
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
