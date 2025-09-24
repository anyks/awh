/**
 * @file: queue.cpp
 * @date: 2025-09-24
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
 * Если используется компилятор Borland C++
 */
#if __BORLANDC__
	typedef unsigned char uint8_t;
	typedef __int64 int64_t;
	typedef unsigned long uintptr_t;
/**
 * Если используется компилятор Microsoft Visual Studio
 */
#elif _MSC_VER
	typedef unsigned char uint8_t;
	typedef __int64 int64_t;
/**
 * Если используется компилятор GNU GCC или Clang
 */
#else
	/**
	 * Подключаем стандартные заголовки
	 */
	#include <cstdint>
#endif

/**
 * Подключаем стандартные заголовки
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
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод контроля памяти
 * 
 * @param size желаемый размер выделения памяти
 * @return     результат выполнения операции
 */
bool awh::Queue::rss(const size_t size) noexcept {
	// Результат работы функции
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
				// Если для записи в буфере ещё есть место
				if(this->_buffer.size() > this->_iter.end){
					// Определяем количество свободного места в буфере
					const size_t available = (this->_buffer.size() - this->_iter.end);
					// Если в буфере больше нет места для добавления данных
					if(!(result = (available >= bytes))){
						// Если при добавлении новых данных мы не переходим через лимит
						if((result = ((this->_buffer.size() + (bytes - available)) < this->_max.memory)))
							// Выделяем ещё памяти
							this->_buffer.resize(this->_buffer.size() + (bytes - available), 0);
					}
				// Если мы не выходим за лимиты, выделяем ещё памяти
				} else if((result = ((this->_buffer.capacity() + bytes) <= this->_max.memory)))
					// Выделяем ещё памяти
					this->_buffer.resize(this->_buffer.size() + bytes, 0);
			// Если буфер данных пустой
			} else if((result = (bytes < this->_max.memory)))
				// Увеличиваем размер вектора
				this->_buffer.resize(bytes, 0);
			// Выводим сообщение об ошибке
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("You are trying to map %s of data into a %s data buffer, which is impossible", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_fmk->bytes(static_cast <double> (bytes)).c_str(), this->_fmk->bytes(static_cast <double> (this->_max.memory)).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("You are trying to map %s of data into a %s data buffer, which is impossible", log_t::flag_t::CRITICAL, this->_fmk->bytes(static_cast <double> (bytes)).c_str(), this->_fmk->bytes(static_cast <double> (this->_max.memory)).c_str());
				#endif
			}
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод удаления записи в очереди
 *
 */
void awh::Queue::pop() noexcept {
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_iter.count > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			std::unique_lock lock(this->_mtx);
			// Размер верхней записи в очереди
			size_t size = 0;
			// Извлекаем текущее значение размера записи
			::memcpy(&size, this->_buffer.data() + this->_iter.begin, sizeof(size));
			// Если размер записи получен
			if(size > 0){
				// Уменьшаем количество записей в очереди
				this->_iter.count--;
				// Увеличиваем смещение
				this->_iter.begin += (size + sizeof(size));
				// Если мы извлекли все данные очереди
				if(this->_iter.begin == this->_iter.end){
					// Выполняем сброс конца очереди
					this->_iter.end = 0;
					// Выполняем сброс начала очереди
					this->_iter.begin = 0;
					// Выполняем сброс количества записей в очереди
					this->_iter.count = 0;
					// Выполняем зануление всего буфера данных
					::memset(this->_buffer.data(), 0, this->_buffer.size());
					// Отправляем сообщение о готовности для записи
					this->_cv.write.notify_all();
				}
			// Выводим сообщение об ошибке
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Queue data buffer is corrupted");
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, "Queue data buffer is corrupted");
				#endif
			}
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
}
/**
 * @brief Метод очистки всех данных очереди
 *
 */
