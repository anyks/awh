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
 * @brief Метод очистки всех данных очереди
 *
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
			vector <decltype(this->_buffer)::value_type> ().swap(this->_buffer);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
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
 * @brief Метод проверки на заполненность очереди
 *
 * @return результат проверки
 */
bool awh::Buffer::empty() const noexcept {
	// Выводим результат проверки
	return this->_buffer.empty();
}
/**
 * @brief Метод получения размера добавленных данных
 *
 * @return размер всех добавленных данных
 */
size_t awh::Buffer::size() const noexcept {
	// Выводим размер данных в буфере
	return this->_buffer.size();
}
/**
 * @brief Метод получения размера выделенной памяти
 *
 * @return размер выделенной памяти
 */
size_t awh::Buffer::capacity() const noexcept {
	// Выводим размер выделенной памяти
	return this->_buffer.capacity();
}
/**
 * @brief Шаблон для метода удаления верхних записей
 *
 * @tparam T тип данных для удаления
 */
template <typename T>
/**
 * @brief Метод удаления верхних записей
 *
 */
void awh::Buffer::pop() noexcept {
	// Если мы не дошли до конца
	if(!this->empty())
		// Выполняем удаление указанного количества байт
		this->erase(sizeof(T));
}
/**
 * Объявляем прототипы для метода удаления верхних записей
 */
template void awh::Buffer::pop <int8_t> () noexcept;
template void awh::Buffer::pop <uint8_t> () noexcept;
template void awh::Buffer::pop <int16_t> () noexcept;
template void awh::Buffer::pop <uint16_t> () noexcept;
template void awh::Buffer::pop <int32_t> () noexcept;
template void awh::Buffer::pop <uint32_t> () noexcept;
template void awh::Buffer::pop <int64_t> () noexcept;
template void awh::Buffer::pop <uint64_t> () noexcept;
template void awh::Buffer::pop <size_t> () noexcept;
template void awh::Buffer::pop <ssize_t> () noexcept;
template void awh::Buffer::pop <float> () noexcept;
template void awh::Buffer::pop <double> () noexcept;
/**
 * @brief Шаблон для метода получения количества элементов в бинарном буфере
 *
 * @tparam T тип данных для подсчёта
 */
template <typename T>
/**
 * @brief Метод получения количества элементов в бинарном буфере
 *
 * @return количество всех добавленных лементов
 */
size_t awh::Buffer::count() const noexcept {
	// Если мы не дошли до конца
	if(!this->empty())
		// Выводим размер данных в буфере
		return (this->size() / sizeof(T));
	// Выводим пустое значение
	return 0;
}
/**
 * Объявляем прототипы для метода получения количества элементов в бинарном буфере
 */
template size_t awh::Buffer::count <int8_t> () const noexcept;
template size_t awh::Buffer::count <uint8_t> () const noexcept;
template size_t awh::Buffer::count <int16_t> () const noexcept;
template size_t awh::Buffer::count <uint16_t> () const noexcept;
template size_t awh::Buffer::count <int32_t> () const noexcept;
template size_t awh::Buffer::count <uint32_t> () const noexcept;
template size_t awh::Buffer::count <int64_t> () const noexcept;
template size_t awh::Buffer::count <uint64_t> () const noexcept;
template size_t awh::Buffer::count <size_t> () const noexcept;
template size_t awh::Buffer::count <ssize_t> () const noexcept;
template size_t awh::Buffer::count <float> () const noexcept;
template size_t awh::Buffer::count <double> () const noexcept;
/**
 * @brief Шаблон для метода извлечения нижнего значения в буфере
 *
 * @tparam T тип данных для извлечения
 */
template <typename T>
/**
 * @brief Метод извлечения нижнего значения в буфере
 *
 * @return данные содержащиеся в буфере
 */
