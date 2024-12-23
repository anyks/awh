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
 * data данные буфера в бинарном виде
 * @return буфер в бинарном виде
 */
vector <char> awh::cmp::Encoder::Buffer::data() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Выполняем добавление данных заголовка в итоговый буфер
	result.insert(result.end(), reinterpret_cast <const char *> (&this->_header), reinterpret_cast <const char *> (&this->_header) + sizeof(this->_header));
	// Если буфер полезной нагрузки не пустой
	if((this->_payload != nullptr) && (this->_header.bytes > 0))
		// Выполняем вставку данных полезной нагрузки
		result.insert(result.end(), this->_payload.get(), this->_payload.get() + this->_header.bytes);
	// Выводим результат
	return result;
}
/**
 * Оператор извлечения бинарного буфера в бинарном виде
 * @return бинарный буфер в бинарном виде
 */
awh::cmp::Encoder::Buffer::operator vector <char> () const noexcept {
	// Выполняем формирование общего буфера данных
	return this->data();
}
/**
 * push Метод добавления в буфер записи данных для отправки
 * @param id     идентификатор сообщения
 * @param mode   режим отправки буфера данных
 * @param size   общий размер записи целиком
 * @param buffer буфер данных единичного чанка
 * @param bytes  размер буфера данных единичного чанка
 */
