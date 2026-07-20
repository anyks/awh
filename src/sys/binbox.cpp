/**
 * @file: binbox.cpp
 * @date: 2026-02-27
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
 * Стандартный заголовочный файл
 */
#include <cerrno>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/binbox.hpp>
#include <sys/version.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические типы данных в пространство имён BinBox
 *
 */
namespace binbox {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Функция получения строкового типа бинарных данных
	 *
	 * @param buffer буфер бинарных данных
	 * @param result результат работы функции
	 */
	static void extract(const vector <uint8_t> & buffer, string & result) noexcept {
		// Если буфер данных передан
		if(!buffer.empty())
			// Выполняем формирование строки
			result.assign(buffer.begin(), buffer.end());
	}
	/**
	 * @brief Шаблон метода чтения бинарных данных из бинарного контейнера
	 *
	 * @tparam T тип данных извлекаемого результата
	 */
	template <typename T>
	/**
	 * @brief Функция получения бинарного буфера бинарных данных
	 *
	 * @param buffer буфер бинарных данных
	 * @param result результат работы функции
	 */
	static void extract(const vector <uint8_t> & buffer, vector <T> & result) noexcept {
		// Если буфер данных передан
		if(!buffer.empty()){
			// Получаем количество элементов, которое поместится в результирующий буфер
			const size_t count = (buffer.size() / sizeof(T));
			// Выделяем память для результирующего буфера данных
			result.resize(count, 0);
			// Если есть хотя бы один полный элемент для копирования
			if(count > 0)
				// Выполняем копирование только кратного типу количества байт, чтобы не выйти за границы выделенной памяти
				::memcpy(reinterpret_cast <uint8_t *> (&result[0]), &buffer[0], count * sizeof(T));
		}
	}
	/**
	 * @brief Шаблон метода чтения бинарных данных из бинарного контейнера
	 *
	 * @tparam T тип данных извлекаемого результата
	 */
	template <typename T>
	/**
	 * @brief Функция получения основных типов бинарных данных
	 *
	 * @param buffer буфер бинарных данных
	 * @param result результат работы функции
	 */
	static void extract(const vector <uint8_t> & buffer, T & result) noexcept {
		// Если данные являются основными
		if(!buffer.empty())
			// Выполняем копирование полученных данных, ограничивая размер копирования размером буфера, чтобы не выйти за его границы
			::memcpy(&result, buffer.data(), (buffer.size() < sizeof(result) ? buffer.size() : sizeof(result)));
	}
	/**
	 * @brief dump Функция создания дампов записей
	 *
	 * @param name    название контейнера для создания дампа
	 * @param version версия контейнера для создания дампа
	 * @param records контейнер для хранения бинарных данных
	 * @param result  результат бинарного буфера куда будет помещён итоговый дамп
	 * @param fmk     объект фреймворка
	 * @param log     объект для работы с логами
	 */
	static void dump(const string & name, const uint32_t version, const unordered_map <uint64_t, binbox_t::record_t> & records, vector <uint8_t> & result, const fmk_t * fmk, const log_t * log) noexcept {
		// Если список записей передан
		if(!name.empty() && !records.empty()){
			/**
			 * Выполняем обработку ошибки
			 */
			try {
				// Получаем количество бинарных данных
				uint32_t count = 0;
				// Суммарный размер полезной нагрузки всех записей
				uintmax_t payload = 0;
				/**
				 * Выполняем предварительный подсчёт количества записей и суммарного размера полезной нагрузки
				 */
				for(auto & record : records){
					// Если размер записи установлен
					if(record.second.size > 0){
						// Выполняем увеличение количества установленных записей бинарных данных
						count++;
						// Увеличиваем суммарный размер полезной нагрузки (идентификатор + размер + данные)
						payload += (sizeof(record.first) + sizeof(record.second.size) + record.second.size);
					}
				}
				// Получаем размер тела дампа (версия + количество записей + полезная нагрузка)
				const uintmax_t bodySize = (sizeof(version) + sizeof(count) + payload);
				// Получаем заголовок контейнера
				const string header = fmk->format("%s/%s", name.c_str(), static_cast <string> (version_t(version)).c_str());
				// Выполняем очистку результирующего буфера данных
				result.clear();
				// Резервируем память под весь дамп целиком, чтобы избежать повторных реаллокаций
				result.reserve(header.size() + sizeof(bodySize) + bodySize);
				// Выполняем установку заголовка бинарного контейнера
				result.insert(result.end(), header.begin(), header.end());
				// Выполняем установку размера тела дампа
				result.insert(result.end(), reinterpret_cast <const uint8_t *> (&bodySize), reinterpret_cast <const uint8_t *> (&bodySize) + sizeof(bodySize));
				// Выполняем установку версии контейнера
				result.insert(result.end(), reinterpret_cast <const uint8_t *> (&version), reinterpret_cast <const uint8_t *> (&version) + sizeof(version));
				// Выполняем установку количества бинарных данных
				result.insert(result.end(), reinterpret_cast <const uint8_t *> (&count), reinterpret_cast <const uint8_t *> (&count) + sizeof(count));
				/**
				 * Выполняем перебор всех бинарных данных
				 */
				for(auto & record : records){
					// Получаем размер одной записи бинарных данных
					const uintmax_t size = record.second.size;
					// Если размер записи установлен
					if(size > 0){
						// Выполняем установку идентификатора записи бинарных данных
						result.insert(result.end(), reinterpret_cast <const uint8_t *> (&record.first), reinterpret_cast <const uint8_t *> (&record.first) + sizeof(record.first));
						// Выполняем установку размера записи бинарных данных
						result.insert(result.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
						// Выполняем установку полезной нагрузки бинарных данных
						result.insert(result.end(), record.second.buffer.get(), record.second.buffer.get() + size);
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
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::CRITICAL, __FUNCTION__, error.what());
				#endif
			}
		}
	}
	/**
	 * @brief dump Функция извлечения записей из дампа
	 *
	 * @param name    название контейнера для извлечения записей
	 * @param version версия контейнера для извлечения записей
	 * @param buffer  бинарный буфер дампа для извлечения записей контейнера
	 * @param result  результирующий контейнер для извлечения записей из бинарного буфера
	 * @param fmk     объект фреймворка
	 * @param log     объект для работы с логами
	 */
	static void dump(const string & name, const uint32_t version, const vector <uint8_t> & buffer, unordered_map <uint64_t, binbox_t::record_t> & result, const fmk_t * fmk, const log_t * log) noexcept {
		// Если буфер данных передан
		if(!name.empty() && !buffer.empty()){
			/**
			 * Выполняем обработку ошибки
			 */
			try {
				// Получаем заголовок контейнера
				const string header = fmk->format("%s/%s", name.c_str(), static_cast <string> (version_t(version)).c_str());
				// Если размер буфера данных меньше размера заголовка контейнера
				if(buffer.size() < header.size()){
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::CRITICAL, "BinBox container header is invalid");
					// Завершаем извлечение данных из бинарного контейнера
					return;
				}
				// Если заголовок контейнера не совпадает с заголовком в буфере данных
				if(!std::equal(header.begin(), header.end(), buffer.begin())){
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::CRITICAL, "BinBox container header is invalid");
					// Завершаем извлечение данных из бинарного контейнера
					return;
				}
				// Смещение в бинарном буфере
				size_t offset = header.length();
				// Выполняем очистку контейнера результата для загрузки записей
				result.clear();
				// Размер бинарных данных
				uintmax_t size = 0;
				// Если в буфере недостаточно данных для извлечения размера тела дампа
				if((buffer.size() - offset) < sizeof(size)){
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::CRITICAL, "BinBox container size is invalid");
					// Завершаем извлечение данных из бинарного контейнера
					return;
				}
				// Выполняем извлечение размера бинарных данных
				::memcpy(reinterpret_cast <void *> (&size), &buffer[0] + offset, sizeof(size));
				// Выполняем смещение в буфере
				offset += sizeof(size);
				// Если размер бинарных данных не соответствует размеру полезной нагрузки в буфере данных
				if(size != static_cast <uintmax_t> (buffer.size() - offset)){
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::CRITICAL, "BinBox container size is invalid");
					// Завершаем извлечение данных из бинарного контейнера
					return;
				}
				// Общее количество бинарных данных
				uint32_t count = 0;
				// Получаем версию контейнера
				uint32_t version = 0;
				// Если в буфере недостаточно данных для извлечения версии контейнера и количества записей
				if((buffer.size() - offset) < (sizeof(version) + sizeof(count))){
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::CRITICAL, "BinBox container size is invalid");
					// Завершаем извлечение данных из бинарного контейнера
					return;
				}
				// Выполняем извлечение версии контейнера
				::memcpy(reinterpret_cast <void *> (&version), &buffer[0] + offset, sizeof(version));
				// Выполняем смещение в буфере
				offset += sizeof(version);
				// Если текущая версия контейнера выше предыдущего
				if(version_t(AWH_VERSION) > version_t(version))
					// Записываем ошибку в лог
					log->print("Extracted BinBox container v%s is lower than the current container v%s", log_t::flag_t::WARNING, static_cast <string> (version_t(version)).c_str(), AWH_VERSION);
				// Выполняем извлечение количества записей бинарных данных
				::memcpy(reinterpret_cast <void *> (&count), &buffer[0] + offset, sizeof(count));
				// Выполняем смещение в буфере
				offset += sizeof(count);
				// Если количество записей бинарных данных получено
				if(count > 0){
					// Идентификатор записи
					uint64_t idw = 0;
					/**
					 * Выполняем перебор всех записей бинарных данных
					 */
					for(uint32_t i = 0; i < count; i++){
						// Если в буфере недостаточно данных для извлечения идентификатора и размера записи
						if((buffer.size() - offset) < (sizeof(idw) + sizeof(size))){
							// Записываем ошибку в лог
							log->print("%s", log_t::flag_t::CRITICAL, "BinBox record entry cannot be retrieved");
							// Завершаем извлечение данных из бинарного контейнера
							return;
						}
						// Выполняем извлечение идентификатора записи
						::memcpy(reinterpret_cast <void *> (&idw), &buffer[0] + offset, sizeof(idw));
						// Выполняем смещение в буфере
						offset += sizeof(idw);
						// Выполняем извлечение размер записи
						::memcpy(reinterpret_cast <void *> (&size), &buffer[0] + offset, sizeof(size));
						// Выполняем смещение в буфере
						offset += sizeof(size);
						// Если идентификатор записи и размер в буфере данных получены
						if((idw > 0) && (size > 0)){
							// Если в буфере недостаточно данных для извлечения полезной нагрузки записи
							if(static_cast <uintmax_t> (buffer.size() - offset) < size){
								// Записываем ошибку в лог
								log->print("%s", log_t::flag_t::CRITICAL, "BinBox record entry cannot be retrieved");
								// Завершаем извлечение данных из бинарного контейнера
								return;
							}
							// Выполняем добавление данных записи
							auto ret = result.emplace(idw, binbox_t::record_t());
							// Выполняем установку размера буфера данных
							ret.first->second.size = size;
							// Выполняем создание буфера данных
							ret.first->second.buffer = unique_ptr <uint8_t []> (new uint8_t [size]);
							// Выполняем копирование буфера полученных данных
							::memcpy(ret.first->second.buffer.get(), &buffer[0] + offset, size);
							// Выполняем смещение в буфере
							offset += size;
						// Если запись бинарных данных не может быть извлечена
						} else {
							// Записываем ошибку в лог
							log->print("%s", log_t::flag_t::CRITICAL, "BinBox record entry cannot be retrieved");
							// Завершаем извлечение данных из бинарного контейнера
							return;
						}
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
					// Записываем ошибку в лог
					log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer.size()), log_t::flag_t::CRITICAL, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					log->print("%s", log_t::flag_t::CRITICAL, __FUNCTION__, error.what());
				#endif
			}
		}
	}
};

