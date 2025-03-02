/**
 * @file: buffer.cpp
 * @date: 2024-12-28
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

// Подключаем заголовочный файл
#include <sys/buffer.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * clear Метод очистки всех данных очереди
 */
void awh::Buffer::clear() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Память уже выделена для хранения данных
		if(this->_data != nullptr){
			// Выполняем удаление блока данных
			::free(this->_data);
			// Зануляем блок данных
			this->_data = nullptr;
		}
		// Выполняем сброс последнего элемента в очереди
		this->_end = 0;
		// Выполняем сброс первого элемента в очереди
		this->_begin = 0;
		// Выполняем сброс текущего размера очереди
		this->_size = 0;
		// Выполняем сброс всего размера добавленных данных
		this->_bytes = 0;
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
 * empty Метод проверки на заполненность очереди
 * @return результат проверки
 */
bool awh::Buffer::empty() const noexcept {
	// Выводим результат проверки
	return (this->_end <= this->_begin);
}
/**
 * size Метод получения размера добавленных данных
 * @return размер всех добавленных данных
 */
size_t awh::Buffer::size() const noexcept {
	// Если мы не дошли до конца
	if(!this->empty())
		// Выводим размер данных в буфере
		return (this->_end - this->_begin);
	// Выводим пустое значение
	return 0;
}
/**
 * capacity Метод получения размера выделенной памяти
 * @return размер выделенной памяти 
 */
size_t awh::Buffer::capacity() const noexcept {
	// Выводим размер выделенной памяти
	return this->_size;
}
/**
 * get Получения данных указанного элемента в очереди
 * @return указатель на элемент очереди
 */
const uint8_t * awh::Buffer::get() const noexcept {
	// Если мы не дошли до конца
	if(!this->empty())
		// Выводим буфер данных
		return (this->_data + this->_begin);
	// Выводим пустое значение
	return nullptr;
}
/**
 * erase Метод удаления указанного количества байт
 * @param size количество байт для удаления
 */
