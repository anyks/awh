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

/**
 * Подключаем заголовочный файл
 */
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
		const lock_guard <std::mutex> lock(this->_mtx);
		// Выполняем очистку буфера данных
		this->_buffer.clear();
		// Если размер выделенной памяти выше максимального размера буфера
		if(this->_buffer.capacity() > AWH_BUFFER_SIZE)
			// Выполняем очистку временного буфера данных
			vector <uint8_t> ().swap(this->_buffer);
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
	return this->_buffer.empty();
}
/**
 * size Метод получения размера добавленных данных
 * @return размер всех добавленных данных
 */
size_t awh::Buffer::size() const noexcept {
	// Выводим размер данных в буфере
	return this->_buffer.size();
}
/**
 * capacity Метод получения размера выделенной памяти
 * @return размер выделенной памяти 
 */
size_t awh::Buffer::capacity() const noexcept {
	// Выводим размер выделенной памяти
	return this->_buffer.capacity();
}
/**
 * get Получения данных указанного элемента в очереди
 * @return указатель на элемент очереди
 */
const uint8_t * awh::Buffer::get() const noexcept {
	// Если мы не дошли до конца
	if(!this->_buffer.empty())
		// Выводим буфер данных
		return this->_buffer.data();
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
		// Выполняем блокировку потока
		const lock_guard <std::mutex> lock(this->_mtx);
		// Если мы не дошли до конца
		if(!this->_buffer.empty())
			// Выполняем удаление указанного количества байт вначале буфера
			this->_buffer.erase(this->_buffer.begin(), this->_buffer.begin() + size);
		// Если размер выделенной памяти выше максимального размера буфера
		if(this->_buffer.empty() && (this->_buffer.capacity() > AWH_BUFFER_SIZE))
			// Выполняем очистку временного буфера данных
			vector <uint8_t> ().swap(this->_buffer);
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
 * reserve Метод резервирования размера очереди
 * @param size размер выделяемой памяти
 */
void awh::Buffer::reserve(const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <std::mutex> lock(this->_mtx);
		// Выделяем нужное количество памяти буферу данных
		this->_buffer.reserve(size);
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
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Добавляем новые данные в буфер
			this->_buffer.insert(this->_buffer.end(), reinterpret_cast <const uint8_t *> (buffer), reinterpret_cast <const uint8_t *> (buffer) + size);
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
	return this->_buffer.size();
}
/**
 * operator Получения бинарных данных буфера
 * @return бинарные данные буфера
 */
awh::Buffer::operator const char * () const noexcept {
	// Выводим содержимое контейнера
	return reinterpret_cast <const char *> (this->_buffer.data());
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
			// Выполняем блокировку потока для текущего объекта
			const lock_guard <std::mutex> lock1(this->_mtx);
			// Выполняем блокировку потока для перемещаемого объекта
			const lock_guard <std::mutex> lock2(buffer._mtx);
			// Выполняем перемещения данных буфера
			this->_buffer = std::move(buffer._buffer);
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
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Выполняем перемещения данных буфера
			this->_buffer.assign(buffer._buffer.begin(), buffer._buffer.end());
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
	if(!this->_buffer.empty() && (index < this->_buffer.size()))
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
		(this->_buffer.size() == buffer._buffer.size()) &&
		(::memcmp(this->_buffer.data(), buffer._buffer.data(), this->_buffer.size()) == 0)
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
			// Выполняем блокировку потока для текущего объекта
			const lock_guard <std::mutex> lock1(this->_mtx);
			// Выполняем блокировку потока для перемещаемого объекта
			const lock_guard <std::mutex> lock2(buffer._mtx);
			// Выполняем перемещения данных буфера
			this->_buffer = std::move(buffer._buffer);
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
			// Выполняем блокировку потока
			const lock_guard <std::mutex> lock(this->_mtx);
			// Выполняем перемещения данных буфера
			this->_buffer.assign(buffer._buffer.begin(), buffer._buffer.end());
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
