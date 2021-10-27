/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <socks5/core.hpp>

/**
 * ipToHex Метод конвертации IP адреса в бинарный буфер
 * @param ip     индернет адрес в виде строки
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       бинарный буфер IP адреса
 */
vector <char> awh::Socks5::ipToHex(const string & ip, const int family) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если IP адрес передан
	if(!ip.empty()){
		// Результат конвертации
		int conv = 0;
		// Определяем тип подключения
		switch(family){
			// Если тип адреса IPv4
			case AF_INET: {
				// Увеличиваем память бинарного буфера
				result.resize(sizeof(struct in_addr), 0x0);
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
				result.resize(sizeof(struct in6_addr), 0x0);
				// Выполняем конвертацию IP адреса
				conv = inet_pton(family, ip6.c_str(), result.data());
			} break;
		}
		// Если конвертация не выполнена
		if(conv <= 0){
			// Если формат IP адреса не верный
			if(conv == 0)
				// Выводим сообщение об ошибке
				this->log->print("%s: [%s]", log_t::flag_t::CRITICAL, "not in presentation format", ip.c_str());
			// Иначе это ошибка функции конвертации
			else this->log->print("%s: [%s]", log_t::flag_t::CRITICAL, "address conversion could not be performed", ip.c_str());
			// Очищаем бинарный буфер
			result.clear();
		}
	}
	// Выводим результат
	return result;
}
/**
 * hexToIp Метод конвертации бинарного буфера в IP адрес
 * @param buffer бинарный буфер для конвертации
 * @param size   размер бинарного буфера
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       IP адрес в виде строки
 */
string awh::Socks5::hexToIp(const char * buffer, const size_t size, const int family) const noexcept {
	// Результат работы функции
	string result = "";
	// Если бинарный буфер передан
	if((buffer != nullptr) && (size > 0)){
		// Определяем тип подключения
		switch(family){
			// Если тип адреса IPv4
			case AF_INET: {
				// Создаём буфер строки адреса
				char str[INET_ADDRSTRLEN];
				// Выполняем конвертацию
				if(inet_ntop(family, buffer, str, INET_ADDRSTRLEN) == nullptr)
					// Выводим сообщение об ошибке
					this->log->print("%s", log_t::flag_t::CRITICAL, "ip address could not be obtained");
				// Получаем IPv4 адрес
				else result = str;
			} break;
			// Если тип адреса IPv6
			case AF_INET6: {
				// Создаём буфер строки адреса
				char str[INET6_ADDRSTRLEN];
				// Выполняем конвертацию
				if(inet_ntop(family, buffer, str, INET6_ADDRSTRLEN) == nullptr)
					// Выводим сообщение об ошибке
					this->log->print("%s", log_t::flag_t::CRITICAL, "ip address could not be obtained");
				// Получаем IPv6 адрес
				else result = str;
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * setText Метод установки в буфер текстовых данных
 * @param text текст для установки
 * @return     текущее значение смещения
 */
u_short awh::Socks5::setText(const string & text) const noexcept {
	// Результат работы функции
	u_short result = 0;
	// Если текст передан
	if(!text.empty()){
		// Добавляем в буфер размер текста
		this->buffer.push_back(static_cast <char> (text.size()));
		// Добавляем в буфер сам текст
		this->buffer.insert(this->buffer.end(), text.begin(), text.end());
	}
	// Выводим результат
	return result;
}
/**
 * getText Метод извлечения текстовых данных из буфера
 * @param buffer буфер данных для извлечения текста
 * @param size   размер буфера данных
 * @return       текст содержащийся в буфере данных
 */
const string awh::Socks5::getText(const char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	string result = "";
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Размер текста
		uint8_t length = 0;
		// Извлекаем размер текста
		memcpy(&length, buffer, sizeof(length));
		// Если размер текста получен, извлекаем текстовые данные
		if(length > 0) result.assign(buffer + sizeof(length), length);
	}
	// Выводим результат
	return result;
}
/**
 * setOctet Метод установки октета
 * @param octet  октет для установки
 * @param offset размер смещения в буфере
 * @return       текущее значение смещения
 */
u_short awh::Socks5::setOctet(const uint8_t octet, const u_short offset) const noexcept {
	// Устанавливаем первый октет
	memcpy(this->buffer.data() + offset, &octet, sizeof(octet));
	// Выводим размер смещения
	return (offset + sizeof(octet));
}
/**
 * isEnd Метод проверки завершения обработки
 * @return результат проверки
 */
bool awh::Socks5::isEnd() const noexcept {
	// Выполняем проверку завершения работы
	return (
		(this->state == state_t::BROKEN) ||
		(this->state == state_t::CONNECT) ||
		(this->state == state_t::HANDSHAKE)
	);
}
/**
 * isConnected Метод проверки на подключение клиента
 * @return результат проверки
 */
bool awh::Socks5::isConnected() const noexcept {
	// Выполняем проверку запроса клиента
	return (this->state == state_t::CONNECT);
}
/**
 * isHandshake Метод проверки рукопожатия
 * @return проверка рукопожатия
 */
bool awh::Socks5::isHandshake() const noexcept{
	// Выполняем проверку рукопожатия
	return (this->state == state_t::HANDSHAKE);
}
/**
 * getCode Метод получения кода сообщения
 * @return код сообщения
 */
uint8_t awh::Socks5::getCode() const noexcept {
	// Выводим код сообщения
	return this->code;
}
/**
 * getMessage Метод получения сообщения
 * @param code код сообщения
 * @return     текстовое значение кода
 */
const string & awh::Socks5::getMessage(const uint8_t code) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Выполняем поиск кода сообщения
	auto it = this->messages.find(code);
	// Если сообщение получено, выводим его
	if(it != this->messages.end()) return it->second;
	// Выводим результат
	return result;
}
/**
 * get Метод извлечения буфера запроса/ответа
 * @return бинарный буфер
 */
const vector <char> & awh::Socks5::get() const noexcept {
	// Выводим бинарный буфер
	return this->buffer;
}
/**
 * setUrl Метод установки URL параметров REST запроса
 * @param url параметры REST запроса
 */
void awh::Socks5::setUrl(const uri_t::url_t & url) noexcept {
	// Устанавливаем URL адрес REST запроса
	if(!url.empty()) this->url = url;
}
