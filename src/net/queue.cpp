/**
 * @file: queue.cpp
 * @date: 2026-02-07
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем стандартные модули
 */
#include <cmath>

/**
 * Подключаем заголовочный файл
 */
#include <net/queue.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод сдвига всех данных к началу буфера при фрагментации
 *
 */
void awh::Network_Queue::compact() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если позиция чтения уже в начале буфера, нет необходимости сдвигать данные
		if(this->_read == 0)
			// Выходим из функции, так как данные уже в начале буфера
			return;
		// Вычисляем размер данных, которые нужно сдвинуть
		const size_t size = (this->_write - this->_read);
		// Если есть данные для сдвига
		if(size > 0)
			// Используем memmove для перекрывающихся областей
			std::memmove(this->_buffer, this->_buffer + this->_read, size);
		// Обновляем позицию записи на новый конец данных
		this->_write = size;
		// Сбрасываем позицию чтения в начало буфера
		this->_read = 0;
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
 * @brief Метод быстрого получения размера записи (без проверок - вызывается только для валидных позиций)
 *
 * @return размер данных в очереди
 */
size_t awh::Network_Queue::recordSize(const size_t pos) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем размер записи в зависимости от типа очереди
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если очередь для потоков данных (например, TCP)
			case static_cast <uint8_t> (type_t::TCP):
				// Для TCP очереди размер записи хранится в первых 4 байтах
				return this->_total;
			// Если очередь для потоков данных (например, UDP)
			case static_cast <uint8_t> (type_t::UDP):
				// На x86/x64 разрешён unaligned access - используем прямой cast вместо memcpy
				return (* reinterpret_cast <const size_t *> (this->_buffer + pos));
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(pos), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию (0) в случае ошибки
	return 0;
}
/**
 * @brief Метод установки размера записи (прямой доступ)
 *
 * @param pos  позиция записи для обновления размера
 * @param size новый размер данных в очереди
 */
