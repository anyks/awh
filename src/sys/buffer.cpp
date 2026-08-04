/**
 * @file: buffer.cpp
 * @date: 2025-10-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация бинарного смарт-буфера — учёт диапазонов записей,
 *        транзакционная запись с откатом при отсутствии фиксации,
 *        обход итераторами и предоставление невладеющих обёрток доступа к данным
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstring>
#include <cstdlib>
#include <utility>
#include <algorithm>

/**
 * Подключаем заголовочный файл проекта
 */
#include <sys/buffer.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 */
awh::Buffer::Range::Range() noexcept :
 end(0), begin(0), reserved(0),
 maxMemory(AWH_MAX_MEMORY_BUFFER) {}

/**
 * @brief Метод получения указателя на зарезервированную область
 *
 * @return указатель для записи данных либо nullptr при ошибке резервирования
 *
 */
void * awh::Buffer::Writer::get() noexcept {
	// Возвращаем указатель на зарезервированную область
	return this->_data;
}
/**
 * @brief Метод получения размера зарезервированной области
 *
 * @return размер доступного для записи места
 *
 */
size_t awh::Buffer::Writer::size() const noexcept {
	// Возвращаем размер зарезервированной области
	return this->_capacity;
}
/**
 * @brief Метод проверки корректности резервирования
 *
 * @return результат проверки
 *
 */
bool awh::Buffer::Writer::valid() const noexcept {
	// Возвращаем результат проверки
	return (this->_data != nullptr);
}
/**
 * @brief Метод отмены записи (зафиксировано не будет ничего)
 *
 */
void awh::Buffer::Writer::cancel() noexcept {
	// Сбрасываем количество фиксируемых данных
	this->_committed = 0;
}
/**
 * @brief Метод немедленной фиксации записанных данных в буфер
 *
 * @return количество зафиксированных байт
 *
 */
size_t awh::Buffer::Writer::apply() noexcept {
	// Результат фиксации
	size_t result = 0;
	// Если фиксация ещё не выполнялась и буфер установлен
	if(!this->_applied && (this->_buffer != nullptr)){
		// Выполняем фиксацию записанных данных в буфер
		result = this->_buffer->commit(this->_committed);
		// Помечаем фиксацию как выполненную
		this->_applied = true;
	}
	// Возвращаем количество зафиксированных байт
	return result;
}
/**
 * @brief Метод указания количества фактически записанных байт
 *
 * @param size количество записанных байт
 * @return     количество байт которое будет зафиксировано
 *
 */
size_t awh::Buffer::Writer::commit(const size_t size) noexcept {
	// Ограничиваем количество фиксируемых данных зарезервированным размером
	this->_committed = ((size < this->_capacity) ? size : this->_capacity);
	// Возвращаем количество фиксируемых данных
	return this->_committed;
}
/**
 * @brief Конструктор перемещения
 *
 * @param other обёртка для перемещения
 *
 */
awh::Buffer::Writer::Writer(Writer && other) noexcept :
 _applied(other._applied),
 _capacity(other._capacity),
 _committed(other._committed),
 _data(other._data), _buffer(other._buffer) {
	// Помечаем источник как отработавший
	other._buffer = nullptr;
	// Сбрасываем указатель источника
	other._data = nullptr;
	// Сбрасываем размер источника
	other._capacity = 0;
	// Сбрасываем количество фиксируемых данных источника
	other._committed = 0;
	// Помечаем фиксацию источника как выполненную
	other._applied = true;
}
/**
 * @brief Конструктор
 *
 * @param buffer   буфер для записи
 * @param data     указатель на зарезервированную область
 * @param capacity размер зарезервированной области
 *
 */
awh::Buffer::Writer::Writer(Buffer * buffer, void * data, const size_t capacity) noexcept :
 _applied(false), _capacity(capacity), _committed(0), _data(data), _buffer(buffer) {}
/**
 * @brief Деструктор (автоматически фиксирует записанные данные)
 *
 */
awh::Buffer::Writer::~Writer() noexcept {
	// Выполняем фиксацию записанных данных
	this->apply();
}

/**
 * @brief Метод контроля памяти
 *
 * @details Гарантирует наличие как минимум size свободных байт в хвосте буфера.
 *          При необходимости переиспользует уже извлечённое место в начале буфера
 *          (компактизация) и/или увеличивает буфер в пределах максимального лимита.
 *
 * @param size желаемый размер свободного места в хвосте буфера
 * @return     результат выполнения операции
 *
 */