T awh::Buffer::back() const noexcept {
	// Результат работы функции
	T result = 0;
	// Если контейнер не пустой
	if(!this->empty()){
		// Получаем размер данных
		const size_t size = sizeof(result);
		// Выполняем копирование данных контейнера
		::memcpy(&result, this->get() + (this->_buffer.size() - size), size);
	}
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода извлечения нижнего значения в буфере
 */
template int8_t awh::Buffer::back() const noexcept;
template uint8_t awh::Buffer::back() const noexcept;
template int16_t awh::Buffer::back() const noexcept;
template uint16_t awh::Buffer::back() const noexcept;
template int32_t awh::Buffer::back() const noexcept;
template uint32_t awh::Buffer::back() const noexcept;
template int64_t awh::Buffer::back() const noexcept;
template uint64_t awh::Buffer::back() const noexcept;
template size_t awh::Buffer::back() const noexcept;
template ssize_t awh::Buffer::back() const noexcept;
template float awh::Buffer::back() const noexcept;
template double awh::Buffer::back() const noexcept;
/**
 * @brief Шаблон для метода извлечения верхнего значения в буфере
 *
 * @tparam T тип данных для извлечения
 */
template <typename T>
/**
 * @brief Метод извлечения верхнего значения в буфере
 *
 * @return данные содержащиеся в буфере
 */
T awh::Buffer::front() const noexcept {
	// Результат работы функции
	T result = 0;
	// Если контейнер не пустой
	if(!this->empty()){
		// Получаем размер данных
		const size_t size = sizeof(result);
		// Выполняем копирование данных контейнера
		::memcpy(&result, this->get(), size);
	}
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода извлечения верхнего значения в буфере
 */
template int8_t awh::Buffer::front() const noexcept;
template uint8_t awh::Buffer::front() const noexcept;
template int16_t awh::Buffer::front() const noexcept;
template uint16_t awh::Buffer::front() const noexcept;
template int32_t awh::Buffer::front() const noexcept;
template uint32_t awh::Buffer::front() const noexcept;
template int64_t awh::Buffer::front() const noexcept;
template uint64_t awh::Buffer::front() const noexcept;
template size_t awh::Buffer::front() const noexcept;
template ssize_t awh::Buffer::front() const noexcept;
template float awh::Buffer::front() const noexcept;
template double awh::Buffer::front() const noexcept;
/**
 * @brief Шаблон для метода извлечения содержимого контейнера по его индексу
 *
 * @tparam T тип данных для извлечения
 */
template <typename T>
/**
 * @brief Метод извлечения содержимого контейнера по его индексу
 *
 * @param index индекс массива для извлечения
 * @return      данные содержащиеся в буфере
 */
T awh::Buffer::at(const size_t index) const noexcept {
	// Результат работы функции
	T result = 0;
	// Если контейнер не пустой
	if(!this->empty() && (index < this->size())){
		// Получаем размер данных
		const size_t size = sizeof(result);
		// Выполняем копирование данных контейнера
		::memcpy(&result, this->get() + (index * size), size);
	}
	// Выводим результат
	return result;
}
/**
 * Объявляем прототипы для метода извлечения содержимого контейнера по его индексу
 */
template int8_t awh::Buffer::at(const size_t) const noexcept;
template uint8_t awh::Buffer::at(const size_t) const noexcept;
template int16_t awh::Buffer::at(const size_t) const noexcept;
template uint16_t awh::Buffer::at(const size_t) const noexcept;
template int32_t awh::Buffer::at(const size_t) const noexcept;
template uint32_t awh::Buffer::at(const size_t) const noexcept;
template int64_t awh::Buffer::at(const size_t) const noexcept;
template uint64_t awh::Buffer::at(const size_t) const noexcept;
template size_t awh::Buffer::at(const size_t) const noexcept;
template ssize_t awh::Buffer::at(const size_t) const noexcept;
template float awh::Buffer::at(const size_t) const noexcept;
template double awh::Buffer::at(const size_t) const noexcept;
/**
 * @brief Шаблон для метода установки значений в уже существующем буфере
 *
 * @tparam T тип данных для установки
 */
template <typename T>
/**
 * @brief Метод установки значений в уже существующем буфере
 *
 * @param value значение для установки
 * @param index индекс значения для установки
 */
void awh::Buffer::set(const T value, const size_t index) noexcept {
	// Если контейнер не пустой
	if(!this->empty() && (index < this->size())){
		// Получаем размер данных
		const size_t size = sizeof(value);
		// Выполняем установку значения
		::memcpy(const_cast <uint8_t *> (this->get() + (index * size)), &value, size);
	}
}
/**
 * Объявляем прототипы для метода установки значений в уже существующем буфере
 */
template void awh::Buffer::set(const int8_t, const size_t) noexcept;
template void awh::Buffer::set(const uint8_t, const size_t) noexcept;
template void awh::Buffer::set(const int16_t, const size_t) noexcept;
template void awh::Buffer::set(const uint16_t, const size_t) noexcept;
template void awh::Buffer::set(const int32_t, const size_t) noexcept;
template void awh::Buffer::set(const uint32_t, const size_t) noexcept;
template void awh::Buffer::set(const int64_t, const size_t) noexcept;
template void awh::Buffer::set(const uint64_t, const size_t) noexcept;
template void awh::Buffer::set(const size_t, const size_t) noexcept;
template void awh::Buffer::set(const ssize_t, const size_t) noexcept;
template void awh::Buffer::set(const float, const size_t) noexcept;
template void awh::Buffer::set(const double, const size_t) noexcept;
/**
 * @brief Получения данных указанного элемента в очереди
 *
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
 * @brief Метод удаления указанного количества байт
 *
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
			vector <decltype(this->_buffer)::value_type> ().swap(this->_buffer);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
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
 * @brief Метод резервирования размера очереди
 *
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
		#if DEBUG_MODE
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
 * @brief Метод добавления бинарного буфера данных в очередь
 *
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
			#if DEBUG_MODE
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
 * @brief Получения размера данных в буфере
 *
 * @return размер данных в буфере
 */
awh::Buffer::operator size_t() const noexcept {
	// Выводим размер контейнера
	return this->_buffer.size();
}
/**
 * @brief Получения бинарных данных буфера
 *
 * @return бинарные данные буфера
 */
awh::Buffer::operator const char * () const noexcept {
	// Выводим содержимое контейнера
	return reinterpret_cast <const char *> (this->_buffer.data());
}
/**
 * @brief Оператор перемещения
 *
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
			this->_buffer = ::move(buffer._buffer);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer.get(), buffer.size()), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Оператор копирования
 *
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
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer.get(), buffer.size()), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Оператор извлечения символов буфера по его индексу
 *
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
 * @brief Оператор сравнения двух буферов
 *
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
 * @brief Конструктор перемещения
 *
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
			this->_buffer = ::move(buffer._buffer);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer.get(), buffer.size()), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Конструктор копирования
 *
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
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer.get(), buffer.size()), log_t::flag_t::CRITICAL, error.what());
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
