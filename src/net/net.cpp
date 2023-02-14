/**
 * @file: net.cpp
 * @date: 2023-02-14
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

// Подключаем заголовочный файл
#include <net/net.hpp>

/**
 * zerro Метод заполнения недостающих элементов нулями
 * @param num  число для заполнения нулями
 * @param size максимальная длина строки
 * @return     полученное число строки
 */
string & awh::Net::zerro(string && num, const uint8_t size) const noexcept {
	// Если число меньше максимальной длины строки
	if(static_cast <uint8_t> (num.size()) < size){
		// Создаём строку для добавления
		const string tmp(size - static_cast <uint8_t> (num.size()), '0');
		// Добавляем недостающие нули в наше число
		num.insert(num.begin(), tmp.begin(), tmp.end());
	}
	// Выводим результат
	return num;
}
/**
 * clear Метод очистки данных IP адреса
 */
void awh::Net::clear() noexcept {
	// Выполняем сброс буфера данных
	this->_buffer.clear();
	// Устанавливаем тип IP адреса
	this->_type = type_t::NONE;
}
/**
 * type Метод извлечение типа IP адреса
 * @return тип IP адреса
 */
awh::Net::type_t awh::Net::type() const noexcept {
	// Выполняем тип IP адреса
	return this->_type;
}
/**
 * host Метод определения типа хоста
 * @param host хост для определения
 * @return     определённый тип хоста
 */
