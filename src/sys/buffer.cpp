/**
 * @file: buffer.cpp
 * @date: 2024-03-31
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
#include <sys/buffer.hpp>

/**
 * clear Метод очистки буфера данных
 */
void awh::Buffer::clear() noexcept {
	// Выполняем сброс размера основного буфера данных
	this->_size = 0;
	// Выполняем сброс смещения в основном буфере данных
	this->_offset = 0;
	// Определяем режим работы буфера
	switch(static_cast <uint8_t> (this->_mode)){
		// Если установлен режим копирования полученных данных
		case static_cast <uint8_t> (mode_t::COPY):
			// Выполняем сброс временного буфера
			this->_tmp.reset(nullptr);
		break;
		// Если установлен режим не копировать полученные данные
		case static_cast <uint8_t> (mode_t::NO_COPY):
			// Выполняем сброс основного буфера данных
			this->_buffer = nullptr;
		break;
	}
	// Выполняем очистку буфера данных
	this->_data.clear();
	// Если размер выделенной памяти выше максимального размера буфера
	if(this->_data.capacity() > AWH_BUFFER_SIZE)
		// Выполняем очистку временного буфера данных
		vector <char> ().swap(this->_data);
}
/**
 * empty Метод проверки на пустоту буфера
 * @return результат проверки
 */
bool awh::Buffer::empty() const noexcept {
	// Выводим результат проверки
	return (this->_data.empty() && ((this->_size == this->_offset) || (this->_size == 0)));
}
/**
 * size Метод получения размера данных в буфере
 * @return размер данных в буфере
 */
size_t awh::Buffer::size() const noexcept {
	// Если буфер данных уже заполнен
	if(!this->_data.empty())
		// Выводим размер буфера данных
		return this->_data.size();
	// Если буфер данных ещё не заполнен
	else return (this->_size - this->_offset);
}
/**
 * data Метод получения бинарных данных буфера
 * @return бинарные данные буфера
 */
awh::Buffer::data_t awh::Buffer::data() const noexcept {
	// Если буфер данных уже заполнен
	if(!this->_data.empty())
		// Выводим бинарные данные буфера полезной нагрузки
		return this->_data.data();
	// Если буфер данных ещё не заполнен
	else {
		// Определяем режим работы буфера
		switch(static_cast <uint8_t> (this->_mode)){
			// Если установлен режим копирования полученных данных
			case static_cast <uint8_t> (mode_t::COPY):
				// Выводим бинарные данные буфера полезной нагрузки
				return (this->_tmp.get() + this->_offset);
			// Если установлен режим не копировать полученные данные
			case static_cast <uint8_t> (mode_t::NO_COPY):
				// Выводим бинарные данные буфера полезной нагрузки
				return (this->_buffer + this->_offset);
		}
	}
	// Сообщаем, что ничего не найдено
	return nullptr;
}
/**
 * commit Метод фиксации изменений в буфере для перехода к следующей итерации
 */
void awh::Buffer::commit() noexcept {
	// Если буфер данных не заполнен
	if(this->_data.empty() && (this->_offset < this->_size)){
		// Определяем режим работы буфера
		switch(static_cast <uint8_t> (this->_mode)){
			// Если установлен режим копирования полученных данных
			case static_cast <uint8_t> (mode_t::COPY):
				// Выполняем добавление буфера
				this->_data.insert(this->_data.end(), this->_tmp.get() + this->_offset, this->_tmp.get() + this->_size);
			break;
			// Если установлен режим не копировать полученные данные
			case static_cast <uint8_t> (mode_t::NO_COPY):
				// Выполняем добавление буфера
				this->_data.insert(this->_data.end(), this->_buffer + this->_offset, this->_buffer + this->_size);
			break;
		}
		// Выполняем сброс размера основного буфера данных
		this->_size = 0;
		// Выполняем сброс смещения в основном буфере данных
		this->_offset = 0;
		// Определяем режим работы буфера
		switch(static_cast <uint8_t> (this->_mode)){
			// Если установлен режим копирования полученных данных
			case static_cast <uint8_t> (mode_t::COPY):
				// Выполняем сброс временного буфера
				this->_tmp.reset(nullptr);
			break;
			// Если установлен режим не копировать полученные данные
			case static_cast <uint8_t> (mode_t::NO_COPY):
				// Выполняем сброс основного буфера данных
				this->_buffer = nullptr;
			break;
		}
	}
}
/**
 * erase Метод удаления из буфера байт
 * @param size количество байт для удаления
 */
void awh::Buffer::erase(const size_t size) noexcept {
	// Если количество байт для удаления передано
	if(size > 0){
		// Если буфер данных уже заполнен
		if(!this->_data.empty()){
			// Если размер буфера больше количества удаляемых байт
			if(this->_data.size() >= size)
				// Удаляем количество обработанных байт
				this->_data.erase(this->_data.begin(), this->_data.begin() + size);
				// vector <decltype(this->_data)::value_type> (this->_data.begin() + bytes, this->_data.end()).swap(this->_data);
			// Если байт в буфере меньше
			else {
				// Выполняем очистку буфера данных
				this->_data.clear();
				// Если размер выделенной памяти выше максимального размера буфера
				if(this->_data.capacity() > AWH_BUFFER_SIZE)
					// Выполняем очистку временного буфера данных
					vector <char> ().swap(this->_data);
			}
		// Если временный буфер не заполнен
		} else {
			// Если размер используемых данных превышает переданный размер
			if((this->_size - this->_offset) >= size)
				// Выполняем увеличение смещения в буфере
				this->_offset += size;
			// Иначе просто зануляем размеры
			else {
				// Выполняем сброс размера основного буфера данных
				this->_size = 0;
				// Выполняем сброс смещения в основном буфере данных
				this->_offset = 0;
				// Определяем режим работы буфера
				switch(static_cast <uint8_t> (this->_mode)){
					// Если установлен режим копирования полученных данных
					case static_cast <uint8_t> (mode_t::COPY):
						// Выполняем сброс временного буфера
						this->_tmp.reset(nullptr);
					break;
					// Если установлен режим не копировать полученные данные
					case static_cast <uint8_t> (mode_t::NO_COPY):
						// Выполняем сброс основного буфера данных
						this->_buffer = nullptr;
					break;
				}
			}
		}
	}
}
/**
 * emplace Метод добавления нового сырого буфера
 * @param buffer бинарный буфер данных для добавления
 * @param size   размер бинарного буфера данных
 */
