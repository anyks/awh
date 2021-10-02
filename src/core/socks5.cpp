/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <core/socks5.hpp>

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
 * @param family тип протокола интернета AF_INET или AF_INET6
 * @return       IP адрес в виде строки
 */
string awh::Socks5::hexToIp(const vector <char> & buffer, const int family) const noexcept {
	// Результат работы функции
	string result = "";
	// Если бинарный буфер передан
	if(!buffer.empty()){
		// Определяем тип подключения
		switch(family){
			// Если тип адреса IPv4
			case AF_INET: {
				// Создаём буфер строки адреса
				char str[INET_ADDRSTRLEN];
				// Выполняем конвертацию
				if(inet_ntop(family, buffer.data(), str, INET_ADDRSTRLEN) == nullptr)
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
				if(inet_ntop(family, buffer.data(), str, INET6_ADDRSTRLEN) == nullptr)
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
 * setOctet Метод установки октета
 * @param buffer сформированный бинарный буфер
 * @param octet  октет для установки
 * @param offset размер смещения в буфере
 * @return       текущее значение смещения
 */
u_short awh::Socks5::setOctet(vector <char> & buffer, const uint8_t octet, const u_short offset) const noexcept {
	// Устанавливаем первый октет
	memcpy(buffer.data() + offset, &octet, sizeof(octet));
	// Выводим размер смещения
	return (offset + sizeof(octet));
}
/**
 * isEnd Метод проверки завершения обработки
 * @return результат проверки
 */
bool awh::Socks5::isEnd() const noexcept {

}
/**
 * isHandshake Метод получения флага рукопожатия
 * @return флаг получения рукопожатия
 */
bool awh::Socks5::isHandshake() const noexcept{

}
/**
 * setUrl Метод установки URL параметров REST запроса
 * @param url параметры REST запроса
 */
void awh::Socks5::setUrl(const uri_t::url_t & url) noexcept {

}