awh::Net::type_t awh::Net::host(const string & host) const noexcept {
	// Результат полученных данных
	type_t result = type_t::NONE;
	// Если хост передан
	if(!host.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e(			
			// Определение MAC адреса
			"^(?:([a-f\\d]{2}(?:\\:[a-f\\d]{2}){5})|"
			// Определение IPv4 адреса
			"(\\d{1,3}(?:\\.\\d{1,3}){3})|"
			// Определение IPv6 адреса
			"(?:\\[?(\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:\\:\\:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){1,7})?)|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]?)|"
			// Определение сети
			"((?:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\/(?:\\d{1,3}(?:\\.\\d{1,3}){3}|\\d+))|"
			// Определение домена
			"((?:\\*\\.)?[\\w\\-\\.\\*]+\\.(?:xn\\-{2})?[a-z1]+)|"
			// Определение HTTP адреса
			"(https?\\:\\/\\/[^\\r\\n\\t\\s]+(?:\\/(?:[^\\r\\n\\t\\s]+)?)?)|"
			// Определение адреса файловой системы
			"(\\.{0,2}\\/\\w+(?:\\/[\\w\\.\\-]+)*))$",
			regex::ECMAScript | regex::icase
		);
		// Выполняем проверку
		regex_search(host, match, e);
		// Если результат найден
		if(!match.empty() && (match.size() == 8)){
			// Выполняем перебор всех полученных вариантов
			for(size_t i = 0; i < match.size(); i++){
				// Если данные получены
				if(!match.str(i).empty()){
					// Определяем тип хоста
					switch(i){
						// Если мы определили MAC-адрес
						case 1: return type_t::MAC;
						// Если мы определили IPv4-адрес
						case 2: return type_t::IPV4;
						// Если мы определили IPv6-адрес
						case 3: return type_t::IPV6;
						// Если мы определили сеть
						case 4: return type_t::NETW;
						// Если мы определили доменное имя
						case 5: return type_t::DOMN;
						// Если мы определили HTTP метод
						case 6: return type_t::HTTP;
						// Если мы определили адрес в файловой системе
						case 7: return type_t::ADDR;
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * parse Метод парсинга IP адреса
 * @param ip адрес интернет подключения для парсинга
 * @return   результат работы парсинга
 */
bool awh::Net::parse(const string & ip) noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP адрес передан
	if((result = !ip.empty())){
		// Определяем тип переданного IP адреса
		switch((uint8_t) (this->_type = this->host(ip))){
			// Если IP адрес является адресом IPv4
			case (uint8_t) type_t::IPV4: {
				// Выполняем инициализацию буфера
				this->_buffer.resize(4);
				// Позиция разделителя
				size_t start = 0, stop = 0, index = 0;
				// Выполняем поиск разделителя
				while((stop = ip.find('.', start)) != string::npos){
					// Извлекаем полученное число
					this->_buffer[index] = stoi(ip.substr(start, stop));
					// Выполняем смещение
					start = (stop + 1);
					// Увеличиваем смещение индекса
					index++;
				}
				// Выполняем установку последнего октета
				this->_buffer[index] = stoi(ip.substr(start));
			} break;
			// Если IP адрес является адресом IPv6
			case (uint8_t) type_t::IPV6: {
				// Создаём список всех хексетов
				vector <wstring> data;
				// Выполняем инициализацию буфера
				this->_buffer.resize(16);
				// Выполняем сплит данных IP адреса
				this->_fmk->split(((ip.front() == '[') && (ip.back() == ']') ? ip.substr(1, ip.length() - 2) : ip), ":", data);
				// Если данные IP адреса получены
				if((result = !data.empty())){
					// Создаём результирующий буфер данных
					vector <uint16_t> buffer(8, 0);
					// Если в начале IP адреса пропущены нули
					if(data.front().empty()){
						// Если последний элемент массива больше 4-х символов
						if(data.back().length() > 4){
							// Получаем данные IPv4 адреса
							const string & ip = this->_fmk->convert(data.back());
							// Если элемент является IPv4 адресом
							if((result = (this->host(ip) == type_t::IPV4)))
								// Выполняем добавление адреса IPv4
								return this->parse(ip);
						// Если IP адрес состоит из нормальных хексетов
						} else {
							// Устанавливаем индекс последнего элемента
							uint8_t index = 8;
							// Выполняем перебор всех хексеков
							for(auto it = data.rbegin(); it != data.rend(); ++it){
								// Если хексет установлен
								if(!it->empty())
									// Добавляем хексет в список
									buffer[--index] = this->_fmk->hexToDec(this->_fmk->convert(* it));
								// Выходим из цикла
								else break;
							}
						}
					// Если IP адрес передан полностью или не до конца
					} else {
						// Устанавливаем индекс первого элемента
						uint8_t index = 0;
						// Выполняем перебор всего списка хексетов
						for(auto it = data.begin(); it != data.end(); ++it){
							// Если хексет установлен
							if(!it->empty())
								// Добавляем хексет в список
								buffer[index++] = this->_fmk->hexToDec(this->_fmk->convert(* it));
							// Выходим из цикла
							else break;
						}
						// Если не все хексеты собраны
						if(index < 8){
							// Устанавливаем индекс последнего элемента
							uint8_t index = 8;
							// Выполняем перебор всех хексеков
							for(auto it = data.rbegin(); it != data.rend(); ++it){
								// Если хексет установлен
								if(!it->empty())
									buffer[--index] = this->_fmk->hexToDec(this->_fmk->convert(* it));
								// Выходим из цикла
								else break;
							}
						}
					}
					// Выполняем копирование бинарных данных в буфер
					memcpy(this->_buffer.data(), buffer.data(), (buffer.size() * 2));
				}
			} break;
			// Все остальные варианты мы пропускаем
			default: result = false;
		}
	}
	// Выводим результат
	return result;
}
/**
 * get Метод извлечения данных IP адреса
 * @param format формат формирования IP адреса
 * @return       сформированная строка IP адреса
 */
string awh::Net::get(const format_t format) const noexcept {
	// Результат работы функции
	string result = "";
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty()){
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4: {
				// Переходим по всему массиву
				for(uint8_t i = 0; i < static_cast <uint8_t> (this->_buffer.size()); i++){
					// Если строка уже существует, добавляем разделитель
					if(!result.empty())
						// Добавляем разделитель
						result.append(1, '.');
					// Добавляем октет в версию
					result.append(format == format_t::LONG ? this->zerro(to_string(this->_buffer[i])) : to_string(this->_buffer[i]));
				}
			} break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Значение хексета
				uint16_t num = 0;
				// Количество разделителей и количество хексетов в буфере
				uint8_t separators = 0, count = static_cast <uint8_t> (this->_buffer.size());
				// Переходим по всему массиву
				for(uint8_t i = 0; i < count; i += 2){
					// Выполняем получение значение числа
					memcpy(&num, this->_buffer.data() + i, sizeof(num));
					// Если нужно выводить в кратком виде
					if(format == format_t::SHORT){
						// Если Число установлено
						if(num > 0){
							// Добавляем разделитель
							if(!result.empty()) result.append(1, ':');
							// Добавляем октет в версию
							result.append(this->_fmk->itoa(static_cast <int64_t> (num), 16));
						// Заменяем нули разделителем
						} else if((++separators < 2) || (i == (count - 2)))
							// Добавляем разделитель
							result.append(1, ':');
					// Если форматы вывода указаны полные
					} else {
						// Если строка уже существует, добавляем разделитель
						if(!result.empty())
							// Добавляем разделитель
							result.append(1, ':');
						// Добавляем октет в версию
						result.append(
							format == format_t::LONG ?
							this->zerro(this->_fmk->itoa(static_cast <int64_t> (num), 16), 4) :
							this->_fmk->itoa(static_cast <int64_t> (num), 16)
						);
					}
				}
			} break;
		}
	}
	// Выводим результат
	return result;
}