void awh::Buffer::erase(const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если мы не дошли до конца
		if(!this->empty()){
			// Выполняем блокировку потока
			const lock_guard <mutex> lock(this->_mtx);
			// Выполняем смещение начала чтения в буфере
			this->_begin += size;
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
 * batch Метод установки размера выделяемой памяти
 * @param batch размер выделяемой памяти
 */
void awh::Buffer::batch(const size_t batch) noexcept {
	// Выполняем блокировку потока
	const lock_guard <mutex> lock(this->_mtx);
	// Если размер батча увеличен
	if(batch > 0)
		// Выполняем установку нового значения батча
		this->_batch = batch;
	// Выполняем установку батча по умолчанию
	else this->_batch = static_cast <size_t> (BATCH);
}
/**
 * reserve Метод резервирования размера очереди
 * @param size размер выделяемой памяти
 */
void awh::Buffer::reserve(const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Если данные ещё не выделены
		if(this->_data == nullptr){
			// Выделяем память для добавления данных
			this->_data = reinterpret_cast <uint8_t *> (::malloc((size > 0 ? size : static_cast <size_t> (BATCH)) * sizeof(uint8_t)));
			// Если память не выделенна
			if(this->_data == nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::CRITICAL, "Memory allocation error");
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
		// Если размер батча стал выше чем был
		} else if((size > this->_batch) && (size > this->_size)) {
			// Выполняем увеличение размеров памяти
			this->_data = reinterpret_cast <uint8_t *> (::realloc(this->_data, size * sizeof(uint8_t)));
			// Если память не выделенна
			if(this->_data == nullptr){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::CRITICAL, "Memory allocation error");
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
		// Если размер батча увеличен
		if(size > 0)
			// Выполняем установку нового значения батча
			this->_batch = size;
		// Выполняем установку батча по умолчанию
		else this->_batch = static_cast <size_t> (BATCH);
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
 * push Метод добавления бинарного буфера данных в очередь
 * @param buffer бинарный буфер для добавления
 * @param size   размер бинарного буфера
 */
void awh::Buffer::push(const void * buffer, const size_t size) noexcept {
	// Если данные переданы правильно
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если произошён сбой параметров буфера
			if(this->_end > this->_size)
				// Выполняем очистку буфера данных
				this->clear();
			// Выполняем блокировку потока
			const lock_guard <mutex> lock(this->_mtx);
			// Если память ещё не выделена
			if(this->_data == nullptr){
				// Увеличиваем размер хранимых данных
				this->_size += size;
				// Выделяем память для добавления данных
				this->_data = reinterpret_cast <uint8_t *> (::malloc(this->_size * sizeof(uint8_t)));
				// Если память не выделенна
				if(this->_data == nullptr){
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
				// Выполняем копирование переданных данных в выделенную память
				} else ::memcpy(this->_data, buffer, size);
			// Если данные уже выделены
			} else {
				// Если размер буфера выше размера блока данных
				if(this->_size > this->_batch){
					// Получаем размер актуальных данных в буфере
					const size_t actual = (this->_end - this->_begin);
					// Если мы можем перенести данные в начало
					if((this->_size - actual) >= size){
						// Выделяем новую порцию данных
						uint8_t * data = reinterpret_cast <uint8_t *> (::malloc(this->_size * sizeof(uint8_t)));
						// Если память не выделенна
						if(data == nullptr){
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
							// Выполняем удаление старого буфера данных
							::free(this->_data);
							// Выходим из приложения
							::exit(EXIT_FAILURE);
						// Если память выделенна удачно
						} else {
							// Копируем в новую порцию данных список указателей
							::memcpy(data, this->_data + this->_begin, actual * sizeof(uint8_t));
							// Выполняем удаление старого буфера данных
							::free(this->_data);
							// Присваиваем старому буферу данных новый указатель
							this->_data = data;
							// Выполняем сброс начала расположения данных
							this->_begin = 0;
							// Выполняем сброс конца расположения данных
							this->_end = actual;
						}
					}
				}
				// Если в буфере данных достаточно памяти для добавления новых данных
				if((this->_size - this->_end) >= size)
					// Выполняем копирование переданных данных в выделенную память
					::memcpy(this->_data + this->_end, buffer, size);
				// Если в буфере данных места больше нет
				else {
					// Увеличиваем размер хранимых данных
					this->_size += size;
					// Выделяем новую порцию данных
					this->_data = reinterpret_cast <uint8_t *> (::realloc(this->_data, this->_size * sizeof(uint8_t)));
					// Если память не выделенна
					if(this->_data == nullptr){
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
					// Выполняем копирование переданных данных в выделенную память
					} else ::memcpy(this->_data + this->_end, buffer, size);
				}
			}
			// Увеличиваем общий размер данных
			this->_end += size;
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
	}
}
/**
 * operator Получения размера данных в буфере
 * @return размер данных в буфере
 */
awh::Buffer::operator size_t() const noexcept {
	// Выводим размер контейнера
	return this->size();
}
/**
 * operator Получения бинарных данных буфера
 * @return бинарные данные буфера
 */
awh::Buffer::operator const char * () const noexcept {
	// Выводим содержимое контейнера
	return reinterpret_cast <const char *> (this->get());
}
/**
 * operator Оператор перемещения
 * @param buffer буфер для перемещения
 * @return       текущий контейнер буфера
 */
awh::Buffer & awh::Buffer::operator = (buffer_t && buffer) noexcept {
	// Если данные переданы правильно
	if(!buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку буфера данных
			this->clear();
			// Выполняем установку указателя на память
			this->_data = buffer._data;
			// Выполняем установку конечной позиции в буфере
			this->_end = buffer._end;
			// Выполням установку текущего размера очереди
			this->_size = buffer._size;
			// Выполняем установку начальной позиции в буфере
			this->_begin = buffer._begin;
			// Выполняем установку максимального размера блока памяти
			this->_batch = buffer._batch;
			// Выполняем установку размера всех добавленных данных
			this->_bytes = buffer._bytes;
			// Выполняем зануление конца позиции в буфере
			buffer._end = 0;
			// Выполняем зануление текущего размера очереди
			buffer._size = 0;
			// Выполняем зануление начальной позиции в буфере
			buffer._begin = 0;
			// Выполняем зануление максимального размера блока памяти
			buffer._batch = 0;
			// Выполняем зануление размера всех добавленных данных
			buffer._bytes = 0;
			// Выполняем удаление данных в буфере
			buffer._data = nullptr;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer.get(), buffer.size()), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * operator Оператор копирования
 * @param buffer буфер для копирования
 * @return       текущий контейнер буфера
 */
awh::Buffer & awh::Buffer::operator = (const buffer_t & buffer) noexcept {
	// Если данные переданы правильно
	if(!buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку буфера данных
			this->clear();
			// Выполняем установку максимального размера блока памяти
			this->_batch = buffer._batch;
			// Выполняем добавление памяти из полученного буфера
			this->push(buffer.get(), buffer.size());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer.get(), buffer.size()), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * operator Оператор извлечения символов буфера по его индексу
 * @param index индекс буфера
 * @return      символ находящийся в буфере
 */
uint8_t awh::Buffer::operator [](const size_t index) const noexcept {
	// Если буфер не пустой и индекс существует
	if(!this->empty() && (index >= this->_begin) && (index < this->_end))
		// Выводим индекс массива
		return this->get()[index];
	// Выводим пустое значение
	return 0;
}
/**
 * operator Оператор сравнения двух буферов
 * @param buffer буфер для сравнения
 * @return       результат сравнения
 */
bool awh::Buffer::operator == (const buffer_t & buffer) const noexcept {
	// Выполняем сравнение данных
	return (
		(this->_end == buffer._end) &&
		(this->_begin == buffer._begin) &&
		(this->_size == buffer._size) &&
		(this->_batch == buffer._batch) &&
		(this->_bytes == buffer._bytes) &&
		(::memcmp(this->_data, buffer._data, this->_size) == 0)
	);
}
/**
 * Buffer Конструктор перемещения
 * @param buffer буфер данных для перемещения
 */
awh::Buffer::Buffer(buffer_t && buffer) noexcept {
	// Если данные переданы правильно
	if(!buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку буфера данных
			this->clear();
			// Выполняем установку указателя на память
			this->_data = buffer._data;
			// Выполняем установку конечной позиции в буфере
			this->_end = buffer._end;
			// Выполням установку текущего размера очереди
			this->_size = buffer._size;
			// Выполняем установку начальной позиции в буфере
			this->_begin = buffer._begin;
			// Выполняем установку максимального размера блока памяти
			this->_batch = buffer._batch;
			// Выполняем установку размера всех добавленных данных
			this->_bytes = buffer._bytes;
			// Выполняем зануление конца позиции в буфере
			buffer._end = 0;
			// Выполняем зануление текущего размера очереди
			buffer._size = 0;
			// Выполняем зануление начальной позиции в буфере
			buffer._begin = 0;
			// Выполняем зануление максимального размера блока памяти
			buffer._batch = 0;
			// Выполняем зануление размера всех добавленных данных
			buffer._bytes = 0;
			// Выполняем удаление данных в буфере
			buffer._data = nullptr;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer.get(), buffer.size()), log_t::flag_t::CRITICAL, error.what());
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
 * Buffer Конструктор копирования
 * @param buffer буфер данных для копирования
 */
awh::Buffer::Buffer(const buffer_t & buffer) noexcept {
	// Если данные переданы правильно
	if(!buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку буфера данных
			this->clear();
			// Выполняем установку максимального размера блока памяти
			this->_batch = buffer._batch;
			// Выполняем добавление памяти из полученного буфера
			this->push(buffer.get(), buffer.size());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer.get(), buffer.size()), log_t::flag_t::CRITICAL, error.what());
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
 * Buffer Конструктор
 * @param log объект для работы с логами
 */
awh::Buffer::Buffer(const log_t * log) noexcept :
 _end(0), _begin(0), _size(0), _batch(BATCH), _bytes(0), _data(nullptr), _log(log) {}
/**
 * Buffer Конструктор
 * @param batch максимальный размер выделяемой памяти
 * @param log   объект для работы с логами
 */
awh::Buffer::Buffer(const size_t batch, const log_t * log) noexcept :
 _end(0), _begin(0), _size(0), _batch(batch), _bytes(0), _data(nullptr), _log(log) {}
/**
 * ~Buffer Деструктор
 */
awh::Buffer::~Buffer() noexcept {
	// Память уже выделена для хранения данных
	if(this->_data != nullptr)
		// Выполняем удаление блока данных
		::free(this->_data);
}
