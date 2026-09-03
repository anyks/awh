/**
 * @file queue.cpp
 * @date 2025-10-26
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация бинарной очереди — последовательное добавление, чтение и извлечение записей произвольного размера
 *        в непрерывной памяти с контролем лимитов и обходом итераторами
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <algorithm>

/**
 * Подключаем заголовочный файл проекта
 */
#include <sys/lib.hpp>
#include <sys/global.hpp>
#include <container/queue.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 */
awh::Queue::Range::Range() noexcept :
 end(0), begin(0), count(0), offset(0) {}

/**
 * @brief Конструктор
 *
 */
awh::Queue::Max::Max() noexcept :
 memory(AWH_MAX_MEMORY_QUEUE),
 records(AWH_MAX_RECORDS_QUEUE) {}

/**
 * @brief Метод контроля памяти
 *
 * @note Метод является внутренним и вызывается под уже захваченной блокировкой
 *
 * @param size желаемый размер выделения памяти
 * @return     результат выполнения операции
 *
 */
bool awh::Queue::rss(const size_t size) noexcept {
	// Переменная результата
	bool result = false;
	// Если размер данных передан и буфер данных не достиг предела
	if(size > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем общий размер добавляемых данных
			const size_t bytes = (size + sizeof(size_t));
			// Если буфер данных не пустой
			if(!this->_buffer.empty()){
				// Определяем количество свободного места в буфере
				size_t available = 0;
				// Если для записи в буфере ещё есть место
				if(this->_buffer.size() > this->_range.end)
					// Определяем количество свободного места в буфере
					available = (this->_buffer.size() - this->_range.end);
				// Если в буфере больше нет места для добавления данных
				if(!(result = (available >= bytes))){
					// Если в начале буфера есть освобождённое место
					if(this->_range.begin > 0){
						// Определяем размер данных в буфере для перемещения
						const size_t length = (this->_range.end - this->_range.begin);
						// Если нам есть чего перемещать
						if(length > 0)
							// Выполняем перемещение верхней границы памяти
							::memmove(&this->_buffer[0], &this->_buffer[this->_range.begin], length);
						// Выполняем смещение верхней границы буфера
						this->_range.begin = 0;
						// Выполняем смещение нижней границы буфера
						this->_range.end = length;
						// Определяем количество свободного места в буфере
						available = (this->_buffer.size() - this->_range.end);
					}
					// Если в буфере больше нет места для добавления данных
					if(!(result = (available >= bytes))){
						// Если при добавлении новых данных мы не переходим через лимит
						if((result = ((this->_buffer.size() + (bytes - available)) <= this->_max.memory)))
							// Выделяем ещё памяти
							this->_buffer.resize(this->_buffer.size() + (bytes - available), 0);
						// Если памяти для добавления данных недостаточно
						else {
							// Формируем сообщение об ошибке
							const string message = (this->_fmk != nullptr) ?
								("You are trying to map " + this->_fmk->bytes(static_cast <double> (this->_buffer.size() + (bytes - available))) +
								 " of data into a " + this->_fmk->bytes(static_cast <double> (this->_max.memory)) +
								 " data buffer, which is impossible") :
								"There is not enough memory in the reserved queue to add a new portion of data";
							// Записываем ошибку в лог
							this->error(__PRETTY_FUNCTION__, size, message.c_str());
						}
					}
				}
			// Если буфер данных пустой и его размер не превышает лимит
			} else if((result = (bytes <= this->_max.memory)))
				// Увеличиваем размер вектора
				this->_buffer.resize(bytes, 0);
			// Если памяти для добавления данных недостаточно
			else {
				// Формируем сообщение об ошибке
				const string message = (this->_fmk != nullptr) ?
					("You are trying to map " + this->_fmk->bytes(static_cast <double> (bytes)) +
					 " of data into a " + this->_fmk->bytes(static_cast <double> (this->_max.memory)) +
					 " data buffer, which is impossible") :
					"There is not enough memory in the reserved queue to add a new portion of data";
				// Записываем ошибку в лог
				this->error(__PRETTY_FUNCTION__, size, message.c_str());
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Записываем ошибку в лог
			this->error(__PRETTY_FUNCTION__, size, error.what());
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод вывода сообщения об ошибке
 *
 * @param func    название функции, в которой произошла ошибка
 * @param size    размер данных, связанный с ошибкой
 * @param message текст сообщения об ошибке
 *
 */
void awh::Queue::error([[maybe_unused]] const char * func, [[maybe_unused]] const size_t size, const char * message) const noexcept {
	// Если объект лога установлен
	if(this->_log != nullptr){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", func, make_tuple(size), log_t::flag_t::CRITICAL, message);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, message);
		#endif
	// Если объект логирования не установлен
	} else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", func, message);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			::fprintf(stderr, "ERROR! %s\n\n", message);
		#endif
	}
}
/**
 * @brief Метод удаления записи в очереди
 *
 */
void awh::Queue::pop() noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_range.count > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Размер верхней записи в очереди
			size_t size = 0;
			// Сбрасываем смещение чтения данных
			this->_range.offset = 0;
			// Извлекаем текущее значение размера записи
			::memcpy(&size, &this->_buffer[this->_range.begin], sizeof(size));
			// Если размер записи получен
			if(size > 0){
				// Уменьшаем количество записей в очереди
				this->_range.count--;
				// Увеличиваем смещение
				this->_range.begin += (size + sizeof(size));
				// Если мы извлекли все данные очереди
				if((this->_range.count == 0) || (this->_range.begin >= this->_range.end)){
					// Выполняем сброс конца очереди
					this->_range.end = 0;
					// Выполняем сброс начала очереди
					this->_range.begin = 0;
					// Выполняем сброс количества записей в очереди
					this->_range.count = 0;
				}
			// Записываем ошибку в лог
			} else this->error(__PRETTY_FUNCTION__, 0, "Queue data buffer is corrupted");
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Записываем ошибку в лог
			this->error(__PRETTY_FUNCTION__, 0, error.what());
		}
	}
}
/**
 * @brief Метод очистки всех данных очереди
 *
 */
