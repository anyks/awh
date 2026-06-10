/**
 * @file: queue.cpp
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
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <algorithm>

/**
 * Подключаем заголовочный файл
 */
#include <sys/queue.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод контроля памяти
 *
 * @param size желаемый размер выделения памяти
 * @return     результат выполнения операции
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
						const size_t size = (this->_range.end - this->_range.begin);
						// Если нам есть чего перемещать
						if(size > 0)
							// Выполняем перемещение верхней границы памяти
							::memmove(&this->_buffer[0], &this->_buffer[this->_range.begin], size);
						// Выполняем смещение верхней границы буфера
						this->_range.begin = 0;
						// Выполняем смещение нижней границы буфера
						this->_range.end = size;
						// Определяем количество свободного места в буфере
						available = (this->_buffer.size() - this->_range.end);
					}
					// Если в буфере больше нет места для добавления данных
					if(!(result = (available >= bytes))){
						// Если при добавлении новых данных мы не переходим через лимит
						if((result = ((this->_buffer.size() + (bytes - available)) < this->_max.memory)))
							// Выделяем ещё памяти
							this->_buffer.resize(this->_buffer.size() + (bytes - available), 0);
						// Записываем ошибку в лог
						else if(this->_log != nullptr) {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug(
									"You are trying to map %s of data into a %s data buffer, which is impossible",
									__PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::CRITICAL,
									this->_fmk->bytes(static_cast <double> (this->_buffer.size() + (bytes - available))).c_str(),
									this->_fmk->bytes(static_cast <double> (this->_max.memory)).c_str()
								);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print(
									"You are trying to map %s of data into a %s data buffer, which is impossible",
									log_t::flag_t::CRITICAL,
									this->_fmk->bytes(static_cast <double> (this->_buffer.size() + (bytes - available))).c_str(),
									this->_fmk->bytes(static_cast <double> (this->_max.memory)).c_str()
								);
							#endif
						// Если объект логирования не установлен
						} else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, "There is not enough memory in the reserved queue to add a new portion of data");
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								::fprintf(stderr, "ERROR! %s\n\n", "There is not enough memory in the reserved queue to add a new portion of data");
							#endif
						}
					}
				}
			// Если буфер данных пустой
			} else if((result = (bytes <= this->_max.memory)))
				// Увеличиваем размер вектора
				this->_buffer.resize(bytes, 0);
			// Записываем ошибку в лог
			else if(this->_log != nullptr) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug(
						"You are trying to map %s of data into a %s data buffer, which is impossible",
						__PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::CRITICAL,
						this->_fmk->bytes(static_cast <double> (bytes)).c_str(),
						this->_fmk->bytes(static_cast <double> (this->_max.memory)).c_str()
					);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print(
						"You are trying to map %s of data into a %s data buffer, which is impossible",
						log_t::flag_t::CRITICAL, this->_fmk->bytes(static_cast <double> (bytes)).c_str(),
						this->_fmk->bytes(static_cast <double> (this->_max.memory)).c_str()
					);
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, "There is not enough memory in the reserved queue to add a new portion of data");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", "There is not enough memory in the reserved queue to add a new portion of data");
				#endif
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод удаления записи в очереди
 *
 */
void awh::Queue::pop() noexcept {
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_range.count > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Размер верхней записи в очереди
			size_t size = 0;
			// Сбрасываем смещение чтения данных
			this->_offset = 0;
			// Извлекаем текущее значение размера записи
			::memcpy(&size, &this->_buffer[this->_range.begin], sizeof(size));
			// Если размер записи получен
			if(size > 0){
				// Уменьшаем количество записей в очереди
				this->_range.count--;
				// Увеличиваем смещение
				this->_range.begin += (size + sizeof(size));
				// Если мы извлекли все данные очереди
				if(this->_range.begin == this->_range.end){
					// Выполняем сброс конца очереди
					this->_range.end = 0;
					// Выполняем сброс начала очереди
					this->_range.begin = 0;
					// Выполняем сброс количества записей в очереди
					this->_range.count = 0;
					// Выполняем зануление всего буфера данных
					::memset(&this->_buffer[0], 0, this->_buffer.size());
				}
			// Записываем ошибку в лог
			} else if(this->_log != nullptr) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Queue data buffer is corrupted");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Queue data buffer is corrupted");
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, "Queue data buffer is corrupted");
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", "Queue data buffer is corrupted");
				#endif
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
}
/**
 * @brief Метод очистки всех данных очереди
 *
 */
