/**
 * @file: queue.cpp
 * @date: 2024-12-17
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

// Подключаем заголовочный файл
#include <sys/queue.hpp>

/**
 * realloc Метод увеличения памяти под записи
 */
void awh::Queue::realloc() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если записи уже некуда добавлять
		if(this->_end == this->_size){
			// Если начальная позиция смещена, то нам нужно просто компенсировать разницу
			if(this->_begin > 0){
				// Если очередь уже пустая
				if(this->_begin == this->_end)
					// Просто очищаем все данные
					this->clear();
				// Если очередь не пустая
				else {
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					{
						// Выделяем новую порцию данных
						uint64_t * data = reinterpret_cast <uint64_t *> (::malloc(this->_size * sizeof(uint64_t)));
						// Заполняем нулями буфер размеро данных
						// ::memset(data, 0, this->_size * sizeof(uint64_t));
						// Копируем в новую порцию данных список указателей
						::memcpy(data, this->_data + this->_begin, (this->_size - this->_begin) * sizeof(uint64_t));
						// Выполняем удаление старого буфера данных
						::free(this->_data);
						// Присваиваем старому буферу данных новый указатель
						this->_data = data;
					}{
						// Выделяем новую порцию данных для списка размеров
						size_t * data = reinterpret_cast <size_t *> (::malloc(this->_size * sizeof(size_t)));
						// Заполняем нулями буфер размеро данных
						// ::memset(data, 0, this->_size * sizeof(size_t));
						// Копируем в новую порцию данных список указателей
						::memcpy(data, this->_sizes + this->_begin, (this->_size - this->_begin) * sizeof(size_t));
						// Выполняем удаление старого буфера данных
						::free(this->_sizes);
						// Присваиваем старому буферу данных новый указатель
						this->_sizes = data;
					}
					// Смещаем конец записей
					this->_end = (this->_size - this->_begin);
					// Смещаем начальную позицию в самое начало
					this->_begin = 0;
				}
			// Если нужно выделить новую порцию данных
			} else {
				// Выполняем блокировку потока
				const lock_guard <std::mutex> lock(this->_mtx);
				// Устанавличаем новый размер записей
				this->_size = (this->_end + this->_batch);
				{
					/*
					// Выделяем новую порцию данных
					uint64_t * data = reinterpret_cast <uint64_t *> (::malloc(this->_size * sizeof(uint64_t)));
					// Заполняем нулями буфер размеро данных
					// ::memset(data, 0, this->_size * sizeof(uint64_t));
					// Копируем в новую порцию данных список указателей
					::memcpy(data, this->_data, this->_end * sizeof(uint64_t));
					// Выполняем удаление старого буфера данных
					::free(this->_data);
					// Присваиваем старому буферу данных новый указатель
					this->_data = data;
					*/
					// Выделяем новую порцию данных	
					this->_data = reinterpret_cast <uint64_t *> (::realloc(this->_data, this->_size * sizeof(uint64_t)));
					// Заполняем нулями буфер размеро данных
					// ::memset(this->_data + this->_end, 0, this->_batch);
				}{
					/*
					// Выделяем новую порцию данных
					size_t * data = reinterpret_cast <size_t *> (::malloc(this->_size * sizeof(size_t)));
					// Заполняем нулями буфер размеро данных
					// ::memset(data, 0, this->_size * sizeof(size_t));
					// Копируем в новую порцию данных список указателей
					::memcpy(data, this->_sizes, this->_end * sizeof(size_t));
					// Выполняем удаление старого буфера данных
					::free(this->_sizes);
					// Присваиваем старому буферу данных новый указатель
					this->_sizes = data;
					*/
					// Выделяем новую порцию данных	
					this->_sizes = reinterpret_cast <size_t *> (::realloc(this->_sizes, this->_size * sizeof(size_t)));
					// Заполняем нулями буфер размеро данных
					// ::memset(this->_sizes + this->_end, 0, this->_batch);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Memory allocation error");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * clear Метод очистки всех данных очереди
 */
void awh::Queue::clear() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если данные есть в списке
		if(this->_end > 0){
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Выполняем удаление всех добавленных данных в очередь
			for(size_t i = this->_begin; i < this->_end; i++){
				// Выполняем получение указателя на данные
				uintptr_t ptr = static_cast <uintptr_t> (this->_data[i]);
				// Выполняем удаление выделенных данных
				::free(reinterpret_cast <uint8_t *> (ptr));
			}
			// Выполняем удаление блока указателей данных
			::free(this->_data);
			// Выполняем удаление блока размеров данных
			::free(this->_sizes);
			// Выполняем сброс конечной позиции очереди
			this->_end = 0;
			// Выполняем сброс начальной позиции очереди
			this->_begin = 0;
			// Выполняем сброс общего размера данных
			this->_bytes = 0;
			// Устанавливаем размер очереди
			this->_size = this->_batch;
			// Выполняем выделение памяти для буферов размеров данных
			this->_sizes = reinterpret_cast <size_t *> (::malloc(this->_size * sizeof(size_t)));
			// Выполняем выделение памяти для буферов данных
			this->_data = reinterpret_cast <uint64_t *> (::malloc(this->_size * sizeof(uint64_t)));
			// Заполняем нулями буфер размеро данных
			// ::memset(this->_sizes, 0, this->_batch * sizeof(size_t));
			// Заполняем нулями буфер памяти данных
			// ::memset(this->_data, 0, this->_batch * sizeof(uint64_t));
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Memory allocation error");
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
 * empty Метод проверки на заполненность очереди
 * @return результат проверки
 */
bool awh::Queue::empty() const noexcept {
	// Выводим проверку на пустоту очереди
	return (this->_begin == this->_end);
}
/**
 * count Количество добавленных элементов
 * @return количество добавленных элементов
 */
size_t awh::Queue::count() const noexcept {
	// Выводим количество записей в очереди
	return (!this->empty() ? (this->_end - this->_begin) : 0);
}
/**
 * reserve Метод резервирования размера очереди
 * @param count размер очереди для аллокации
 */
void awh::Queue::reserve(const size_t count) noexcept {
	{
		// Выполняем блокировку потока
		const lock_guard <std::mutex> lock(this->_mtx);
		// Если размер батча увеличен
		if(count > 0)
			// Выполняем установку нового значения батча
			this->_batch = count;
		// Выполняем установку батча по умолчанию
		else this->_batch = static_cast <size_t> (BATCH);
	}
	// Выполняем увеличения записей под данные
	this->realloc();
}
/**
 * pop Метод удаления записи в очереди
 * @param pos позиция в очереди
 */
void awh::Queue::pop(const pos_t pos) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем позицию в очереди
		switch(static_cast <uint8_t> (pos)){
			// Если позиция с конца очереди
			case static_cast <uint8_t> (pos_t::BACK): {
				// Если нам есть чего удалять
				if(this->_end > 0){
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Выполняем уменьшение общего размера данных
					this->_bytes -= this->_sizes[this->_end - 1];
					// Выполняем получение указателя на данные
					uintptr_t ptr = static_cast <uintptr_t> (this->_data[this->_end - 1]);
					// Выполняем удаление выделенных данных
					::free(reinterpret_cast <uint8_t *> (ptr));
					// Выполняем смещение нижней границы
					this->_end--;
				}
			} break;
			// Если позиция с начала очереди
			case static_cast <uint8_t> (pos_t::FRONT): {
				// Если нам есть еще куда смещать
				if(this->_begin < this->_end){
					// Выполняем блокировку потока
					const lock_guard <std::mutex> lock(this->_mtx);
					// Выполняем уменьшение общего размера данных
					this->_bytes -= this->_sizes[this->_begin];
					// Выполняем получение указателя на данные
					uintptr_t ptr = static_cast <uintptr_t> (this->_data[this->_begin]);
					// Выполняем удаление выделенных данных
					::free(reinterpret_cast <uint8_t *> (ptr));
					// Выполняем смещение первого элемента
					this->_begin++;
				}
			} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (pos)), log_t::flag_t::CRITICAL, error.what());
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
 * size Метод получения размера добавленных данных
 * @param pos позиция в очереди
 * @return    размер всех добавленных данных
 */
size_t awh::Queue::size(const pos_t pos) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем позицию в очереди
		switch(static_cast <uint8_t> (pos)){
			// Если позиция в очереди не определена
			case static_cast <uint8_t> (pos_t::NONE):
				// Выводим размер хранимых данных
				return this->_bytes;
			// Если позиция с конца очереди
			case static_cast <uint8_t> (pos_t::BACK): {
				// Если записи есть в очереди
				if(!this->empty())
					// Выводим размер последнего элемента
					return this->_sizes[this->_end - 1];
			} break;
			// Если позиция с начала очереди
			case static_cast <uint8_t> (pos_t::FRONT): {
				// Если записи есть в очереди
				if(!this->empty())
					// Выводим размер первого элемента
					return this->_sizes[this->_begin];
			} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (pos)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим пустое значение
	return 0;
}
/**
 * get Получения данных указанного элемента в очереди
 * @param pos позиция в очереди
 * @return    указатель на элемент очереди
 */
const void * awh::Queue::get(const pos_t pos) const noexcept {
	// Если записи есть в очереди
	if(!this->empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем позицию в очереди
			switch(static_cast <uint8_t> (pos)){
				// Если позиция с конца очереди
				case static_cast <uint8_t> (pos_t::BACK): {
					// Выполняем получение указателя на данные
					uintptr_t ptr = static_cast <uintptr_t> (this->_data[this->_end - 1]);
					// Выполняем извлечение запрашиваемых данных
					return reinterpret_cast <uint8_t *> (ptr);
				}
				// Если позиция с начала очереди
				case static_cast <uint8_t> (pos_t::FRONT): {
					// Выполняем получение указателя на данные
					uintptr_t ptr = static_cast <uintptr_t> (this->_data[this->_begin]);
					// Выполняем извлечение запрашиваемых данных
					return reinterpret_cast <uint8_t *> (ptr);
				}
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (pos)), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим пустое значение
	return nullptr;
}
/**
 * push Метод добавления бинарного буфера данных в очередь
 * @param buffer бинарный буфер для добавления
 * @param size   размер бинарного буфера
 */
void awh::Queue::push(const void * buffer, const size_t size) noexcept {
	// Если данные переданы правильно
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем увеличения записей под данные
			this->realloc();
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Выделяем память для добавления данных
			uint8_t * data = reinterpret_cast <uint8_t *> (::malloc(size * sizeof(uint8_t)));
			// Выполняем копирование переданных данных в выделенную память
			::memcpy(data, buffer, size);
			// Получаем адрес указателя выделенных данных
			const uint64_t ptr = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (data));
			// Выполням установку данных в очередь
			::memcpy(this->_data + this->_end, &ptr, sizeof(ptr));
			// Выполняем установку добавление размера буфера данных в очередь
			::memcpy(this->_sizes + this->_end, &size, sizeof(size));
			// Выполняем смещение в буфере
			this->_end++;
			// Увеличиваем размер добавленных данных
			this->_bytes += size;
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
	}
}
/**
 * push Метод добавления бинарного буфера данных в очередь
 * @param buffers список бинарных буферов для добавления
 * @param size    общий размер добавляемых данных
 */
void awh::Queue::push(const vector <buffer_t> & buffers, const size_t size) noexcept {
	// Если данные переданы правильно
	if(!buffers.empty() && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем увеличения записей под данные
			this->realloc();
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Выделяем память для добавления данных
			uint8_t * data = reinterpret_cast <uint8_t *> (::malloc(size * sizeof(uint8_t)));
			// Смещение в бинарном буфере
			size_t offset = 0, bytes = 0;
			// Выполняем перебор всего списка добавляемых буферов
			for(auto & buffer : buffers){
				// Увеличиваем количество переданных байт
				bytes += buffer.second;
				// Если буфер установлен правильно
				if((buffer.first != nullptr) && (buffer.second > 0) && (bytes <= size)){
					// Выполняем копирование переданных данных в выделенную память
					::memcpy(data + offset, buffer.first, buffer.second);
					// Выполняем смещение в буфере
					offset += buffer.second;
				// Если размеры оказались битыми
				} else if(bytes > size) {
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(bytes, size), log_t::flag_t::CRITICAL, "Sizes of the transferred data blocks do not match");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, "Sizes of the transferred data blocks do not match");
					#endif
					// Выполняем удаление выделенной памяти
					::free(data);
					// Выходим из функции
					return;
				}
			}
			// Если количество байт не совпадает
			if(bytes != size){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(bytes, size), log_t::flag_t::CRITICAL, "Sizes of the transferred data blocks do not match");
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Sizes of the transferred data blocks do not match");
				#endif
				// Выполняем удаление выделенной памяти
				::free(data);
				// Выходим из функции
				return;
			}
			// Получаем адрес указателя выделенных данных
			const uint64_t ptr = static_cast <uint64_t> (reinterpret_cast <uintptr_t> (data));
			// Выполням установку данных в очередь
			::memcpy(this->_data + this->_end, &ptr, sizeof(ptr));
			// Выполняем установку добавление размера буфера данных в очередь
			::memcpy(this->_sizes + this->_end, &size, sizeof(size));
			// Выполняем смещение в буфере
			this->_end++;
			// Увеличиваем размер добавленных данных
			this->_bytes += size;
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
}
/**
 * Queue Конструктор
 * @param log объект для работы с логами
 */