void awh::Queue::clear() noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Выполняем сброс конца очереди
	this->_range.end = 0;
	// Выполняем сброс начала очереди
	this->_range.begin = 0;
	// Выполняем сброс количества записей в очереди
	this->_range.count = 0;
	// Сбрасываем смещение чтения данных
	this->_range.offset = 0;
}
/**
 * @brief Метод полной очистки памяти
 *
 */
void awh::Queue::reset() noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Если буфер данных не пустой
	if(!this->_buffer.empty()){
		// Выполняем сброс конца очереди
		this->_range.end = 0;
		// Выполняем сброс начала очереди
		this->_range.begin = 0;
		// Выполняем сброс количества записей в очереди
		this->_range.count = 0;
		// Сбрасываем смещение чтения данных
		this->_range.offset = 0;
		// Выполняем освобождение памяти
		vector <decltype(this->_buffer)::value_type> ().swap(this->_buffer);
	}
}
/**
 * @brief Количество добавленных элементов
 *
 * @return количество добавленных элементов
 *
 */
size_t awh::Queue::count() const noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Возвращаем количество записей в очереди
	return this->_range.count;
}
/**
 * @brief Метод получения размера добавленных данных
 *
 * @return размер добавленных данных
 *
 */
size_t awh::Queue::size() const noexcept {
	// Переменная результата
	size_t result = 0;
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_range.count > 0)){
		// Извлекаем текущее значение размера записи
		::memcpy(&result, &this->_buffer[this->_range.begin], sizeof(result));
		// Вычитаем смещение чтения данных (смещение не может превышать размер записи)
		result -= ::min(result, this->_range.offset);
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод вывода размера занимаемой памяти очередью
 *
 * @return количество памяти которую занимает очередь
 *
 */
size_t awh::Queue::capacity() const noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Возвращаем размер занимаемой памяти
	return this->_buffer.capacity();
}
/**
 * @brief Шаблон для метода получения конечного итератора
 *
 * @tparam T тип данных для подсчёта
 *
 */
template <typename T>
/**
 * @brief Метод получения конечного итератора
 *
 * @return конечный итератор
 *
 */
awh::Queue::Iterator <T> awh::Queue::end() noexcept {
	// Выполняем установку конечного значения итератора
	return Iterator <T> (reinterpret_cast <T *> (&this->_buffer[0] + this->_range.end));
}
/**
 * Объявляем прототипы для метода получения конечного итератора
 */
template awh::Queue::Iterator <int8_t> awh::Queue::end <int8_t> () noexcept;
template awh::Queue::Iterator <uint8_t> awh::Queue::end <uint8_t> () noexcept;
template awh::Queue::Iterator <int16_t> awh::Queue::end <int16_t> () noexcept;
template awh::Queue::Iterator <uint16_t> awh::Queue::end <uint16_t> () noexcept;
template awh::Queue::Iterator <int32_t> awh::Queue::end <int32_t> () noexcept;
template awh::Queue::Iterator <uint32_t> awh::Queue::end <uint32_t> () noexcept;
template awh::Queue::Iterator <int64_t> awh::Queue::end <int64_t> () noexcept;
template awh::Queue::Iterator <uint64_t> awh::Queue::end <uint64_t> () noexcept;
template awh::Queue::Iterator <float> awh::Queue::end <float> () noexcept;
template awh::Queue::Iterator <double> awh::Queue::end <double> () noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template awh::Queue::Iterator <char> awh::Queue::end <char> () noexcept;
#endif
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template awh::Queue::Iterator <size_t> awh::Queue::end <size_t> () noexcept;
	template awh::Queue::Iterator <ssize_t> awh::Queue::end <ssize_t> () noexcept;
#endif
/**
 * @brief Шаблон для метода получение начального итератора
 *
 * @tparam T тип данных для подсчёта
 *
 */
template <typename T>
/**
 * @brief Метод получение начального итератора
 *
 * @return начальный итератор
 *
 */
