/**
 * @file: buffer.cpp
 * @date: 2025-10-26
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
 * Стандартные библиотеки
 */
#include <cstring>
#include <cstdlib>
#include <algorithm>

/**
 * Подключаем заголовочный файл
 */
#include <sys/buffer.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод контроля памяти
 *
 * @param size желаемый размер выделения памяти
 * @return     результат выполнения операции
 */
bool awh::Buffer::rss(const size_t size) noexcept {
	// Результат работы функции
	bool result = false;
	// Если размер данных передан и буфер данных не достиг предела
	if(size > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если буфер данных не пустой
			if(!this->_buffer.empty()){
				// Определяем количество свободного места в буфере
				size_t available = 0;
				// Если для записи в буфере ещё есть место
				if(this->_buffer.size() > this->_range.end)
					// Определяем количество свободного места в буфере
					available = (this->_buffer.size() - this->_range.end);
				// Если в буфере больше нет места для добавления данных
				if(!(result = (available >= size))){
					// Если в начале буфера есть освобождённое место
					if(this->_range.begin > 0){
						// Определяем размер данных в буфере для перемещения
						const size_t bytes = (this->_range.end - this->_range.begin);
						// Если нам есть чего перемещать
						if(bytes > 0)
							// Выполняем перемещение верхней границы памяти
							::memmove(&this->_buffer[0], &this->_buffer[this->_range.begin], bytes);
						// Выполняем смещение верхней границы буфера
						this->_range.begin = 0;
						// Выполняем смещение нижней границы буфера
						this->_range.end = bytes;
						// Определяем количество свободного места в буфере
						available = (this->_buffer.size() - this->_range.end);
					}
					// Если в буфере больше нет места для добавления данных
					if(!(result = (available >= size))){
						// Если при добавлении новых данных мы не переходим через лимит
						if((result = ((this->_buffer.size() + (size - available)) < this->_maxMemory)))
							// Выделяем ещё памяти
							this->_buffer.resize(this->_buffer.size() + (size - available), 0);
					}
				}
			// Если буфер данных пустой
			} else if((result = (size <= this->_maxMemory)))
				// Увеличиваем размер вектора
				this->_buffer.resize(size, 0);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект лога установлен
			if(this->_log != nullptr){
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
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод очистки всех данных буфера
 *
 */
void awh::Buffer::clear() noexcept {
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_range.end > 0)){
		// Выполняем сброс конца буфера
		this->_range.end = 0;
		// Выполняем сброс начала буфера
		this->_range.begin = 0;
		// Выполняем зануление всего буфера данных
		::memset(&this->_buffer[0], 0, this->_buffer.size());
	}
}
/**
 * @brief Метод полной очистки памяти
 *
 */
void awh::Buffer::reset() noexcept {
	// Если буфер данных не пустой
	if(!this->_buffer.empty()){
		// Выполняем очистку буфера данных
		this->clear();
		// Выполняем освобождение памяти
		vector <decltype(this->_buffer)::value_type> ().swap(this->_buffer);
	}
}
/**
 * @brief Метод проверки на заполненность буфера
 *
 * @return результат проверки
 */
bool awh::Buffer::empty() const noexcept {
	// Выводим результат проверки
	return (
		(this->_range.end == 0) ||
		(this->_range.end == this->_range.begin)
	);
}
/**
 * @brief Метод получения размера добавленных данных
 *
 * @return размер всех добавленных данных
 */
size_t awh::Buffer::size() const noexcept {
	// Выводим размер добавленных данных в буфер
	return (this->_range.end - this->_range.begin);
}
/**
 * @brief Метод вывода размера занимаемой памяти очередью
 *
 * @return количество памяти которую занимает буфер
 */
size_t awh::Buffer::capacity() const noexcept {
	// Выводим количество выделенной памяти
	return this->_buffer.capacity();
}
/**
 * @brief Метод извлечения буфера сырых данных
 *
 * @return буфер сырых данных
 */
const vector <char> & awh::Buffer::raw() const noexcept {
	// Если буфер данных не пустой
	if(!this->_buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если буфер не соответствует итераторам
			if((this->_range.begin > 0) || (this->_range.end < this->_buffer.size())){
				// Выполняем усечение буфера
				vector <decltype(this->_buffer)::value_type> (
					this->_buffer.begin() + this->_range.begin,
					this->_buffer.begin() + this->_range.end
				).swap(const_cast <buffer_t *> (this)->_buffer);
				// Выполняем сброс верхнего итератора
				const_cast <buffer_t *> (this)->_range.begin = 0;
				// Выполняем сброс нижнего итератора
				const_cast <buffer_t *> (this)->_range.end = this->_buffer.size();
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект лога установлен
			if(this->_log != nullptr){
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
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	// Если буфер пустой
	} else {
		// Выполняем сброс нижнего итератора
		const_cast <buffer_t *> (this)->_range.end = 0;
		// Выполняем сброс верхнего итератора
		const_cast <buffer_t *> (this)->_range.begin = 0;
	}
	// Выводим значение буфера как есть
	return this->_buffer;
}
/**
 * @brief Шаблон для метода получения конечного итератора
 *
 * @tparam T тип данных для подсчёта
 */
template <typename T>
/**
 * @brief Метод получения конечного итератора
 *
 * @return конечный итератор
 */
awh::Buffer::Iterator <T> awh::Buffer::end() noexcept {
	// Выполняем установку конечного значения итератора
	return Iterator <T> (reinterpret_cast <T *> (&this->_buffer[0] + this->_range.end));
}
/**
 * Объявляем прототипы для метода получения конечного итератора
 */
template awh::Buffer::Iterator <int8_t> awh::Buffer::end <int8_t> () noexcept;
template awh::Buffer::Iterator <uint8_t> awh::Buffer::end <uint8_t> () noexcept;
template awh::Buffer::Iterator <int16_t> awh::Buffer::end <int16_t> () noexcept;
template awh::Buffer::Iterator <uint16_t> awh::Buffer::end <uint16_t> () noexcept;
template awh::Buffer::Iterator <int32_t> awh::Buffer::end <int32_t> () noexcept;
template awh::Buffer::Iterator <uint32_t> awh::Buffer::end <uint32_t> () noexcept;
template awh::Buffer::Iterator <int64_t> awh::Buffer::end <int64_t> () noexcept;
template awh::Buffer::Iterator <uint64_t> awh::Buffer::end <uint64_t> () noexcept;
template awh::Buffer::Iterator <float> awh::Buffer::end <float> () noexcept;
template awh::Buffer::Iterator <double> awh::Buffer::end <double> () noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template awh::Buffer::Iterator <char> awh::Buffer::end <char> () noexcept;
#endif
/**
 * Если операционной системой является MacOS X или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template awh::Buffer::Iterator <size_t> awh::Buffer::end <size_t> () noexcept;
	template awh::Buffer::Iterator <ssize_t> awh::Buffer::end <ssize_t> () noexcept;
#endif
/**
 * @brief Шаблон для метода получение начального итератора
 *
 * @tparam T тип данных для подсчёта
 */
template <typename T>
/**
 * @brief Метод получение начального итератора
 *
 * @return начальный итератор
 */
awh::Buffer::Iterator <T> awh::Buffer::begin() noexcept {
	// Выполняем установку начального значения итератора
	return Iterator <T> (reinterpret_cast <T *> (&this->_buffer[0] + this->_range.begin));
}
/**
 * Объявляем прототипы для метода получение начального итератора
 */
template awh::Buffer::Iterator <int8_t> awh::Buffer::begin <int8_t> () noexcept;
template awh::Buffer::Iterator <uint8_t> awh::Buffer::begin <uint8_t> () noexcept;
template awh::Buffer::Iterator <int16_t> awh::Buffer::begin <int16_t> () noexcept;
template awh::Buffer::Iterator <uint16_t> awh::Buffer::begin <uint16_t> () noexcept;
template awh::Buffer::Iterator <int32_t> awh::Buffer::begin <int32_t> () noexcept;
template awh::Buffer::Iterator <uint32_t> awh::Buffer::begin <uint32_t> () noexcept;
template awh::Buffer::Iterator <int64_t> awh::Buffer::begin <int64_t> () noexcept;
template awh::Buffer::Iterator <uint64_t> awh::Buffer::begin <uint64_t> () noexcept;
template awh::Buffer::Iterator <float> awh::Buffer::begin <float> () noexcept;
template awh::Buffer::Iterator <double> awh::Buffer::begin <double> () noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template awh::Buffer::Iterator <char> awh::Buffer::begin <char> () noexcept;
#endif
/**
 * Если операционной системой является MacOS X или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template awh::Buffer::Iterator <size_t> awh::Buffer::begin <size_t> () noexcept;
	template awh::Buffer::Iterator <ssize_t> awh::Buffer::begin <ssize_t> () noexcept;
#endif
/**
 * @brief Шаблон для метода удаления верхних записей
 *
 * @tparam T тип данных для удаления
 */
template <typename T>
/**
 * @brief Метод удаления записи в буфера
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
template void awh::Buffer::pop <float> () noexcept;
template void awh::Buffer::pop <double> () noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template void awh::Buffer::pop <char> () noexcept;
#endif
/**
 * Если операционной системой является MacOS X или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template void awh::Buffer::pop <size_t> () noexcept;
	template void awh::Buffer::pop <ssize_t> () noexcept;
#endif
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
template size_t awh::Buffer::count <float> () const noexcept;
template size_t awh::Buffer::count <double> () const noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template size_t awh::Buffer::count <char> () const noexcept;
#endif
/**
 * Если операционной системой является MacOS X или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Buffer::count <size_t> () const noexcept;
	template size_t awh::Buffer::count <ssize_t> () const noexcept;
#endif
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
		// Если данных достаточно в буфере
		if(this->_range.end > size)
			// Выполняем копирование данных контейнера
			::memcpy(&result, &this->_buffer[this->_range.end - size], size);
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
template float awh::Buffer::back() const noexcept;
template double awh::Buffer::back() const noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template char awh::Buffer::back() const noexcept;
#endif
/**
 * Если операционной системой является MacOS X или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Buffer::back() const noexcept;
	template ssize_t awh::Buffer::back() const noexcept;
#endif
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
		// Если данные есть в буфере
		if((this->_range.end - this->_range.begin) >= size)
			// Выполняем копирование данных контейнера
			::memcpy(&result, &this->_buffer[this->_range.begin], size);
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
template float awh::Buffer::front() const noexcept;
template double awh::Buffer::front() const noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template char awh::Buffer::front() const noexcept;
#endif
/**
 * Если операционной системой является MacOS X или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Buffer::front() const noexcept;
	template ssize_t awh::Buffer::front() const noexcept;
#endif
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
	if(!this->empty() && (index < this->count <T> ())){
		// Получаем размер данных
		const size_t size = sizeof(result);
		// Определяем смещение в буфере
		const size_t offset = (this->_range.begin + (index * size));
		// Если в буфере данных есть данные
		if((offset + size) <= this->_range.end)
			// Выполняем копирование данных контейнера
			::memcpy(&result, &this->_buffer[offset], size);
		// Если данных нет в буфере
		else if(this->_log != nullptr) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("There is no data in the buffer at INDEX=%zu", __PRETTY_FUNCTION__, std::make_tuple(index), log_t::flag_t::WARNING, index);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("There is no data in the buffer at INDEX=%zu", log_t::flag_t::WARNING, index);
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "WARNING! Called function:\n%s\n\nMessage:\nThere is no data in the buffer at INDEX=%zu\n\n", __PRETTY_FUNCTION__, index);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "WARNING! There is no data in the buffer at INDEX=%zu\n\n", index);
			#endif
		}
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
template float awh::Buffer::at(const size_t) const noexcept;
template double awh::Buffer::at(const size_t) const noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template char awh::Buffer::at(const size_t) const noexcept;
#endif
/**
 * Если операционной системой является MacOS X или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Buffer::at(const size_t) const noexcept;
	template ssize_t awh::Buffer::at(const size_t) const noexcept;
#endif
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
	if(!this->empty() && (index < this->count <T> ())){
		// Получаем размер данных
		const size_t size = sizeof(value);
		// Определяем смещение в буфере
		const size_t offset = (this->_range.begin + (index * size));
		// Если в буфере данных есть данные
		if((offset + size) <= this->_range.end)
			// Выполняем установку значения
			::memcpy(const_cast <char *> (&this->_buffer[offset]), &value, size);
		// Если данных нет в буфере
		else if(this->_log != nullptr) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("There is no data in the buffer at INDEX=%zu", __PRETTY_FUNCTION__, std::make_tuple(value, index), log_t::flag_t::WARNING, index);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("There is no data in the buffer at INDEX=%zu", log_t::flag_t::WARNING, index);
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "WARNING! Called function:\n%s\n\nMessage:\nThere is no data in the buffer at INDEX=%zu\n\n", __PRETTY_FUNCTION__, index);
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "WARNING! There is no data in the buffer at INDEX=%zu\n\n", index);
			#endif
		}
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
template void awh::Buffer::set(const float, const size_t) noexcept;
template void awh::Buffer::set(const double, const size_t) noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template void awh::Buffer::set(const char, const size_t) noexcept;
#endif
/**
 * Если операционной системой является MacOS X или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template void awh::Buffer::set(const size_t, const size_t) noexcept;
	template void awh::Buffer::set(const ssize_t, const size_t) noexcept;
#endif
/**
 * @brief Получения данных указанного элемента в буфера
 *
 * @return указатель на элемент буфера
 */
const void * awh::Buffer::data() const noexcept {
	// Если буфер данных не пустой
	if(!this->empty())
		// Выводим текущий результат
		return (&this->_buffer[0] + this->_range.begin);
	// Выводим значение по умолчанию
	return nullptr;
}
/**
 * @brief Метод удаления указанного количества байт
 *
 * @param size количество байт для удаления
 */
void awh::Buffer::erase(const size_t size) noexcept {
	// Если буфер данных не пустой
	if(!this->empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если размер удаляемых данных не выше максимального буфера
			if((this->_range.end - this->_range.begin) >= size)
				// Выполняем удаление указанного количества данных
				this->_range.begin += size;
			// Удаляем все что есть
			else this->_range.begin = this->_range.end;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект лога установлен
			if(this->_log != nullptr){
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
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
}
/**
 * @brief Метод резервирования размера буфера
 *
 * @param size размер выделяемой памяти
 */
void awh::Buffer::reserve(const size_t size) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выделяем нужное количество памяти буферу данных
		this->_buffer.reserve(size);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
}
/**
 * @brief Шаблон для добавления числа в буфер
 *
 * @tparam T тип данных для добавления
 */
template <typename T>
/**
 * @brief Метод добавления числа в буфер
 *
 * @param value значение для добавления
 * @return       результат добавления данных
 */
bool awh::Buffer::push(const T value) noexcept {
	// Выполняем добавление числа
	return this->push(&value, sizeof(value));
}
/**
 * Объявляем прототипы для метода добавления числа в буфер
 */
template bool awh::Buffer::push(const int8_t) noexcept;
template bool awh::Buffer::push(const uint8_t) noexcept;
template bool awh::Buffer::push(const int16_t) noexcept;
template bool awh::Buffer::push(const uint16_t) noexcept;
template bool awh::Buffer::push(const int32_t) noexcept;
template bool awh::Buffer::push(const uint32_t) noexcept;
template bool awh::Buffer::push(const int64_t) noexcept;
template bool awh::Buffer::push(const uint64_t) noexcept;
template bool awh::Buffer::push(const float) noexcept;
template bool awh::Buffer::push(const double) noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template bool awh::Buffer::push(const char) noexcept;
#endif
/**
 * Если операционной системой является MacOS X или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template bool awh::Buffer::push(const size_t) noexcept;
	template bool awh::Buffer::push(const ssize_t) noexcept;
#endif
/**
 * @brief Метод добавления текста в буфер
 *
 * @param text текст для добавления
 * @return     результат добавления данных
 */
bool awh::Buffer::push(const char * text) noexcept {
	// Если текст передан не пустой
	if(text != nullptr)
		// Выполняем добавление текста
		return this->push(text, ::strlen(text));
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод добавления текста в буфер
 *
 * @param text текст для добавления
 * @return     результат добавления данных
 */
bool awh::Buffer::push(const string & text) noexcept {
	// Если текст передан не пустой
	if(!text.empty())
		// Выполняем добавление текста
		return this->push(text.c_str(), text.length());
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод добавления бинарного буфера данных в буфер
 *
 * @param buffer бинарный буфер для добавления
 * @return       результат добавления данных
 */
bool awh::Buffer::push(buffer_t && buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем перемещение буфера данных
		this->_buffer = ::move(buffer._buffer);
		// Выполняем копирование последнего итератора
		this->_range.end = buffer._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = buffer._range.begin;
		// Выполняем копирование максимального размера памяти
		this->_maxMemory = buffer._maxMemory;
		// Выполняем сброс последнего итератора стороннего буфера
		buffer._range.end = 0;
		// Выполняем сброс начального итератора стороннего буфера
		buffer._range.begin = 0;
		// Выводим положительный результат
		return true;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод добавления бинарного буфера данных в буфер
 *
 * @param buffer бинарный буфер для добавления
 * @return       результат добавления данных
 */
bool awh::Buffer::push(const buffer_t & buffer) noexcept {
	// Если буфер данных передан не пустой
	if(!buffer.empty())
		// Выполняем добавление бинарного буфера данных
		return this->push(static_cast <const char *> (buffer), static_cast <size_t> (buffer));
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод добавления бинарного буфера данных в буфер
 *
 * @param buffer бинарный буфер для добавления
 * @return       результат добавления данных
 */
bool awh::Buffer::push(const vector <char> & buffer) noexcept {
	// Если буфер данных передан не пустой
	if(!buffer.empty())
		// Выполняем добавление бинарного буфера данных
		return this->push(&buffer[0], buffer.size());
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод добавления бинарного буфера данных в буфер
 *
 * @param buffer бинарный буфер для добавления
 * @param size   размер бинарного буфера
 * @return       результат добавления данных
 */
bool awh::Buffer::push(const void * buffer, const size_t size) noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы верные
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем помещаются ли данные в буфер
			if((this->size() + size) <= this->_maxMemory){
				// Выполняем выделение памяти
				if((result = this->rss(size))){
					// Выполняем добавление самих данных полезной нагрузки
					::memcpy(&this->_buffer[this->_range.end], buffer, size);
					// Увеличиваем смещение конца данных буфера
					this->_range.end += size;
				// Выполняем сброс буфера
				} else {
					// Если объект лога установлен
					if(this->_log != nullptr){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(size), log_t::flag_t::CRITICAL, "Binary data buffer is corrupted");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, "Binary data buffer is corrupted");
						#endif
					// Если объект логирования не установлен
					} else {
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, "Binary data buffer is corrupted");
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							::fprintf(stderr, "ERROR! %s\n\n", "Binary data buffer is corrupted");
						#endif
					}
					// Выполняем сброс конца буфера
					this->_range.end = 0;
					// Выполняем сброс начала буфера
					this->_range.begin = 0;
					// Выполняем зануление всего буфера данных
					::memset(&this->_buffer[0], 0, this->_buffer.size());
				}
			// Если данные больше не помещаются в буфер
			} else {
				// Если объект лога установлен
				if(this->_log != nullptr){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"You are trying to map %s of data into a %s data buffer, which is impossible",
							__PRETTY_FUNCTION__, std::make_tuple(size), log_t::flag_t::CRITICAL,
							this->_fmk->bytes(static_cast <double> (this->size() + size)).c_str(),
							this->_fmk->bytes(static_cast <double> (this->_maxMemory)).c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print(
							"You are trying to map %s of data into a %s data buffer, which is impossible",
							log_t::flag_t::CRITICAL, this->_fmk->bytes(static_cast <double> (this->size() + size)).c_str(),
							this->_fmk->bytes(static_cast <double> (this->_maxMemory)).c_str()
						);
					#endif
				// Если объект логирования не установлен
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, "There is not enough memory in the reserved buffer to add a new portion of data");
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						::fprintf(stderr, "ERROR! %s\n\n", "There is not enough memory in the reserved buffer to add a new portion of data");
					#endif
				}
				// Выполняем сброс конца буфера
				this->_range.end = 0;
				// Выполняем сброс начала буфера
				this->_range.begin = 0;
				// Выполняем зануление всего буфера данных
				::memset(&this->_buffer[0], 0, this->_buffer.size());
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если объект лога установлен
			if(this->_log != nullptr){
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
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки максимального размера потребления памяти
 *
 * @param size максимальный размер потребления памяти
 */
void awh::Buffer::setMaxMemory(const size_t size) noexcept {
	// Если максимальный размер потребляемой памяти передан
	if(size > 0)
		// Выполняем установку максимального размера потребляемой памяти
		this->_maxMemory = size;
}
/**
 * @brief Метод обмена очередями
 *
 * @param buffer бинарный буфер для обмена
 */
void awh::Buffer::swap(Buffer & buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект фреймворка установлен
		if((buffer._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = buffer._fmk;
		// Если объект для работы с логами установлен
		if((buffer._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = buffer._log;
		// Если объект фреймворка установлен
		if((this->_fmk != nullptr) && (buffer._fmk == nullptr))
			// Копируем объект фреймворка
			buffer._fmk = this->_fmk;
		// Если объект для работы с логами установлен
		if((this->_log != nullptr) && (buffer._log == nullptr))
			// Копируем объект для работы с логами установлен
			buffer._log = this->_log;
		// Выполняем обмен буферами данных
		this->_buffer.swap(buffer._buffer);
		// Выполняем обмен последними итераторами
		this->_range.end += (buffer._range.end - (buffer._range.end = this->_range.end));
		// Выполняем обмен начальными итераторами
		this->_range.begin += (buffer._range.begin - (buffer._range.begin = this->_range.begin));
		// Выполняем обмен максимальными размерами памяти
		this->_maxMemory += (buffer._maxMemory - (buffer._maxMemory = this->_maxMemory));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
}
/**
 * @brief Метод установки объекта логирования
 *
 * @param log объект работы с логами
 */
void awh::Buffer::setLogger(const log_t * log) noexcept {
	// Выполняем установку объекта логирования
	this->_log = log;
}
/**
 * @brief Получения размера данных в буфера
 *
 * @return размер данных в буфера
 */
awh::Buffer::operator size_t() const noexcept {
	// Выводим размер буфера
	return this->size();
}
/**
 * @brief Получения бинарных данных буфера
 *
 * @return бинарные данные буфера
 */
awh::Buffer::operator const char * () const noexcept {
	// Выводим буфер данных
	return reinterpret_cast <const char *> (this->data());
}
/**
 * @brief Получения бинарных данных буфера
 *
 * @return бинарные данные буфера
 */
awh::Buffer::operator const vector <char> & () const noexcept {
	// Выводим результат
	return this->raw();
}
/**
 * @brief Оператор копирования
 *
 * @param buffer бинарный буфер для копирования
 * @return       текущий контейнер буфера
 */
awh::Buffer & awh::Buffer::operator = (const char * buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем копирование начального итератора
		this->_range.begin = 0;
		// Выполняем копирование последнего итератора
		this->_range.end = ::strlen(buffer);
		// Выделяем нужный нам размер памяти
		this->_buffer.reserve(this->_range.end);
		// Перемещаем данные (на самом деле — копируем, но эффективно)
		this->_buffer.assign(buffer, buffer + this->_range.end);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор перемещения
 *
 * @param buffer бинарный буфер для перемещения
 * @return       текущий контейнер буфера
 */
awh::Buffer & awh::Buffer::operator = (string && buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем копирование начального итератора
		this->_range.begin = 0;
		// Выполняем копирование последнего итератора
		this->_range.end = buffer.length();
		// Выделяем нужный нам размер памяти
		this->_buffer.reserve(buffer.length());
		// Перемещаем данные (на самом деле — копируем, но эффективно)
		this->_buffer.assign(buffer.begin(), buffer.end());
		// Очищаем исходную строку
		buffer.clear();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param buffer бинарный буфер для копирования
 * @return       текущий контейнер буфера
 */
awh::Buffer & awh::Buffer::operator = (const string & buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем копирование начального итератора
		this->_range.begin = 0;
		// Выполняем копирование последнего итератора
		this->_range.end = buffer.length();
		// Выделяем нужный нам размер памяти
		this->_buffer.reserve(buffer.length());
		// Копируем данные из строки в буфер
		this->_buffer.assign(buffer.begin(), buffer.end());
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор перемещения
 *
 * @param buffer бинарный буфер для перемещения
 * @return       текущий контейнер буфера
 */
awh::Buffer & awh::Buffer::operator = (vector <char> && buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем копирование начального итератора
		this->_range.begin = 0;
		// Выполняем копирование последнего итератора
		this->_range.end = buffer.size();
		// Выполняем перемещение буфера данных
		this->_buffer = ::move(buffer);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param buffer бинарный буфер для копирования
 * @return       текущий контейнер буфера
 */
awh::Buffer & awh::Buffer::operator = (const vector <char> & buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем копирование начального итератора
		this->_range.begin = 0;
		// Выполняем копирование последнего итератора
		this->_range.end = buffer.size();
		// Выполняем перемещение буфера данных
		this->_buffer.assign(buffer.begin(), buffer.end());
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор перемещения
 *
 * @param buffer бинарный буфер для перемещения
 * @return       текущий контейнер буфера
 */
awh::Buffer & awh::Buffer::operator = (buffer_t && buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект фреймворка установлен
		if((buffer._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = buffer._fmk;
		// Если объект для работы с логами установлен
		if((buffer._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = buffer._log;
		// Выполняем перемещение буфера данных
		this->_buffer = ::move(buffer._buffer);
		// Выполняем копирование последнего итератора
		this->_range.end = buffer._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = buffer._range.begin;
		// Выполняем копирование максимального размера памяти
		this->_maxMemory = buffer._maxMemory;
		// Выполняем сброс последнего итератора стороннего буфера
		buffer._range.end = 0;
		// Выполняем сброс начального итератора стороннего буфера
		buffer._range.begin = 0;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param buffer бинарный буфер для копирования
 * @return       текущий контейнер буфера
 */
awh::Buffer & awh::Buffer::operator = (const buffer_t & buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект фреймворка установлен
		if((buffer._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = buffer._fmk;
		// Если объект для работы с логами установлен
		if((buffer._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = buffer._log;
		// Выполняем копирование последнего итератора
		this->_range.end = buffer._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = buffer._range.begin;
		// Выполняем копирование максимального размера памяти
		this->_maxMemory = buffer._maxMemory;
		// Выполняем перемен буферами данных
		this->_buffer.assign(buffer._buffer.begin(), buffer._buffer.end());
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор сравнения двух очередей
 *
 * @param buffer бинарный буфер для сравнения
 * @return       результат сравнения
 */
bool awh::Buffer::operator == (const buffer_t & buffer) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Выполняем сравнения всей внутренней составляющей
		 */
		return (
			(this->_range.end == buffer._range.end) &&
			(this->_range.begin == buffer._range.begin) &&
			(this->_buffer.size() == buffer._buffer.size()) &&
			(::memcmp(&this->_buffer[0], &buffer._buffer[0], this->_buffer.size()) == 0)
		);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Выводим результат
	return false;
}
/**
 * @brief Конструктор перемещения
 *
 * @param buffer бинарный буфер для перемещения
 */
awh::Buffer::Buffer(buffer_t && buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект фреймворка установлен
		if((buffer._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = buffer._fmk;
		// Если объект для работы с логами установлен
		if((buffer._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = buffer._log;
		// Выполняем перемещение буфера данных
		this->_buffer = ::move(buffer._buffer);
		// Выполняем копирование последнего итератора
		this->_range.end = buffer._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = buffer._range.begin;
		// Выполняем копирование максимального размера памяти
		this->_maxMemory = buffer._maxMemory;
		// Выполняем сброс последнего итератора стороннего буфера
		buffer._range.end = 0;
		// Выполняем сброс начального итератора стороннего буфера
		buffer._range.begin = 0;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
}
/**
 * @brief Конструктор копирования
 *
 * @param buffer бинарный буфер для копирования
 */
awh::Buffer::Buffer(const buffer_t & buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект фреймворка установлен
		if((buffer._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = buffer._fmk;
		// Если объект для работы с логами установлен
		if((buffer._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = buffer._log;
		// Выполняем копирование последнего итератора
		this->_range.end = buffer._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = buffer._range.begin;
		// Выполняем копирование максимального размера памяти
		this->_maxMemory = buffer._maxMemory;
		// Выполняем перемен буферами данных
		this->_buffer.assign(buffer._buffer.begin(), buffer._buffer.end());
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если объект лога установлен
		if(this->_log != nullptr){
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
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Buffer::Buffer(const fmk_t * fmk, const log_t * log) noexcept :
 _maxMemory(AWH_MAX_MEMORY_BUFFER), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Buffer::~Buffer() noexcept {}
/**
 * @brief Оператор [>>] чтения из потока буфера
 *
 * @param is     поток для чтения
 * @param buffer буфер для присвоения
 */
istream & awh::operator >> (istream & is, buffer_t & buffer) noexcept {
	// Буфер данных в текстовом виде
	string text = "";
	// Считываем текстовые данные
	is >> text;
	// Если текстовые данные получены
	if(!text.empty())
		// Устанавливаем текстовые данные в буфер
		buffer.push(text.c_str(), text.length());
	// Выводим результат
	return is;
}
/**
 * @brief Оператор [<<] вывода в поток буфера
 *
 * @param os     поток куда нужно вывести данные
 * @param buffer буфер извлечения
 */
ostream & awh::operator << (ostream & os, const buffer_t & buffer) noexcept {
	// Записываем в поток версию
	os << string(static_cast <const char *> (buffer), static_cast <size_t> (buffer));
	// Выводим результат
	return os;
}
