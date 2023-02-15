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
 * v4 Извлечения адреса IPv4 в чистом виде
 * @return адрес IPv4 в чистом виде
 */
uint32_t awh::Net::v4() const noexcept {
	// Результат работы функции
	uint32_t result = 0;
	// Если в буфере данных достаточно
	if(this->_buffer.size() == 4)
		// Выполняем копирование данных адреса IPv4
		memcpy(&result, this->_buffer.data(), this->_buffer.size());
	// Выводим результат
	return result;
}
/**
 * v4 Метод установки адреса IPv4 в чистом виде
 * @param addr адрес IPv4 в чистом виде
 */
void awh::Net::v4(const uint32_t addr) noexcept {
	// Выполняем выделение памяти для IPv4 адреса
	this->_buffer.resize(4, 0);
	// Если IPv4 адрес передан
	if(addr > 0)
		// Выполняем копирование данных адреса IPv4
		memcpy(this->_buffer.data(), &addr, sizeof(addr));
}
/**
 * v6 Извлечения адреса IPv6 в чистом виде
 * @return адрес IPv6 в чистом виде
 */
array <uint64_t, 2> awh::Net::v6() const noexcept {
	// Результат работы функции
	array <uint64_t, 2> result;
	// Если в буфере данных достаточно
	if(this->_buffer.size() == 16)
		// Выполняем копирование данных адреса IPv4
		memcpy(result.data(), this->_buffer.data(), this->_buffer.size());
	// Выводим результат
	return result;
}
/**
 * v6 Метод установки адреса IPv6 в чистом виде
 * @param addr адрес IPv6 в чистом виде
 */
void awh::Net::v6(const array <uint64_t, 2> & addr) noexcept {
	// Выполняем выделение памяти для IPv6 адреса
	this->_buffer.resize(16, 0);
	// Если IPv6 адрес передан
	if(!addr.empty())
		// Выполняем копирование данных адреса IPv6
		memcpy(this->_buffer.data(), addr.data(), sizeof(addr));
}
/**
 * impose Метод наложения маски сети (получение сетевого адреса)
 * @param mask маска сети для наложения
 */
void awh::Net::impose(const string & mask) noexcept {
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask);
		// Если префикс сети получен, выполняем применение префикса
		if(prefix > 0) this->impose(prefix);
	}
}
/**
 * impose Метод наложения префикса (получение сетевого адреса)
 * @param prefix префикс для наложения
 */
void awh::Net::impose(const uint8_t prefix) noexcept {
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty() && (prefix > 0)){
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4: {
				// Если префикс укладывается в диапазон адреса
				if(prefix <= 32){
					// Определяем номер хексета
					const uint8_t num = ceil(prefix / 8);
					// Если префикс кратен 8
					if((prefix % 8) == 0)
						// Зануляем все остальные биты
						memset(this->_buffer.data() + num, 0, this->_buffer.size() - num);
					// Если префикс не кратен 8
					else {
						// Данные хексета
						uint8_t oct = 0;
						// Получаем нужное нам значение октета
						memcpy(&oct, this->_buffer.data() + num, sizeof(oct));
						// Переводим октет в бинарный вид
						bitset <8> bits(oct);
						// Зануляем все лишние элементы
						for(uint8_t i = 0; i < (8 - (prefix % 8)); i++)
							// Зануляем все лишние биты
							bits.set(i, 0);
						// Устанавливаем новое значение октета
						oct = static_cast <uint16_t> (bits.to_ulong());
						// Устанавливаем новое значение октета
						memcpy(this->_buffer.data() + num, &oct, sizeof(oct));
						// Зануляем все остальные биты
						memset(this->_buffer.data() + (num + 1), 0, this->_buffer.size() - (num + 1));
					}
				}
			} break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Если префикс укладывается в диапазон адреса
				if(prefix <= 128){
					// Определяем номер хексета
					const uint8_t num = ceil(prefix / 16);
					// Если префикс кратен 16
					if((prefix % 16) == 0)
						// Зануляем все остальные биты
						memset(this->_buffer.data() + (num * 2), 0, this->_buffer.size() - (num * 2));
					// Если префикс не кратен 16
					else {
						// Данные хексета
						uint16_t hex = 0;
						// Получаем нужное нам значение хексета
						memcpy(&hex, this->_buffer.data() + (num * 2), sizeof(hex));
						// Переводим хексет в бинарный вид
						bitset <16> bits(hex);
						// Зануляем все лишние элементы
						for(uint8_t i = 0; i < (16 - (prefix % 16)); i++)
							// Зануляем все лишние биты
							bits.set(i, 0);
						// Устанавливаем новое значение хексета
						hex = static_cast <uint16_t> (bits.to_ulong());
						// Устанавливаем новое значение хексета
						memcpy(this->_buffer.data() + (num * 2), &hex, sizeof(hex));
						// Зануляем все остальные биты
						memset(this->_buffer.data() + ((num * 2) + 2), 0, this->_buffer.size() - ((num * 2) + 2));
					}
				}
			} break;
		}
	}
}
/**
 * dempose Метод наложения маски сети (получение адреса хоста)
 * @param mask маска сети для наложения
 */