awh::Queue::Iterator <T> awh::Queue::begin() noexcept {
	// Выполняем установку начального значения итератора
	return Iterator <T> (reinterpret_cast <T *> (&this->_buffer[0] + this->_range.begin));
}
/**
 * Объявляем прототипы для метода получение начального итератора
 */
template awh::Queue::Iterator <int8_t> awh::Queue::begin <int8_t> () noexcept;
template awh::Queue::Iterator <uint8_t> awh::Queue::begin <uint8_t> () noexcept;
template awh::Queue::Iterator <int16_t> awh::Queue::begin <int16_t> () noexcept;
template awh::Queue::Iterator <uint16_t> awh::Queue::begin <uint16_t> () noexcept;
template awh::Queue::Iterator <int32_t> awh::Queue::begin <int32_t> () noexcept;
template awh::Queue::Iterator <uint32_t> awh::Queue::begin <uint32_t> () noexcept;
template awh::Queue::Iterator <int64_t> awh::Queue::begin <int64_t> () noexcept;
template awh::Queue::Iterator <uint64_t> awh::Queue::begin <uint64_t> () noexcept;
template awh::Queue::Iterator <float> awh::Queue::begin <float> () noexcept;
template awh::Queue::Iterator <double> awh::Queue::begin <double> () noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template awh::Queue::Iterator <char> awh::Queue::begin <char> () noexcept;
#endif
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template awh::Queue::Iterator <size_t> awh::Queue::begin <size_t> () noexcept;
	template awh::Queue::Iterator <ssize_t> awh::Queue::begin <ssize_t> () noexcept;
#endif
/**
 * @brief Шаблон для метода получения конечного константного итератора
 *
 * @tparam T тип данных для подсчёта
 *
 */
template <typename T>
/**
 * @brief Метод получения конечного константного итератора
 *
 * @return конечный константный итератор
 *
 */
awh::Queue::Const_Iterator <T> awh::Queue::end() const noexcept {
	// Выполняем установку конечного значения итератора
	return Const_Iterator <T> (reinterpret_cast <const T *> (&this->_buffer[0] + this->_range.end));
}
/**
 * @brief Шаблон для метода получения конечного константного итератора
 *
 * @tparam T тип данных для подсчёта
 *
 */
template <typename T>
/**
 * @brief Метод получения конечного константного итератора
 *
 * @return конечный константный итератор
 *
 */
awh::Queue::Const_Iterator <T> awh::Queue::cend() const noexcept {
	// Выполняем установку конечного значения итератора
	return this->template end <T> ();
}
/**
 * Объявляем прототипы для метода получения конечного константного итератора
 */
template awh::Queue::Const_Iterator <int8_t> awh::Queue::end <int8_t> () const noexcept;
template awh::Queue::Const_Iterator <uint8_t> awh::Queue::end <uint8_t> () const noexcept;
template awh::Queue::Const_Iterator <int16_t> awh::Queue::end <int16_t> () const noexcept;
template awh::Queue::Const_Iterator <uint16_t> awh::Queue::end <uint16_t> () const noexcept;
template awh::Queue::Const_Iterator <int32_t> awh::Queue::end <int32_t> () const noexcept;
template awh::Queue::Const_Iterator <uint32_t> awh::Queue::end <uint32_t> () const noexcept;
template awh::Queue::Const_Iterator <int64_t> awh::Queue::end <int64_t> () const noexcept;
template awh::Queue::Const_Iterator <uint64_t> awh::Queue::end <uint64_t> () const noexcept;
template awh::Queue::Const_Iterator <float> awh::Queue::end <float> () const noexcept;
template awh::Queue::Const_Iterator <double> awh::Queue::end <double> () const noexcept;
/**
 * Объявляем прототипы для метода получения конечного константного итератора
 */
template awh::Queue::Const_Iterator <int8_t> awh::Queue::cend <int8_t> () const noexcept;
template awh::Queue::Const_Iterator <uint8_t> awh::Queue::cend <uint8_t> () const noexcept;
template awh::Queue::Const_Iterator <int16_t> awh::Queue::cend <int16_t> () const noexcept;
template awh::Queue::Const_Iterator <uint16_t> awh::Queue::cend <uint16_t> () const noexcept;
template awh::Queue::Const_Iterator <int32_t> awh::Queue::cend <int32_t> () const noexcept;
template awh::Queue::Const_Iterator <uint32_t> awh::Queue::cend <uint32_t> () const noexcept;
template awh::Queue::Const_Iterator <int64_t> awh::Queue::cend <int64_t> () const noexcept;
template awh::Queue::Const_Iterator <uint64_t> awh::Queue::cend <uint64_t> () const noexcept;
template awh::Queue::Const_Iterator <float> awh::Queue::cend <float> () const noexcept;
template awh::Queue::Const_Iterator <double> awh::Queue::cend <double> () const noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template awh::Queue::Const_Iterator <char> awh::Queue::end <char> () const noexcept;
	template awh::Queue::Const_Iterator <char> awh::Queue::cend <char> () const noexcept;
