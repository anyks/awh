/**
 * @file: headers.cpp
 * @date: 2025-10-03
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
 * Наши модули
 */
#include <cityhash/city.h>

/**
 * Подключаем заголовочный файл
 */
#include <http/headers.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Оператор извлечения указателя заголовка
 *
 * @return указатель заголовка
 */
awh::Headers::Iterator::pointer awh::Headers::Iterator::operator -> () noexcept {
	// Выводим результат
	return &this->_it->second;
}
/**
 * @brief Оператор разыменования заголовка
 *
 * @return значение заголовка
 */
awh::Headers::Iterator::reference awh::Headers::Iterator::operator * () const noexcept {
	// Выводим результат
	return this->_it->second;
}
/**
 * @brief Оператор смещения вперед
 *
 * @return значение текущего итератора
 */
awh::Headers::Iterator & awh::Headers::Iterator::operator ++ () noexcept {
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
	return (* this);
}
/**
 * @brief Оператор смещения назад
 *
 * @return значение текущего итератора
 */
awh::Headers::Iterator & awh::Headers::Iterator::operator -- () noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем смещение итератора
		--this->_it;
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
	return (* this);
}
/**
 * @brief Оператор сравнения соответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 */
bool awh::Headers::Iterator::operator == (const iterator_t & other) const noexcept {
	// Выводим результат
	return (
		(this->_it->first == other._it->first) &&
		this->_fmk->compare(this->_it->second.second, other._it->second.second)
	);
}
/**
 * @brief Оператора сравнения несоответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 */
bool awh::Headers::Iterator::operator != (const iterator_t & other) const noexcept {
	// Выводим результат
	return (
		(this->_it->first != other._it->first) ||
		!this->_fmk->compare(this->_it->second.second, other._it->second.second)
	);
}
/**
 * @brief Метод генерации идентификатора заголовка
 *
 * @param name название заголовка для генерации идентификатора
 * @return     сгенерированный идентификатор заголовка
 */
uint32_t awh::Headers::id(const string name) const noexcept {
	// Результат работы функции
	uint32_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Переводим название заголовка в нижний регистр
		this->_fmk->transform(name, fmk_t::transform_t::LOWER);
		// Если размер имени умещается в 4 байт
		if(name.size() <= 4)
			// Выполняем копирование данных имени
			::memcpy(&result, name.data(), name.size());
		// Получаем идентификатор обратного вызова
		else return ::CityHash32(name.c_str(), name.size());
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим полученный результат
	return result;
}
/**
 * @brief Метод очистки всех данных очереди
 *
 */