void awh::Queue::clear() noexcept {
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_range.count > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Сбрасываем смещение чтения данных
			this->_offset = 0;
			// Выполняем сброс конца очереди
			this->_range.end = 0;
			// Выполняем сброс начала очереди
			this->_range.begin = 0;
			// Выполняем сброс количества записей в очереди
			this->_range.count = 0;
			// Выполняем зануление всего буфера данных
			::memset(&this->_buffer[0], 0, this->_buffer.size());
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
}
/**
 * @brief Метод полной очистки памяти
 *
 */
void awh::Queue::reset() noexcept {
	// Если буфер данных не пустой
	if(!this->_buffer.empty()){
		// Выполняем очистку буфера данных
		this->clear();
		// Выполняем освобождение памяти
		vector <decltype(this->_buffer)::value_type> ().swap(this->_buffer);
	}
}
/**
 * @brief Количество добавленных элементов
 *
 * @return количество добавленных элементов
 */
size_t awh::Queue::count() const noexcept {
	// Возвращаем количество записей в очереди
	return this->_range.count;
}
/**
 * @brief Метод получения размера добавленных данных
 *
 * @return размер добавленных данных
 */
size_t awh::Queue::size() const noexcept {
	// Переменная результата
	size_t result = 0;
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_range.count > 0)){
		// Извлекаем текущее значение размера записи
		::memcpy(&result, &this->_buffer[this->_range.begin], sizeof(result));
		// Вычитаем смещение чтения данных
		result -= this->_offset;
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод вывода размера занимаемой памяти очередью
 *
 * @return количество памяти которую занимает очередь
 */
size_t awh::Queue::capacity() const noexcept {
	// Возвращаем размер занимаемой памяти
	return this->_buffer.capacity();
}
/**
 * @brief Получения данных указанного элемента в очереди
 *
 * @return указатель на элемент очереди
 */
const void * awh::Queue::data() const noexcept {
	// Переменная результата
	const void * result = nullptr;
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_range.count > 0))
		// Возвращаем данные записи в бинарном виде
		result = (&this->_buffer[0] + this->_range.begin + sizeof(size_t) + this->_offset);
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод фиксации прочитанного размера данных
 *
 * @param size размер данных для фиксации
 */
void awh::Queue::commit(const size_t size) noexcept {
	// Выполняем фиксацию смещения
	this->_offset += size;
}
/**
 * @brief Метод проверки на заполненность очереди
 *
 * @param timeout время ожидания в миллисекундах
 * @return        результат проверки
 */
bool awh::Queue::empty(const uint32_t timeout) const noexcept {
	// Проверяем пустая ли очередь в данный момент
	return (this->_range.count == 0);
}
/**
 * @brief Метод добавления бинарного буфера данных в очередь
 *
 * @param buffer бинарный буфер для добавления
 * @param size   размер бинарного буфера
 * @return       текущий размер очереди
 */