void awh::cmp::Encoder::Buffer::push(const uint64_t id, const mode_t mode, const size_t size, const void * buffer, const size_t bytes) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Устанавливаем идентификатор сообщения
		this->_header.id = id;
		// Устанавливаем режим отравки буфера данных
		this->_header.mode = mode;
		// Выполняем установку общего размера записи
		this->_header.size = size;
		// Устанавливаем актуальный размер данных
		this->_header.bytes = bytes;
		// Если данные переданы верные
		if((buffer != nullptr) && (bytes > 0)){
			// Выполняем формирование буфера данных
			this->_payload = std::unique_ptr <uint8_t []> (new uint8_t [bytes]);
			// Выполняем копирование буфера полученных данных
			::memcpy(this->_payload.get(), buffer, bytes);
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
			::fprintf(stderr, "Called function:\n%s\n\nMessage:\n%s\n", __PRETTY_FUNCTION__, "Memory allocation error");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			::fprintf(stderr, "%s\n", "Memory allocation error");
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * back Метод получения последней записи протокола
 * @return объект данных последней записи
 */
vector <char> awh::cmp::Encoder::back() const noexcept {
	// Если записи в протоколе существуют
	if(!this->_data.empty())
		// Выводим запрошенный буфер данных
		return this->_data.back()->data();
	// Выводим результат
	return vector <char> ();
}
/**
 * front Метод получения первой записи протокола
 * @return объект данных первой записи
 */
vector <char> awh::cmp::Encoder::front() const noexcept {
	// Если записи в протоколе существуют
	if(!this->_data.empty())
		// Выводим запрошенный буфер данных
		return this->_data.front()->data();
	// Выводим результат
	return vector <char> ();
}
/**
 * empty Метод проверки на пустоту контейнера
 * @return результат проверки
 */
bool awh::cmp::Encoder::empty() const noexcept {
	// Выводим результат проверки
	return this->_data.empty();
}
/**
 * size Метод получения количества подготовленных буферов
 * @return количество подготовленных буферов
 */
size_t awh::cmp::Encoder::size() const noexcept {
	// Выводим количество записей в протоколе
	return this->_data.size();
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
		// Выполняем удаление всех данных
		this->_data.clear();
		// Очищаем выделенную память для записей
		std::deque <std::unique_ptr <buffer_t>> ().swap(this->_data);
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
		if(!this->_data.empty())
			// Выполняем удаление первой записи
			this->_data.pop_front();
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
			// Получаем размер заголовка
			const size_t headerSize = sizeof(header_t);
			// Если размер данных больше размера чанка
			if((headerSize + size) > this->_chunkSize){
				// Параметры обхода буфера данных
				size_t actual = 0, offset = 0;
				// Получаем индекс новой записи
				const uint64_t id = this->_count++;
				// Выполняем формирование буфера до тех пор пока все не добавим
				while((size - offset) > 0){
					// Выполняем создание буфера данных
					std::unique_ptr <buffer_t> data = std::unique_ptr <buffer_t> (new buffer_t);
					// Если данные не помещаются в буфере
					if((headerSize + (size - offset)) > this->_chunkSize){
						// Формируем актуальный размер данных буфера
						actual = (this->_chunkSize - headerSize);
						// Добавляем в буфер новую запись
						data->push(id, (offset == 0 ? mode_t::BEGIN : mode_t::CONTINE), size, reinterpret_cast <const char *> (buffer) + offset, actual);
					// Если данные помещаются в буфере
					} else {
						// Формируем актуальный размер данных буфера
						actual = (size - offset);
						// Добавляем в буфер новую запись
						data->push(id, mode_t::END, size, reinterpret_cast <const char *> (buffer) + offset, actual);
					}
					// Увеличиваем смещение в буфере
					offset += actual;
					// Выполняем добавление буфера данных в список
					this->_data.push_back(std::move(data));
				}
			// Если размер данных помещается в буфер
			} else {
				// Выполняем создание буфера данных
				std::unique_ptr <buffer_t> data = std::unique_ptr <buffer_t> (new buffer_t);
				// Добавляем в буфер данных наши записи
				data->push(this->_count++, mode_t::END, size, buffer, size);
				// Выполняем добавление буфера данных в список
				this->_data.push_back(std::move(data));
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
 * back Метод получения последней записи протокола
 * @return объект данных последней записи
 */
vector <char> awh::cmp::Decoder::back() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если записи в протоколе существуют
	if(!this->_data.empty() && (this->_data.size(queue_t::pos_t::BACK) > 0)){
		// Выделяем память под указанный буфер данных
		result.resize(this->_data.size(queue_t::pos_t::BACK), 0);
		// Выполняем копирование буфера данных
		::memcpy(result.data(), reinterpret_cast <const char *> (this->_data.get(queue_t::pos_t::BACK)), this->_data.size(queue_t::pos_t::BACK));
	}
	// Выводим результат
	return result;
}
/**
 * front Метод получения первой записи протокола
 * @return объект данных первой записи
 */
vector <char> awh::cmp::Decoder::front() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если записи в протоколе существуют
	if(!this->_data.empty() && (this->_data.size(queue_t::pos_t::FRONT) > 0)){
		// Выделяем память под указанный буфер данных
		result.resize(this->_data.size(queue_t::pos_t::FRONT), 0);
		// Выполняем копирование буфера данных
		::memcpy(result.data(), reinterpret_cast <const char *> (this->_data.get(queue_t::pos_t::FRONT)), this->_data.size(queue_t::pos_t::FRONT));
	}
	// Выводим результат
	return result;
}
/**
 * empty Метод проверки на пустоту контейнера
 * @return результат проверки
 */
bool awh::cmp::Decoder::empty() const noexcept {
	// Выводим результат проверки
	return this->_data.empty();
}
/**
 * size Метод получения количества подготовленных буферов
 * @return количество подготовленных буферов
 */
size_t awh::cmp::Decoder::size() const noexcept {
	// Выводим количество записей в протоколе
	return this->_data.size();
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
		this->_data.clear();
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
		if(!this->_data.empty())
			// Выполняем удаление первой записи
			this->_data.pop();
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
				this->_buffer.emplace(reinterpret_cast <const char *> (buffer), size);
				// Запускаем препарирование данных
				const size_t result = this->prepare(static_cast <awh::buffer_t::data_t> (this->_buffer), static_cast <size_t> (this->_buffer));
				// Если количество обработанных данных больше нуля
				if(result > 0){
					// Удаляем количество обработанных байт
					this->_buffer.erase(result);
					// Фиксируем изменение в буфере
					this->_buffer.commit();
				}
			// Если данных во временном буфере ещё нет
			} else {
				// Запускаем препарирование данных напрямую
				const size_t result = this->prepare(buffer, size);
				// Если данных из буфера обработано меньше чем передано
				if((size - result) > 0)
					// Добавляем полученные данные в бинарный буфер
					this->_buffer.emplace(reinterpret_cast <const char *> (buffer) + result, size - result);
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
								::memcpy(i->second->payload.get() + i->second->offset, reinterpret_cast <const uint8_t *> (buffer) + result, header.bytes);
							// Устанавливаем смещение в временном буфере данных
							i->second->offset += header.bytes;
							// Если запись мы получили последнюю
							if(header.mode == mode_t::END){
								// Если данные мы собрали правильно
								if(i->second->size == i->second->offset)
									// Выполняем перемещение данных в очередь
									this->_data.push(i->second->payload.get(), i->second->size);
								// Выводим сообщение об ошибке
								else this->_log->print("CMP Decoder: [SIZE=%zu, MAX_SIZE=%zu] %s", log_t::flag_t::CRITICAL, i->second->offset, i->second->size, "we received damage during the data collection process");
								// Выполняем удаление данных из временного контейнера
								this->_tmp.erase(i);
							}
						// Если запись не найдена во временном блоке данных
						} else {
							// Выполняем создание нового буфера данных
							std::unique_ptr <buffer_t> data = std::unique_ptr <buffer_t> (new buffer_t);
							// Выполняем установку размера буфера данных полезной нагрузки
							data->size = header.size;
							// Устанавливаем смещение в временном буфере данных
							data->offset = header.bytes;
							// Если размер полезной нагрузки установлен
							if(data->offset > 0){
								// Выделяем память для полезной нагрузки временного буфера данных
								data->payload = std::unique_ptr <uint8_t []> (new uint8_t [data->size]);
								// Выполняем копирование данных полезной нагрузки
								::memcpy(data->payload.get(), reinterpret_cast <const uint8_t *> (buffer) + result, data->offset);
							}
							// Если запись мы получили последнюю
							if(header.mode == mode_t::END){
								// Если данные мы собрали правильно
								if(data->size == data->offset)
									// Выполняем перемещение данных в очередь
									this->_data.push(data->payload.get(), data->size);
								// Выводим сообщение об ошибке
								else this->_log->print("CMP Decoder: [SIZE=%zu, MAX_SIZE=%zu] %s", log_t::flag_t::CRITICAL, data->offset, data->size, "we received damage during the data process");
							// Выполняем добавление записи во временный объект
							} else this->_tmp.emplace(id, std::move(data));
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