void awh::Net::dempose(const string & mask) noexcept {
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask);
		// Если префикс сети получен, выполняем применение префикса
		if(prefix > 0) this->dempose(prefix);
	}
}
/**
 * dempose Метод наложения префикса (получение адреса хоста)
 * @param prefix префикс для наложения
 */
void awh::Net::dempose(const uint8_t prefix) noexcept {
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty() && (prefix > 0)){
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4: {
				// Если префикс укладывается в диапазон адреса
				if(prefix <= 32){
					// Определяем номер октета
					const uint8_t num = ceil(prefix / 8);
					// Зануляем все остальные биты
					memset(this->_buffer.data(), 0, num);
					// Если префикс не кратен 8
					if((prefix % 8) != 0){
						// Данные октета
						uint8_t oct = 0;
						// Получаем нужное нам значение октета
						memcpy(&oct, this->_buffer.data() + num, sizeof(oct));
						// Переводим октет в бинарный вид
						bitset <8> bits(oct);
						// Зануляем все лишние элементы
						for(uint8_t i = (8 - (prefix % 8)); i < 8; i++)
							// Зануляем все лишние биты
							bits.set(i, 0);
						// Устанавливаем новое значение октета
						oct = static_cast <uint16_t> (bits.to_ulong());
						// Устанавливаем новое значение октета
						memcpy(this->_buffer.data() + num, &oct, sizeof(oct));
					}
				}
			} break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Если префикс укладывается в диапазон адреса
				if(prefix <= 128){
					// Определяем номер хексета
					const uint8_t num = ceil(prefix / 16);
					// Зануляем все остальные биты
					memset(this->_buffer.data(), 0, (num * 2));
					// Если префикс не кратен 16
					if((prefix % 16) != 0){
						// Данные хексета
						uint16_t hex = 0;
						// Получаем нужное нам значение хексета
						memcpy(&hex, this->_buffer.data() + (num * 2), sizeof(hex));
						// Переводим хексет в бинарный вид
						bitset <16> bits(hex);
						// Зануляем все лишние элементы
						for(uint8_t i = (16 - (prefix % 16)); i < 16; i++)
							// Зануляем все лишние биты
							bits.set(i, 0);
						// Устанавливаем новое значение хексета
						hex = static_cast <uint16_t> (bits.to_ulong());
						// Устанавливаем новое значение хексета
						memcpy(this->_buffer.data() + (num * 2), &hex, sizeof(hex));
					}
				}
			} break;
		}
	}
}
/**
 * mask2Prefix Метод перевода маски сети в префикс адреса
 * @param mask маска сети для перевода
 * @return     полученный префикс адреса
 */