#endif
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template awh::Queue::Const_Iterator <size_t> awh::Queue::end <size_t> () const noexcept;
	template awh::Queue::Const_Iterator <size_t> awh::Queue::cend <size_t> () const noexcept;
	template awh::Queue::Const_Iterator <ssize_t> awh::Queue::end <ssize_t> () const noexcept;
	template awh::Queue::Const_Iterator <ssize_t> awh::Queue::cend <ssize_t> () const noexcept;
#endif
/**
 * @brief Шаблон для метода получения начального константного итератора
 *
 * @tparam T тип данных для подсчёта
 *
 */
template <typename T>
/**
 * @brief Метод получения начального константного итератора
 *
 * @return начальный константный итератор
 *
 */
awh::Queue::Const_Iterator <T> awh::Queue::begin() const noexcept {
	// Выполняем установку начального значения итератора
	return Const_Iterator <T> (reinterpret_cast <const T *> (&this->_buffer[0] + this->_range.begin));
}
/**
 * @brief Шаблон для метода получения начального константного итератора
 *
 * @tparam T тип данных для подсчёта
 *
 */
template <typename T>
/**
 * @brief Метод получения начального константного итератора
 *
 * @return начальный константный итератор
 *
 */
awh::Queue::Const_Iterator <T> awh::Queue::cbegin() const noexcept {
	// Выполняем установку начального значения итератора
	return this->template begin <T> ();
}
/**
 * Объявляем прототипы для метода получения начального константного итератора
 */
template awh::Queue::Const_Iterator <int8_t> awh::Queue::begin <int8_t> () const noexcept;
template awh::Queue::Const_Iterator <uint8_t> awh::Queue::begin <uint8_t> () const noexcept;
template awh::Queue::Const_Iterator <int16_t> awh::Queue::begin <int16_t> () const noexcept;
template awh::Queue::Const_Iterator <uint16_t> awh::Queue::begin <uint16_t> () const noexcept;
template awh::Queue::Const_Iterator <int32_t> awh::Queue::begin <int32_t> () const noexcept;
template awh::Queue::Const_Iterator <uint32_t> awh::Queue::begin <uint32_t> () const noexcept;
template awh::Queue::Const_Iterator <int64_t> awh::Queue::begin <int64_t> () const noexcept;
template awh::Queue::Const_Iterator <uint64_t> awh::Queue::begin <uint64_t> () const noexcept;
template awh::Queue::Const_Iterator <float> awh::Queue::begin <float> () const noexcept;
template awh::Queue::Const_Iterator <double> awh::Queue::begin <double> () const noexcept;
/**
 * Объявляем прототипы для метода получения начального константного итератора
 */
template awh::Queue::Const_Iterator <int8_t> awh::Queue::cbegin <int8_t> () const noexcept;
template awh::Queue::Const_Iterator <uint8_t> awh::Queue::cbegin <uint8_t> () const noexcept;
template awh::Queue::Const_Iterator <int16_t> awh::Queue::cbegin <int16_t> () const noexcept;
template awh::Queue::Const_Iterator <uint16_t> awh::Queue::cbegin <uint16_t> () const noexcept;
template awh::Queue::Const_Iterator <int32_t> awh::Queue::cbegin <int32_t> () const noexcept;
template awh::Queue::Const_Iterator <uint32_t> awh::Queue::cbegin <uint32_t> () const noexcept;
template awh::Queue::Const_Iterator <int64_t> awh::Queue::cbegin <int64_t> () const noexcept;
template awh::Queue::Const_Iterator <uint64_t> awh::Queue::cbegin <uint64_t> () const noexcept;
template awh::Queue::Const_Iterator <float> awh::Queue::cbegin <float> () const noexcept;
template awh::Queue::Const_Iterator <double> awh::Queue::cbegin <double> () const noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template awh::Queue::Const_Iterator <char> awh::Queue::begin <char> () const noexcept;
	template awh::Queue::Const_Iterator <char> awh::Queue::cbegin <char> () const noexcept;
#endif
/**
 * Если size_t и ssize_t являются самостоятельными типами
 */
#if __AWH_DISTINCT_SIZE_TYPES__
	template awh::Queue::Const_Iterator <size_t> awh::Queue::begin <size_t> () const noexcept;
	template awh::Queue::Const_Iterator <size_t> awh::Queue::cbegin <size_t> () const noexcept;
	template awh::Queue::Const_Iterator <ssize_t> awh::Queue::begin <ssize_t> () const noexcept;
	template awh::Queue::Const_Iterator <ssize_t> awh::Queue::cbegin <ssize_t> () const noexcept;
#endif
/**
 * @brief Получения данных указанного элемента в очереди
 *
 * @return указатель на элемент очереди
 *
 */
const void * awh::Queue::data() const noexcept {
	// Переменная результата
	const void * result = nullptr;
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_range.count > 0))
		// Возвращаем данные записи в бинарном виде
		result = (&this->_buffer[0] + this->_range.begin + sizeof(size_t) + this->_range.offset);
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод фиксации прочитанного размера данных
 *
 * @param size размер данных для фиксации
 *
 */