awh::Queue::Queue(const log_t * log) noexcept :
 _end(0), _begin(0), _size(0), _batch(BATCH), _bytes(0),
 _sizes(nullptr), _data(nullptr), _log(log) {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем размер очереди
		this->_size = this->_batch;
		// Выполняем выделение памяти для буферов размеров данных
		this->_sizes = reinterpret_cast <size_t *> (::malloc(this->_size * sizeof(size_t)));
		// Выполняем выделение памяти для буферов данных
		this->_data = reinterpret_cast <uint64_t *> (::malloc(this->_size * sizeof(uint64_t)));
		// Заполняем нулями буфер размеро данных
		// ::memset(this->_sizes, 0, this->_size * sizeof(size_t));
		// Заполняем нулями буфер памяти данных
		// ::memset(this->_data, 0, this->_size * sizeof(uint64_t));
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::bad_alloc &) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(log), log_t::flag_t::CRITICAL, "Memory allocation error");
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
		#endif
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * ~Queue Деструктор
 */
awh::Queue::~Queue() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если данные есть в списке
		if(this->_end > 0){
			// Выполняем удаление всех добавленных данных в очередь
			for(size_t i = this->_begin; i < this->_end; i++){
				// Выполняем получение указателя на данные
				uintptr_t ptr = static_cast <uintptr_t> (this->_data[i]);
				// Выполняем удаление выделенных данных
				::free(reinterpret_cast <uint8_t *> (ptr));
			}
			// Выполняем удаление блока указателей данных
			::free(this->_data);
			// Выполняем удаление блока размеров данных
			::free(this->_sizes);
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