bool awh::Buffer::rss(const size_t size) noexcept {
	// Если выделять ничего не требуется
	if(size == 0)
		// Сообщаем что место уже есть
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем количество свободного места в хвосте буфера
		size_t tail = ((this->_buffer.size() > this->_range.end) ? (this->_buffer.size() - this->_range.end) : 0);
		// Если в хвосте уже достаточно места
		if(tail >= size)
			// Сообщаем что выделение не требуется
			return true;
		// Определяем размер полезных (ещё не извлечённых) данных
		const size_t payload = (this->_range.end - this->_range.begin);
		// Если в начале буфера есть освобождённое место — выполняем компактизацию
		if(this->_range.begin > 0){
			// Если есть что перемещать
			if(payload > 0)
				// Сдвигаем полезные данные в начало буфера
				::memmove(&this->_buffer[0], &this->_buffer[0] + this->_range.begin, payload);
			// Сбрасываем начало буфера
			this->_range.begin = 0;
			// Смещаем конец буфера на размер полезных данных
			this->_range.end = payload;
			// Пересчитываем количество свободного места в хвосте
			tail = (this->_buffer.size() - this->_range.end);
			// Если после компактизации места стало достаточно
			if(tail >= size)
				// Сообщаем что выделение не требуется
				return true;
		}
		/**
		 * Если запрошенный объём не помещается в лимит вместе с полезными данными -
		 * проверка выполняется вычитанием, чтобы сложение не переполнило разрядность
		 */
		if(size > (this->_range.maxMemory - payload))
			// Сообщаем что выделить память невозможно
			return false;
		// Определяем требуемый итоговый размер буфера данных
		const size_t required = (payload + size);
		/**
		 * Растим хранилище с запасом, удваивая текущий размер: рост ровно до требуемого
		 * объёма означал бы переаллокацию с переинициализацией хвоста на каждой дозаписи,
		 * а потоковая запись состоит из множества мелких дозаписей подряд
		 */
		size_t target = required;
		// Вычисляем удвоенный размер текущего хранилища (с защитой от переполнения)
		const size_t doubled = ((this->_buffer.size() < (SIZE_MAX / 2)) ? (this->_buffer.size() * 2) : SIZE_MAX);
		// Если удвоенный размер больше требуемого - растим с запасом
		if(doubled > target)
			// Увеличиваем целевой размер хранилища
			target = doubled;
		// Ограничиваем целевой размер максимальным лимитом потребления памяти
		if(target > this->_range.maxMemory)
			// Урезаем целевой размер до лимита
			target = this->_range.maxMemory;
		// Увеличиваем буфер данных до целевого размера
		this->_buffer.resize(target);
		// Сообщаем об успешном выделении памяти
		return true;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод вывода сообщения об ошибке в лог
 *
 * @param func    название функции в которой произошла ошибка
 * @param message текст сообщения об ошибке
 * @param flag    флаг важности сообщения
 *
 */
void awh::Buffer::error(const char * func, const char * message, const log_t::flag_t flag) const noexcept {
	// Если объект лога установлен
	if(this->_log != nullptr){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", func, {}, flag, message);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", flag, message);
		#endif
	// Если объект логирования не установлен
	} else {
		// Определяем текстовый префикс важности сообщения
		const char * prefix = ((flag == log_t::flag_t::WARNING) ? "WARNING" : "ERROR");
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в поток ошибок
			::fprintf(stderr, "%s! Called function:\n%s\n\nMessage:\n%s\n\n", prefix, func, message);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в поток ошибок
			::fprintf(stderr, "%s! %s\n\n", prefix, message);
		#endif
	}
}
/**
 * @brief Метод очистки всех данных буфера
 *
 */
void awh::Buffer::clear() noexcept {
	// Выполняем сброс конца буфера
	this->_range.end = 0;
	// Выполняем сброс начала буфера
	this->_range.begin = 0;
}
/**
 * @brief Метод полной очистки памяти
 *
 */
void awh::Buffer::reset() noexcept {
	// Выполняем сброс конца буфера
	this->_range.end = 0;
	// Выполняем сброс начала буфера
	this->_range.begin = 0;
	// Выполняем освобождение памяти
	vector <decltype(this->_buffer)::value_type> ().swap(this->_buffer);
}
/**
 * @brief Метод проверки на заполненность буфера
 *
 * @return результат проверки
 *
 */
bool awh::Buffer::empty() const noexcept {
	// Возвращаем результат проверки
	return (this->_range.end == this->_range.begin);
}
/**
 * @brief Метод получения размера добавленных данных
 *
 * @return размер всех добавленных данных
 *
 */
size_t awh::Buffer::size() const noexcept {
	// Возвращаем размер добавленных данных в буфер
	return (this->_range.end - this->_range.begin);
}
/**
 * @brief Метод вывода размера занимаемой памяти очередью
 *
 * @return количество памяти которую занимает буфер
 *
 */
size_t awh::Buffer::capacity() const noexcept {
	// Возвращаем количество выделенной памяти
	return this->_buffer.capacity();
}
/**
 * @brief Метод извлечения буфера сырых данных
 *
 * @details Нормализует внутреннее хранилище: переносит полезные данные в начало
 *          и усекает буфер до их размера, после чего возвращает его как есть.
 *
 * @return буфер сырых данных
 *
 */