void awh::Buffer::emplace(const char * buffer, const size_t size) noexcept {
	// Если буфер данных уже заполнен
	if(!this->_data.empty()){
		// Выполняем сброс размера основного буфера данных
		this->_size = 0;
		// Выполняем сброс смещения в основном буфере данных
		this->_offset = 0;
		// Определяем режим работы буфера
		switch(static_cast <uint8_t> (this->_mode)){
			// Если установлен режим копирования полученных данных
			case static_cast <uint8_t> (mode_t::COPY):
				// Выполняем сброс временного буфера
				this->_tmp.reset(nullptr);
			break;
			// Если установлен режим не копировать полученные данные
			case static_cast <uint8_t> (mode_t::NO_COPY):
				// Выполняем сброс основного буфера данных
				this->_buffer = nullptr;
			break;
		}
		// Выполняем добавление буфера
		this->_data.insert(this->_data.end(), buffer, buffer + size);
		/*
		// Выполняем увеличение буфера данных
		this->_data.resize(this->_data.size() + size);
		// Выполняем копирование новых полученных данных
		::memcpy(this->_data.data() + (this->_data.size() - size), buffer, size);
		*/
	// Если временный буфер не заполнен, тогда работаем с основным
	} else if((this->_offset == this->_size) || (this->_size == 0)) {
		// Выполняем сброс смещения в основном буфере данных
		this->_offset = 0;
		// Запоминаем размер буфера данных
		this->_size = size;
		// Определяем режим работы буфера
		switch(static_cast <uint8_t> (this->_mode)){
			// Если установлен режим копирования полученных данных
			case static_cast <uint8_t> (mode_t::COPY): {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем сброс временного буфера
					this->_tmp.reset(nullptr);
					// Выполняем создание буфера данных
					this->_tmp = std::unique_ptr <char []> (new char [this->_size]);
					// Выполняем копирование переданного буфера данных в временный буфер данных
					::memcpy(this->_tmp.get(), buffer, this->_size);
				/**
				 * Если возникает ошибка
				 */
				} catch(const bad_alloc &) {
					// Выводим сообщение в лог
					::fprintf(stderr, "Buffer emplace: %s\n", "memory allocation error");
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				}
			} break;
			// Если установлен режим не копировать полученные данные
			case static_cast <uint8_t> (mode_t::NO_COPY):
				// Запоминаем полученный буфер данных
				this->_buffer = buffer;
			break;
		}
	// Если в буфере ещё не обработанны предыдущие байты
	} else if(this->_size > this->_offset) {
		// Определяем режим работы буфера
		switch(static_cast <uint8_t> (this->_mode)){
			// Если установлен режим копирования полученных данных
			case static_cast <uint8_t> (mode_t::COPY): {
				// Устанавливаем ранее скопированные данные
				this->_data.insert(this->_data.end(), this->_tmp.get() + this->_offset, this->_tmp.get() + this->_size);
				// Выполняем сброс временного буфера
				this->_tmp.reset(nullptr);
			} break;
			// Если установлен режим не копировать полученные данные
			case static_cast <uint8_t> (mode_t::NO_COPY): {
				// Устанавливаем ранее полученные данные
				this->_data.insert(this->_data.end(), this->_buffer + this->_offset, this->_buffer + this->_size);
				// Выполняем сброс основного буфера данных
				this->_buffer = nullptr;
			} break;
		}
		// Выполняем сброс размера основного буфера данных
		this->_size = 0;
		// Выполняем сброс смещения в основном буфере данных
		this->_offset = 0;
		// Устанавливаем только что полученные данные
		this->_data.insert(this->_data.end(), buffer, buffer + size);
	}
}
/**
 * operator Получения размера данных в буфере
 * @return размер данных в буфере
 */
awh::Buffer::operator size_t() const noexcept {
	// Выводим размер данных в буфере
	return this->size();
}
/**
 * operator Получения бинарных данных буфера
 * @return бинарные данные буфера
 */
awh::Buffer::operator awh::Buffer::data_t() const noexcept {
	// Выводим бинарные данные буфера
	return this->data();
}
/**
 * operator Получения режима работы буфера
 * @return режим работы буфера
 */
awh::Buffer::operator awh::Buffer::mode_t() const noexcept {
	// Выводим режим работы буфера
	return this->_mode;
}
/**
 * Оператор [=] установки режима работы буфера
 * @param mode режим работы буфера для установки
 * @return     текущий объект
 */
awh::Buffer & awh::Buffer::operator = (const mode_t mode) noexcept {
	// Выполняем очистку буфера данных
	this->clear();
	// Выполняем установку режима работы буфера
	this->_mode = mode;
	// Выводим текущий объект
	return (* this);
}