void awh::Queue::commit(const size_t size) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_range.count > 0)){
		// Размер верхней записи в очереди
		size_t length = 0;
		// Извлекаем текущее значение размера записи
		::memcpy(&length, &this->_buffer[this->_range.begin], sizeof(length));
		// Выполняем фиксацию смещения, не позволяя ему превысить размер записи
		this->_range.offset = ::min(length, this->_range.offset + size);
	}
}
/**
 * @brief Метод проверки на заполненность очереди
 *
 * @note При установленном таймауте и активной потокобезопасности метод блокирует
 *       поток до появления данных в очереди либо до истечения времени ожидания.
 *
 * @param timeout время ожидания в миллисекундах
 * @return        результат проверки
 *
 */
bool awh::Queue::empty(const uint32_t timeout) const noexcept {
	// Если ожидание не требуется либо потокобезопасность отключена
	if((timeout == 0) || !this->_mtx.enabled){
		// Выполняем блокировку потока
		const locker_t <> lock(this->_mtx);
		// Возвращаем результат проверки на пустоту очереди
		return (this->_range.count == 0);
	}
	// Выполняем блокировку потока для ожидания появления данных
	unique_lock <std::mutex> lock(static_cast <std::mutex &> (this->_mtx));
	// Ожидаем появления записей в очереди либо истечения таймаута
	this->_cv.wait_for(lock, chrono::milliseconds(timeout), [this]() noexcept -> bool {
		// Условием выхода из ожидания является появление хотя бы одной записи
		return (this->_range.count > 0);
	});
	// Возвращаем результат проверки на пустоту очереди
	return (this->_range.count == 0);
}
/**
 * @brief Метод добавления бинарного буфера данных в очередь
 *
 * @param buffer бинарный буфер для добавления
 * @param size   размер бинарного буфера
 * @return       текущий размер очереди
 *
 */