const vector <uint8_t> & awh::Buffer::raw() const noexcept {
	// Получаем неконстантный указатель на текущий объект
	buffer_t * self = const_cast <buffer_t *> (this);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем размер полезных данных
		const size_t payload = this->size();
		// Если полезных данных нет
		if(payload == 0){
			// Освобождаем память буфера данных
			vector <decltype(this->_buffer)::value_type> ().swap(self->_buffer);
			// Сбрасываем начало буфера
			self->_range.begin = 0;
			// Сбрасываем конец буфера
			self->_range.end = 0;
		// Если буфер не соответствует диапазону данных
		} else if((this->_range.begin > 0) || (this->_range.end < this->_buffer.size())) {
			// Если данные смещены от начала буфера
			if(this->_range.begin > 0)
				// Переносим полезные данные в начало буфера
				::memmove(&self->_buffer[0], &self->_buffer[0] + this->_range.begin, payload);
			// Усекаем буфер до размера полезных данных
			self->_buffer.resize(payload);
			// Сбрасываем начало буфера
			self->_range.begin = 0;
			// Устанавливаем конец буфера на размер полезных данных
			self->_range.end = payload;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем значение буфера как есть
	return this->_buffer;
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
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template awh::Buffer::Iterator <size_t> awh::Buffer::end <size_t> () noexcept;
	template awh::Buffer::Iterator <ssize_t> awh::Buffer::end <ssize_t> () noexcept;
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
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template awh::Buffer::Iterator <size_t> awh::Buffer::begin <size_t> () noexcept;
	template awh::Buffer::Iterator <ssize_t> awh::Buffer::begin <ssize_t> () noexcept;
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
awh::Buffer::Const_Iterator <T> awh::Buffer::end() const noexcept {
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
awh::Buffer::Const_Iterator <T> awh::Buffer::cend() const noexcept {
	// Выполняем установку конечного значения итератора
	return this->template end <T> ();
}
/**
 * Объявляем прототипы для метода получения конечного константного итератора
 */
template awh::Buffer::Const_Iterator <int8_t> awh::Buffer::end <int8_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint8_t> awh::Buffer::end <uint8_t> () const noexcept;
template awh::Buffer::Const_Iterator <int16_t> awh::Buffer::end <int16_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint16_t> awh::Buffer::end <uint16_t> () const noexcept;
template awh::Buffer::Const_Iterator <int32_t> awh::Buffer::end <int32_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint32_t> awh::Buffer::end <uint32_t> () const noexcept;
template awh::Buffer::Const_Iterator <int64_t> awh::Buffer::end <int64_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint64_t> awh::Buffer::end <uint64_t> () const noexcept;
template awh::Buffer::Const_Iterator <float> awh::Buffer::end <float> () const noexcept;
template awh::Buffer::Const_Iterator <double> awh::Buffer::end <double> () const noexcept;
/**
 * Объявляем прототипы для метода получения конечного константного итератора
 */
template awh::Buffer::Const_Iterator <int8_t> awh::Buffer::cend <int8_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint8_t> awh::Buffer::cend <uint8_t> () const noexcept;
template awh::Buffer::Const_Iterator <int16_t> awh::Buffer::cend <int16_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint16_t> awh::Buffer::cend <uint16_t> () const noexcept;
template awh::Buffer::Const_Iterator <int32_t> awh::Buffer::cend <int32_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint32_t> awh::Buffer::cend <uint32_t> () const noexcept;
template awh::Buffer::Const_Iterator <int64_t> awh::Buffer::cend <int64_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint64_t> awh::Buffer::cend <uint64_t> () const noexcept;
template awh::Buffer::Const_Iterator <float> awh::Buffer::cend <float> () const noexcept;
template awh::Buffer::Const_Iterator <double> awh::Buffer::cend <double> () const noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template awh::Buffer::Const_Iterator <char> awh::Buffer::end <char> () const noexcept;
	template awh::Buffer::Const_Iterator <char> awh::Buffer::cend <char> () const noexcept;
#endif
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template awh::Buffer::Const_Iterator <size_t> awh::Buffer::end <size_t> () const noexcept;
	template awh::Buffer::Const_Iterator <size_t> awh::Buffer::cend <size_t> () const noexcept;
	template awh::Buffer::Const_Iterator <ssize_t> awh::Buffer::end <ssize_t> () const noexcept;
	template awh::Buffer::Const_Iterator <ssize_t> awh::Buffer::cend <ssize_t> () const noexcept;
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
awh::Buffer::Const_Iterator <T> awh::Buffer::begin() const noexcept {
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
awh::Buffer::Const_Iterator <T> awh::Buffer::cbegin() const noexcept {
	// Выполняем установку начального значения итератора
	return this->template begin <T> ();
}
/**
 * Объявляем прототипы для метода получения начального константного итератора
 */
template awh::Buffer::Const_Iterator <int8_t> awh::Buffer::begin <int8_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint8_t> awh::Buffer::begin <uint8_t> () const noexcept;
template awh::Buffer::Const_Iterator <int16_t> awh::Buffer::begin <int16_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint16_t> awh::Buffer::begin <uint16_t> () const noexcept;
template awh::Buffer::Const_Iterator <int32_t> awh::Buffer::begin <int32_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint32_t> awh::Buffer::begin <uint32_t> () const noexcept;
template awh::Buffer::Const_Iterator <int64_t> awh::Buffer::begin <int64_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint64_t> awh::Buffer::begin <uint64_t> () const noexcept;
template awh::Buffer::Const_Iterator <float> awh::Buffer::begin <float> () const noexcept;
template awh::Buffer::Const_Iterator <double> awh::Buffer::begin <double> () const noexcept;
/**
 * Объявляем прототипы для метода получения начального константного итератора
 */
template awh::Buffer::Const_Iterator <int8_t> awh::Buffer::cbegin <int8_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint8_t> awh::Buffer::cbegin <uint8_t> () const noexcept;
template awh::Buffer::Const_Iterator <int16_t> awh::Buffer::cbegin <int16_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint16_t> awh::Buffer::cbegin <uint16_t> () const noexcept;
template awh::Buffer::Const_Iterator <int32_t> awh::Buffer::cbegin <int32_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint32_t> awh::Buffer::cbegin <uint32_t> () const noexcept;
template awh::Buffer::Const_Iterator <int64_t> awh::Buffer::cbegin <int64_t> () const noexcept;
template awh::Buffer::Const_Iterator <uint64_t> awh::Buffer::cbegin <uint64_t> () const noexcept;
template awh::Buffer::Const_Iterator <float> awh::Buffer::cbegin <float> () const noexcept;
template awh::Buffer::Const_Iterator <double> awh::Buffer::cbegin <double> () const noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template awh::Buffer::Const_Iterator <char> awh::Buffer::begin <char> () const noexcept;
	template awh::Buffer::Const_Iterator <char> awh::Buffer::cbegin <char> () const noexcept;
#endif
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template awh::Buffer::Const_Iterator <size_t> awh::Buffer::begin <size_t> () const noexcept;
	template awh::Buffer::Const_Iterator <size_t> awh::Buffer::cbegin <size_t> () const noexcept;
	template awh::Buffer::Const_Iterator <ssize_t> awh::Buffer::begin <ssize_t> () const noexcept;
	template awh::Buffer::Const_Iterator <ssize_t> awh::Buffer::cbegin <ssize_t> () const noexcept;
#endif
/**
 * @brief Шаблон для метода удаления верхних записей
 *
 * @tparam T тип данных для удаления
 *
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
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template void awh::Buffer::pop <size_t> () noexcept;
	template void awh::Buffer::pop <ssize_t> () noexcept;
#endif
/**
 * @brief Шаблон для метода получения количества элементов в бинарном буфере
 *
 * @tparam T тип данных для подсчёта
 *
 */
template <typename T>
/**
 * @brief Метод получения количества элементов в бинарном буфере
 *
 * @return количество всех добавленных лементов
 *
 */
size_t awh::Buffer::count() const noexcept {
	// Если мы не дошли до конца
	if(!this->empty())
		// Возвращаем размер данных в буфере
		return (this->size() / sizeof(T));
	// Возвращаем пустое значение
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
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Buffer::count <size_t> () const noexcept;
	template size_t awh::Buffer::count <ssize_t> () const noexcept;
#endif
/**
 * @brief Шаблон для метода извлечения нижнего значения в буфере
 *
 * @tparam T тип данных для извлечения
 *
 */
template <typename T>
/**
 * @brief Метод извлечения нижнего значения в буфере
 *
 * @return данные содержащиеся в буфере
 *
 */
T awh::Buffer::back() const noexcept {
	// Переменная результата
	T result = 0;
	// Если контейнер не пустой
	if(!this->empty()){
		// Получаем размер данных
		const size_t size = sizeof(result);
		// Если данных достаточно в буфере
		if((this->_range.end - this->_range.begin) >= size)
			// Выполняем копирование данных контейнера
			::memcpy(&result, &this->_buffer[0] + (this->_range.end - size), size);
	}
	// Возвращаем результат
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
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Buffer::back() const noexcept;
	template ssize_t awh::Buffer::back() const noexcept;
#endif
/**
 * @brief Шаблон для метода извлечения верхнего значения в буфере
 *
 * @tparam T тип данных для извлечения
 *
 */
template <typename T>
/**
 * @brief Метод извлечения верхнего значения в буфере
 *
 * @return данные содержащиеся в буфере
 *
 */
T awh::Buffer::front() const noexcept {
	// Переменная результата
	T result = 0;
	// Если контейнер не пустой
	if(!this->empty()){
		// Получаем размер данных
		const size_t size = sizeof(result);
		// Если данные есть в буфере
		if((this->_range.end - this->_range.begin) >= size)
			// Выполняем копирование данных контейнера
			::memcpy(&result, &this->_buffer[0] + this->_range.begin, size);
	}
	// Возвращаем результат
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
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Buffer::front() const noexcept;
	template ssize_t awh::Buffer::front() const noexcept;
#endif
/**
 * @brief Шаблон для метода извлечения содержимого контейнера по его индексу
 *
 * @tparam T тип данных для извлечения
 *
 */
template <typename T>
/**
 * @brief Метод извлечения содержимого контейнера по его индексу
 *
 * @param index индекс массива для извлечения
 * @return      данные содержащиеся в буфере
 *
 */
T awh::Buffer::at(const size_t index) const noexcept {
	// Переменная результата
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
			::memcpy(&result, &this->_buffer[0] + offset, size);
		// Если данных нет в буфере
		else this->error(__PRETTY_FUNCTION__, ("There is no data in the buffer at INDEX=" + to_string(index)).c_str(), log_t::flag_t::WARNING);
	}
	// Возвращаем результат
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
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::Buffer::at(const size_t) const noexcept;
	template ssize_t awh::Buffer::at(const size_t) const noexcept;
#endif
/**
 * @brief Шаблон для метода установки значений в уже существующем буфере
 *
 * @tparam T тип данных для установки
 *
 */
template <typename T>
/**
 * @brief Метод установки значений в уже существующем буфере
 *
 * @param value значение для установки
 * @param index индекс значения для установки
 *
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
			::memcpy(&this->_buffer[0] + offset, &value, size);
		// Если данных нет в буфере
		else this->error(__PRETTY_FUNCTION__, ("There is no data in the buffer at INDEX=" + to_string(index)).c_str(), log_t::flag_t::WARNING);
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
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template void awh::Buffer::set(const size_t, const size_t) noexcept;
	template void awh::Buffer::set(const ssize_t, const size_t) noexcept;
#endif
/**
 * @brief Получения данных указанного элемента в буфера
 *
 * @return указатель на элемент буфера
 *
 */
const void * awh::Buffer::data() const noexcept {
	// Если буфер данных не пустой
	if(!this->empty())
		// Возвращаем текущий результат
		return (&this->_buffer[0] + this->_range.begin);
	// Возвращаем значение по умолчанию
	return nullptr;
}
/**
 * @brief Метод удаления указанного количества байт из начала буфера
 *
 * @param size количество байт для удаления
 *
 */
void awh::Buffer::erase(const size_t size) noexcept {
	// Если буфер данных не пустой
	if(!this->empty()){
		// Если удаляется весь объём полезных данных
		if(size >= (this->_range.end - this->_range.begin)){
			// Сбрасываем конец буфера (переиспользуем хранилище с начала)
			this->_range.end = 0;
			// Сбрасываем начало буфера
			this->_range.begin = 0;
		// Иначе смещаем начало буфера на размер удаляемых данных
		} else this->_range.begin += size;
	}
}
/**
 * @brief Метод извлечения (удаления) указанного количества уже обработанных байт из начала буфера
 *
 * @param size количество байт для извлечения
 *
 */
void awh::Buffer::consume(const size_t size) noexcept {
	// Выполняем удаление указанного количества байт из начала буфера
	this->erase(size);
}
/**
 * @brief Метод подготовки места в хвосте буфера для прямой записи (zero-copy)
 *
 * @details После записи данных по полученному указателю необходимо вызвать commit(n).
 *          Указатель действителен только до следующей модификации буфера.
 *          Для безопасной работы рекомендуется использовать метод write().
 *
 * @param size требуемое количество свободных байт в хвосте буфера
 * @return     указатель на начало свободной области либо nullptr при ошибке
 *
 */
void * awh::Buffer::prepare(const size_t size) noexcept {
	// Если размер выделения не передан
	if(size == 0)
		// Возвращаем пустое значение
		return nullptr;
	// Если не удалось выделить запрошенное количество памяти
	if(!this->rss(size)){
		// Формируем сообщение об ошибке
		string message = "There is not enough memory in the reserved buffer to add a new portion of data";
		// Если объект фреймворка установлен
		if(this->_fmk != nullptr)
			// Формируем подробное сообщение об ошибке
			message = "You are trying to map " + this->_fmk->bytes(static_cast <double> (this->size() + size)) +
			          " of data into a " + this->_fmk->bytes(static_cast <double> (this->_range.maxMemory)) + " data buffer, which is impossible";
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, message.c_str());
		// Возвращаем пустое значение
		return nullptr;
	}
	// Запоминаем зарезервированный объём для контроля последующей фиксации
	this->_range.reserved = size;
	// Возвращаем указатель на начало свободной области в хвосте буфера
	return (&this->_buffer[0] + this->_range.end);
}
/**
 * @brief Метод фиксации записанных в хвост буфера данных (zero-copy)
 *
 * @param size количество фактически записанных в хвост байт
 * @return     количество зафиксированных байт
 *
 */
size_t awh::Buffer::commit(const size_t size) noexcept {
	// Определяем количество свободного места в хвосте буфера
	const size_t tail = ((this->_buffer.size() > this->_range.end) ? (this->_buffer.size() - this->_range.end) : 0);
	/**
	 * Ограничиваем количество фиксируемых данных зарезервированным объёмом: хранилище
	 * растёт с запасом, и свободного места в хвосте больше, чем было запрошено, а
	 * фиксация сверх резервирования протащила бы в буфер неинициализированные байты
	 */
	size_t count = ((size < this->_range.reserved) ? size : this->_range.reserved);
	// Дополнительно ограничиваем количество фиксируемых данных доступным местом
	count = ((count < tail) ? count : tail);
	// Увеличиваем смещение конца данных буфера
	this->_range.end += count;
	// Сбрасываем зарезервированный объём (резервирование израсходовано)
	this->_range.reserved = 0;
	// Возвращаем количество зафиксированных данных
	return count;
}
/**
 * @brief Метод получения RAII-обёртки для безопасной прямой записи в хвост буфера (zero-copy)
 *
 * @param size требуемое количество свободных байт в хвосте буфера
 * @return     объект записи с автоматической фиксацией данных
 *
 */
awh::Buffer::Writer awh::Buffer::write(const size_t size) noexcept {
	// Выполняем подготовку места в хвосте буфера
	void * data = this->prepare(size);
	// Возвращаем объект записи с автоматической фиксацией данных
	return Writer(this, data, ((data != nullptr) ? size : 0));
}
/**
 * @brief Метод резервирования размера буфера
 *
 * @param size размер выделяемой памяти
 *
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
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Шаблон для добавления числа в буфер
 *
 * @tparam T тип данных для добавления
 *
 */
template <typename T>
/**
 * @brief Метод добавления числа в буфер
 *
 * @param value значение для добавления
 * @return       результат добавления данных
 *
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
 * Если операционной системой является macOS или Linux
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
 *
 */
bool awh::Buffer::push(const char * text) noexcept {
	// Если текст передан не пустой
	if(text != nullptr)
		// Выполняем добавление текста
		return this->push(text, ::strlen(text));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод добавления текста в буфер
 *
 * @param text текст для добавления
 * @return     результат добавления данных
 *
 */
bool awh::Buffer::push(string_view text) noexcept {
	// Если текст передан не пустой
	if(!text.empty())
		// Выполняем добавление текста
		return this->push(text.data(), text.size());
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод добавления текста в буфер
 *
 * @param text текст для добавления
 * @return     результат добавления данных
 *
 */
bool awh::Buffer::push(const string & text) noexcept {
	// Если текст передан не пустой
	if(!text.empty())
		// Выполняем добавление текста
		return this->push(text.c_str(), text.length());
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод добавления бинарного буфера данных в буфер
 *
 * @details Если текущий буфер пуст — выполняется перемещение хранилища (zero-copy),
 *          иначе данные дописываются в хвост.
 *
 * @param buffer бинарный буфер для добавления
 * @return       результат добавления данных
 *
 */
bool awh::Buffer::push(buffer_t && buffer) noexcept {
	// Если сторонний буфер пустой
	if(buffer.empty())
		// Возвращаем значение по умолчанию
		return false;
	// Если текущий буфер пустой — выполняем перемещение хранилища
	if(this->empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем перемещение буфера данных
			this->_buffer = ::move(buffer._buffer);
			// Копируем последний итератор
			this->_range.end = buffer._range.end;
			// Копируем начальный итератор
			this->_range.begin = buffer._range.begin;
			// Сбрасываем последний итератор стороннего буфера
			buffer._range.end = 0;
			// Сбрасываем начальный итератор стороннего буфера
			buffer._range.begin = 0;
			// Возвращаем результат
			return true;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Записываем ошибку в лог
			this->error(__PRETTY_FUNCTION__, error.what());
			// Возвращаем значение по умолчанию
			return false;
		}
	}
	// Дописываем данные стороннего буфера в хвост текущего
	const bool result = this->push(static_cast <const char *> (buffer), static_cast <size_t> (buffer));
	// Если данные успешно добавлены
	if(result)
		// Очищаем сторонний буфер
		buffer.clear();
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод добавления бинарного буфера данных в буфер
 *
 * @param buffer бинарный буфер для добавления
 * @return       результат добавления данных
 *
 */
bool awh::Buffer::push(const buffer_t & buffer) noexcept {
	// Если буфер данных передан не пустой
	if(!buffer.empty())
		// Выполняем добавление бинарного буфера данных
		return this->push(static_cast <const uint8_t *> (buffer), static_cast <size_t> (buffer));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод добавления бинарного буфера данных в буфер
 *
 * @param buffer бинарный буфер для добавления
 * @return       результат добавления данных
 *
 */
bool awh::Buffer::push(const vector <uint8_t> & buffer) noexcept {
	// Если буфер данных передан не пустой
	if(!buffer.empty())
		// Выполняем добавление бинарного буфера данных
		return this->push(&buffer[0], buffer.size());
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод добавления бинарного буфера данных в буфер
 *
 * @param buffer бинарный буфер для добавления
 * @param size   размер бинарного буфера
 * @return       результат добавления данных
 *
 */
bool awh::Buffer::push(const void * buffer, const size_t size) noexcept {
	// Если данные переданы неверные
	if((buffer == nullptr) || (size == 0))
		// Возвращаем значение по умолчанию
		return false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем выделение памяти под новую порцию данных
		if(!this->prepare(size))
			// Сообщаем что данные добавить не удалось (существующие данные сохраняются)
			return false;
		// Выполняем добавление самих данных полезной нагрузки
		::memcpy(&this->_buffer[0] + this->_range.end, buffer, size);
		// Увеличиваем смещение конца данных буфера
		this->_range.end += size;
		// Сообщаем об успешном добавлении данных
		return true;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки максимального размера потребления памяти
 *
 * @param size максимальный размер потребления памяти
 *
 */
void awh::Buffer::setMaxMemory(const size_t size) noexcept {
	// Если максимальный размер потребляемой памяти передан
	if(size > 0)
		// Выполняем установку максимального размера потребляемой памяти
		this->_range.maxMemory = size;
}
/**
 * @brief Метод обмена очередями
 *
 * @param buffer бинарный буфер для обмена
 *
 */
void awh::Buffer::swap(Buffer & buffer) noexcept {
	// Если объект фреймворка установлен у стороннего буфера
	if((buffer._fmk != nullptr) && (this->_fmk == nullptr))
		// Копируем объект фреймворка
		this->_fmk = buffer._fmk;
	// Если объект для работы с логами установлен у стороннего буфера
	if((buffer._log != nullptr) && (this->_log == nullptr))
		// Копируем объект для работы с логами
		this->_log = buffer._log;
	// Если объект фреймворка установлен у текущего буфера
	if((this->_fmk != nullptr) && (buffer._fmk == nullptr))
		// Копируем объект фреймворка
		buffer._fmk = this->_fmk;
	// Если объект для работы с логами установлен у текущего буфера
	if((this->_log != nullptr) && (buffer._log == nullptr))
		// Копируем объект для работы с логами
		buffer._log = this->_log;
	// Выполняем обмен буферами данных
	this->_buffer.swap(buffer._buffer);
	/**
	 * Выполняем обмен диапазонами записей
	 *
	 * @note Обмен ведётся структурой целиком, и все её поля меняются местами разом,
	 *       включая предельный размер памяти. Обменивать его вслед отдельно не нужно -
	 *       так он вернулся бы на прежнее место, - да и нельзя: ссылку на поле
	 *       упакованной структуры взять не даёт компилятор
	 */
	::swap(this->_range, buffer._range);
}
/**
 * @brief Метод установки объекта логирования
 *
 * @param log объект работы с логами
 *
 */
void awh::Buffer::setLogger(const log_t * log) noexcept {
	// Выполняем установку объекта логирования
	this->_log = log;
}
/**
 * @brief Получения размера данных в буфера
 *
 * @return размер данных в буфера
 *
 */
awh::Buffer::operator size_t() const noexcept {
	// Возвращаем размер буфера
	return this->size();
}
/**
 * @brief Получения бинарных данных буфера
 *
 * @return бинарные данные буфера
 *
 */
awh::Buffer::operator const char * () const noexcept {
	// Возвращаем буфер данных
	return reinterpret_cast <const char *> (this->data());
}
/**
 * @brief Получения бинарных данных буфера
 *
 * @return бинарные данные буфера
 *
 */
awh::Buffer::operator const uint8_t * () const noexcept {
	// Возвращаем буфер данных
	return reinterpret_cast <const uint8_t *> (this->data());
}
/**
 * @brief Получения бинарных данных буфера
 *
 * @return бинарные данные буфера
 *
 */
awh::Buffer::operator const vector <uint8_t> & () const noexcept {
	// Возвращаем результат
	return this->raw();
}
/**
 * @brief Оператор копирования
 *
 * @param buffer бинарный буфер для копирования
 * @return       текущий контейнер буфера
 *
 */
awh::Buffer & awh::Buffer::operator = (const char * buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Сбрасываем начальный итератор
		this->_range.begin = 0;
		// Если строка для копирования не передана
		if(buffer == nullptr){
			// Сбрасываем последний итератор
			this->_range.end = 0;
			// Очищаем буфер данных
			this->_buffer.clear();
		// Если строка для копирования передана
		} else {
			// Устанавливаем последний итератор
			this->_range.end = ::strlen(buffer);
			// Копируем данные из строки в буфер
			this->_buffer.assign(buffer, buffer + this->_range.end);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор перемещения
 *
 * @param buffer бинарный буфер для перемещения
 * @return       текущий контейнер буфера
 *
 */
awh::Buffer & awh::Buffer::operator = (string && buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Сбрасываем начальный итератор
		this->_range.begin = 0;
		// Устанавливаем последний итератор
		this->_range.end = buffer.length();
		// Копируем данные из строки в буфер
		this->_buffer.assign(buffer.begin(), buffer.end());
		// Очищаем исходную строку
		buffer.clear();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param buffer бинарный буфер для копирования
 * @return       текущий контейнер буфера
 *
 */
awh::Buffer & awh::Buffer::operator = (const string & buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Сбрасываем начальный итератор
		this->_range.begin = 0;
		// Устанавливаем последний итератор
		this->_range.end = buffer.length();
		// Копируем данные из строки в буфер
		this->_buffer.assign(buffer.begin(), buffer.end());
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор перемещения
 *
 * @param buffer бинарный буфер для перемещения
 * @return       текущий контейнер буфера
 *
 */
awh::Buffer & awh::Buffer::operator = (vector <uint8_t> && buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Сбрасываем начальный итератор
		this->_range.begin = 0;
		// Устанавливаем последний итератор
		this->_range.end = buffer.size();
		// Выполняем перемещение буфера данных
		this->_buffer = ::move(buffer);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param buffer бинарный буфер для копирования
 * @return       текущий контейнер буфера
 *
 */
awh::Buffer & awh::Buffer::operator = (const vector <uint8_t> & buffer) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Сбрасываем начальный итератор
		this->_range.begin = 0;
		// Устанавливаем последний итератор
		this->_range.end = buffer.size();
		// Копируем данные из буфера
		this->_buffer.assign(buffer.begin(), buffer.end());
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор перемещения
 *
 * @param buffer бинарный буфер для перемещения
 * @return       текущий контейнер буфера
 *
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
			// Копируем объект для работы с логами
			this->_log = buffer._log;
		// Выполняем перемещение буфера данных
		this->_buffer = ::move(buffer._buffer);
		// Копируем последний итератор
		this->_range.end = buffer._range.end;
		// Копируем начальный итератор
		this->_range.begin = buffer._range.begin;
		// Копируем максимальный размер памяти
		this->_range.maxMemory = buffer._range.maxMemory;
		// Сбрасываем последний итератор стороннего буфера
		buffer._range.end = 0;
		// Сбрасываем начальный итератор стороннего буфера
		buffer._range.begin = 0;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param buffer бинарный буфер для копирования
 * @return       текущий контейнер буфера
 *
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
			// Копируем объект для работы с логами
			this->_log = buffer._log;
		// Копируем последний итератор
		this->_range.end = buffer._range.end;
		// Копируем начальный итератор
		this->_range.begin = buffer._range.begin;
		// Копируем максимальный размер памяти
		this->_range.maxMemory = buffer._range.maxMemory;
		// Копируем данные буфера
		this->_buffer.assign(buffer._buffer.begin(), buffer._buffer.end());
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор сравнения двух очередей
 *
 * @param buffer бинарный буфер для сравнения
 * @return       результат сравнения
 *
 */
bool awh::Buffer::operator == (const buffer_t & buffer) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Получаем размер полезных данных текущего буфера
		const size_t size = this->size();
		// Если размеры полезных данных не совпадают
		if(size != buffer.size())
			// Сообщаем что буферы не равны
			return false;
		// Если оба буфера пустые
		if(size == 0)
			// Сообщаем что буферы равны
			return true;
		// Сравниваем полезные данные буферов
		return (::memcmp(&this->_buffer[0] + this->_range.begin, &buffer._buffer[0] + buffer._range.begin, size) == 0);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем результат
	return false;
}
/**
 * @brief Разрешаем пустое значение объекта
 *
 */
awh::Buffer::Buffer() noexcept : _fmk(nullptr), _log(nullptr) {}
/**
 * @brief Конструктор перемещения
 *
 * @param buffer бинарный буфер для перемещения
 *
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
			// Копируем объект для работы с логами
			this->_log = buffer._log;
		// Выполняем перемещение буфера данных
		this->_buffer = ::move(buffer._buffer);
		// Копируем последний итератор
		this->_range.end = buffer._range.end;
		// Копируем начальный итератор
		this->_range.begin = buffer._range.begin;
		// Копируем максимальный размер памяти
		this->_range.maxMemory = buffer._range.maxMemory;
		// Сбрасываем последний итератор стороннего буфера
		buffer._range.end = 0;
		// Сбрасываем начальный итератор стороннего буфера
		buffer._range.begin = 0;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Конструктор копирования
 *
 * @param buffer бинарный буфер для копирования
 *
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
			// Копируем объект для работы с логами
			this->_log = buffer._log;
		// Копируем последний итератор
		this->_range.end = buffer._range.end;
		// Копируем начальный итератор
		this->_range.begin = buffer._range.begin;
		// Копируем максимальный размер памяти
		this->_range.maxMemory = buffer._range.maxMemory;
		// Копируем данные буфера
		this->_buffer.assign(buffer._buffer.begin(), buffer._buffer.end());
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Buffer::Buffer(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
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
 *
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
	// Возвращаем результат
	return is;
}
/**
 * @brief Оператор [<<] вывода в поток буфера
 *
 * @param os     поток куда нужно вывести данные
 * @param buffer буфер извлечения
 *
 */
ostream & awh::operator << (ostream & os, const buffer_t & buffer) noexcept {
	// Получаем размер полезных данных буфера
	const size_t size = static_cast <size_t> (buffer);
	// Если в буфере есть данные
	if(size > 0)
		// Записываем в поток данные буфера
		os.write(static_cast <const char *> (buffer), static_cast <streamsize> (size));
	// Возвращаем результат
	return os;
}
