/**
 * @file: cmp.cpp
 * @date: 2024-11-03
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

/**
 * Подключаем заголовочный файл
 */
#include <sys/cmp.hpp>

/**
 * data данные буфера в бинарном виде
 * @return буфер в бинарном виде
 */
std::vector <char> awh::ClusterMessageProtocol::Buffer::data() const noexcept {
	// Результат работы функции
	std::vector <char> result;
	// Выполняем добавление данных заголовка в итоговый буфер
	result.insert(result.end(), reinterpret_cast <const char *> (&this->_header), reinterpret_cast <const char *> (&this->_header) + sizeof(this->_header));
	// Если буфер полезной нагрузки не пустой
	if((this->_payload != nullptr) && (this->_header.bytes > 0))
		// Выполняем вставку данных полезной нагрузки
		result.insert(result.end(), this->_payload.get(), this->_payload.get() + this->_header.bytes);
	// Выводим результат
	return result;
}
/**
 * Оператор извлечения бинарного буфера в бинарном виде
 * @return бинарный буфер в бинарном виде
 */
awh::ClusterMessageProtocol::Buffer::operator std::vector <char> () const noexcept {
	// Выполняем формирование общего буфера данных
	return this->data();
}
/**
 * push Метод добавления в буфер записи данных для отправки
 * @param index  индекс текущей записи
 * @param mode   режим отправки буфера данных
 * @param size   общий размер записи целиком
 * @param buffer буфер данных единичного чанка
 * @param bytes  размер буфера данных единичного чанка
 */
void awh::ClusterMessageProtocol::Buffer::push(const size_t index, const mode_t mode, const size_t size, const void * buffer, const size_t bytes) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Устанавливаем режим отравки буфера данных
		this->_header.mode = mode;
		// Выполняем установку общего размера записи
		this->_header.size = size;
		// Устанавливаем актуальный размер данных
		this->_header.bytes = bytes;
		// Устанавливаем индекс записи
		this->_header.index = index;
		// Если данные переданы верные
		if((buffer != nullptr) && (bytes > 0)){
			// Выполняем формирование буфера данных
			this->_payload = unique_ptr <uint8_t []> (new uint8_t [bytes]);
			// Выполняем копирование буфера полученных данных
			::memcpy(this->_payload.get(), buffer, bytes);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const bad_alloc &) {
		// Выводим сообщение об ошибке
		fprintf(stderr, "CMP: %s", "memory allocation error");
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * front Метод получения первой записи протокола
 * @return объект данных первой записи
 */
std::vector <char> awh::ClusterMessageProtocol::front() const noexcept {
	// Если записи в протоколе существуют
	if(!this->_data.empty())
		// Выводим запрошенный буфер данных
		return this->_data.begin()->second->data();
	// Выводим результат
	return std::vector <char> ();
}
/**
 * back Метод получения последней записи протокола
 * @return объект данных последней записи
 */
std::vector <char> awh::ClusterMessageProtocol::back() const noexcept {
	// Если записи в протоколе существуют
	if(!this->_data.empty())
		// Выводим запрошенный буфер данных
		return this->_data.rbegin()->second->data();
	// Выводим результат
	return std::vector <char> ();
}
/**
 * empty Метод проверки на пустоту контейнера
 * @return результат проверки
 */
bool awh::ClusterMessageProtocol::empty() const noexcept {
	// Выводим результат проверки
	return this->_data.empty();
}
/**
 * size Метод получения количества подготовленных буферов
 * @return количество подготовленных буферов
 */
size_t awh::ClusterMessageProtocol::size() const noexcept {
	// Выводим количество записей в протоколе
	return this->_data.size();
}
/**
 * erase Метод удаления записи протокола
 * @param index индекс конкретной записи
 */
void awh::ClusterMessageProtocol::erase(const size_t index) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Если список записей не пустой
		if(!this->_data.empty()){
			// Выполняем перебор всего списка записей
			for(auto i = this->_data.begin(); i != this->_data.end();){
				// Если индекс соответствует записи
				if(i->first == index)
					// Выполняем удаление записи
					i = this->_data.erase(i);
				// Пропускаем запись и ищем дальше
				else ++i;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение об ошибке
		this->_log->print("CMP: %s", log_t::flag_t::CRITICAL, error.what());
	}
}
/**
 * items Метод получения списка записей
 * @return список записей в протоколе
 */
std::set <size_t> awh::ClusterMessageProtocol::items() const noexcept {
	// Список индексов записей
	std::set <size_t> result;
	// Если записи существуют
	if(!this->_data.empty()){
		// Выполняем перебор всего списка записей
		for(auto & item : this->_data)
			// Добавляем полученный индекс в список записей
			result.emplace(item.first);
	}
	// Выводим полученный результат
	return result;
}
/**
 * pid Получение идентификатора процесса
 * @param index индекс конкретной записи
 * @return      идентификатор процесса
 */
pid_t awh::ClusterMessageProtocol::pid(const size_t index) const noexcept {
	// Если список записей не пустой
	if(!this->_data.empty()){
		// Выполняем поиск указанной записи
		auto i = this->_data.find(index);
		// Если запись найдена
		if(i != this->_data.end())
			// Выводим идентификатор процесса
			return i->second->_header.pid;
	}
	// Выводим результат
	return 0;
}
/**
 * at Метод извлечения данных конкретной записи
 * @param index индекс конкретной записи
 * @return      буфер данных записи
 */
std::vector <char> awh::ClusterMessageProtocol::at(const size_t index) const noexcept {
	// Результат работы функции
	std::vector <char> result;
	// Если список записей не пустой
	if(!this->_data.empty()){
		// Выполняем получение всего списка записей
		auto ret = this->_data.equal_range(index);
		// Выполняем перебор всего списка данных
		for(auto i = ret.first; i != ret.second; ++i){
			// Если запись существует
			if((i->second->_header.bytes > 0) && (i->second->_payload != nullptr))
				// Выполняем добавление полученных данных в результирующий буфер
				result.insert(result.end(), reinterpret_cast <char *> (i->second->_payload.get()), reinterpret_cast <char *> (i->second->_payload.get()) + i->second->_header.bytes);
		}
	}
	// Выводим результат
	return result;
}
/**
 * clear Метод очистки данных
 */
void awh::ClusterMessageProtocol::clear() noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем сброс индекса последней записи
		this->_index = 0;
		// Выполняем удаление буфера временных данных
		this->_tmp.clear();
		// Выполняем удаление всех данных
		this->_data.clear();
		// Очищаем выделенную память для временного буфера данных
		std::multimap <size_t, std::unique_ptr <buffer_t>> ().swap(this->_tmp);
		// Очищаем выделенную память для записей
		std::multimap <size_t, std::unique_ptr <buffer_t>> ().swap(this->_data);
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение об ошибке
		this->_log->print("CMP: %s", log_t::flag_t::CRITICAL, error.what());
	}
}
/**
 * pop Метод удаления первой записи протокола
 */
