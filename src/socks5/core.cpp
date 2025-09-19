/**
 * @file: core.cpp
 * @date: 2021-12-19
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
 * Подключаем заголовочный файл
 */
#include <socks5/core.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод конвертации IP адреса в бинарный буфер
 *
 * @param ip     индернет адрес в виде строки
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       бинарный буфер IP адреса
 */
vector <char> awh::Socks5::ipToHex(const string & ip, const int32_t family) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если IP адрес передан
	if(!ip.empty()){
		// Результат конвертации
		int32_t conv = 0;
		/**
		 * Определяем тип подключения
		 */
		switch(family){
			// Если тип адреса IPv4
			case AF_INET: {
				// Увеличиваем память бинарного буфера
				result.resize(sizeof(struct in_addr), 0x00);
				// Выполняем конвертацию IP адреса
				conv = inet_pton(family, ip.c_str(), result.data());
			} break;
			// Если тип адреса IPv6
			case AF_INET6: {
				// Получаем IPv6 адрес
				string ip6 = ip;
				// Если передан IP адрес с экранированием
				if((ip6.front() == '[') && (ip6.back() == ']'))
					// Удаляем лишние символы
					ip6.assign(ip6.begin() + 1, ip6.end() - 1);
				// Увеличиваем память бинарного буфера
				result.resize(sizeof(struct in6_addr), 0x00);
				// Выполняем конвертацию IP адреса
				conv = inet_pton(family, ip6.c_str(), result.data());
			} break;
		}
		// Если конвертация не выполнена
		if(conv <= 0){
			// Если формат IP адреса не верный
			if(conv == 0)
				// Выводим сообщение об ошибке
				this->_log->print("Not in presentation format: [%s]", log_t::flag_t::CRITICAL, ip.c_str());
			// Иначе это ошибка функции конвертации
			else this->_log->print("Address conversion could not be performed: [%s]", log_t::flag_t::CRITICAL, ip.c_str());
			// Очищаем бинарный буфер
			result.clear();
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод конвертации бинарного буфера в IP адрес
 *
 * @param buffer бинарный буфер для конвертации
 * @param size   размер бинарного буфера
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       IP адрес в виде строки
 */
string awh::Socks5::hexToIp(const char * buffer, const size_t size, const int32_t family) const noexcept {
	// Результат работы функции
	string result = "";
	// Если бинарный буфер передан
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Определяем тип подключения
		 */
		switch(family){
			// Если тип адреса IPv4
			case AF_INET: {
				// Создаём буфер строки адреса
				char str[INET_ADDRSTRLEN];
				// Выполняем конвертацию
				if(::inet_ntop(family, buffer, str, INET_ADDRSTRLEN) == nullptr)
					// Выводим сообщение об ошибке
					this->_log->print("IP-address could not be obtained", log_t::flag_t::CRITICAL);
				// Получаем IPv4 адрес
				else result = str;
			} break;
			// Если тип адреса IPv6
			case AF_INET6: {
				// Создаём буфер строки адреса
				char str[INET6_ADDRSTRLEN];
				// Выполняем конвертацию
				if(::inet_ntop(family, buffer, str, INET6_ADDRSTRLEN) == nullptr)
					// Выводим сообщение об ошибке
					this->_log->print("IP-address could not be obtained", log_t::flag_t::CRITICAL);
				// Получаем IPv6 адрес
				else result = str;
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки в буфер текстовых данных
 *
 * @param text текст для установки
 * @return     текущее значение смещения
 */
uint16_t awh::Socks5::text(const string & text) const noexcept {
	// Результат работы функции
	uint16_t result = 0;
	// Если текст передан
	if(!text.empty()){
		// Добавляем в буфер размер текста
		this->_buffer.push_back(static_cast <char> (text.size()));
		// Добавляем в буфер сам текст
		this->_buffer.insert(this->_buffer.end(), text.begin(), text.end());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения текстовых данных из буфера
 *
 * @param buffer буфер данных для извлечения текста
 * @param size   размер буфера данных
 * @return       текст содержащийся в буфере данных
 */
string awh::Socks5::text(const char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	string result = "";
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Размер текста
		uint8_t length = 0;
		// Извлекаем размер текста
		::memcpy(&length, buffer, sizeof(length));
		// Если размер текста получен, извлекаем текстовые данные
		if(length > 0)
			// Устанавливаем текстовые данные буфера
			result.assign(buffer + sizeof(length), length);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки октета
 *
 * @param octet  октет для установки
 * @param offset размер смещения в буфере
 * @return       текущее значение смещения
 */
uint16_t awh::Socks5::octet(const uint8_t octet, const uint16_t offset) const noexcept {
	// Устанавливаем первый октет
	::memcpy(this->_buffer.data() + offset, &octet, sizeof(octet));
	// Выводим размер смещения
	return (offset + sizeof(octet));
}
/**
 * @brief Метод проверки активного состояния
 *
 * @param state состояние которое необходимо проверить
 */
bool awh::Socks5::is(const state_t state) const noexcept {
	/**
	 * Определяем запрашиваемое состояние
	 */
	switch(static_cast <uint8_t> (state)){
		// Если проверяется режим завершения сбора данных
		case static_cast <uint8_t> (state_t::END):
			// Выполняем проверку завершения сбора данных
			return (
				(this->_state == state_t::BROKEN) ||
				(this->_state == state_t::CONNECT) ||
				(this->_state == state_t::HANDSHAKE)
			);
		// Если проверяется режим подключения к серверу
		case static_cast <uint8_t> (state_t::CONNECT):
			// Выполняем проверку подключения к серверу
			return (this->_state == state_t::CONNECT);
		// Если проверяется режим выполненного рукопожатия
		case static_cast <uint8_t> (state_t::HANDSHAKE):
			// Выполняем проверку на удачное рукопожатие
			return (this->_state == state_t::HANDSHAKE);
	}
	// Выводим результат
	return false;
}
/**
 * @brief Метод получения кода сообщения
 *
 * @return код сообщения
 */
uint8_t awh::Socks5::code() const noexcept {
	// Выводим код сообщения
	return this->_code;
}
/**
 * @brief Метод получения сообщения
 *
 * @param code код сообщения
 * @return     текстовое значение кода
 */
const string & awh::Socks5::message(const uint8_t code) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Выполняем поиск кода сообщения
	auto i = this->_responses.find(code);
	// Если сообщение получено, выводим его
	if(i != this->_responses.end())
		// Выполняем получение текстовое значение кода
		return i->second;
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения буфера запроса/ответа
 *
 * @return бинарный буфер
 */
const vector <char> & awh::Socks5::get() const noexcept {
	// Выводим бинарный буфер
	return this->_buffer;
}
/**
 * @brief Метод установки URL параметров REST запроса
 *
 * @param url параметры REST запроса
 */
void awh::Socks5::url(const uri_t::url_t & url) noexcept {
	// Устанавливаем URL адрес REST запроса
	this->_url = url;
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 */
awh::Socks5::Socks5(const log_t * log) noexcept :
 _code(0x00), _state(state_t::METHOD), _log(log) {
	/**
	 * Список ответов сервера
	 */
	this->_responses = {
		{0x00, "successful"},
		{0x01, "SOCKS server error"},
		{0x02, "connection is prohibited by a set of rules"},
		{0x03, "network unavailable"},
		{0x04, "host unreachable"},
		{0x05, "connection denied"},
		{0x06, "TTL expiration"},
		{0x07, "command not supported"},
		{0x08, "address type not supported"},
		{0x09, "until X'FF' are undefined"},
		{0x10, "login or password is not set"},
		{0x11, "unsupported protocol version"},
		{0x12, "login or password is not correct"},
		{0x13, "agreement version not supported"}
	};
}