/**
 * @brief Оператор преобразования в сырой итератор
 *
 * @return iterator итератор для преобразования
 */
awh::BinBox::Iterator::operator awh::BinBox::Iterator::iterator() noexcept {
	// Возвращаем текущее значение итератора
	return this->_it;
}
/**
 * @brief Оператор извлечения указателя заголовка
 *
 * @return указатель заголовка
 */
awh::BinBox::Iterator::pointer awh::BinBox::Iterator::operator -> () noexcept {
	// Возвращаем результат
	return &this->_it->second;
}
/**
 * @brief Оператор разыменования заголовка
 *
 * @return значение заголовка
 */
awh::BinBox::Iterator::reference awh::BinBox::Iterator::operator * () const noexcept {
	// Возвращаем результат
	return this->_it->second;
}
/**
 * @brief Оператор смещения вперед
 *
 * @return значение текущего итератора
 */
awh::BinBox::Iterator & awh::BinBox::Iterator::operator ++ () noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем смещение итератора
		++this->_it;
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
	return (* this);
}
/**
 * @brief Оператор сравнения соответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 */
bool awh::BinBox::Iterator::operator == (const Iterator & other) const noexcept {
	// Возвращаем результат
	return (this->_it == other._it);
}
/**
 * @brief Оператора сравнения несоответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 */