void awh::Queue::clear() noexcept {
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_iter.count > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			std::unique_lock lock(this->_mtx);
			// Выполняем сброс конца очереди
			this->_iter.end = 0;
			// Выполняем сброс начала очереди
			this->_iter.begin = 0;
			// Выполняем сброс количества записей в очереди
			this->_iter.count = 0;
			// Выполняем зануление всего буфера данных
			::memset(this->_buffer.data(), 0, this->_buffer.size());
			// Отправляем сообщение о готовности для записи
			this->_cv.write.notify_all();
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
}
/**
 * @brief Метод полной очистки памяти
 * 
 */
void awh::Queue::reset() noexcept {
	// Если буфер данных не пустой
	if(!this->_buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем очистку буфера данных
			this->clear();
			// Выполняем блокировку потока
			std::unique_lock lock(this->_mtx);
			// Выполняем освобождение памяти
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
}
/**
 * @brief Количество добавленных элементов
 *
 * @return количество добавленных элементов
 */
size_t awh::Queue::count() const noexcept {
	// Выводим количество записей в очереди
	return this->_iter.count;
}
/**
 * @brief Метод получения размера добавленных данных
 *
 * @return размер всех добавленных данных
 */
size_t awh::Queue::size() const noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_iter.count > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Извлекаем текущее значение размера записи
			::memcpy(&result, this->_buffer.data() + this->_iter.begin, sizeof(result));
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод вывода размера занимаемой памяти очередью
 * 
 * @return количество памяти которую занимает очередь
 */
size_t awh::Queue::capacity() const noexcept {
	// Выводим размер занимаемой памяти
	return this->_buffer.capacity();
}
/**
 * @brief Получения данных указанного элемента в очереди
 *
 * @return указатель на элемент очереди
 */
const void * awh::Queue::data() const noexcept {
	// Результат работы функции
	const void * result = nullptr;
	// Если буфер данных не пустой и записи есть
	if(!this->_buffer.empty() && (this->_iter.count > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выводим данные записи в бинарном виде
			result = (this->_buffer.data() + this->_iter.begin + sizeof(size_t));
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки на заполненность очереди
 *
 * @param timeout время ожидания в миллисекундах
 * @return        результат проверки
 */
bool awh::Queue::empty(const uint32_t timeout) const noexcept {
	// Результат работы функции
	bool result = true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если очередь завершила работу
		if(this->_terminate)
			// Сообщаем, что очередь пустая
			return result;
		// Если очередь пустая
		if((result = (this->_iter.count == 0))){
			// Если таймаут ожидания установлен
			if(timeout > 0){
				// Выполняем блокировку потока
				std::unique_lock lock(this->_mtx);
				// Выполняем ожидание на поступление данных
				this->_cv.read.wait_for(lock, std::chrono::duration(std::chrono::milliseconds(timeout)), [this]() noexcept -> bool {
					// Если в очереди появились данные
					return (this->_terminate || (this->_iter.count > 0));
				});
				// Проверяем пустая ли очередь в данный момент
				result = (this->_terminate || (this->_iter.count == 0));
			}
		}
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
	// Выводим результат
	return result;
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
			// Выполняем блокировку потока
			std::unique_lock lock(this->_mtx);
			// Если очередь завершила работу
			if(this->_terminate)
				// Выводим текущий размер очереди
				return this->_iter.count;
			// Если все данные добавлены в очередь
			if(this->_iter.count >= this->_max.records){
				// Выполняем ожидание доступности записей
				this->_cv.write.wait(lock, [this]() noexcept -> bool {
					// Завершаем ожидание очистки очереди, когда очередь освобождается
					return (this->_terminate || (this->_iter.count < this->_max.records));
				});
				// Если очередь завершила работу
				if(this->_terminate)
					// Выводим текущий размер очереди
					return this->_iter.count;
			}
			// Выполняем выделение памяти
			if(this->rss(size)){
				// Увеличиваем количество записей в очереди
				this->_iter.count++;
				// Выполняем запись данных в буфер
				::memcpy(this->_buffer.data() + this->_iter.end, reinterpret_cast <const uint8_t *> (&size), sizeof(size));
				// Увеличиваем смещение конца данных буфера
				this->_iter.end += sizeof(size);
				// Выполняем добавление самих данных полезной нагрузки
				::memcpy(this->_buffer.data() + this->_iter.end, buffer, size);
				// Увеличиваем смещение конца данных буфера
				this->_iter.end += size;
				// Отправляем сообщение, что очередь готова на чтение
				this->_cv.read.notify_one();
			// Если память не выделена
			} else {
				// Выполняем ожидание доступности записей
				this->_cv.write.wait(lock, [this]() noexcept -> bool {
					// Завершаем ожидание очистки очереди, когда очередь освобождается
					return (this->_terminate || (this->_iter.end == 0));
				});
				// Если очередь завершила работу
				if(this->_terminate)
					// Выводим текущий размер очереди
					return this->_iter.count;
			}
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
	// Выводим результат
	return this->_iter.count;
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
			// Выполняем блокировку потока
			std::unique_lock lock(this->_mtx);
			// Если очередь завершила работу
			if(this->_terminate)
				// Выводим текущий размер очереди
				return this->_iter.count;
			// Если все данные добавлены в очередь
			if(this->_iter.count >= this->_max.records){
				// Выполняем ожидание доступности записей
				this->_cv.write.wait(lock, [this]() noexcept -> bool {
					// Завершаем ожидание очистки очереди, когда очередь освобождается
					return (this->_terminate || (this->_iter.count < this->_max.records));
				});
				// Если очередь завершила работу
				if(this->_terminate)
					// Выводим текущий размер очереди
					return this->_iter.count;
			}
			// Выполняем выделение памяти
			if(this->rss(size)){
				// Увеличиваем количество записей в очереди
				this->_iter.count++;
				// Выполняем запись данных в буфер
				::memcpy(this->_buffer.data() + this->_iter.end, reinterpret_cast <const uint8_t *> (&size), sizeof(size));
				// Увеличиваем смещение конца данных буфера
				this->_iter.end += sizeof(size);
				// Выполняем перебор всех записей
				for(auto & record : records){
					// Выполняем добавление самих данных полезной нагрузки
					::memcpy(this->_buffer.data() + this->_iter.end, record.first, record.second);
					// Увеличиваем смещение конца данных буфера
					this->_iter.end += record.second;
				}
				// Отправляем сообщение, что очередь готова на чтение
				this->_cv.read.notify_one();
			// Если память не выделена
			} else {
				// Выполняем ожидание доступности записей
				this->_cv.write.wait(lock, [this]() noexcept -> bool {
					// Завершаем ожидание очистки очереди, когда очередь освобождается
					return (this->_terminate || (this->_iter.end == 0));
				});
				// Если очередь завершила работу
				if(this->_terminate)
					// Выводим текущий размер очереди
					return this->_iter.count;
			}
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
	// Выводим результат
	return this->_iter.count;
}
/**
 * @brief Метод установки максимального размера потребления памяти
 * 
 * @param size максимальный размер потребления памяти
 */
void awh::Queue::setMaxMemory(const size_t size) noexcept {
	// Если максимальный размер потребляемой памяти передан
	if(size > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			std::unique_lock lock(this->_mtx);
			// Выполняем установку максимального размера потребляемой памяти
			this->_max.memory = size;
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
}
/**
 * @brief Метод установки максимального количества записей очереди
 * 
 * @param count максимальное количество записей очереди
 */
void awh::Queue::setMaxRecords(const size_t count) noexcept {
	// Если количество записей передано
	if(count > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			std::unique_lock lock(this->_mtx);
			// Выполняем установку максимального количества сообщений очереди
			this->_max.records = count;
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
		// Активируем завершение работы текущей очереди
		this->_terminate = true;
		// Активируем завершение работы сторонней очереди
		queue._terminate = true;
		// Разблокируем очередь на чтение текущей очереди
		this->_cv.read.notify_all();
		// Разблокируем очередь на запись текущей очереди
		this->_cv.write.notify_all();
		// Разблокируем очередь на чтение сторонней очереди
		queue._cv.read.notify_all();
		// Разблокируем очередь на запись сторонней очереди
		queue._cv.write.notify_all();
		// Выполняем блокировку потока текущей очереди
		std::unique_lock lock1(this->_mtx);
		// Выполняем блокировку потока сторонней очереди
		std::unique_lock lock2(queue._mtx);
		// Выполняем обмен буферами данных
		this->_buffer.swap(queue._buffer);
		// Выполняем обмен последними итераторами
		this->_iter.end += (queue._iter.end - (queue._iter.end = this->_iter.end));
		// Выполняем обмен начальными итераторами
		this->_iter.begin += (queue._iter.begin - (queue._iter.begin = this->_iter.begin));
		// Выполняем обмен количествами добавленных записями
		this->_iter.count += (queue._iter.count - (queue._iter.count = this->_iter.count));
		// Выполняем обмен максимальными размерами памяти
		this->_max.memory += (queue._max.memory - (queue._max.memory = this->_max.memory));
		// Выполняем обмен максимальными количествами записей
		this->_max.records += (queue._max.records - (queue._max.records = this->_max.records));
		// Деактивируем флаг завершения работы текущей очереди
		this->_terminate = false;
		// Деактивируем флаг завершения работы сторонней очереди
		queue._terminate = false;
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
 * @brief Получения размера данных в очереди
 *
 * @return размер данных в очереди
 */
awh::Queue::operator size_t() const noexcept {
	// Выводим размер очереди
	return this->size();
}
/**
 * @brief Получения бинарных данных очереди
 *
 * @return бинарные данные очереди
 */
awh::Queue::operator const char * () const noexcept {
	// Выводим данные записи очереди
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
		// Активируем завершение работы текущей очереди
		this->_terminate = true;
		// Активируем завершение работы сторонней очереди
		queue._terminate = true;
		// Разблокируем очередь на чтение текущей очереди
		this->_cv.read.notify_all();
		// Разблокируем очередь на запись текущей очереди
		this->_cv.write.notify_all();
		// Разблокируем очередь на чтение сторонней очереди
		queue._cv.read.notify_all();
		// Разблокируем очередь на запись сторонней очереди
		queue._cv.write.notify_all();
		// Выполняем блокировку потока текущей очереди
		std::unique_lock lock1(this->_mtx);
		// Выполняем блокировку потока сторонней очереди
		std::unique_lock lock2(queue._mtx);
		// Выполняем перемен буферами данных
		this->_buffer = ::move(queue._buffer);
		// Выполняем копирование последнего итератора
		this->_iter.end = queue._iter.end;
		// Выполняем копирование начального итератора
		this->_iter.begin = queue._iter.begin;
		// Выполняем копирование количества добавленных записей
		this->_iter.count = queue._iter.count;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
		// Выполняем сброс последнего итератора сторонней очереди
		queue._iter.end = 0;
		// Выполняем сброс начального итератора сторонней очереди
		queue._iter.begin = 0;
		// Выполняем сброс количества добавленных записей сторонней очереди
		queue._iter.count = 0;
		// Деактивируем флаг завершения работы текущей очереди
		this->_terminate = false;
		// Деактивируем флаг завершения работы сторонней очереди
		queue._terminate = false;
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
	// Выводим текущий объект
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
		// Активируем завершение работы текущей очереди
		this->_terminate = true;
		// Разблокируем очередь на чтение текущей очереди
		this->_cv.read.notify_all();
		// Разблокируем очередь на запись текущей очереди
		this->_cv.write.notify_all();
		// Выполняем блокировку потока текущей очереди
		std::unique_lock lock(this->_mtx);
		// Выполняем перемен буферами данных
		this->_buffer = queue._buffer;
		// Выполняем копирование последнего итератора
		this->_iter.end = queue._iter.end;
		// Выполняем копирование начального итератора
		this->_iter.begin = queue._iter.begin;
		// Выполняем копирование количества добавленных записей
		this->_iter.count = queue._iter.count;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
		// Деактивируем флаг завершения работы текущей очереди
		this->_terminate = false;
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
	// Выводим текущий объект
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
			(this->_iter.end == queue._iter.end) &&
			(this->_iter.begin == queue._iter.begin) &&
			(this->_iter.count == queue._iter.count) &&
			(this->_max.memory == queue._max.memory) &&
			(this->_max.records == queue._max.records) &&
			(this->_buffer.size() == queue._buffer.size()) &&
			(::memcmp(this->_buffer.data(), queue._buffer.data(), this->_buffer.size()) == 0)
		);
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
	// Выводим результат
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
		// Активируем завершение работы текущей очереди
		this->_terminate = true;
		// Активируем завершение работы сторонней очереди
		queue._terminate = true;
		// Разблокируем очередь на чтение текущей очереди
		this->_cv.read.notify_all();
		// Разблокируем очередь на запись текущей очереди
		this->_cv.write.notify_all();
		// Разблокируем очередь на чтение сторонней очереди
		queue._cv.read.notify_all();
		// Разблокируем очередь на запись сторонней очереди
		queue._cv.write.notify_all();
		// Выполняем блокировку потока текущей очереди
		std::unique_lock lock1(this->_mtx);
		// Выполняем блокировку потока сторонней очереди
		std::unique_lock lock2(queue._mtx);
		// Выполняем перемен буферами данных
		this->_buffer = ::move(queue._buffer);
		// Выполняем копирование последнего итератора
		this->_iter.end = queue._iter.end;
		// Выполняем копирование начального итератора
		this->_iter.begin = queue._iter.begin;
		// Выполняем копирование количества добавленных записей
		this->_iter.count = queue._iter.count;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
		// Выполняем сброс последнего итератора сторонней очереди
		queue._iter.end = 0;
		// Выполняем сброс начального итератора сторонней очереди
		queue._iter.begin = 0;
		// Выполняем сброс количества добавленных записей сторонней очереди
		queue._iter.count = 0;
		// Деактивируем флаг завершения работы текущей очереди
		this->_terminate = false;
		// Деактивируем флаг завершения работы сторонней очереди
		queue._terminate = false;
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
 * @brief Конструктор копирования
 *
 * @param queue очередь для копирования
 */
awh::Queue::Queue(const queue_t & queue) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Активируем завершение работы текущей очереди
		this->_terminate = true;
		// Разблокируем очередь на чтение текущей очереди
		this->_cv.read.notify_all();
		// Разблокируем очередь на запись текущей очереди
		this->_cv.write.notify_all();
		// Выполняем блокировку потока текущей очереди
		std::unique_lock lock(this->_mtx);
		// Выполняем перемен буферами данных
		this->_buffer = queue._buffer;
		// Выполняем копирование последнего итератора
		this->_iter.end = queue._iter.end;
		// Выполняем копирование начального итератора
		this->_iter.begin = queue._iter.begin;
		// Выполняем копирование количества добавленных записей
		this->_iter.count = queue._iter.count;
		// Выполняем копирование максимального размера памяти
		this->_max.memory = queue._max.memory;
		// Выполняем копирование максимального количества записей
		this->_max.records = queue._max.records;
		// Деактивируем флаг завершения работы текущей очереди
		this->_terminate = false;
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Queue::Queue(const fmk_t * fmk, const log_t * log) noexcept : _terminate(false), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Queue::~Queue() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Активируем завершение работы очереди
		this->_terminate = true;
		// Разблокируем очередь на чтение
		this->_cv.read.notify_all();
		// Разблокируем очередь на запись
		this->_cv.write.notify_all();
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