size_t awh::Queue::push(const void * buffer, const size_t size) noexcept {
	// Переменная результата
	size_t result = 0;
	// Если данные переданы верные
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			const locker_t <> lock(this->_mtx);
			// Если достигнут предел количества записей в очереди
			if(this->_range.count >= this->_max.records)
				// Записываем ошибку в лог
				this->error(__PRETTY_FUNCTION__, size, "Queue has reached the maximum number of records");
			// Выполняем выделение памяти
			else if(this->rss(size)) {
				// Увеличиваем количество записей в очереди
				this->_range.count++;
				// Выполняем запись данных в буфер
				::memcpy(&this->_buffer[this->_range.end], reinterpret_cast <const uint8_t *> (&size), sizeof(size));
				// Увеличиваем смещение конца данных буфера
				this->_range.end += sizeof(size);
				// Выполняем добавление самих данных полезной нагрузки
				::memcpy(&this->_buffer[this->_range.end], buffer, size);
				// Увеличиваем смещение конца данных буфера
				this->_range.end += size;
				// Уведомляем ожидающие потоки о появлении данных в очереди
				this->_cv.notify_all();
			}
			// Запоминаем текущее количество записей в очереди
			result = this->_range.count;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Записываем ошибку в лог
			this->error(__PRETTY_FUNCTION__, size, error.what());
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод добавления бинарного буфера данных в очередь
 *
 * @param records список бинарных буферов для добавления
 * @param size    общий размер добавляемых данных
 * @return        текущий размер очереди
 *
 */
size_t awh::Queue::push(const vector <record_t> & records, const size_t size) noexcept {
	// Переменная результата
	size_t result = 0;
	// Если данные переданы верные
	if(!records.empty() && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			const locker_t <> lock(this->_mtx);
			// Если достигнут предел количества записей в очереди
			if(this->_range.count >= this->_max.records)
				// Записываем ошибку в лог
				this->error(__PRETTY_FUNCTION__, size, "Queue has reached the maximum number of records");
			// Если предел количества записей не достигнут
			else {
				// Получаем фактический суммарный размер всех записей
				size_t total = 0;
				/**
				 * Выполняем подсчёт фактического размера всех записей
				 */
				for(auto & record : records)
					// Увеличиваем суммарный размер записей
					total += record.second;
				// Если фактический размер записей не совпадает с указанным
				if(total != size)
					// Записываем ошибку в лог
					this->error(__PRETTY_FUNCTION__, size, "Total size of records does not match the specified size");
				// Выполняем выделение памяти
				else if(this->rss(size)) {
					// Увеличиваем количество записей в очереди
					this->_range.count++;
					// Выполняем запись данных в буфер
					::memcpy(&this->_buffer[this->_range.end], reinterpret_cast <const uint8_t *> (&size), sizeof(size));
					// Увеличиваем смещение конца данных буфера
					this->_range.end += sizeof(size);
					/**
					 * Выполняем перебор всех записей
					 */
					for(auto & record : records){
						// Если запись содержит данные
						if((record.first != nullptr) && (record.second > 0)){
							// Выполняем добавление самих данных полезной нагрузки
							::memcpy(&this->_buffer[this->_range.end], record.first, record.second);
							// Увеличиваем смещение конца данных буфера
							this->_range.end += record.second;
						}
					}
					// Уведомляем ожидающие потоки о появлении данных в очереди
					this->_cv.notify_all();
				}
			}
			// Запоминаем текущее количество записей в очереди
			result = this->_range.count;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Записываем ошибку в лог
			this->error(__PRETTY_FUNCTION__, size, error.what());
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки максимального размера потребления памяти
 *
 * @param size максимальный размер потребления памяти
 *
 */
void awh::Queue::setMaxMemory(const size_t size) noexcept {
	// Если максимальный размер потребляемой памяти передан
	if(size > 0){
		// Выполняем блокировку потока
		const locker_t <> lock(this->_mtx);
		// Выполняем установку максимального размера потребляемой памяти
		this->_max.memory = size;
	}
}
/**
 * @brief Метод установки максимального количества записей очереди
 *
 * @param count максимальное количество записей очереди
 *
 */
void awh::Queue::setMaxRecords(const size_t count) noexcept {
	// Если количество записей передано
	if(count > 0){
		// Выполняем блокировку потока
		const locker_t <> lock(this->_mtx);
		// Выполняем установку максимального количества сообщений очереди
		this->_max.records = count;
	}
}
/**
 * @brief Метод обмена очередями
 *
 * @param queue очередь для обмена
 *
 */
void awh::Queue::swap(queue_t & queue) noexcept {
	// Если выполняется обмен с самим собой
	if(this == &queue)
		// Выходим из функции
		return;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Объекты блокировки потоков для обеих очередей
		unique_lock <std::mutex> lock1, lock2;
		// Если хотя бы для одной из очередей включена потокобезопасность
		if(this->_mtx.enabled || queue._mtx.enabled){
			// Готовим блокировку текущей очереди
			lock1 = unique_lock <std::mutex> (static_cast <std::mutex &> (this->_mtx), std::defer_lock);
			// Готовим блокировку сторонней очереди
			lock2 = unique_lock <std::mutex> (static_cast <std::mutex &> (queue._mtx), std::defer_lock);
			// Выполняем захват обоих мьютексов без риска взаимной блокировки
			::lock(lock1, lock2);
		}
		// Если объект фреймворка установлен
		if((queue._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = queue._fmk;
		// Если объект для работы с логами установлен
		if((queue._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = queue._log;
		// Если объект фреймворка установлен
		if((this->_fmk != nullptr) && (queue._fmk == nullptr))
			// Копируем объект фреймворка
			queue._fmk = this->_fmk;
		// Если объект для работы с логами установлен
		if((this->_log != nullptr) && (queue._log == nullptr))
			// Копируем объект для работы с логами установлен
			queue._log = this->_log;
		// Выполняем обмен буферами данных
		this->_buffer.swap(queue._buffer);
		/**
		 * Выполняем обмен смещениями чтения данных
		 *
		 * @note Поле принадлежит упакованной структуре, и обменивается оно копированием
		 *       значений, как и соседние: ссылку на поле упакованной структуры взять
		 *       нельзя, а обмен из стандартной библиотеки берёт именно ссылки
		 */
		{
			// Сохраняем текущее значение смещения чтения данных
			const size_t offset = this->_range.offset;
			// Выполняем обмен значениями смещения чтения данных
			this->_range.offset = queue._range.offset;
			// Записываем сохранённое значение смещения чтения данных
			queue._range.offset = offset;
		}
		/**
		 * Выполняем обмен последними итераторами (поля упакованной структуры обмениваем через копирование значений)
		 */
		{
			// Сохраняем текущее значение конца очереди
			const size_t end = this->_range.end;
			// Выполняем обмен значениями конца очереди
			this->_range.end = queue._range.end;
			// Записываем сохранённое значение конца очереди
			queue._range.end = end;
		}
		/**
		 * Выполняем обмен начальными итераторами
		 */
		{
			// Сохраняем текущее значение начала очереди
			const size_t begin = this->_range.begin;
			// Выполняем обмен значениями начала очереди
			this->_range.begin = queue._range.begin;
			// Записываем сохранённое значение начала очереди
			queue._range.begin = begin;
		}
		/**
		 * Выполняем обмен количествами добавленных записей
		 */
		{
			// Сохраняем текущее количество записей
			const size_t count = this->_range.count;
			// Выполняем обмен количествами записей
			this->_range.count = queue._range.count;
			// Записываем сохранённое количество записей
			queue._range.count = count;
		}
		/**
		 * Выполняем обмен максимальными размерами памяти (поля упакованной структуры обмениваем через копирование значений)
		 */
		{
			// Сохраняем текущий максимальный размер памяти
			const size_t memory = this->_max.memory;
			// Выполняем обмен максимальными размерами памяти
			this->_max.memory = queue._max.memory;
			// Записываем сохранённый максимальный размер памяти
			queue._max.memory = memory;
		}
		/**
		 * Выполняем обмен максимальными количествами записей (поля упакованной структуры обмениваем через копирование значений)
		 */
		{
			// Сохраняем текущее максимальное количество записей
			const size_t records = this->_max.records;
			// Выполняем обмен максимальными количествами записей
			this->_max.records = queue._max.records;
			// Записываем сохранённое максимальное количество записей
			queue._max.records = records;
		}
		// Уведомляем ожидающие потоки обеих очередей о возможном появлении данных
		this->_cv.notify_all();
		// Уведомляем ожидающие потоки сторонней очереди
		queue._cv.notify_all();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, 0, error.what());
	}
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности работы потоков
 *
 */
void awh::Queue::threadSafety(const bool mode) noexcept {
	// Активируем либо отключаем работу мьютекса блокировки доступа к данным очереди
	this->_mtx.enabled = mode;
	// Пробуждаем все ожидающие потоки, чтобы они не зависли при отключении потокобезопасности
	this->_cv.notify_all();
}
/**
 * @brief Метод установки объекта логирования
 *
 * @param log объект работы с логами
 *
 */
void awh::Queue::setLogger(const log_t * log) noexcept {
	// Выполняем блокировку потока
	const locker_t <> lock(this->_mtx);
	// Устанавливаем объект логирования
	this->_log = log;
}
/**
 * @brief Получения размера данных в очереди
 *
 * @return размер данных в очереди
 *
 */
awh::Queue::operator size_t() const noexcept {
	// Возвращаем размер очереди (блокировка выполняется внутри метода size)
	return this->size();
}
/**
 * @brief Получения бинарных данных очереди
 *
 * @return бинарные данные очереди
 *
 */
awh::Queue::operator const char * () const noexcept {
	// Возвращаем данные записи очереди (блокировка выполняется внутри метода data)
	return reinterpret_cast <const char *> (this->data());
}
/**
 * @brief Получения бинарных данных очереди
 *
 * @return бинарные данные очереди
 *
 */
awh::Queue::operator const uint8_t * () const noexcept {
	// Возвращаем данные записи очереди (блокировка выполняется внутри метода data)
	return reinterpret_cast <const uint8_t *> (this->data());
}
/**
 * @brief Оператор перемещения
 *
 * @param queue очередь для перемещения
 * @return      текущий контейнер очереди
 *
 */
awh::Queue & awh::Queue::operator = (queue_t && queue) noexcept {
	// Если выполняется присваивание самому себе
	if(this == &queue)
		// Возвращаем текущий объект
		return (* this);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Объекты блокировки потоков для обеих очередей
		unique_lock <std::mutex> lock1, lock2;
		// Если хотя бы для одной из очередей включена потокобезопасность
		if(this->_mtx.enabled || queue._mtx.enabled){
			// Готовим блокировку текущей очереди
			lock1 = unique_lock <std::mutex> (static_cast <std::mutex &> (this->_mtx), std::defer_lock);
			// Готовим блокировку сторонней очереди
			lock2 = unique_lock <std::mutex> (static_cast <std::mutex &> (queue._mtx), std::defer_lock);
			// Выполняем захват обоих мьютексов без риска взаимной блокировки
			::lock(lock1, lock2);
		}
		// Если объект фреймворка установлен
		if((queue._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = queue._fmk;
		// Если объект для работы с логами установлен
		if((queue._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = queue._log;
		// Выполняем перемещение буфера данных
		this->_buffer = ::move(queue._buffer);
		// Выполняем копирование последнего итератора
		this->_range.end = queue._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = queue._range.begin;
		// Выполняем копирование количества добавленных записей
		this->_range.count = queue._range.count;
		// Выполняем установку смещения чтения данных
		this->_range.offset = queue._range.offset;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
		// Выполняем сброс последнего итератора сторонней очереди
		queue._range.end = 0;
		// Выполняем сброс начального итератора сторонней очереди
		queue._range.begin = 0;
		// Выполняем сброс количества добавленных записей сторонней очереди
		queue._range.count = 0;
		// Выполняем сброс смещения чтения данных сторонней очереди
		queue._range.offset = 0;
		// Уведомляем ожидающие потоки о возможном появлении данных
		this->_cv.notify_all();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, 0, error.what());
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param queue очередь для копирования
 * @return      текущий контейнер очереди
 *
 */
awh::Queue & awh::Queue::operator = (const queue_t & queue) noexcept {
	// Если выполняется присваивание самому себе
	if(this == &queue)
		// Возвращаем текущий объект
		return (* this);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Объекты блокировки потоков для обеих очередей
		unique_lock <std::mutex> lock1, lock2;
		// Если хотя бы для одной из очередей включена потокобезопасность
		if(this->_mtx.enabled || queue._mtx.enabled){
			// Готовим блокировку текущей очереди
			lock1 = unique_lock <std::mutex> (static_cast <std::mutex &> (this->_mtx), std::defer_lock);
			// Готовим блокировку сторонней очереди
			lock2 = unique_lock <std::mutex> (static_cast <std::mutex &> (queue._mtx), std::defer_lock);
			// Выполняем захват обоих мьютексов без риска взаимной блокировки
			::lock(lock1, lock2);
		}
		// Если объект фреймворка установлен
		if((queue._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = queue._fmk;
		// Если объект для работы с логами установлен
		if((queue._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = queue._log;
		// Выполняем копирование последнего итератора
		this->_range.end = queue._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = queue._range.begin;
		// Выполняем копирование количества добавленных записей
		this->_range.count = queue._range.count;
		// Выполняем установку смещения чтения данных
		this->_range.offset = queue._range.offset;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
		// Выполняем копирование буфера данных
		this->_buffer.assign(queue._buffer.begin(), queue._buffer.end());
		// Уведомляем ожидающие потоки о возможном появлении данных
		this->_cv.notify_all();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, 0, error.what());
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор сравнения двух очередей
 *
 * @param queue очередь для сравнения
 * @return      результат сравнения
 *
 */
bool awh::Queue::operator == (const queue_t & queue) const noexcept {
	// Если выполняется сравнение с самим собой
	if(this == &queue)
		// Очереди равны
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Объекты блокировки потоков для обеих очередей
		unique_lock <std::mutex> lock1, lock2;
		// Если хотя бы для одной из очередей включена потокобезопасность
		if(this->_mtx.enabled || queue._mtx.enabled){
			// Готовим блокировку текущей очереди
			lock1 = unique_lock <std::mutex> (static_cast <std::mutex &> (this->_mtx), std::defer_lock);
			// Готовим блокировку сторонней очереди
			lock2 = unique_lock <std::mutex> (static_cast <std::mutex &> (queue._mtx), std::defer_lock);
			// Выполняем захват обоих мьютексов без риска взаимной блокировки
			::lock(lock1, lock2);
		}
		// Если не совпадает количество записей или смещение чтения данных
		if((this->_range.count != queue._range.count) || (this->_range.offset != queue._range.offset))
			// Очереди не равны
			return false;
		// Получаем размер активных данных каждой из текущей очереди
		const size_t lengthA = (this->_range.end - this->_range.begin);
		// Получаем размер активных данных каждой из очереди для сравнения
		const size_t lengthB = (queue._range.end - queue._range.begin);
		// Если размер активных данных не совпадает
		if(lengthA != lengthB)
			// Очереди не равны
			return false;
		// Если активных данных нет
		if(lengthA == 0)
			// Очереди равны
			return true;
		// Выполняем сравнение только активных данных очередей
		return (::memcmp(&this->_buffer[0] + this->_range.begin, &queue._buffer[0] + queue._range.begin, lengthA) == 0);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, 0, error.what());
	}
	// Возвращаем результат
	return false;
}
/**
 * @brief Разрешаем пустое значение объекта
 *
 */
awh::Queue::Queue() noexcept : _fmk(nullptr), _log(nullptr) {}
/**
 * @brief Конструктор перемещения
 *
 * @param queue очередь для перемещения
 *
 */
awh::Queue::Queue(queue_t && queue) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Объект блокировки потока сторонней очереди
		unique_lock <std::mutex> lock;
		// Если для сторонней очереди включена потокобезопасность
		if(queue._mtx.enabled)
			// Выполняем блокировку сторонней очереди
			lock = unique_lock <std::mutex> (static_cast <std::mutex &> (queue._mtx));
		// Если объект фреймворка установлен
		if((queue._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = queue._fmk;
		// Если объект для работы с логами установлен
		if((queue._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = queue._log;
		// Выполняем перемещение буфера данных
		this->_buffer = ::move(queue._buffer);
		// Выполняем копирование последнего итератора
		this->_range.end = queue._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = queue._range.begin;
		// Выполняем копирование количества добавленных записей
		this->_range.count = queue._range.count;
		// Выполняем установку смещения чтения данных
		this->_range.offset = queue._range.offset;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
		// Выполняем сброс последнего итератора сторонней очереди
		queue._range.end = 0;
		// Выполняем сброс начального итератора сторонней очереди
		queue._range.begin = 0;
		// Выполняем сброс количества добавленных записей сторонней очереди
		queue._range.count = 0;
		// Выполняем сброс смещения чтения данных сторонней очереди
		queue._range.offset = 0;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, 0, error.what());
	}
}
/**
 * @brief Конструктор копирования
 *
 * @param queue очередь для копирования
 *
 */
awh::Queue::Queue(const queue_t & queue) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Объект блокировки потока сторонней очереди
		unique_lock <std::mutex> lock;
		// Если для сторонней очереди включена потокобезопасность
		if(queue._mtx.enabled)
			// Выполняем блокировку сторонней очереди
			lock = unique_lock <std::mutex> (static_cast <std::mutex &> (queue._mtx));
		// Если объект фреймворка установлен
		if((queue._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = queue._fmk;
		// Если объект для работы с логами установлен
		if((queue._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = queue._log;
		// Выполняем копирование буфера данных
		this->_buffer = queue._buffer;
		// Выполняем копирование последнего итератора
		this->_range.end = queue._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = queue._range.begin;
		// Выполняем копирование количества добавленных записей
		this->_range.count = queue._range.count;
		// Выполняем установку смещения чтения данных
		this->_range.offset = queue._range.offset;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, 0, error.what());
	}
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Queue::Queue(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Queue::~Queue() noexcept {}