bool awh::BinBox::Iterator::operator != (const Iterator & other) const noexcept {
	// Возвращаем результат
	return (this->_it != other._it);
}
/**
 * @brief Конструктор
 *
 * @param it  итератор для установки
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::BinBox::Iterator::Iterator(iterator it, const fmk_t * fmk, const log_t * log) noexcept :
 _it(it), _fmk(fmk), _log(log) {}

/**
 * @brief Метод очистки всех данных
 *
 */
void awh::BinBox::clear() noexcept {
	// Выполняем очистку контейнера
	this->_records.clear();
}
/**
 * @brief Метод проверки на пустое значение контейнера
 *
 * @return результат проверки
 */
bool awh::BinBox::empty() const noexcept {
	// Возвращаем проверку на пустоту очереди
	return this->_records.empty();
}
/**
 * @brief Метод получения количества записей в контейнере
 *
 * @return количество записей в контейнере
 */
size_t awh::BinBox::count() const noexcept {
	// Возвращаем количество записей в контейнере
	return this->_records.size();
}
/**
 * @brief Метод получения названия контейнера
 *
 * @return название контейнера
 */
string awh::BinBox::getName() const noexcept {
	// Возвращаем название контейнера
	return this->_name;
}
/**
 * @brief Метод установки названия контейнера
 *
 * @param name название контейнера
 */
void awh::BinBox::setName(string_view name) noexcept {
	// Устанавливаем название контейнера
	this->_name = name;
}
/**
 * @brief Метод получения версии контейнера
 *
 * @return версия контейнера
 */
string awh::BinBox::getVersion() const noexcept {
	// Возвращаем версию контейнера
	return static_cast <string> (version_t(this->_version));
}
/**
 * @brief Метод установки версии контейнера
 *
 * @param version версия контейнера для установки
 */
void awh::BinBox::setVersion(string_view version) noexcept {
	// Получаем версию контейнера (создаём строку, так как string_view не гарантирует нуль-терминацию)
	this->_version = static_cast <uint32_t> (version_t(string(version)));
}
/**
 * @brief Метод удаления записи по ключу
 *
 * @param key ключ для удаления записи
 * @return    результат работы функции
 */