void awh::Network_Queue::recordSize(const size_t pos, const size_t size) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Устанавливаем размер записи в зависимости от типа очереди
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если очередь для потоков данных (например, TCP)
			case static_cast <uint8_t> (type_t::TCP): {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("It is not possible to set the size of an individual record for TCP payload", __PRETTY_FUNCTION__, std::make_tuple(pos, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("It is not possible to set the size of an individual record for TCP payload", log_t::flag_t::WARNING);
				#endif
			} break;
			// Если очередь для потоков данных (например, UDP)
			case static_cast <uint8_t> (type_t::UDP):
				// На x86/x64 разрешён unaligned access - используем прямой cast вместо memcpy
				(* reinterpret_cast <size_t *> (this->_buffer + pos)) = size;
			break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(pos, size), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод очистки очереди от всех данных
 *
 */
void awh::Network_Queue::clear() noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Сбрасываем позицию чтения в начало буфера
		this->_read = 0;
		// Сбрасываем позицию записи в начало буфера
		this->_write = 0;
		// Сбрасываем кэшированный размер полезных данных
		this->_total = 0;
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
 * @brief Метод получения типа сетевой очереди
 *
 * @return тип сетевой очереди
 */
awh::Network_Queue::type_t awh::Network_Queue::type() const noexcept {
	// Возвращаем текущий тип сетевой очереди
	return this->_type;
}
/**
 * @brief Метод Установки типа сетевой очереди
 *
 */
void awh::Network_Queue::type(const type_t type) noexcept {
	// Устанавливаем новый тип сетевой очереди
	this->_type = type;
}
/**
 * @brief Метод получения общего размера полезных данных в очереди (без учёта метаданных)
 *
 * @return размер данных в очереди
 */
size_t awh::Network_Queue::size() const noexcept {
	// Возвращаем кэшированный размер полезных данных в очереди
	return this->_total;
}
/**
 * @brief Метод получения количества записей в очереди
 *
 * @return количество записей в очереди
 */
size_t awh::Network_Queue::count() const noexcept {
	// Выводим количество записей в очереди
	return this->_count;
}
/**
 * @brief Метод определения доступного пространства для новых данных (в байтах полезной нагрузки)
 *
 * @return доступное пространство для новых данных в очереди
 */
size_t awh::Network_Queue::available() const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Вычисляем использованное пространство в очереди
		const size_t used = (this->_write - this->_read);
		// Вычисляем доступное пространство в очереди
		const size_t space = (AWH_NETWORK_QUEUE_BUFFER_SIZE - used);
		/**
		 * Определяем размер записи в зависимости от типа очереди
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если очередь для потоков данных (например, TCP)
			case static_cast <uint8_t> (type_t::TCP):
				// Выводим размер доступного пространства для новых данных в очереди (без учёта метаданных)
				return space;
			// Если очередь для потоков данных (например, UDP)
			case static_cast <uint8_t> (type_t::UDP):
				// Вычитаем место под заголовок новой записи
				return ((space < sizeof(size_t)) ? 0 : (space - sizeof(size_t)));
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
	// Выводим результат по умолчанию (0) в случае ошибки
	return 0;
}
/**
 * @brief Метод проверки на пустоту очереди
 *
 * @return результат проверки на пустоту очереди
 */
bool awh::Network_Queue::empty() const noexcept {
	// Проверяем, если позиция чтения больше или равна позиции записи, значит очередь пуста
	return (this->_read >= this->_write);
}
/**
 * @brief Метод удаления верхней записи из очереди
 *
 * @param size размер данных для удаления из очереди
 * @return     результат удаления верхней записи из очереди (true при успехе, false если очередь пуста)
 */
bool awh::Network_Queue::pop(const size_t size) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если очередь пуста, нет данных для удаления
		if(this->empty())
			// Выводим результат по умолчанию (false) при попытке удалить из пустой очереди
			return false;
		/**
		 * Определяем размер записи в зависимости от типа очереди и удаляем её, обновляя позиции и кэшированный размер данных
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если очередь для потоков данных (например, TCP)
			case static_cast <uint8_t> (type_t::TCP): {
				// Если размер для удаления из очереди меньше общего размера данных в очереди
				if(size < AWH_NETWORK_QUEUE_BUFFER_SIZE){
					// Определяем фактический размер данных для удаления, не превышающий текущий размер данных в очереди
					const size_t actual = ::min(this->_total, size);
					// Сдвигаем позицию чтения на размер текущей записи, effectively удаляя её из очереди
					this->_read += actual;
					// Уменьшаем кэшированный размер полезных данных на размер удалённой записи
					this->_total -= actual;
					// Уменьшаем счётчик записей в очереди
					this->_count -= actual;
					// Оптимизация: сброс позиций при полном опустошении
					if(this->_read >= this->_write){
						// Сбрасываем позицию чтения в начало буфера
						this->_read = 0;
						// Сбрасываем позицию записи в начало буфера
						this->_write = 0;
					}
				// Выполняем очистку всех данных в очереди
				} else if(size >= AWH_NETWORK_QUEUE_BUFFER_SIZE) {
					// Сбрасываем позицию чтения в начало буфера
					this->_read = 0;
					// Сбрасываем позицию записи в начало буфера
					this->_write = 0;
					// Сбрасываем кэшированный размер полезных данных
					this->_total = 0;
				}
			} break;
			// Если очередь для потоков данных (например, UDP)
			case static_cast <uint8_t> (type_t::UDP): {
				// Получаем размер данных полезной нагрузки текущей верхней записи в очереди
				const size_t size = this->recordSize(this->_read);
				// Вычисляем полный размер записи, включая заголовок размера
				const size_t record = (size + sizeof(size_t));
				// Уменьшаем счётчик записей в очереди
				this->_count--;
				// Сдвигаем позицию чтения на размер текущей записи, effectively удаляя её из очереди
				this->_read += record;
				// Уменьшаем кэшированный размер полезных данных на размер удалённой записи
				this->_total -= size;
				// Оптимизация: сброс позиций при полном опустошении
				if(this->_read >= this->_write){
					// Сбрасываем позицию чтения в начало буфера
					this->_read = 0;
					// Сбрасываем позицию записи в начало буфера
					this->_write = 0;
				}
			} break;
		}
		// Выводим результат (true при успешном удалении записи из очереди)
		return true;
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
	// Выводим результат по умолчанию (false) в случае ошибки
	return false;
}
/**
 * @brief Метод добавления данных в очередь
 *
 * @param data данные для добавления в очередь
 * @param size размер данных для добавления в очередь
 * @return     количество данных, успешно добавленных в очередь (0 при неудаче, когда недостаточно места)
 */
size_t awh::Network_Queue::push(const void * data, const size_t size) noexcept {
	// Результат работы функции
	size_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если размер данных для добавления в очередь передан корректно
		if(size > 0){
			/**
			 * Определяем тип очереди для получения данных
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если очередь для потоков данных (например, TCP)
				case static_cast <uint8_t> (type_t::TCP): {
					// Устанавливаем результат добавленных данных в очередь
					result = size;
					// Быстрая проверка на переполнение буфера
					if((AWH_NETWORK_QUEUE_BUFFER_SIZE - this->_write) < result){
						// Недостаточно места в конце - пробуем сжать
						this->compact();
						// Выполняем перерасчёт добавляемых данных
						result = ::min(result, AWH_NETWORK_QUEUE_BUFFER_SIZE - this->_write);
						// Проверяем ещё раз после сжатия
						if(result == 0)
							// Даже после сжатия нет места
							return result;
					}
					// Копируем данные в буфер очереди
					::memcpy(this->_buffer + this->_write, data, result);
					// Увеличиваем счётчик записей в очереди
					this->_count += result;
				} break;
				// Если очередь для потоков данных (например, UDP)
				case static_cast <uint8_t> (type_t::UDP): {
					// Устанавливаем результат добавленных данных в очередь
					result = size;
					// Определяем размер записи = размер данных + заголовок (size_t)
					const size_t record = (sizeof(size_t) + result);
					// Быстрая проверка на переполнение буфера
					if((AWH_NETWORK_QUEUE_BUFFER_SIZE - this->_write) < record){
						// Недостаточно места в конце - пробуем сжать
						this->compact();
						// Проверяем ещё раз после сжатия
						if((AWH_NETWORK_QUEUE_BUFFER_SIZE - this->_write) < record)
							// Даже после сжатия нет места
							return 0;
					}
					// Устанавливаем размер записи (прямой доступ)
					this->recordSize(this->_write, result);
					// Увеличиваем позицию записи на размер заголовка
					this->_write += sizeof(size_t);
					// Копируем данные в буфер очереди
					::memcpy(this->_buffer + this->_write, data, result);
					// Увеличиваем счётчик записей в очереди
					this->_count++;
				} break;
			}
			// Увеличиваем размер полезных данных в очереди
			this->_write += result;
			// Обновляем кэшированный размер полезных данных
			this->_total += result;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат работы функции
	return result;
}
/**
 * @brief Метод получения данных из очереди (без удаления - для чтения)
 *
 * @param data данные для получения из очереди (устанавливается указатель на данные в очереди)
 * @param size размер данных для получения из очереди
 * @return     результат (true при успехе, false если очередь пуста)
 */
bool awh::Network_Queue::front(const void ** data, size_t & size) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если очередь пуста, нет данных для удаления
		if(this->empty())
			// Выводим результат по умолчанию (false) при попытке удалить из пустой очереди
			return false;
		/**
		 * Определяем тип очереди для получения данных
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если очередь для потоков данных (например, TCP)
			case static_cast <uint8_t> (type_t::TCP): {
				// Получаем размер данных полезной нагрузки текущей верхней записи в очереди
				size = this->recordSize(this->_read);
				// Устанавливаем указатель на данные полезной нагрузки текущей верхней записи в очереди
				* data = (this->_buffer + this->_read);
			} break;
			// Если очередь для потоков данных (например, UDP)
			case static_cast <uint8_t> (type_t::UDP): {
				// Получаем размер данных полезной нагрузки текущей верхней записи в очереди
				size = this->recordSize(this->_read);
				// Устанавливаем указатель на данные полезной нагрузки текущей верхней записи в очереди
				* data = (this->_buffer + (this->_read + sizeof(size_t)));
			} break;
		}
		// Выводим результат (true при успешном получении данных из очереди)
		return true;
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
	// Выводим результат по умолчанию (false) в случае ошибки
	return false;
}
/**
 * @brief Конструктор инициализации сетевой очереди
 *
 * @param fmk объект фреймворка для доступа к его функциям
 * @param log объект для работы с логами
 */
awh::Network_Queue::Network_Queue(const fmk_t * fmk, const log_t * log) noexcept :
 _type(type_t::UDP), _read(0), _write(0),
 _total(0), _count(0), _fmk(fmk), _log(log) {}
