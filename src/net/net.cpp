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
 * initLocalNet Метод инициализации списка локальных адресов
 */
void awh::Net::initLocalNet() noexcept {
	// Если список локальных адресов пустой
	if(this->_locals.empty()){
		{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 128;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("::");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV6, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 128;
			// Устанавливаем IP адрес
			local.begin->parse("::1");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV6, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 32;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("2001::");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV6, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 32;
			// Устанавливаем IP адрес
			local.begin->parse("2001:db8::");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV6, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 96;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("64:ff9b::");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV6, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 16;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("2002::");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV6, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 10;
			// Устанавливаем IP адрес начала диапазона
			local.begin->parse("fe80::");
			// Устанавливаем IP адрес конца диапазона
			local.end->parse("febf::");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV6, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 10;
			// Устанавливаем IP адрес начала диапазона
			local.begin->parse("fec0::");
			// Устанавливаем IP адрес конца диапазона
			local.end->parse("feff::");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV6, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 7;
			// Устанавливаем IP адрес
			local.begin->parse("fc00::");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV6, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 8;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("ff00::");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV6, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 8;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("0.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 32;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("0.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 10;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("100.64.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 16;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("169.254.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 4;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("224.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 24;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("224.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 8;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("224.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 8;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("239.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 4;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("240.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 32;
			// Устанавливаем зарезервированный флаг
			local.reserved = true;
			// Устанавливаем IP адрес
			local.begin->parse("255.255.255.255");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 8;
			// Устанавливаем IP адрес
			local.begin->parse("10.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 8;
			// Устанавливаем IP адрес
			local.begin->parse("127.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 12;
			// Устанавливаем IP адрес
			local.begin->parse("172.16.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 24;
			// Устанавливаем IP адрес
			local.begin->parse("192.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 29;
			// Устанавливаем IP адрес
			local.begin->parse("192.0.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 32;
			// Устанавливаем IP адрес
			local.begin->parse("192.0.0.170");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 32;
			// Устанавливаем IP адрес
			local.begin->parse("192.0.0.171");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 24;
			// Устанавливаем IP адрес
			local.begin->parse("192.0.2.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 24;
			// Устанавливаем IP адрес
			local.begin->parse("192.88.99.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 32;
			// Устанавливаем IP адрес
			local.begin->parse("192.88.99.1");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 16;
			// Устанавливаем IP адрес
			local.begin->parse("192.168.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 24;
			// Устанавливаем IP адрес
			local.begin->parse("198.51.100.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 15;
			// Устанавливаем IP адрес
			local.begin->parse("198.18.0.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}{
			// Создаём объект локального адреса
			local_t local(this->_fmk, this->_log);
			// Устанавливаем префикс сети
			local.prefix = 24;
			// Устанавливаем IP адрес
			local.begin->parse("203.0.113.0");
			// Добавляем адрес в список локальных адресов
			this->_locals.emplace(type_t::IPV4, std::move(local));
		}
	}
}
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
	if(addr > 0){
		// Устанавливаем тип IP адреса
		this->_type = type_t::IPV4;
		// Выполняем копирование данных адреса IPv4
		memcpy(this->_buffer.data(), &addr, sizeof(addr));
	}
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
	if(!addr.empty()){
		// Устанавливаем тип IP адреса
		this->_type = type_t::IPV6;
		// Выполняем копирование данных адреса IPv6
		memcpy(this->_buffer.data(), addr.data(), sizeof(addr));
	}
}
/**
 * impose Метод наложения маски сети
 * @param mask маска сети для наложения
 * @param addr тип получаемого адреса
 */
void awh::Net::impose(const string & mask, const addr_t addr) noexcept {
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask);
		// Если префикс сети получен, выполняем применение префикса
		if(prefix > 0) this->impose(prefix, addr);
	}
}
/**
 * impose Метод наложения префикса
 * @param prefix префикс для наложения
 * @param addr тип получаемого адреса
 */
void awh::Net::impose(const uint8_t prefix, const addr_t addr) noexcept {
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
					// Определяем тип получаемого адреса
					switch((uint8_t) addr){
						// Если мы хотим получить адрес хоста
						case (uint8_t) addr_t::HOST: {
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
						} break;
						// Если мы хотим получить сетевой адрес
						case (uint8_t) addr_t::NETW: {
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
						} break;
					}
				}
			} break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Если префикс укладывается в диапазон адреса
				if(prefix <= 128){
					// Определяем номер хексета
					const uint8_t num = ceil(prefix / 16);
					// Определяем тип получаемого адреса
					switch((uint8_t) addr){
						// Если мы хотим получить адрес хоста
						case (uint8_t) addr_t::HOST: {
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
						} break;
						// Если мы хотим получить сетевой адрес
						case (uint8_t) addr_t::NETW: {
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
						} break;
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
		// Создаём объкт для работы с адресами
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
		// Создаём объкт для работы с адресами
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
						net.impose(prefix, addr_t::NETW);
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
						net.impose(prefix, addr_t::NETW);
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
 * mapping Метод проверки соотвествия IP адреса указанной сети
 * @param network сеть для проверки соответствия
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес сети передан
	if((result = !network.empty())){
		// Создаём объкт для работы с адресами
		net_t net(this->_fmk, this->_log);
		// Если парсинг адреса сети выполнен
		if((result = net.parse(network))){
			// Если сеть и IP адрес принадлежат одной версии сети
			if((result = (this->_type == net.type()))){
				// Определяем тип IP адреса
				switch((uint8_t) this->_type){
					// Если IP адрес определён как IPv4
					case (uint8_t) type_t::IPV4: {
						// Буфер данных текущего адреса
						array <uint8_t, 4> nwk, addr;
						// Получаем значение адреса сети
						const uint32_t ip1 = net.v4();
						// Получаем значение текущего адреса
						const uint32_t ip2 = this->v4();
						// Выполняем копирование данных текущего адреса в буфер
						memcpy(nwk.data(), &ip1, sizeof(ip1));
						// Выполняем копирование данных текущего адреса в буфер
						memcpy(addr.data(), &ip2, sizeof(ip2));
						// Выполняем сравнение двух массивов
						for(uint8_t i = 0; i < 4; i++){
							// Если октет адреса соответствует октету сети
							result = ((addr[i] == nwk[i]) || (nwk[i] == 0));
							// Если проверка не вышла, выходим
							if(!result) break;
						}
					} break;
					// Если IP адрес определён как IPv6
					case (uint8_t) type_t::IPV6: {
						// Буфер данных текущего адреса
						array <uint16_t, 8> nwk, addr;
						// Получаем значение адреса сети
						const array <uint64_t, 2> & ip1 = net.v6();
						// Получаем значение текущего адреса
						const array <uint64_t, 2> & ip2 = this->v6();
						// Выполняем копирование данных текущего адреса в буфер
						memcpy(nwk.data(), ip1.data(), sizeof(ip1));
						// Выполняем копирование данных текущего адреса в буфер
						memcpy(addr.data(), ip2.data(), sizeof(ip2));
						// Выполняем сравнение двух массивов
						for(uint8_t i = 0; i < 8; i++){
							// Если хексет адреса соответствует хексет сети
							result = ((addr[i] == nwk[i]) || (nwk[i] == 0));
							// Если проверка не вышла, выходим
							if(!result) break;
						}
					} break;
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * range Метод проверки вхождения IP адреса в диапазон адресов
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @return      результат првоерки
 */
bool awh::Net::range(const Net & begin, const Net & end, string & mask) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask);
		// Если префикс сети получен, выполняем проверку вхождения адреса в диапазон адресов
		if(prefix > 0) result = this->range(begin, end, prefix);
	}
	// Выводим результат
	return result;
}
/**
 * range Метод проверки вхождения IP адреса в диапазон адресов
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @return       результат првоерки
 */
bool awh::Net::range(const Net & begin, const Net & end, const uint8_t prefix) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если типы адресов совпадают
	if((this->type() == begin.type()) && (this->type() == end.type())){
		// Создаём данные первого элемента
		net_t net1(this->_fmk, this->_log);
		// Создаём данные второго элемента
		net_t net2(this->_fmk, this->_log);
		// Создаём данные третьего элемента
		net_t net3(this->_fmk, this->_log);
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4: {
				// Устанавливаем новое значение адреса для первого элемента
				net1 = this->v4();
				// Устанавливаем новое значение адреса для второго элемента
				net2 = begin.v4();
				// Устанавливаем новое значение адреса для третьего элемента
				net3 = end.v4();
			} break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6: {
				// Устанавливаем новое значение адреса для первого элемента
				net1 = this->v6();
				// Устанавливаем новое значение адреса для второго элемента
				net2 = begin.v6();
				// Устанавливаем новое значение адреса для третьего элемента
				net3 = end.v6();
			} break;
		}
		// Извлекаем хост для первого элемента
		net1.impose(prefix, addr_t::HOST);
		// Извлекаем хост для второго элемента
		net2.impose(prefix, addr_t::HOST);
		// Извлекаем хост для третьего элемента
		net3.impose(prefix, addr_t::HOST);
		// Выполняем определение результата вхождения адреса в диапазон
		result = ((net1 >= net2) && (net1 <= net3));
	}
	// Выводим результат
	return result;
}
/**
 * range Метод проверки вхождения IP адреса в диапазон адресов
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @return      результат првоерки
 */
bool awh::Net::range(const string & begin, const string & end, string & mask) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask);
		// Если префикс сети получен, выполняем проверку вхождения адреса в диапазон адресов
		if(prefix > 0) result = this->range(begin, end, prefix);
	}
	// Выводим результат
	return result;
}
/**
 * range Метод проверки вхождения IP адреса в диапазон адресов
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @return       результат првоерки
 */
bool awh::Net::range(const string & begin, const string & end, const uint8_t prefix) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty() && !begin.empty() && !end.empty()){
		// Создаём данные первого элемента
		net_t net1(this->_fmk, this->_log);
		// Создаём данные второго элемента
		net_t net2(this->_fmk, this->_log);
		// Создаём данные третьего элемента
		net_t net3(this->_fmk, this->_log);
		// Устанавливаем новое значение адреса для второго элемента
		net2 = begin;
		// Устанавливаем новое значение адреса для третьего элемента
		net3 = end;
		// Определяем тип IP адреса
		switch((uint8_t) this->_type){
			// Если IP адрес определён как IPv4
			case (uint8_t) type_t::IPV4:
				// Устанавливаем новое значение адреса для первого элемента
				net1 = this->v4();
			break;
			// Если IP адрес определён как IPv6
			case (uint8_t) type_t::IPV6:
				// Устанавливаем новое значение адреса для первого элемента
				net1 = this->v6();
			break;
		}
		// Если типы адресов совпадают
		if((net1.type() == net2.type()) && (net1.type() == net3.type())){
			// Извлекаем хост для первого элемента
			net1.impose(prefix, addr_t::HOST);
			// Извлекаем хост для второго элемента
			net2.impose(prefix, addr_t::HOST);
			// Извлекаем хост для третьего элемента
			net3.impose(prefix, addr_t::HOST);
			// Выполняем определение результата вхождения адреса в диапазон
			result = ((net1 >= net2) && (net1 <= net3));
		}
	}
	// Выводим результат
	return result;
}
/**
 * mapping Метод проверки соотвествия IP адреса указанной сети
 * @param network сеть для проверки соответствия
 * @param mask    маска сети для наложения
 * @param addr    тип получаемого адреса
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const string & mask, const addr_t addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес сети передан
	if((result = (!network.empty() && !mask.empty()))){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask);
		// Если префикс сети получен, выполняем проверку адреса соответствию сети
		if(prefix > 0) result = this->mapping(network, prefix, addr);
	}
	// Выводим результат
	return result;
}
/**
 * mapping Метод проверки соотвествия IP адреса указанной сети
 * @param network сеть для проверки соответствия
 * @param prefix  префикс для наложения
 * @param addr    тип получаемого адреса
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const uint8_t prefix, const addr_t addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес сети передан
	if((result = (!network.empty() && (prefix > 0)))){
		// Создаём объкт для работы с адресами
		net_t net(this->_fmk, this->_log);
		// Если парсинг адреса сети выполнен
		if((result = net.parse(network))){
			// Если сеть и IP адрес принадлежат одной версии сети
			if((result = (this->_type == net.type()))){
				// Определяем тип IP адреса
				switch((uint8_t) this->_type){
					// Если IP адрес определён как IPv4
					case (uint8_t) type_t::IPV4: {
						// Копируем текущий IP адрес
						net = this->v4();
						// Накладываем префикс сети
						net.impose(prefix, addr);
						// Выводим результат проверки
						return (net.v4() == (net = network).v4());
					} break;
					// Если IP адрес определён как IPv6
					case (uint8_t) type_t::IPV6: {
						// Копируем текущий IP адрес
						net = this->v6();
						// Накладываем префикс сети
						net.impose(prefix, addr);
						// Получаем данные IPv6 текущего адреса
						const array <uint64_t, 2> addr = net.v6();
						// Устанавливаем данные сети
						net = network;
						// Выполняем получение данных IPv6 сетевого адреса
						const array <uint64_t, 2> nwk = net.v6();
						// Выводим результат проверки
						return (memcmp(addr.data(), nwk.data(), sizeof(addr)) == 0);
					} break;
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * mode Метод определения режима дислокации IP адреса
 * @return режим дислокации
 */
awh::Net::mode_t awh::Net::mode() const noexcept {
	// Результат работы функции
	mode_t result = mode_t::NONE;
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty()){
		// Создаём объкт для работы с адресами
		net_t net(this->_fmk, this->_log);
		// Выполняем инициализацию списка локальных адресов
		const_cast <net_t *> (this)->initLocalNet();
		// Выполняем группировку нужного нам вида адресов
		auto ret = this->_locals.equal_range(this->_type);
		// Перебираем все локальные адреса
		for(auto it = ret.first; it != ret.second; ++it){
			// Определяем тип IP адреса
			switch((uint8_t) this->_type){
				// Если IP адрес определён как IPv4
				case (uint8_t) type_t::IPV4: {
					// Устанавливаем IP адрес
					net = this->v4();
					// Если получен диапазон IP адресов
					if(it->second.end->type() == type_t::IPV4){
						// Если адрес входит в диапазон адресов
						if(net.range(* it->second.begin.get(), * it->second.end.get(), it->second.prefix)){
							// Если адрес зарезервирован
							if(it->second.reserved)
								// Устанавливаем результат
								return mode_t::RESERV;
							// Иначе устанавливаем, что адрес локальный
							else return mode_t::LOCAL;
						}
					// Если диапазон адресов для этой проверки не установлен
					} else {
						// Устанавливаем префикс сети
						net.impose(it->second.prefix, net_t::addr_t::NETW);
						// Если проверяемые сети совпадают
						if(net.v4() == it->second.begin->v4()){
							// Если адрес зарезервирован
							if(it->second.reserved)
								// Устанавливаем результат
								return mode_t::RESERV;
							// Иначе устанавливаем, что адрес локальный
							else return mode_t::LOCAL;
						}
					}
				} break;
				// Если IP адрес определён как IPv6
				case (uint8_t) type_t::IPV6: {
					// Устанавливаем IP адрес
					net = this->v6();
					// Если получен диапазон IP адресов
					if(it->second.end->type() == type_t::IPV6){
						// Если адрес входит в диапазон адресов
						if(net.range(* it->second.begin.get(), * it->second.end.get(), it->second.prefix)){
							// Если адрес зарезервирован
							if(it->second.reserved)
								// Устанавливаем результат
								return mode_t::RESERV;
							// Иначе устанавливаем, что адрес локальный
							else return mode_t::LOCAL;
						}
					// Если диапазон адресов для этой проверки не установлен
					} else {
						// Устанавливаем префикс сети
						net.impose(it->second.prefix, net_t::addr_t::NETW);
						// Если проверяемые сети совпадают
						if(memcmp(net.v6().data(), it->second.begin->v6().data(), (sizeof(uint64_t) * 2)) == 0){
							// Если адрес зарезервирован
							if(it->second.reserved)
								// Устанавливаем результат
								return mode_t::RESERV;
							// Иначе устанавливаем, что адрес локальный
							else return mode_t::LOCAL;
						}
					}
				} break;
			}
		}
		// Если результат не определён
		if(result == mode_t::NONE)
			// Устанавливаем, что файл ялвяется глобальным
			result = mode_t::GLOBAL;
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
bool awh::Net::operator < (const net_t & addr) const noexcept {
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
bool awh::Net::operator > (const net_t & addr) const noexcept {
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
bool awh::Net::operator <= (const net_t & addr) const noexcept {
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
bool awh::Net::operator >= (const net_t & addr) const noexcept {
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
bool awh::Net::operator != (const net_t & addr) const noexcept {
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
bool awh::Net::operator == (const net_t & addr) const noexcept {
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