bool awh::BinBox::erase(string_view key) noexcept {
	// Если ключ передан
	if(!key.empty())
		// Выполняем удаление указанной записи
		return this->erase(this->idw(key));
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод удаления записи по идентификатору ключа
 *
 * @param idw идентификатор ключа для удаления записи
 * @return    результат работы функции
 */
bool awh::BinBox::erase(const uint64_t idw) noexcept {
	// Переменная результата
	bool result = false;
	// Если бинарные данные контейнера созданы
	if((idw > 0) && !this->_records.empty()){
		// Выполняем поиск ключа в базе данных
		auto i = this->_records.find(idw);
		// Если ключ найден в базе данных
		if((result = (i != this->_records.end())))
			// Выполняем удаление записи указанных данных
			this->_records.erase(i);
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief erase Метод удаления записи по итератору
 *
 * @param it итератор записи для удаления
 * @return   следующий итератор
 */
awh::BinBox::iterator_t awh::BinBox::erase(const iterator_t & it) noexcept {
	// Выполняем удаление указанного заголовка
	auto i = this->_records.erase(static_cast <iterator_t::iterator> (const_cast <iterator_t &> (it)));
	// Возвращаем результат
	return iterator_t(i, this->_fmk, this->_log);
}
/**
 * @brief Метод загрузки контейнера из файла
 *
 * @param filename путь к файлу для загрузки
 */
void awh::BinBox::load(string_view filename) noexcept {
	// Если путь к файлу указан и объект работы с файловой системой создан
	if(!filename.empty() && (this->_fs != nullptr)){
		// Создаём бинарный буфер данных для загрузки из файла
		vector <uint8_t> buffer;
		// Выполняем загрузку данных из файла
		this->_fs->read(filename, buffer);
		// Извлекаем из бинарного буфера дампа, записи контейнера
		::binbox::dump(this->_name, this->_version, buffer, this->_records, this->_fmk, this->_log);
	}
}
/**
 * @brief Метод сохранения контейнера в файл
 *
 * @param filename путь к файлу для сохранения
 */
void awh::BinBox::save(string_view filename) noexcept {
	// Если путь к файлу указан и объект работы с файловой системой создан
	if(!filename.empty() && (this->_fs != nullptr)){
		// Переменная результата
		vector <uint8_t> result;
		// Выполняем дамп бинарного контейнера в бинарный буфер данных
		::binbox::dump(this->_name, this->_version, this->_records, result, this->_fmk, this->_log);
		// Если есть данные для сохранения дампа
		if(!result.empty())
			// Записываем в файл бинарные данные
			this->_fs->write(filename, result);
	}
}
/**
 * @brief Метод генерирования идентификатора ключа
 *
 * @param key ключ для генерации
 * @return    идентификатор ключа
 */
uint64_t awh::BinBox::idw(string_view key) const noexcept {
	// Переменная результата
	uint64_t result = 0;
	// Если объект работы с криптографией инициализирован
	if(this->_crypto != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если название функции передано
			if(!key.empty())
				// Получаем идентификатор обратного вызова
				return this->_crypto->hash <uint64_t> (key);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(key), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод проверки на существование ключа
 *
 * @param key ключ для проверки
 * @return    результат проверки
 */
bool awh::BinBox::has(string_view key) noexcept {
	// Если ключ передан
	if(!key.empty())
		// Выполняем проверку существования ключа
		return this->has(this->idw(key));
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод проверки на существование идентификатора ключа
 *
 * @param idw идентификатор ключа для проверки
 * @return    результат проверки
 */
bool awh::BinBox::has(const uint64_t idw) noexcept {
	// Если ключ передан
	if(idw > 0){
		// Если бинарные данные контейнера созданы
		if(!this->_records.empty())
			// Формируем результат
			return (this->_records.find(idw) != this->_records.end());
	}
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод получения размера данных по ключу
 *
 * @param key ключ записи
 * @return    размер данных записи
 */
size_t awh::BinBox::size(string_view key) const noexcept {
	// Выполняем извлечение размера данных в бинарном контейнере
	return this->size(this->idw(key));
}
/**
 * @brief Метод получения размера данных по идентификатору ключа
 *
 * @param idw идентификатор ключа
 * @return    размер данных записи
 */
size_t awh::BinBox::size(const uint64_t idw) const noexcept {
	// Если ключ передан
	if(idw > 0){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Если записи в бинарном контейнере присутствуют
			if(!this->_records.empty()){
				// Выполняем поиск ключа в базе данных
				auto i = this->_records.find(idw);
				// Если ключ найден в базе данных
				if(i != this->_records.end()){
					// Если данных достаточно в контейнере
					if((i->second.size > 0) && (i->second.buffer != nullptr))
						// Выполняем извлечение размера данных в контейнере
						return i->second.size;
					// Если контейнер по каким-то причинам оказался битым
					else this->_log->print("Metadata [%llu] in container does not match content", log_t::flag_t::CRITICAL, idw);
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, __FUNCTION__, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод получения данных по ключу
 *
 * @param key ключ записи
 * @return    запрашиваемые данные по ключу
 */
void * awh::BinBox::get(string_view key) const noexcept {
	// Выполняем извлечение данных из бинарного контейнера
	return this->get(this->idw(key));
}
/**
 * @brief Метод получения данных по идентификатору ключа
 *
 * @param idw идентификатор ключа
 * @return    запрашиваемые данные по идентификатору ключа
 */
void * awh::BinBox::get(const uint64_t idw) const noexcept {
	// Если ключ передан
	if(idw > 0){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Если записи в бинарном контейнере присутствуют
			if(!this->_records.empty()){
				// Выполняем поиск ключа в базе данных
				auto i = this->_records.find(idw);
				// Если ключ найден в базе данных
				if(i != this->_records.end()){
					// Если данных достаточно в контейнере
					if((i->second.size > 0) && (i->second.buffer != nullptr))
						// Выполняем извлечение запрашиваемых данных
						return i->second.buffer.get();
					// Если контейнер по каким-то причинам оказался битым
					else this->_log->print("Metadata [%llu] in container does not match content", log_t::flag_t::CRITICAL, idw);
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, __FUNCTION__, error.what());
			#endif
		}
	}
	// Возвращаем значение по умолчанию
	return nullptr;
}
/**
 * @brief Шаблон метода чтения бинарных данных из бинарного контейнера
 *
 * @tparam T тип извлекаемого значения
 */
template <typename T>
/**
 * @brief Метод чтения бинарных данных из бинарного контейнера
 *
 * @param key ключ записи
 * @return    результат работы функции
 */
T awh::BinBox::get(string_view key) noexcept {
	// Выполняем извлечение данных из бинарного контейнера
	return this->get <T> (this->idw(key));
}
/**
 * Объявляем прототипы для чтения бинарных данных из бинарного контейнера
 */
template char awh::BinBox::get <char> (string_view) noexcept;
template bool awh::BinBox::get <bool> (string_view) noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template int8_t awh::BinBox::get <int8_t> (string_view) noexcept;
#endif
template uint8_t awh::BinBox::get <uint8_t> (string_view) noexcept;
template int16_t awh::BinBox::get <int16_t> (string_view) noexcept;
template uint16_t awh::BinBox::get <uint16_t> (string_view) noexcept;
template int32_t awh::BinBox::get <int32_t> (string_view) noexcept;
template uint32_t awh::BinBox::get <uint32_t> (string_view) noexcept;
template int64_t awh::BinBox::get <int64_t> (string_view) noexcept;
template uint64_t awh::BinBox::get <uint64_t> (string_view) noexcept;
template float awh::BinBox::get <float> (string_view) noexcept;
template double awh::BinBox::get <double> (string_view) noexcept;
template string awh::BinBox::get <string> (string_view) noexcept;
template vector <char> awh::BinBox::get <vector <char>> (string_view) noexcept;
template vector <uint8_t> awh::BinBox::get <vector <uint8_t>> (string_view) noexcept;
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::BinBox::get <size_t> (string_view) noexcept;
	template ssize_t awh::BinBox::get <ssize_t> (string_view) noexcept;
#endif
/**
 * @brief Шаблон метода чтения данных из бинарного контейнера
 *
 * @tparam T тип извлекаемого значения
 */
template <typename T>
/**
 * @brief Метод чтения бинарных данных из бинарного контейнера
 *
 * @param idw идентификатор ключа
 * @return    результат работы функции
 */
T awh::BinBox::get(const uint64_t idw) noexcept {
	// Переменная результата
	T result;
	// Если данные являются основными
	if(is_integral <T>::value || is_floating_point <T>::value || is_array <T>::value){
		// Буфер результата по умолчанию
		uint8_t buffer[sizeof(T)];
		// Заполняем нулями буфер данных
		::memset(buffer, 0, sizeof(T));
		// Выполняем установку результата по умолчанию
		::memcpy(&result, reinterpret_cast <T *> (buffer), sizeof(T));
	}
	// Если ключ передан
	if(idw > 0){
		// Создаём буфер данных для извлечения данных
		vector <uint8_t> buffer;
		// Если данные буфера были извлечены удачно
		if(this->get(idw, buffer) && !buffer.empty()){
			// Если данные являются основными
			if(is_class <T>::value || is_array <T>::value ||
			   is_enum <T>::value || is_integral <T>::value || is_floating_point <T>::value)
				// Выполняем получение данных
				::binbox::extract(buffer, result);
			// Если извлекаемые данные не поддерживаются, то выводим сообщение об ошибке
			else this->_log->print("Data type to set [%llu] could not be determined", log_t::flag_t::WARNING, idw);
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * Объявляем прототипы для чтения данных из бинарного контейнера
 */
template char awh::BinBox::get <char> (const uint64_t) noexcept;
template bool awh::BinBox::get <bool> (const uint64_t) noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template int8_t awh::BinBox::get <int8_t> (const uint64_t) noexcept;
#endif
template uint8_t awh::BinBox::get <uint8_t> (const uint64_t) noexcept;
template int16_t awh::BinBox::get <int16_t> (const uint64_t) noexcept;
template uint16_t awh::BinBox::get <uint16_t> (const uint64_t) noexcept;
template int32_t awh::BinBox::get <int32_t> (const uint64_t) noexcept;
template uint32_t awh::BinBox::get <uint32_t> (const uint64_t) noexcept;
template int64_t awh::BinBox::get <int64_t> (const uint64_t) noexcept;
template uint64_t awh::BinBox::get <uint64_t> (const uint64_t) noexcept;
template float awh::BinBox::get <float> (const uint64_t) noexcept;
template double awh::BinBox::get <double> (const uint64_t) noexcept;
template string awh::BinBox::get <string> (const uint64_t) noexcept;
template vector <char> awh::BinBox::get <vector <char>> (const uint64_t) noexcept;
template vector <uint8_t> awh::BinBox::get <vector <uint8_t>> (const uint64_t) noexcept;
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template size_t awh::BinBox::get <size_t> (const uint64_t) noexcept;
	template ssize_t awh::BinBox::get <ssize_t> (const uint64_t) noexcept;
#endif
/**
 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
 *
 * @param key    ключ записи
 * @param buffer бинарный буфер для чтения данных
 * @return       результат работы функции
 */
bool awh::BinBox::get(string_view key, vector <uint8_t> & buffer) noexcept {
	// Если ключ передан
	if(!key.empty())
		// Выполняем извлечение запрошенной записи
		return this->get(this->idw(key), buffer);
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
 *
 * @param idw    идентификатор ключа
 * @param buffer бинарный буфер для чтения данных
 * @return       результат работы функции
 */
bool awh::BinBox::get(const uint64_t idw, vector <uint8_t> & buffer) noexcept {
	// Переменная результата
	bool result = false;
	// Если ключ передан
	if(idw > 0){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Если записи в бинарном контейнере присутствуют
			if(!this->_records.empty()){
				// Выполняем поиск ключа в базе данных
				auto i = this->_records.find(idw);
				// Если ключ найден в базе данных
				if((result = (i != this->_records.end()))){
					// Выполняем очистку буфера данных
					buffer.clear();
					// Если данных достаточно в контейнере
					if((result = ((i->second.size > 0) && (i->second.buffer != nullptr))))
						// Выполняем извлечение запрашиваемых данных
						buffer.insert(buffer.end(), i->second.buffer.get(), i->second.buffer.get() + i->second.size);
					// Если контейнер по каким-то причинам оказался битым
					else this->_log->print("Metadata [%llu] in container does not match content", log_t::flag_t::CRITICAL, idw);
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw, buffer.size()), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, __FUNCTION__, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
 *
 * @param key    ключ записи
 * @param buffer бинарный буфер для чтения данных
 * @param size   размер извлекаемых бинарных данных
 * @return       результат работы функции
 */
bool awh::BinBox::get(string_view key, uint8_t ** buffer, size_t * size) noexcept {
	// Если ключ передан
	if(!key.empty())
		// Выполняем извлечение запрошенной записи
		return this->get(this->idw(key), buffer, size);
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод чтения данных из бинарного контейнера в бинарный буфер
 *
 * @param idw    идентификатор ключа
 * @param buffer бинарный буфер для чтения данных
 * @param size   размер извлекаемых бинарных данных
 * @return       результат работы функции
 */
bool awh::BinBox::get(const uint64_t idw, uint8_t ** buffer, size_t * size) noexcept {
	// Переменная результата
	bool result = false;
	// Если ключ передан и указатели для возврата данных корректны
	if((idw > 0) && (buffer != nullptr) && (size != nullptr)){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Если записи в бинарном контейнере присутствуют
			if(!this->_records.empty()){
				// Выполняем поиск ключа в базе данных
				auto i = this->_records.find(idw);
				// Если ключ найден в базе данных
				if((result = (i != this->_records.end()))){
					// Если данных достаточно в контейнере
					if((result = ((i->second.size > 0) && (i->second.buffer != nullptr)))){
						// Выполняем извлечение размеров данных
						(* size) = i->second.size;
						// Выполняем извлечение запрашиваемых данных
						(* buffer) = i->second.buffer.get();
					// Если контейнер по каким-то причинам оказался битым
					} else this->_log->print("Metadata [%llu] in container does not match content", log_t::flag_t::CRITICAL, idw);
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
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, __FUNCTION__, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Шаблон метода добавления данных в бинарный контейнер
 *
 * @tparam T тип добавляемого значения
 */
template <typename T>
/**
 * @brief Метод добавления простого типа данных
 *
 * @param idw   идентификатор ключа
 * @param value значение данных
 * @return      результат работы функции
 */
bool awh::BinBox::add(const uint64_t idw, const T value) noexcept {
	// Если ключ записи передан и данные являются простыми
	if(idw > 0)
		// Выполняем добавление данных
		return this->add(idw, reinterpret_cast <const uint8_t *> (&value), sizeof(value));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * Объявляем прототипы для добавления простого типа данных в бинарный контейнер
 */
template bool awh::BinBox::add <char> (const uint64_t, const char) noexcept;
template bool awh::BinBox::add <bool> (const uint64_t, const bool) noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template bool awh::BinBox::add <int8_t> (const uint64_t, const int8_t) noexcept;
#endif
template bool awh::BinBox::add <uint8_t> (const uint64_t, const uint8_t) noexcept;
template bool awh::BinBox::add <int16_t> (const uint64_t, const int16_t) noexcept;
template bool awh::BinBox::add <uint16_t> (const uint64_t, const uint16_t) noexcept;
template bool awh::BinBox::add <int32_t> (const uint64_t, const int32_t) noexcept;
template bool awh::BinBox::add <uint32_t> (const uint64_t, const uint32_t) noexcept;
template bool awh::BinBox::add <int64_t> (const uint64_t, const int64_t) noexcept;
template bool awh::BinBox::add <uint64_t> (const uint64_t, const uint64_t) noexcept;
template bool awh::BinBox::add <float> (const uint64_t, const float) noexcept;
template bool awh::BinBox::add <double> (const uint64_t, const double) noexcept;
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template bool awh::BinBox::add <size_t> (const uint64_t, const size_t) noexcept;
	template bool awh::BinBox::add <ssize_t> (const uint64_t, const ssize_t) noexcept;
#endif
/**
 * @brief Шаблон метода добавления бинарного буфера данных
 *
 * @tparam T тип устанавливаемого значения
 */
template <typename T>
/**
 * @brief Метод добавления бинарного буфера данных
 *
 * @param idw   идентификатор ключа
 * @param value значение данных
 * @return      результат работы функции
 */
bool awh::BinBox::add(const uint64_t idw, const vector <T> & value) noexcept {
	// Если ключ записи передан и данные являются массивом
	if(idw > 0)
		// Выполняем добавление данных
		return this->add(idw, reinterpret_cast <const uint8_t *> (value.data()), value.size() * sizeof(T));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * Объявляем прототипы для добавления бинарного буфера данных в бинарный контейнер
 */
template bool awh::BinBox::add <char> (const uint64_t, const vector <char> &) noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template bool awh::BinBox::add <int8_t> (const uint64_t, const vector <int8_t> &) noexcept;
#endif
template bool awh::BinBox::add <uint8_t> (const uint64_t, const vector <uint8_t> &) noexcept;
template bool awh::BinBox::add <int16_t> (const uint64_t, const vector <int16_t> &) noexcept;
template bool awh::BinBox::add <uint16_t> (const uint64_t, const vector <uint16_t> &) noexcept;
template bool awh::BinBox::add <int32_t> (const uint64_t, const vector <int32_t> &) noexcept;
template bool awh::BinBox::add <uint32_t> (const uint64_t, const vector <uint32_t> &) noexcept;
template bool awh::BinBox::add <int64_t> (const uint64_t, const vector <int64_t> &) noexcept;
template bool awh::BinBox::add <uint64_t> (const uint64_t, const vector <uint64_t> &) noexcept;
template bool awh::BinBox::add <float> (const uint64_t, const vector <float> &) noexcept;
template bool awh::BinBox::add <double> (const uint64_t, const vector <double> &) noexcept;
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template bool awh::BinBox::add <size_t> (const uint64_t, const vector <size_t> &) noexcept;
	template bool awh::BinBox::add <ssize_t> (const uint64_t, const vector <ssize_t> &) noexcept;
#endif
/**
 * @brief Метод добавления строкового типа данных
 *
 * @param idw   идентификатор ключа
 * @param value значение данных
 * @return      результат работы функции
 */
bool awh::BinBox::add(const uint64_t idw, const string & value) noexcept {
	// Если ключ записи передан
	if((idw > 0) && !value.empty())
		// Выполняем добавление данных
		return this->add(idw, value.c_str(), value.length());
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод добавления бинарных данных
 *
 * @param idw    идентификатор ключа
 * @param buffer буфер для записи данных
 * @param size   размер буфера для записи данных
 * @return       результат работы функции
 */
bool awh::BinBox::add(const uint64_t idw, const void * buffer, const size_t size) noexcept {
	// Переменная результата
	bool result = false;
	// Если ключ и данные для записи переданы
	if((idw > 0) && (buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем обработку ошибки
		 */
		try {
			// Выполняем поиск ключа в базе данных
			auto i = this->_records.find(idw);
			// Если ключ найден в базе данных
			if((result = (i != this->_records.end()))){
				// Выполняем установку размера буфера данных
				i->second.size = size;
				// Выполняем удаление ранее созданных данных
				i->second.buffer.reset(nullptr);
				// Выполняем создание буфера данных
				i->second.buffer = unique_ptr <uint8_t []> (new uint8_t [size]);
				// Выполняем копирование буфера полученных данных
				::memcpy(i->second.buffer.get(), buffer, size);
			// Если такие данные ещё не существуют
			} else {
				// Выполняем добавление данных записи
				auto ret = this->_records.emplace(idw, record_t());
				// Выполняем установку размера буфера данных
				ret.first->second.size = size;
				// Выполняем создание буфера данных
				ret.first->second.buffer = unique_ptr <uint8_t []> (new uint8_t [size]);
				// Выполняем копирование буфера полученных данных
				::memcpy(ret.first->second.buffer.get(), buffer, size);
				// Выполняем получение результата создания записи
				result = ret.second;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw, buffer, size), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, __FUNCTION__, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Шаблон метода добавления данных в бинарный контейнер
 *
 * @tparam T тип добавляемого значения
 */
template <typename T>
/**
 * @brief Метод добавления простого типа данных
 *
 * @param key   ключ записи
 * @param value значение данных
 * @return      результат работы функции
 */
bool awh::BinBox::add(string_view key, const T value) noexcept {
	// Если ключ записи передан и данные являются простыми
	if(!key.empty())
		// Выполняем добавление данных
		return this->add(this->idw(key), reinterpret_cast <const uint8_t *> (&value), sizeof(value));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * Объявляем прототипы для добавления простого типа данных в бинарный контейнер
 */
template bool awh::BinBox::add <char> (string_view, const char) noexcept;
template bool awh::BinBox::add <bool> (string_view, const bool) noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template bool awh::BinBox::add <int8_t> (string_view, const int8_t) noexcept;
#endif
template bool awh::BinBox::add <uint8_t> (string_view, const uint8_t) noexcept;
template bool awh::BinBox::add <int16_t> (string_view, const int16_t) noexcept;
template bool awh::BinBox::add <uint16_t> (string_view, const uint16_t) noexcept;
template bool awh::BinBox::add <int32_t> (string_view, const int32_t) noexcept;
template bool awh::BinBox::add <uint32_t> (string_view, const uint32_t) noexcept;
template bool awh::BinBox::add <int64_t> (string_view, const int64_t) noexcept;
template bool awh::BinBox::add <uint64_t> (string_view, const uint64_t) noexcept;
template bool awh::BinBox::add <float> (string_view, const float) noexcept;
template bool awh::BinBox::add <double> (string_view, const double) noexcept;
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template bool awh::BinBox::add <size_t> (string_view, const size_t) noexcept;
	template bool awh::BinBox::add <ssize_t> (string_view, const ssize_t) noexcept;
#endif
/**
 * @brief Шаблон метода добавления данных в бинарный контейнер
 *
 * @tparam T тип добавляемого значения
 */
template <typename T>
/**
 * @brief Метод добавления бинарного буфера данных
 *
 * @param key   ключ записи
 * @param value значение данных
 * @return      результат работы функции
 */
bool awh::BinBox::add(string_view key, const vector <T> & value) noexcept {
	// Если ключ записи передан и данные являются массивом
	if(!key.empty())
		// Выполняем добавление данных
		return this->add(this->idw(key), value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * Объявляем прототипы для добавления бинарного буфера данных в бинарный контейнер
 */
template bool awh::BinBox::add <char> (string_view, const vector <char> &) noexcept;
/**
 * Реализация под операционные системы кроме Sun Solaris
 */
#if !__sun__
	template bool awh::BinBox::add <int8_t> (string_view, const vector <int8_t> &) noexcept;
#endif
template bool awh::BinBox::add <uint8_t> (string_view, const vector <uint8_t> &) noexcept;
template bool awh::BinBox::add <int16_t> (string_view, const vector <int16_t> &) noexcept;
template bool awh::BinBox::add <uint16_t> (string_view, const vector <uint16_t> &) noexcept;
template bool awh::BinBox::add <int32_t> (string_view, const vector <int32_t> &) noexcept;
template bool awh::BinBox::add <uint32_t> (string_view, const vector <uint32_t> &) noexcept;
template bool awh::BinBox::add <int64_t> (string_view, const vector <int64_t> &) noexcept;
template bool awh::BinBox::add <uint64_t> (string_view, const vector <uint64_t> &) noexcept;
template bool awh::BinBox::add <float> (string_view, const vector <float> &) noexcept;
template bool awh::BinBox::add <double> (string_view, const vector <double> &) noexcept;
/**
 * Если операционной системой является macOS или Linux
 */
#if __APPLE__ || __MACH__ || __Linux__
	template bool awh::BinBox::add <size_t> (string_view, const vector <size_t> &) noexcept;
	template bool awh::BinBox::add <ssize_t> (string_view, const vector <ssize_t> &) noexcept;
#endif
/**
 * @brief Метод добавления строкового типа данных
 *
 * @param key   ключ записи
 * @param value значение данных
 * @return      результат работы функции
 */
bool awh::BinBox::add(string_view key, const string & value) noexcept {
	// Если ключ и данные для записи переданы
	if(!key.empty() && !value.empty())
		// Выполняем добавление записи в контейнер
		return this->add(this->idw(key), value);
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод добавления бинарных данных
 *
 * @param key    ключ записи
 * @param buffer буфер для записи данных
 * @param size   размер буфера для записи данных
 * @return       результат работы функции
 */
bool awh::BinBox::add(string_view key, const void * buffer, const size_t size) noexcept {
	// Если ключ и данные для записи переданы
	if(!key.empty() && (buffer != nullptr) && (size > 0))
		// Выполняем добавление записи в контейнер
		return this->add(this->idw(key), buffer, size);
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод обмена данными контейнерами
 *
 * @param binbox объект для обмена
 */
void awh::BinBox::swap(BinBox & binbox) noexcept {
	// Выполняем обмен названиями контейнеров
	this->_name.swap(binbox._name);
	// Выполняем обмен записями в контейнерах
	this->_records.swap(binbox._records);
	// Выполняем обмен объектами работы  файловой системой
	std::swap(this->_fs, binbox._fs);
	// Выполняем обмен объектами работы с криптографией
	std::swap(this->_crypto, binbox._crypto);
}
/**
 * @brief Метод получения конечного итератора
 *
 * @return конечный итератор
 */
awh::BinBox::iterator_t awh::BinBox::end() noexcept {
	// Возвращаем результат
	return iterator_t(this->_records.end(), this->_fmk, this->_log);
}
/**
 * @brief Метод получение начального итератора
 *
 * @return начальный итератор
 */
awh::BinBox::iterator_t awh::BinBox::begin() noexcept {
	// Возвращаем результат
	return iterator_t(this->_records.begin(), this->_fmk, this->_log);
}
/**
 * @brief Метод поиска записи по ключу
 *
 * @param key ключ для поиска записи
 * @return     итератор указанного ключа
 */
awh::BinBox::iterator_t awh::BinBox::find(string_view key) noexcept {
	// Если ключ для поиска передан
	if(!key.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Извлекаем текущий итератор
			return iterator_t(this->_records.find(this->idw(key)), this->_fmk, this->_log);
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
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(key), log_t::flag_t::CRITICAL, error.what());
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
	return iterator_t(this->_records.end(), this->_fmk, this->_log);
}
/**
 * @brief Метод поиска записи по идентификатору ключа
 *
 * @param idw идентификатор ключа для поиска записи
 * @return    итератор указанного идентификатора ключа
 */
awh::BinBox::iterator_t awh::BinBox::find(const uint64_t idw) noexcept {
	// Если идентификатор записи передан
	if(idw > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Извлекаем текущий итератор
			return iterator_t(this->_records.find(idw), this->_fmk, this->_log);
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
					this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(idw), log_t::flag_t::CRITICAL, error.what());
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
	return iterator_t(this->_records.end(), this->_fmk, this->_log);
}
/**
 * @brief Оператор проверки на существование контейнера
 *
 * @return результат проверки
 */
awh::BinBox::operator bool() const noexcept {
	// Возвращаем результат проверки
	return !this->_records.empty();
}
/**
 * @brief Оператор извлечения бинарного буфера данных
 *
 * @return бинарный буфер данных
 */
awh::BinBox::operator vector <uint8_t> () const noexcept {
	// Переменная результата
	vector <uint8_t> result;
	// Выполняем дамп бинарного контейнера в бинарный буфер данных
	::binbox::dump(this->_name, this->_version, this->_records, result, this->_fmk, this->_log);
	// Возвращаем результат
	return result;
}
/**
 * @brief Оператор перемещения
 *
 * @param binbox объект для перемещения
 * @return       текущий контейнер буфера
 */
awh::BinBox & awh::BinBox::operator = (BinBox && binbox) noexcept {
	// Выполняем копирование объекта работы с файловой системой
	this->_fs = binbox._fs;
	// Выполняем копирование объекта работы с криптографией
	this->_crypto = binbox._crypto;
	// Выполняем перемещение записей контейнера
	this->_records = ::move(binbox._records);
	// Выполняем копирование версии контейнера
	this->_version = binbox._version;
	// Выполняем копирование названия контейнера
	this->_name = ::move(binbox._name);
	// Восстанавливаем название контейнера в перемещаемом объекте
	binbox._name = AWH_SHORT_NAME;
	// Восстанавливаем версию контейнера в перемещаемом объекте
	binbox._version = static_cast <uint32_t> (version_t(AWH_VERSION));
	// Возвращаем текущее значение объекта
	return (* this);
}
/**
 * @brief Оператор установки буфера бинарных данных
 *
 * @param buffer буфер бинарных данных
 * @return       текущий объект
 */
awh::BinBox & awh::BinBox::operator = (const vector <uint8_t> & buffer) noexcept {
	// Извлекаем из бинарного буфера дампа, записи контейнера
	::binbox::dump(this->_name, this->_version, buffer, this->_records, this->_fmk, this->_log);
	// Возвращаем текущее значение объекта
	return (* this);
}
/**
 * @brief Конструктор перемещения
 *
 * @param binbox объект для перемещения
 */
awh::BinBox::BinBox(BinBox && binbox) noexcept {
	// Выполняем копирование объекта работы с файловой системой
	this->_fs = binbox._fs;
	// Выполняем копирование объекта работы с криптографией
	this->_crypto = binbox._crypto;
	// Выполняем перемещение записей контейнера
	this->_records = ::move(binbox._records);
	// Выполняем копирование версии контейнера
	this->_version = binbox._version;
	// Выполняем копирование названия контейнера
	this->_name = ::move(binbox._name);
	// Восстанавливаем название контейнера в перемещаемом объекте
	binbox._name = AWH_SHORT_NAME;
	// Восстанавливаем версию контейнера в перемещаемом объекте
	binbox._version = static_cast <uint32_t> (version_t(AWH_VERSION));
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::BinBox::BinBox(const fmk_t * fmk, const log_t * log) noexcept :
 _name{AWH_SHORT_NAME}, _fs(nullptr), _crypto(nullptr), _fmk(fmk), _log(log) {
	// Выполняем создание объекта работы с файловой системой
	this->_fs = make_unique <fs_t> (fmk, log);
	// Выполняем инициализацию объекта работы с криптографией
	this->_crypto = make_unique <crypto_t> (fmk, log);
	// Выполняем установку версии бинарного контейнера
	this->_version = static_cast <uint32_t> (version_t(AWH_VERSION));
}
/**
 * @brief Деструктор
 *
 */
awh::BinBox::~BinBox() noexcept {}