void awh::ClusterMessageProtocol::pop() noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Если список записей не пустой
		if(!this->_data.empty())
			// Выполняем удаление первой записи
			this->_data.erase(this->_data.begin());
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение об ошибке
		this->_log->print("CMP: %s", log_t::flag_t::CRITICAL, error.what());
	}
}
/**
 * push Метод добавления новой записи в протокол
 * @param buffer буфер данных для добавления
 * @param size   размер буфера данных
 */
void awh::ClusterMessageProtocol::push(const void * buffer, const size_t size) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Получаем размер заголовка
		const size_t headerSize = sizeof(header_t);
		// Если размер данных больше размера чанка
		if((headerSize + size) > this->_chunkSize){
			// Получаем общий размер данных
			size_t actual = 0, offset = 0;
			// Выполняем формирование буфера до тех пор пока все не добавим
			while((size - offset) > 0){
				// Выполняем добавление буфера данных в список
				auto i = this->_data.emplace(this->_index, unique_ptr <buffer_t> (new buffer_t));
				// Если данные не помещаются в буфере
				if((headerSize + (size - offset)) > this->_chunkSize){
					// Формируем актуальный размер данных буфера
					actual = (this->_chunkSize - headerSize);
					// Добавляем в буфер новую запись
					i->second->push(this->_index, (offset == 0 ? mode_t::BEGIN : mode_t::CONTINE), size, reinterpret_cast <const char *> (buffer) + offset, actual);
				// Если данные помещаются в буфере
				} else {
					// Формируем актуальный размер данных буфера
					actual = (size - offset);
					// Добавляем в буфер новую запись
					i->second->push(this->_index, mode_t::END, size, reinterpret_cast <const char *> (buffer) + offset, actual);
				}
				// Увеличиваем смещение в буфере
				offset += actual;
			}
		// Если размер данных помещается в буфер
		} else {
			// Выполняем добавление буфера данных в список
			auto i = this->_data.emplace(this->_index, unique_ptr <buffer_t> (new buffer_t));
			// Добавляем в буфер данных наши записи
			i->second->push(this->_index, mode_t::END, size, buffer, size);
		}
		// Выполняем увеличение номера записи
		this->_index = (this->_data.rbegin()->first + 1);
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение об ошибке
		this->_log->print("CMP: %s", log_t::flag_t::CRITICAL, error.what());
	/**
	 * Если возникает ошибка
	 */
	} catch(const bad_alloc &) {
		// Выводим в лог сообщение
		this->_log->print("Memory allocation error", log_t::flag_t::CRITICAL);
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * append Метод добавления записи в бинарном виде
 * @param buffer буфер данных в бинарном виде
 * @param size   размер буфера данных в бинарном виде
 */
void awh::ClusterMessageProtocol::append(const void * buffer, const size_t size) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Получаем размер заголовка
		const size_t headerSize = sizeof(header_t);
		// Если данные переданы
		if((buffer != nullptr) && (size > headerSize)){
			// Формируем объект буфера данных
			auto data = unique_ptr <buffer_t> (new buffer_t);
			// Выполняем получение данных заголовков
			::memcpy(&data->_header, buffer, headerSize);
			// Если оставшиеся данные не соответствуют буферу
			if(data->_header.bytes != (size - headerSize))
				// Выводим сообщение об ошибке
				this->_log->print("Received data buffer was corrupted.", log_t::flag_t::CRITICAL);
			// Если буфер данных соответствует
			else {
				// Выполняем формирование буфера данных
				data->_payload = unique_ptr <uint8_t []> (new uint8_t [data->_header.bytes]);
				// Выполняем копирование буфера полученных данных
				::memcpy(data->_payload.get(), reinterpret_cast <const char *> (buffer) + headerSize, data->_header.bytes);
				// Если все данные получены
				if(data->_header.mode == mode_t::END){
					// Выполняем получение всего списка записей
					auto ret = this->_tmp.equal_range(data->_header.index);
					// Выполняем перебор всего списка данных
					for(auto i = ret.first; i != ret.second; ++i)
						// Выполняем добавление буфера данных в список
						this->_data.emplace(i->first, std::move(i->second));
					// Выполняем удаление уже используемых буферов данных для текущей записи из временного буфера
					this->_tmp.erase(data->_header.index);
					// Выполняем добавление буфера данных в список
					this->_data.emplace(data->_header.index, std::move(data));
					// Выполняем увеличение номера записи
					this->_index = (this->_data.rbegin()->first + 1);
				// Если данные записи получены частично, добавляем их в временный буфер
				} else this->_tmp.emplace(data->_header.index, std::move(data));
			}
		// Выводим сообщение об ошибке
		} else this->_log->print("Вuffer size is too small and is %zu bytes", log_t::flag_t::CRITICAL, size);
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение об ошибке
		this->_log->print("CMP: %s", log_t::flag_t::CRITICAL, error.what());
	/**
	 * Если возникает ошибка
	 */
	} catch(const bad_alloc &) {
		// Выводим в лог сообщение
		this->_log->print("Memory allocation error", log_t::flag_t::CRITICAL);
		// Выходим из приложения
		::exit(EXIT_FAILURE);
	}
}
/**
 * chunkSize Метод установки максимального размера одного блока
 * @param size размер блока данных
 */