uint8_t awh::Net::mask2Prefix(const string & mask) const noexcept {
	// Результат работы функции
	uint8_t result = 0;
	// Если маска сети передана
	if(!mask.empty()){
		// Преобразуем маску в адрес
		net_t net(this->_fmk, this->_log);
		// Выполняем парсинг маски
		if(net.parse(mask) && (this->_type == net.type())){
			// Бинарный контейнер
			bitset <8> bits;
			// Определяем тип IP адреса
			switch((uint8_t) this->_type){
				// Если IP адрес определён как IPv4
				case (uint8_t) type_t::IPV4: {
					// Получаем значение маски в виде адреса
					const uint32_t num = net.v4();
					// Выполняем перебор всего значения буфера
					for(uint8_t i = 0; i < 4; i++){
						// Переводим хексет в бинарный вид
						bits = (reinterpret_cast <const uint8_t *> (&num))[i];
						// Выполняем подсчёт префикса
						result += bits.count();
					}
				} break;
				// Если IP адрес определён как IPv6
				case (uint8_t) type_t::IPV6: {
					// Получаем значение маски в виде адреса
					const array <uint64_t, 2> num = net.v6();
					// Выполняем перебор всего значения буфера
					for(uint8_t i = 0; i < 16; i++){
						// Переводим хексет в бинарный вид
						bits = reinterpret_cast <const uint8_t *> (num.data())[i];
						// Выполняем подсчёт префикса
						result += bits.count();
					}
				} break;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * prefix2Mask Метод преобразования префикса адреса в маску сети
 * @param prefix префикс адреса для преобразования
 * @return       полученная маска сети
 */
string awh::Net::prefix2Mask(const uint8_t prefix) const noexcept {
	// Результат работы функции
	string result = "";
	// Если маска сети передана
	if(prefix > 0){
		// Преобразуем маску в адрес
		net_t net(this->_fmk, this->_log);
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4: {
				// Если префикс укладывается в диапазон адреса
				if(prefix < 32){
					// Выполняем парсинг маски
					if(net.parse("255.255.255.255")){
						// Выполняем установку префикса
						net.impose(prefix);
						// Выводим полученный адрес
						result = net;
					}
				}
			} break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Если префикс укладывается в диапазон адреса
				if(prefix < 128){
					// Выполняем парсинг маски
					if(net.parse("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")){
						// Выполняем установку префикса
						net.impose(prefix);
						// Выводим полученный адрес
						result = net;
					}
				}
			} break;
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
				this->_buffer.resize(4, 0);
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
				this->_buffer.resize(16, 0);
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
/**
 * Оператор вывода IP адреса в качестве строки
 * @return IP адрес в качестве строки
 */
awh::Net::operator std::string() const noexcept {
	// Выводим данные IP адреса в виде строки
	return this->get();
}
/**
 * Оператор [<] сравнения IP адреса
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator < (const net_t & addr) noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4:
				// Выполняем сравнение адресов
				result = (this->v4() < addr.v4());
			break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Получаем данные текущего адреса IPv6
				const auto & first = this->v6();
				// Получаем данные сравниваемого адреса IPv6
				const auto & second = addr.v6();
				// Выполняем сравнение адресов
				result = ((first[0] < second[0]) && (first[1] < second[1]));
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * Оператор [>] сравнения IP адреса
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator > (const net_t & addr) noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4:
				// Выполняем сравнение адресов
				result = (this->v4() > addr.v4());
			break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Получаем данные текущего адреса IPv6
				const auto & first = this->v6();
				// Получаем данные сравниваемого адреса IPv6
				const auto & second = addr.v6();
				// Выполняем сравнение адресов
				result = ((first[0] > second[0]) && (first[1] > second[1]));
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * Оператор [<=] сравнения IP адреса
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator <= (const net_t & addr) noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4:
				// Выполняем сравнение адресов
				result = (this->v4() <= addr.v4());
			break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Получаем данные текущего адреса IPv6
				const auto & first = this->v6();
				// Получаем данные сравниваемого адреса IPv6
				const auto & second = addr.v6();
				// Выполняем сравнение адресов
				result = ((first[0] <= second[0]) && (first[1] <= second[1]));
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * Оператор [>=] сравнения IP адреса
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator >= (const net_t & addr) noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4:
				// Выполняем сравнение адресов
				result = (this->v4() >= addr.v4());
			break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Получаем данные текущего адреса IPv6
				const auto & first = this->v6();
				// Получаем данные сравниваемого адреса IPv6
				const auto & second = addr.v6();
				// Выполняем сравнение адресов
				result = ((first[0] >= second[0]) && (first[1] >= second[1]));
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * Оператор [!=] сравнения IP адреса
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator != (const net_t & addr) noexcept {
	// Результат работы функции
	bool result = false;
	// Определяем тип IP адреса
	switch((uint8_t) this->_type){
		// Если IP адрес определён как IPv4
		case (uint8_t) type_t::IPV4:
			// Выполняем сравнение адресов
			result = (this->v4() != addr.v4());
		break;
		// Если IP адрес определён как IPv6
		case (uint8_t) type_t::IPV6: {
			// Получаем данные текущего адреса IPv6
			const auto & first = this->v6();
			// Получаем данные сравниваемого адреса IPv6
			const auto & second = addr.v6();
			// Выполняем сравнение адресов
			result = ((first[0] != second[0]) || (first[1] != second[1]));
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * Оператор [==] сравнения IP адреса
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator == (const net_t & addr) noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4:
				// Выполняем сравнение адресов
				result = (this->v4() == addr.v4());
			break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Получаем данные текущего адреса IPv6
				const auto & first = this->v6();
				// Получаем данные сравниваемого адреса IPv6
				const auto & second = addr.v6();
				// Выполняем сравнение адресов
				result = ((first[0] == second[0]) && (first[1] == second[1]));
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * Оператор [=] присвоения IP адреса
 * @param addr адрес для присвоения
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const net_t & addr) noexcept {
	// Определяем тип IP адреса
	switch((uint8_t) addr.type()){
		// Если IP адрес определён как IPv4
		case (uint8_t) type_t::IPV4:
			// Устанавливаем IPv4 адрсе
			this->v4(addr.v4());
		break;
		// Если IP адрес определён как IPv6
		case (uint8_t) type_t::IPV6:
			// Устанавливаем IPv6 адрсе
			this->v6(addr.v6());
		break;
	}
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [=] присвоения IP адреса
 * @param ip адрес для присвоения
 * @return   текущий объект
 */
awh::Net & awh::Net::operator = (const string & ip) noexcept {
	// Выполняем установку IP адреса
	this->parse(ip);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [=] присвоения IP адреса
 * @param addr адрес для присвоения
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const uint32_t addr) noexcept {
	// Устанавливаем IPv4
	this->v4(addr);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [=] присвоения IP адреса
 * @param addr адрес для присвоения
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const array <uint64_t, 2> & addr) noexcept {
	// Устанавливаем IPv4
	this->v6(addr);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [>>] чтения из потока IP адреса
 * @param is   поток для чтения
 * @param addr адрес для присвоения
 */
istream & awh::operator >> (istream & is, net_t & addr) noexcept {
	// Адрес интернет-подключения
	string ip = "";
	// Считываем адрес интернет-подключения
	is >> ip;
	// Если адрес интернет-подключения получен
	if(!ip.empty())
		// Устанавливаем IP адрес
		addr.parse(ip);
	// Выводим результат
	return is;
}
/**
 * Оператор [<<] вывода в поток IP адреса
 * @param os   поток куда нужно вывести данные
 * @param addr адрес для присвоения
 */
ostream & awh::operator << (ostream & os, const net_t & addr) noexcept {
	// Записываем в поток IP адрес
	os << addr.get();
	// Выводим результат
	return os;
}