size_t awh::Queue::push(const void * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем выделение памяти
			if(this->rss(size)){
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return this->_range.count;
}
/**
 * @brief Метод добавления бинарного буфера данных в очередь
 *
 * @param records список бинарных буферов для добавления
 * @param size    общий размер добавляемых данных
 * @return        текущий размер очереди
 */
size_t awh::Queue::push(const vector <record_t> & records, const size_t size) noexcept {
	// Если данные переданы верные
	if(!records.empty() && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем выделение памяти
			if(this->rss(size)){
				// Увеличиваем количество записей в очереди
				this->_range.count++;
				// Выполняем запись данных в буфер
				::memcpy(&this->_buffer[this->_range.end], reinterpret_cast <const uint8_t *> (&size), sizeof(size));
				// Увеличиваем смещение конца данных буфера
				this->_range.end += sizeof(size);
				// Выполняем перебор всех записей
				for(auto & record : records){
					// Выполняем добавление самих данных полезной нагрузки
					::memcpy(&this->_buffer[this->_range.end], record.first, record.second);
					// Увеличиваем смещение конца данных буфера
					this->_range.end += record.second;
				}
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
					// Записываем ошибку в лог
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(size), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			// Если объект логирования не установлен
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					::fprintf(stderr, "ERROR! %s\n\n", error.what());
				#endif
			}
		}
	}
	// Возвращаем результат
	return this->_range.count;
}
/**
 * @brief Метод установки максимального размера потребления памяти
 *
 * @param size максимальный размер потребления памяти
 */
void awh::Queue::setMaxMemory(const size_t size) noexcept {
	// Если максимальный размер потребляемой памяти передан
	if(size > 0)
		// Выполняем установку максимального размера потребляемой памяти
		this->_max.memory = size;
}
/**
 * @brief Метод установки максимального количества записей очереди
 *
 * @param count максимальное количество записей очереди
 */
void awh::Queue::setMaxRecords(const size_t count) noexcept {
	// Если количество записей передано
	if(count > 0)
		// Выполняем установку максимального количества сообщений очереди
		this->_max.records = count;
}
/**
 * @brief Метод обмена очередями
 *
 * @param queue очередь для обмена
 */
void awh::Queue::swap(queue_t & queue) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
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
		// Выполняем обмен смещениями чтения данных
		this->_offset += (queue._offset - (queue._offset = this->_offset));
		// Выполняем обмен последними итераторами
		this->_range.end += (queue._range.end - (queue._range.end = this->_range.end));
		// Выполняем обмен начальными итераторами
		this->_range.begin += (queue._range.begin - (queue._range.begin = this->_range.begin));
		// Выполняем обмен количествами добавленных записями
		this->_range.count += (queue._range.count - (queue._range.count = this->_range.count));
		// Выполняем обмен максимальными размерами памяти
		this->_max.memory += (queue._max.memory - (queue._max.memory = this->_max.memory));
		// Выполняем обмен максимальными количествами записей
		this->_max.records += (queue._max.records - (queue._max.records = this->_max.records));
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
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
void awh::Queue::setLogger(const log_t * log) noexcept {
	// Устанавливаем объект логирования
	this->_log = log;
}
/**
 * @brief Получения размера данных в очереди
 *
 * @return размер данных в очереди
 */
awh::Queue::operator size_t() const noexcept {
	// Возвращаем размер очереди
	return this->size();
}
/**
 * @brief Получения бинарных данных очереди
 *
 * @return бинарные данные очереди
 */
awh::Queue::operator const char * () const noexcept {
	// Возвращаем данные записи очереди
	return reinterpret_cast <const char *> (this->data());
}
/**
 * @brief Оператор перемещения
 *
 * @param queue очередь для перемещения
 * @return      текущий контейнер очереди
 */