void awh::ClusterMessageProtocol::chunkSize(const size_t size) noexcept {
	/**
	 * Выполняем обработку ошибки
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <mutex> lock(this->_mtx);
		// Выполняем установку размера чанка
		this->_chunkSize = (size > 0 ? size : CHUNK_SIZE);
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение об ошибке
		this->_log->print("CMP: %s", log_t::flag_t::CRITICAL, error.what());
	}
}
/**
 * Оператор проверки на доступность данных в контейнере
 * @return результат проверки
 */
awh::ClusterMessageProtocol::operator bool() const noexcept {
	// Выводим результат проверки
	return !this->empty();
}
/**
 * Оператор получения количества записей
 * @return количество записей в протоколе
 */
awh::ClusterMessageProtocol::operator size_t() const noexcept {
	// Выводим количество записей в протоколе
	return this->size();
}
/**
 * Оператор извлечения данных конкретной записи
 * @param index индекс конкретной записи
 * @return      буфер данных записи
 */
std::vector <char> awh::ClusterMessageProtocol::operator [] (const size_t index) const noexcept {
	// Выполняем формирование и вывод запрашиваемой записи
	return this->at(index);
}
/**
 * Оператор [=] установки максимального размера одного блока
 * @param size размер блока данных
 * @return     текущий объект протокола
 */
awh::ClusterMessageProtocol & awh::ClusterMessageProtocol::operator = (const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->chunkSize(size);
	// Выводим текущий объект
	return (* this);
}