void awh::Headers::clear() noexcept {
	// Если заголовки заполнены
	if(!this->_items.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Выполняем сброс индекса
			this->_items.clear();
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
void awh::Headers::reset() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем очистку буфера данных
		this->clear();
		// Выполняем блокировку потока
		const lock_guard lock(this->_mtx);
		// Выполняем освобождение памяти индекса
		std::multimap <decltype(this->_items)::key_type, decltype(this->_items)::mapped_type> ().swap(this->_items);
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
bool awh::Headers::empty() const noexcept {
	// Выводим проверку на пустоту очереди
	return this->_items.empty();
}
/**
 * @brief Метод печати содержимого заголовков в формате HTTP/1.1
 *
 * @return заголовки в формате HTTP/1.1
 */
string awh::Headers::print() const noexcept {
	// Результат работы функции
	string result = "";
	// Если список заголовок не пустой
	if(!this->_items.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Заголовок для вывода
			string header = "";
			// Выполняем меребор всего списка заголовков
			for(auto & item : this->_items){
				// Выполняем извлечение заголовка
				header = item.second.first;
				// Выполняем формирование заголовка
				result.append(this->_fmk->format("%s: %s\r\n", this->_fmk->transform(header, fmk_t::transform_t::SMART).c_str(), item.second.second.c_str()));
			}
			// Если результат уже собран
			if(!result.empty())
				// Добавляем последний разделитель
				result.append("\r\n");
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
 * @brief Метод печати содержимого заголовка
 *
 * @param name печать заголовка в формате HTTP/1.1
 * @return     распечатанный заголовок
 */
string awh::Headers::print(const string & name) const noexcept {
	// Если название заголовка передано
	if(!name.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск указанного заголовка
			auto i = this->_items.find(this->id(name));
			// Если заголовок найден
			if(i != this->_items.end()){
				// Выполняем извлечение заголовка
				const string header = i->second.first;
				// Выводим полученный результат
				return this->_fmk->format("%s: %s", this->_fmk->transform(header, fmk_t::transform_t::SMART).c_str(), i->second.second.c_str());
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
	return "";
}
/**
 * @brief Метод удаления заголовка
 *
 * @param name название удаляемого заголовка
 */
void awh::Headers::erase(const string & name) noexcept {
	// Если название заголовка передано
	if(!name.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем идентификатор названия заголовка
			const uint32_t id = this->id(name);
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
			// Выполняем поиск нужного нам заголовка
			auto i = this->_items.find(id);
			// Выполняем переход по всем оставшимся загловкам
			for(auto j = i; j != this->_items.end();){
				// Если заголовок соответствует
				if(id == j->first)
					// Выполняем удаление заголовка
					j = this->_items.erase(j);
				// Выходим из цикла
				else break;
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод проверки существования заголовка
 *
 * @param name название заголовка для проверки
 * @return     результат выполнения проверки
 */
bool awh::Headers::has(const string & name) const noexcept {
	// Если название заголовка передано
	if(!name.empty())
		// Выполняем проверку существования заголовка
		return (this->_items.find(this->id(name)) != this->_items.end());
	// Выводим результат
	return false;
}
/**
 * @brief Количество добавленных заголовков
 *
 * @param name название заголовка количество которых нужно определить
 * @return     количество добавленных заголовков
 */
size_t awh::Headers::count(const string & name) const noexcept {
	// Если название заголовка передано
	if(!name.empty())
		// Выполняем определение количество заголовков
		return this->_items.count(this->id(name));
	// Выводим количество записей в очереди
	return this->_items.size();
}
/**
 * @brief Метод извлечения содержимого заголовка
 *
 * @param name название заголовка
 * @return     содержимое заголовка
 */
const string & awh::Headers::at(const string & name) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Если название заголовка передано
	if(!name.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск нужного нам заголовка
			auto i = this->_items.find(this->id(name));
			// Если нужный нам заголовок найден
			if(i != this->_items.end())
				// Извлекаем содержимое заголовка
				return i->second.second;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод извлечения названий заголовков
 *
 * @return список названий заголовков
 */
vector <string> awh::Headers::names() const noexcept {
	// Результат работы функции
	vector <string> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Заголовок для вывода
		string header = "";
		// Выполняем перебор всего списка найденных заголовков
		for(auto & item : this->_items){
			// Выполняем извлечение заголовка
			header = item.second.first;
			// Добавляем в список собранные названия заголовков
			result.push_back(this->_fmk->transform(header, fmk_t::transform_t::SMART));
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
 * @brief Метод вывода списка значений одинаковых заголовков
 *
 * @param name название заголовка
 * @return     список значений одинаковых заголовков
 */
vector <string> awh::Headers::range(const string & name) const noexcept {
	// Результат работы функции
	vector <string> result;
	// Если название заголовка передано
	if(!name.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем поиск поиск нужных нам записей
			auto ret = this->_items.equal_range(this->id(name));
			// Выполняем перебор всего списка найденных заголовков
			for(auto i = ret.first; i != ret.second; ++i)
				// Добавляем в список собранное содержиое заголовков
				result.push_back(i->second.second);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Шаблон добавления нового заголовка
 *
 * @tparam Name    тип названия добавляемого заголовка
 * @tparam Content тип содержимого добавляемого заголовка
 */
template <typename Name, typename Content>
/**
 * @brief Метод добавления нового заголовка
 *
 * @param name    название заголовка
 * @param content содержимое заголовка
 * @return        общее количество заголовков
 */
size_t awh::Headers::emplace(Name && name, Content && content) noexcept {
	// Если данные переданы верные
	if(!name.empty() && !content.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем общий размер записей
			const size_t size = (name.size() + content.size() + sizeof(uint32_t) + (sizeof(size_t) * 2));
			// Если по памяти мы ещё проходим
			if(size <= this->_max.memory){
				// Если по количеству записей мы проходим
				if(this->_items.size() < this->_max.records){
					// Выполняем блокировку потока
					const lock_guard lock(this->_mtx);
					// Выполняем добавление нового заголовка
					this->_items.emplace(
						this->id(name),
						std::make_pair(
							std::forward <Name> (name),
							std::forward <Content> (content)
						)
					);
				// Если мы достигли максимального количества записей
				} else {
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug(
							"You are trying to add %s headings out of a maximum of %s",
							__PRETTY_FUNCTION__, std::make_tuple(name, content), log_t::flag_t::CRITICAL,
							this->_fmk->bytes(static_cast <double> (this->_items.size())).c_str(),
							this->_fmk->bytes(static_cast <double> (this->_max.records)).c_str()
						);
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print(
							"You are trying to add %s headings out of a maximum of %s",
							log_t::flag_t::CRITICAL, this->_fmk->bytes(static_cast <double> (this->_items.size())).c_str(),
							this->_fmk->bytes(static_cast <double> (this->_max.records)).c_str()
						);
					#endif
				}
			// Выводим сообщение об ошибке
			} else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug(
						"Headers container cannot accommodate %s of memory, since its maximum size is %s of memory",
						__PRETTY_FUNCTION__, std::make_tuple(name, content), log_t::flag_t::CRITICAL,
						this->_fmk->bytes(static_cast <double> (size)).c_str(),
						this->_fmk->bytes(static_cast <double> (this->_max.memory)).c_str()
					);
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print(
						"Headers container cannot accommodate %s of memory, since its maximum size is %s of memory",
						log_t::flag_t::CRITICAL, this->_fmk->bytes(static_cast <double> (size)).c_str(),
						this->_fmk->bytes(static_cast <double> (this->_max.memory)).c_str()
					);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name, content), log_t::flag_t::CRITICAL, error.what());
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
	return this->_items.size();
}
/**
 * Объявляем прототипы для метода добавления нового заголовка
 */
template size_t awh::Headers::emplace <string &&, string &&> (string &&, string &&) noexcept;
template size_t awh::Headers::emplace <string &&, const string &> (string &&, const string &) noexcept;
template size_t awh::Headers::emplace <const string &, string &&> (const string &, string &&) noexcept;
template size_t awh::Headers::emplace <const string &, const string &> (const string &, const string &) noexcept;
/**
 * @brief Метод добавления нового заголовка
 *
 * @param name    название заголовка
 * @param content содержимое заголовка
 * @return        общее количество заголовков
 */
size_t awh::Headers::emplace(const char * name, const char * content) noexcept {
	// Выполняем добавление записи
	return this->emplace(::move(string{name}), ::move(string{content}));
}
/**
 * @brief Метод добавления нового заголовка
 *
 * @param name    название заголовка
 * @param content содержимое заголовка
 * @return        общее количество заголовков
 */
size_t awh::Headers::emplace(const char * name, const string & content) noexcept {
	// Выполняем добавление записи
	return this->emplace(::move(string{name}), content);
}
/**
 * @brief Метод добавления нового заголовка
 *
 * @param name    название заголовка
 * @param content содержимое заголовка
 * @return        общее количество заголовков
 */
size_t awh::Headers::emplace(const string & name, const char * content) noexcept {
	// Выполняем добавление записи
	return this->emplace(name, ::move(string{content}));
}
/**
 * @brief Метод установки максимального размера потребления памяти
 *
 * @param size максимальный размер потребления памяти
 */
void awh::Headers::setMaxMemory(const size_t size) noexcept {
	// Если максимальный размер потребляемой памяти передан
	if(size > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
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
}
/**
 * @brief Метод установки максимального количества заголовков
 *
 * @param count максимальное количество заголовков
 */
void awh::Headers::setMaxRecords(const size_t count) noexcept {
	// Если количество записей передано
	if(count > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем блокировку потока
			const lock_guard lock(this->_mtx);
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
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(count), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод обмена заголовками
 *
 * @param headers заголовки для обмена
 */
void awh::Headers::swap(headers_t & headers) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока текущего контейнера заголовков
		const lock_guard lock1(this->_mtx);
		// Выполняем блокировку потока стороннего контейнера заголовков
		const lock_guard lock2(headers._mtx);
		// Выполняем обмен индексами заголовков
		this->_items.swap(headers._items);
		// Выполняем обмен максимальными размерами памяти
		this->_max.memory += (headers._max.memory - (headers._max.memory = this->_max.memory));
		// Выполняем обмен максимальными количествами записей
		this->_max.records += (headers._max.records - (headers._max.records = this->_max.records));
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
 * @brief Метод получения конечного итератора
 *
 * @return конечный итератор
 */
awh::Headers::iterator_t awh::Headers::end() noexcept {
	// Выводим результат
	return iterator_t(this->_items.end(), this->_fmk, this->_log);
}
/**
 * @brief Метод получение начального итератора
 *
 * @return начальный итератор
 */
awh::Headers::iterator_t awh::Headers::begin() noexcept {
	// Выводим результат
	return iterator_t(this->_items.begin(), this->_fmk, this->_log);
}
/**
 * @brief Метод поиска указанного заголовка
 *
 * @param name название заголовка для поиска
 * @return     итератор указанного заголовка
 */
awh::Headers::iterator_t awh::Headers::find(const string & name) noexcept {
	// Если название заголовка передано
	if(!name.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Извлекаем текущий итератор
			return iterator_t(this->_items.find(this->id(name)), this->_fmk, this->_log);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(name), log_t::flag_t::CRITICAL, error.what());
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
	return iterator_t(this->_items.end(), this->_fmk, this->_log);
}
/**
 * @brief Оператор получения количество заголовков
 *
 * @return количество заголовков
 */
awh::Headers::operator size_t() const noexcept {
	// Выводим результат
	return this->_items.size();
}
/**
 * @brief Оператор печати содержимого заголовков в формате HTTP/1.1
 *
 * @return заголовки в формате HTTP/1.1
 */
awh::Headers::operator string() const noexcept {
	// Выводим результат
	return this->print();
}
/**
 * @brief Оператор получения списка заголовков в том виде как они есть
 *
 * @return список всех добавленных заголовков
 */
awh::Headers::operator vector <item_t> () const noexcept {
	// Результат работы функции
	vector <item_t> result;
	// Если контейнер заголовков заполнен
	if(!this->_items.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Заголовок для вывода
			string header = "";
			// Выполняем перебор всего списка заголовков
			for(auto & item : this->_items){
				// Выполняем извлечение заголовка
				header = item.second.first;
				// Выполняем формирования результирующего списка
				result.push_back(std::make_pair(this->_fmk->transform(header, fmk_t::transform_t::LOWER), item.second.second));
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
 * @brief Оператор получения списка заголовков
 *
 * @return список всех добавленных заголовков
 */
awh::Headers::operator std::unordered_map <string, string> () const noexcept {
	// Результат работы функции
	std::unordered_map <string, string> result;
	// Если контейнер заголовков заполнен
	if(!this->_items.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Заголовок для вывода
			string header = "";
			// Выполняем перебор всего списка заголовков
			for(auto & item : this->_items){
				// Выполняем извлечение заголовка
				header = item.second.first;
				// Выполняем преобразование заголовка
				this->_fmk->transform(header, fmk_t::transform_t::SMART);
				// Выполняем поиск уже существующего заголовка
				auto i = result.find(header);
				// Если заголовок уже найден в списке
				if(i != result.end())
					// Добавляем разделитель заголовков
					i->second.append(this->_fmk->format(", %s", item.second.second.c_str()));
				// Выполняем формирования результирующего списка
				else result.emplace(header, item.second.second);
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
 * @brief Оператор получения списка заголовков
 *
 * @return список всех добавленных заголовков
 */
awh::Headers::operator std::unordered_multimap <string, string> () const noexcept {
	// Результат работы функции
	std::unordered_multimap <string, string> result;
	// Если контейнер заголовков заполнен
	if(!this->_items.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Заголовок для вывода
			string header = "";
			// Выполняем перебор всего списка заголовков
			for(auto & item : this->_items){
				// Выполняем извлечение заголовка
				header = item.second.first;
				// Выполняем формирования результирующего списка
				result.emplace(this->_fmk->transform(header, fmk_t::transform_t::SMART), item.second.second);
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
 * @brief Оператор извлечения содержимого заголовка
 *
 * @param name название заголовка для извлечения
 * @return     содержимое заголовка
 */
const string & awh::Headers::operator[](const char * name) const noexcept {
	// Выводим результат
	return this->at(name);
}
/**
 * @brief Оператор извлечения содержимого заголовка
 *
 * @param name название заголовка для извлечения
 * @return     содержимое заголовка
 */
const string & awh::Headers::operator[](const string & name) const noexcept {
	// Выводим результат
	return this->at(name);
}
/**
 * @brief Оператор перемещения
 *
 * @param headers заголовки для перемещения
 * @return        текущий контейнер заголовков
 */
awh::Headers & awh::Headers::operator = (headers_t && headers) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока текущего контейнера заголовков
		const lock_guard lock1(this->_mtx);
		// Выполняем блокировку потока стороннего контейнера заголовков
		const lock_guard lock2(headers._mtx);
		// Выполняем перенос заголовков контейнера
		this->_items = ::move(headers._items);
		// Выполняем установку максимальными размерами памяти
		this->_max.memory = headers._max.memory;
		// Выполняем установку максимальными количествами записей
		this->_max.records = headers._max.records;
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
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param headers заголовки для копирования
 * @return        текущий контейнер заголовков
 */
awh::Headers & awh::Headers::operator = (const headers_t & headers) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока текущего контейнера заголовков
		const lock_guard lock1(this->_mtx);
		// Выполняем блокировку потока стороннего контейнера заголовков
		const lock_guard lock2(headers._mtx);
		// Выполняем копирование заголовков контейнера
		this->_items = headers._items;
		// Выполняем установку максимальными размерами памяти
		this->_max.memory = headers._max.memory;
		// Выполняем установку максимальными количествами записей
		this->_max.records = headers._max.records;
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
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param headers заголовки для копирования
 * @return        текущий контейнер заголовков
 */
awh::Headers & awh::Headers::operator = (const vector <item_t> & headers) noexcept {
	// Выполняем очистку текущего контейнера
	this->clear();
	// Если заголовки переданы
	if(!headers.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем перебор списка заголовков
			for(auto & header : headers)
				// Выполняем добавление заголовка
				this->emplace(header.first, header.second);
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
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param headers заголовки для копирования
 * @return        текущий контейнер заголовков
 */
awh::Headers & awh::Headers::operator = (const std::unordered_map <string, string> & headers) noexcept {
	// Выполняем очистку текущего контейнера
	this->clear();
	// Если заголовки переданы
	if(!headers.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем перебор списка заголовков
			for(auto & header : headers)
				// Выполняем добавление заголовка
				this->emplace(header.first, header.second);
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
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param headers заголовки для копирования
 * @return        текущий контейнер заголовков
 */
awh::Headers & awh::Headers::operator = (const std::unordered_multimap <string, string> & headers) noexcept {
	// Выполняем очистку текущего контейнера
	this->clear();
	// Если заголовки переданы
	if(!headers.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем перебор списка заголовков
			for(auto & header : headers)
				// Выполняем добавление заголовка
				this->emplace(header.first, header.second);
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
	return (* this);
}
/**
 * @brief Оператор сравнения двух заголовков
 *
 * @param headers заголовки для сравнения
 * @return        результат сравнения
 */
bool awh::Headers::operator == (const headers_t & headers) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если заголовки соответствуют
	if((result = (this->_items.size() == headers._items.size()))){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Номер итератора в контейнере
			size_t index = 0;
			// Получаем первый итератор контейнера
			auto i = this->_items.begin();
			// Выполняем перебор всего количества входящих заголовков
			for(auto & header : headers._items){
				// Выполняем перемещение итератора на нужный нам заголовок
				std::advance(i, index++);
				// Если идентификаторы заголовков одинаковые
				if((result = (i->first == header.first)))
					// Если содержимое заголовков тоже соответствует
					result = this->_fmk->compare(i->second.second, header.second.second);
				// Если результат ложный
				if(!result)
					// Выходим из цикла
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
 * @brief Конструктор перемещения
 *
 * @param headers заголовки для перемещения
 */
awh::Headers::Headers(headers_t && headers) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока текущего контейнера заголовков
		const lock_guard lock1(this->_mtx);
		// Выполняем блокировку потока стороннего контейнера заголовков
		const lock_guard lock2(headers._mtx);
		// Выполняем перенос заголовков контейнера
		this->_items = ::move(headers._items);
		// Выполняем установку максимальными размерами памяти
		this->_max.memory = headers._max.memory;
		// Выполняем установку максимальными количествами записей
		this->_max.records = headers._max.records;
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
 * @param headers заголовки для копирования
 */
awh::Headers::Headers(const headers_t & headers) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока текущего контейнера заголовков
		const lock_guard lock1(this->_mtx);
		// Выполняем блокировку потока стороннего контейнера заголовков
		const lock_guard lock2(headers._mtx);
		// Выполняем копирование заголовков контейнера
		this->_items = headers._items;
		// Выполняем установку максимальными размерами памяти
		this->_max.memory = headers._max.memory;
		// Выполняем установку максимальными количествами записей
		this->_max.records = headers._max.records;
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
awh::Headers::Headers(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Headers::~Headers() noexcept {}
/**
 * @brief Оператор [<<] вывода в поток буфера
 *
 * @param os      поток куда нужно вывести данные
 * @param headers контейнер заголовков
 */
ostream & awh::operator << (ostream & os, const headers_t & headers) noexcept {
	// Записываем в поток версию
	os << headers.print();
	// Выводим результат
	return os;
}