awh::Queue & awh::Queue::operator = (queue_t && queue) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект фреймворка установлен
		if((queue._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = queue._fmk;
		// Если объект для работы с логами установлен
		if((queue._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = queue._log;
		// Выполняем установку смещения чтения данных
		this->_offset = queue._offset;
		// Выполняем перемещение буфера данных
		this->_buffer = ::move(queue._buffer);
		// Выполняем копирование последнего итератора
		this->_range.end = queue._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = queue._range.begin;
		// Выполняем копирование количества добавленных записей
		this->_range.count = queue._range.count;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
		// Выполняем сброс смещения чтения данных сторонней очереди
		queue._offset = 0;
		// Выполняем сброс последнего итератора сторонней очереди
		queue._range.end = 0;
		// Выполняем сброс начального итератора сторонней очереди
		queue._range.begin = 0;
		// Выполняем сброс количества добавленных записей сторонней очереди
		queue._range.count = 0;
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param queue очередь для копирования
 * @return      текущий контейнер очереди
 */
awh::Queue & awh::Queue::operator = (const queue_t & queue) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект фреймворка установлен
		if((queue._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = queue._fmk;
		// Если объект для работы с логами установлен
		if((queue._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = queue._log;
		// Выполняем установку смещения чтения данных
		this->_offset = queue._offset;
		// Выполняем копирование последнего итератора
		this->_range.end = queue._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = queue._range.begin;
		// Выполняем копирование количества добавленных записей
		this->_range.count = queue._range.count;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
		// Выполняем копирование буфера данных
		this->_buffer.assign(queue._buffer.begin(), queue._buffer.end());
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем текущий объект
	return (* this);
}
/**
 * @brief Оператор сравнения двух очередей
 *
 * @param queue очередь для сравнения
 * @return      результат сравнения
 */
bool awh::Queue::operator == (const queue_t & queue) const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Выполняем сравнения всей внутренней составляющей
		 */
		return (
			(this->_offset == queue._offset) &&
			(this->_range.end == queue._range.end) &&
			(this->_range.begin == queue._range.begin) &&
			(this->_range.count == queue._range.count) &&
			(this->_buffer.size() == queue._buffer.size()) &&
			(::memcmp(&this->_buffer[0], &queue._buffer[0], this->_buffer.size()) == 0)
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
	// Возвращаем результат
	return false;
}
/**
 * @brief Конструктор перемещения
 *
 * @param queue очередь для перемещения
 */
awh::Queue::Queue(queue_t && queue) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект фреймворка установлен
		if((queue._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = queue._fmk;
		// Если объект для работы с логами установлен
		if((queue._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = queue._log;
		// Выполняем перемен буферами данных
		this->_buffer = ::move(queue._buffer);
		// Выполняем установку смещения чтения данных
		this->_offset = queue._offset;
		// Выполняем копирование последнего итератора
		this->_range.end = queue._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = queue._range.begin;
		// Выполняем копирование количества добавленных записей
		this->_range.count = queue._range.count;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
		// Выполняем сброс смещения чтения данных сторонней очереди
		queue._offset = 0;
		// Выполняем сброс последнего итератора сторонней очереди
		queue._range.end = 0;
		// Выполняем сброс начального итератора сторонней очереди
		queue._range.begin = 0;
		// Выполняем сброс количества добавленных записей сторонней очереди
		queue._range.count = 0;
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! %s\n\n", error.what());
			#endif
		}
	}
}
/**
 * @brief Конструктор копирования
 *
 * @param queue очередь для копирования
 */
awh::Queue::Queue(const queue_t & queue) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект фреймворка установлен
		if((queue._fmk != nullptr) && (this->_fmk == nullptr))
			// Копируем объект фреймворка
			this->_fmk = queue._fmk;
		// Если объект для работы с логами установлен
		if((queue._log != nullptr) && (this->_log == nullptr))
			// Копируем объект для работы с логами установлен
			this->_log = queue._log;
		// Выполняем перемен буферами данных
		this->_buffer = queue._buffer;
		// Выполняем установку смещения чтения данных
		this->_offset = queue._offset;
		// Выполняем копирование последнего итератора
		this->_range.end = queue._range.end;
		// Выполняем копирование начального итератора
		this->_range.begin = queue._range.begin;
		// Выполняем копирование количества добавленных записей
		this->_range.count = queue._range.count;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		// Если объект логирования не установлен
		} else {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				::fprintf(stderr, "ERROR! Called function:\n%s\n\nMessage:\n%s\n\n", __PRETTY_FUNCTION__, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
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
awh::Queue::Queue(const fmk_t * fmk, const log_t * log) noexcept : _offset(0), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Queue::~Queue() noexcept {}
