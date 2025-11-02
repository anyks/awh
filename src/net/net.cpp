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
 * @copyright: Copyright © 2025
 */

/**
 * Если мы используем порядок байтов Little Endian или Big Endian
 */
#if __BYTE_ORDER__ && __ORDER_LITTLE_ENDIAN__
	/**
	 * Макрос проверки порядка поддержки Little Endian
	 */
	#define IS_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
/**
 * Если мы используем порядок байтов Little Endian
 */
#elif __LITTLE_ENDIAN__ || _LITTLE_ENDIAN
	/**
	 * Включаем макрос поддержки Little Endian
	 */
	#define IS_LITTLE_ENDIAN 1
/**
 * Если мы используем порядок байтов Big Endian
 */
#elif __BIG_ENDIAN__ || _BIG_ENDIAN
	/**
	 * Отключаем макрос поддержки Little Endian
	 */
	#define IS_LITTLE_ENDIAN 0
#else
	/**
	 * @brief Проверка поддержки Little Endian
	 *
	 * @return результат проверки
	 */
	static inline bool isLittleEndian() noexcept {
		// Тестируем порядок байтов
		uint16_t test = 1;
		// Возвращаем результат проверки
		return ((* reinterpret_cast <uint8_t *> (&test)) == 1);
	}
	/**
	 * Макрос проверки поддержки Little Endian
	 */
	#define IS_LITTLE_ENDIAN isLittleEndian()
#endif

/**
 * Стандартные модули
 */
#include <cmath>
#include <bitset>
#include <cstring>
#include <algorithm>

/**
 * Системные модули
 */
#include <arpa/inet.h>

/**
 * Подключаем заголовочный файл
 */
#include <net/net.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Вспомогательная функция для получения размера буфера
 *
 * @tparam T размер буфера данных
 */
template <size_t T = 4>
/**
 * @brief Вспомогательная функция конвертации IPv6-адреса в нужный порядок байт
 *
 * @param src исходный IPv6-адрес
 * @param dst результирующий IPv6-адрес
 */
static void convertEndian(const uint8_t src[T], uint8_t dst[T]) noexcept {
	/**
	 * Если порядок байт Little Endian
	 */
	#if IS_LITTLE_ENDIAN
		// На little-endian: переворачиваем все T байт
		std::reverse_copy(src, src + T, dst);
	/**
	 * Если порядок байт Big Endian
	 */
	#else
		// На big-endian: ничего не делаем
		::memcpy(dst, src, T);
	#endif
}

/**
 * @brief Метод инициализации списка локальных адресов
 *
 */
void awh::Net::initLocalNet() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если список локальных адресов пустой
		if(this->_localsNet.empty()){
			{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 128;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 128;
				// Устанавливаем IP-адрес
				localNet.begin->parse("::1");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("2001::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем IP-адрес
				localNet.begin->parse("2001:db8::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 96;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("64:ff9b::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 16;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("2002::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 10;
				// Устанавливаем IP-адрес начала диапазона
				localNet.begin->parse("fe80::");
				// Устанавливаем IP-адрес конца диапазона
				localNet.end->parse("febf::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 10;
				// Устанавливаем IP-адрес начала диапазона
				localNet.begin->parse("fec0::");
				// Устанавливаем IP-адрес конца диапазона
				localNet.end->parse("feff::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 7;
				// Устанавливаем IP-адрес
				localNet.begin->parse("fc00::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("ff00::");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV6, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("0.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("0.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 10;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("100.64.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 16;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("169.254.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 4;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("224.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("224.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("224.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("239.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 4;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("240.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем зарезервированный флаг
				localNet.reserved = true;
				// Устанавливаем IP-адрес
				localNet.begin->parse("255.255.255.255");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем IP-адрес
				localNet.begin->parse("10.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 8;
				// Устанавливаем IP-адрес
				localNet.begin->parse("127.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 12;
				// Устанавливаем IP-адрес
				localNet.begin->parse("172.16.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 29;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.0.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.0.0.170");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.0.0.171");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.0.2.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.88.99.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 32;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.88.99.1");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 16;
				// Устанавливаем IP-адрес
				localNet.begin->parse("192.168.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем IP-адрес
				localNet.begin->parse("198.51.100.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 15;
				// Устанавливаем IP-адрес
				localNet.begin->parse("198.18.0.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
			}{
				// Создаём объект локального адреса
				localNet_t localNet(this->_exp, this->_fmk, this->_log);
				// Устанавливаем префикс сети
				localNet.prefix = 24;
				// Устанавливаем IP-адрес
				localNet.begin->parse("203.0.113.0");
				// Добавляем адрес в список локальных адресов
				this->_localsNet.emplace(type_t::IPV4, ::move(localNet));
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
 * @brief Метод заполнения недостающих элементов нулями
 *
 * @param num  число для заполнения нулями
 * @param size максимальная длина строки
 * @return     полученное число строки
 */
string && awh::Net::zerro(string && num, const uint8_t size) const noexcept {
	// Если число меньше максимальной длины строки
	if(static_cast <uint8_t> (num.size()) < size){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём строку для добавления
			const string result(size - static_cast <uint8_t> (num.size()), '0');
			// Добавляем недостающие нули в наше число
			num.insert(num.begin(), result.begin(), result.end());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(num, size),
					log_t::flag_t::CRITICAL, error.what()
				);
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
	return ::move(num);
}
/**
 * @brief Метод очистки данных IP-адреса
 *
 */
void awh::Net::clear() noexcept {
	// Выполняем сброс буфера данных
	this->_buffer.clear();
	// Устанавливаем тип IP-адреса
	this->_type = type_t::NONE;
}
/**
 * @brief Метод проверки соответствия адреса зеркалу IPv6 => IPv4
 *
 * @return результат проверки
 */
bool awh::Net::broadcastIPv6ToIPv4() const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём временный буфер данных для сравнения
			vector <uint16_t> buffer(6);
			// Устанавливаем хексет маски
			buffer[5] = 0xFFFF;
			// Если буфер данных принадлежит к вещанию IPv6 => IPv4
			result = (::memcmp(&buffer[0], &this->_buffer[0], (buffer.size() * 2)) == 0);
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
 * @brief Метод извлечения типа IP-адреса
 *
 * @return тип IP-адреса
 */
awh::Net::type_t awh::Net::type() const noexcept {
	// Выполняем тип IP-адреса
	return this->_type;
}
/**
 * @brief Метод установки типа IP-адреса
 *
 * @param type тип IP-адреса для установки
 */
void awh::Net::type(const type_t type) noexcept {
	// Выполняем установку типа IP-адреса
	this->_type = type;
}
/**
 * @brief Метод определения типа хоста
 *
 * @param host хост для определения
 * @return     определённый тип хоста
 */
awh::Net::type_t awh::Net::host(const string & host) const noexcept {
	// Результат полученных данных
	type_t result = type_t::NONE;
	// Если хост передан
	if(!host.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем проверку хоста
			const auto & match = this->_regexp.exec(host, this->_exp);
			// Если результат получен
			if(!match.empty()){
				// Выполняем перебор всех полученных вариантов
				for(uint8_t i = 0; i < static_cast <uint8_t> (match.size()); i++){
					// Если данные получены
					if(!match[i].empty()){
						/**
						 * Определяем тип хоста
						 */
						switch(i){
							// Если мы определили MAC-адрес
							case 1: return type_t::MAC;
							// Если мы определили IPv4-адрес
							case 2: return type_t::IPV4;
							// Если мы определили IPv6-адрес
							case 3: return type_t::IPV6;
							// Если мы определили сеть
							case 4: return type_t::NETWORK;
							// Если мы определили доменная зона
							case 5: return type_t::FQDN;
							// Если мы определили URL-адрес
							case 6: return type_t::URL;
							// Если мы определили адрес в файловой системе
							case 7: return type_t::FS;
						}
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(host), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод извлечения аппаратного адреса в чистом виде
 *
 * @return аппаратный адрес в чистом виде
 */
std::array <uint8_t, 6> awh::Net::mac() const noexcept {
	// Результат работы функции
	std::array <uint8_t, 6> result = {0};
	// Если в буфере данных достаточно
	if(this->_buffer.size() == 6){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем перевод бинарного буфера MAC-адреса в числовой вид
			::memcpy(&result[0], &this->_buffer[0], this->_buffer.size());
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
 * @brief Метод установки аппаратного адреса в чистом виде
 *
 * @param addr аппаратный адрес в чистом виде
 */
void awh::Net::mac(const std::array <uint8_t, 6> & addr) noexcept {
	// Если MAC адрес передан
	if(!addr.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем выделение памяти для MAC адреса
			this->_buffer.resize(6);
			// Устанавливаем тип MAC адреса
			this->_type = type_t::MAC;
			// Выполняем копирование данных адреса MAC
			::memcpy(&this->_buffer[0], &addr[0], this->_buffer.size());
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(addr.front(), addr.back()),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Извлечения адреса IPv4 в чистом виде
 *
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 * @return       адрес IPv4 в чистом виде
 */
uint32_t awh::Net::v4(const endian_t endian) const noexcept {
	// Результат работы функции
	uint32_t result = 0;
	// Если в буфере данных достаточно
	if(this->_buffer.size() == 4){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем какой порядок следования байт установлен
			 */
			switch(static_cast <uint8_t> (endian)){
				// Если установлен порядок следования байт от старшего к младшему
				case static_cast <uint8_t> (endian_t::BIG):
					// Получаем буфер данных IP-адреса
					::convertEndian(&this->_buffer[0], reinterpret_cast <uint8_t *> (&result));
				break;
				// Если установлен порядок следования байт от младшего к старшему
				case static_cast <uint8_t> (endian_t::LITTLE):
					// Выполняем копирование данных адреса IPv4
					::memcpy(&result, &this->_buffer[0], this->_buffer.size());
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
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(static_cast <uint16_t> (endian)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод установки адреса IPv4 в чистом виде
 *
 * @param addr   адрес IPv4 в чистом виде
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 */
void awh::Net::v4(const uint32_t addr, const endian_t endian) noexcept {
	// Если IPv4 адрес передан
	if(addr > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем выделение памяти для IPv4 адреса
			this->_buffer.resize(4);
			// Устанавливаем тип IP-адреса
			this->_type = type_t::IPV4;
			/**
			 * Определяем какой порядок следования байт установлен
			 */
			switch(static_cast <uint8_t> (endian)){
				// Если установлен порядок следования байт от старшего к младшему
				case static_cast <uint8_t> (endian_t::BIG):
					// Устанавливаем буфер данных IP-адреса
					::convertEndian(reinterpret_cast <const uint8_t *> (&addr), &this->_buffer[0]);
				break;
				// Если установлен порядок следования байт от младшего к старшему
				case static_cast <uint8_t> (endian_t::LITTLE):
					// Выполняем копирование данных адреса IPv4
					::memcpy(&this->_buffer[0], &addr, sizeof(addr));
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
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(addr, static_cast <uint16_t> (endian)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Извлечения адреса IPv6 в чистом виде
 *
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 * @return       адрес IPv6 в чистом виде
 */
std::array <uint8_t, 16> awh::Net::v6(const endian_t endian) const noexcept {
	// Результат работы функции
	std::array <uint8_t, 16> result;
	// Если в буфере данных достаточно
	if(this->_buffer.size() == 16){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем какой порядок следования байт установлен
			 */
			switch(static_cast <uint8_t> (endian)){
				// Если установлен порядок следования байт от старшего к младшему
				case static_cast <uint8_t> (endian_t::BIG):
					// Получаем буфер данных IP-адреса
					::convertEndian <16> (&this->_buffer[0], reinterpret_cast <uint8_t *> (&result[0]));
				break;
				// Если установлен порядок следования байт от младшего к старшему
				case static_cast <uint8_t> (endian_t::LITTLE):
					// Выполняем копирование данных адреса IPv6
					::memcpy(&result[0], &this->_buffer[0], this->_buffer.size());
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
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(static_cast <uint16_t> (endian)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод установки адреса IPv6 в чистом виде
 *
 * @param addr   адрес IPv6 в чистом виде
 * @param endian флаг формирования адреса в установленном порядке следовании байт
 */
void awh::Net::v6(const std::array <uint8_t, 16> & addr, const endian_t endian) noexcept {
	// Если IPv6 адрес передан
	if(!addr.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем выделение памяти для IPv6 адреса
			this->_buffer.resize(16);
			// Устанавливаем тип IP-адреса
			this->_type = type_t::IPV6;
			/**
			 * Определяем какой порядок следования байт установлен
			 */
			switch(static_cast <uint8_t> (endian)){
				// Если установлен порядок следования байт от старшего к младшему
				case static_cast <uint8_t> (endian_t::BIG):
					// Устанавливаем буфер данных IP-адреса
					::convertEndian <16> (reinterpret_cast <const uint8_t *> (&addr[0]), &this->_buffer[0]);
				break;
				// Если установлен порядок следования байт от младшего к старшему
				case static_cast <uint8_t> (endian_t::LITTLE):
					// Выполняем копирование данных адреса IPv6
					::memcpy(&this->_buffer[0], &addr[0], sizeof(addr));
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
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(addr.front(), addr.back()),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод наложения маски сети
 *
 * @param mask маска сети для наложения
 * @param addr тип получаемого адреса
 */
void awh::Net::impose(const string & mask, const addr_t addr) noexcept {
	// Выполняем наложение маски сети
	this->impose(mask, addr, this->_type);
}
/**
 * @brief Метод наложения маски сети
 *
 * @param mask маска сети для наложения
 * @param addr тип получаемого адреса
 * @param type тип адреса аппаратного или интернет подключения
 */
void awh::Net::impose(const string & mask, const addr_t addr, const type_t type) noexcept {
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask, type);
		// Если префикс сети получен, выполняем применение префикса
		if(prefix > 0)
			// Выполняем наложение маски сети
			this->impose(prefix, addr, type);
	}
}
/**
 * @brief Метод наложения префикса
 *
 * @param prefix префикс для наложения
 * @param addr тип получаемого адреса
 */
void awh::Net::impose(const uint8_t prefix, const addr_t addr) noexcept {
	// Выполняем наложение префикса адреса
	this->impose(prefix, addr, this->_type);
}
/**
 * @brief Метод наложения префикса
 *
 * @param prefix префикс для наложения
 * @param addr   тип получаемого адреса
 * @param type   тип адреса аппаратного или интернет подключения
 */
void awh::Net::impose(const uint8_t prefix, const addr_t addr, const type_t type) noexcept {
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty() && (prefix > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Если префикс укладывается в диапазон адреса
					if(prefix <= 32){
						// Определяем номер октета
						const uint8_t num = static_cast <uint8_t> (::ceil(static_cast <double> (prefix / 8)));
						/**
						 * Определяем тип получаемого адреса
						 */
						switch(static_cast <uint8_t> (addr)){
							// Если мы хотим получить адрес хоста
							case static_cast <uint8_t> (addr_t::HOST): {
								// Зануляем все остальные биты
								::memset(&this->_buffer[0], 0, num);
								// Если префикс не кратен 8
								if((prefix % 8) != 0){
									// Данные октета
									uint8_t oct = 0;
									// Получаем нужное нам значение октета
									::memcpy(&oct, &this->_buffer[0] + num, sizeof(oct));
									// Переводим октет в бинарный вид
									std::bitset <8> bits(oct);
									// Зануляем все лишние элементы
									for(uint8_t i = (8 - (prefix % 8)); i < 8; i++)
										// Зануляем все лишние биты
										bits.set(i, 0);
									// Устанавливаем новое значение октета
									oct = static_cast <uint16_t> (bits.to_ulong());
									// Устанавливаем новое значение октета
									::memcpy(&this->_buffer[0] + num, &oct, sizeof(oct));
								}
							} break;
							// Если мы хотим получить сетевой адрес
							case static_cast <uint8_t> (addr_t::NETWORK): {
								// Если префикс кратен 8
								if((prefix % 8) == 0)
									// Зануляем все остальные биты
									::memset(&this->_buffer[0] + num, 0, this->_buffer.size() - num);
								// Если префикс не кратен 8
								else {
									// Данные хексета
									uint8_t oct = 0;
									// Получаем нужное нам значение октета
									::memcpy(&oct, &this->_buffer[0] + num, sizeof(oct));
									// Переводим октет в бинарный вид
									std::bitset <8> bits(oct);
									// Зануляем все лишние элементы
									for(uint8_t i = 0; i < (8 - (prefix % 8)); i++)
										// Зануляем все лишние биты
										bits.set(i, 0);
									// Устанавливаем новое значение октета
									oct = static_cast <uint16_t> (bits.to_ulong());
									// Устанавливаем новое значение октета
									::memcpy(&this->_buffer[0] + num, &oct, sizeof(oct));
									// Зануляем все остальные биты
									::memset(&this->_buffer[0] + (num + 1), 0, this->_buffer.size() - (num + 1));
								}
							} break;
						}
					}
				} break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Если префикс укладывается в диапазон адреса
					if(prefix <= 128){
						// Определяем номер хексета
						const uint8_t num = static_cast <uint8_t> (::ceil(static_cast <double> (prefix / 16)));
						/**
						 * Определяем тип получаемого адреса
						 */
						switch(static_cast <uint8_t> (addr)){
							// Если мы хотим получить адрес хоста
							case static_cast <uint8_t> (addr_t::HOST): {
								// Зануляем все остальные биты
								::memset(&this->_buffer[0], 0, (num * 2));
								// Если префикс не кратен 16
								if((prefix % 16) != 0){
									// Данные хексета
									uint16_t hex = 0;
									// Получаем нужное нам значение хексета
									::memcpy(&hex, &this->_buffer[0] + (num * 2), sizeof(hex));
									// Переводим хексет в бинарный вид
									std::bitset <16> bits(hex);
									// Зануляем все лишние элементы
									for(uint8_t i = (16 - (prefix % 16)); i < 16; i++)
										// Зануляем все лишние биты
										bits.set(i, 0);
									// Устанавливаем новое значение хексета
									hex = static_cast <uint16_t> (bits.to_ulong());
									// Устанавливаем новое значение хексета
									::memcpy(&this->_buffer[0] + (num * 2), &hex, sizeof(hex));
								}
							} break;
							// Если мы хотим получить сетевой адрес
							case static_cast <uint8_t> (addr_t::NETWORK): {
								// Если префикс кратен 16
								if((prefix % 16) == 0)
									// Зануляем все остальные биты
									::memset(&this->_buffer[0] + (num * 2), 0, this->_buffer.size() - (num * 2));
								// Если префикс не кратен 16
								else {
									// Данные хексета
									uint16_t hex = 0;
									// Получаем нужное нам значение хексета
									::memcpy(&hex, &this->_buffer[0] + (num * 2), sizeof(hex));
									// Переводим хексет в бинарный вид
									std::bitset <16> bits(hex);
									// Зануляем все лишние элементы
									for(uint8_t i = 0; i < (16 - (prefix % 16)); i++)
										// Зануляем все лишние биты
										bits.set(i, 0);
									// Устанавливаем новое значение хексета
									hex = static_cast <uint16_t> (bits.to_ulong());
									// Устанавливаем новое значение хексета
									::memcpy(&this->_buffer[0] + (num * 2), &hex, sizeof(hex));
									// Зануляем все остальные биты
									::memset(&this->_buffer[0] + ((num * 2) + 2), 0, this->_buffer.size() - ((num * 2) + 2));
								}
							} break;
						}
					}
				} break;
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
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(prefix, static_cast <uint16_t> (addr), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод перевода маски сети в префикс адреса
 *
 * @param mask маска сети для перевода
 * @return     полученный префикс адреса
 */
uint8_t awh::Net::mask2Prefix(const string & mask) const noexcept {
	// Выполняем преобразование маски сети в префикс адреса
	return this->mask2Prefix(mask, this->_type);
}
/**
 * @brief Метод перевода маски сети в префикс адреса
 *
 * @param mask маска сети для перевода
 * @param type тип адреса аппаратного или интернет подключения
 * @return     полученный префикс адреса
 */
uint8_t awh::Net::mask2Prefix(const string & mask, const type_t type) const noexcept {
	// Результат работы функции
	uint8_t result = 0;
	// Если маска сети передана
	if(!mask.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_t net(this->_exp, this->_fmk, this->_log);
			// Выполняем парсинг маски
			if(net.parse(mask) && (type == net.type())){
				// Бинарный контейнер
				std::bitset <8> bits;
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (type)){
					// Если IP-адрес определён как IPv4
					case static_cast <uint8_t> (type_t::IPV4): {
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
					// Если IP-адрес определён как IPv6
					case static_cast <uint8_t> (type_t::IPV6): {
						// Получаем значение маски в виде адреса
						const std::array <uint8_t, 16> num = net.v6();
						// Выполняем перебор всего значения буфера
						for(uint8_t i = 0; i < 16; i++){
							// Переводим хексет в бинарный вид
							bits = reinterpret_cast <const uint8_t *> (&num[0])[i];
							// Выполняем подсчёт префикса
							result += bits.count();
						}
					} break;
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
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(mask, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод преобразования префикса адреса в маску сети
 *
 * @param prefix префикс адреса для преобразования
 * @return       полученная маска сети
 */
string awh::Net::prefix2Mask(const uint8_t prefix) const noexcept {
	// Выполняем перевод префикса адреса в маску сети
	return this->prefix2Mask(prefix, this->_type);
}
/**
 * @brief Метод преобразования префикса адреса в маску сети
 *
 * @param prefix префикс адреса для преобразования
 * @param type   тип адреса аппаратного или интернет подключения
 * @return       полученная маска сети
 */
string awh::Net::prefix2Mask(const uint8_t prefix, const type_t type) const noexcept {
	// Результат работы функции
	string result = "";
	// Если маска сети передана
	if(prefix > 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_t net(this->_exp, this->_fmk, this->_log);
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Если префикс укладывается в диапазон адреса
					if(prefix <= 32){
						// Выполняем парсинг маски
						if(net.parse("255.255.255.255")){
							// Выполняем установку префикса
							net.impose(prefix, addr_t::NETWORK);
							// Выводим полученный адрес
							result = net;
						}
					}
				} break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Если префикс укладывается в диапазон адреса
					if(prefix <= 128){
						// Выполняем парсинг маски
						if(net.parse("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")){
							// Выполняем установку префикса
							net.impose(prefix, addr_t::NETWORK);
							// Выводим полученный адрес
							result = net;
						}
					}
				} break;
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
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(prefix, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @return      результат првоерки
 */
bool awh::Net::range(const Net & begin, const Net & end, const string & mask) const noexcept {
	// Выполняем проверку вхождения IP-адреса в диапазон адресов
	return this->range(begin, end, mask, this->_type);
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @param type  тип адреса аппаратного или интернет подключения
 * @return      результат првоерки
 */
bool awh::Net::range(const Net & begin, const Net & end, const string & mask, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask, type);
		// Если префикс сети получен, выполняем проверку вхождения адреса в диапазон адресов
		if(prefix > 0)
			// Выполняем получение результата
			result = this->range(begin, end, prefix, type);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @return       результат првоерки
 */
bool awh::Net::range(const Net & begin, const Net & end, const uint8_t prefix) const noexcept {
	// Выполняем проверку вхождения iP-адреса в диапазон адресов
	return this->range(begin, end, prefix, this->_type);
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @param type   тип адреса аппаратного или интернет подключения
 * @return       результат првоерки
 */
bool awh::Net::range(const Net & begin, const Net & end, const uint8_t prefix, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если типы адресов совпадают
	if((type == begin.type()) && (type == end.type())){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объекты сетевых модулей
			net_t net1(this->_exp, this->_fmk, this->_log),
			      net2(this->_exp, this->_fmk, this->_log),
			      net3(this->_exp, this->_fmk, this->_log);
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Устанавливаем новое значение адреса для первого элемента
					net1 = this->v4();
					// Устанавливаем новое значение адреса для второго элемента
					net2 = begin.v4();
					// Устанавливаем новое значение адреса для третьего элемента
					net3 = end.v4();
				} break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
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
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(static_cast <uint16_t> (prefix), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @return      результат првоерки
 */
bool awh::Net::range(const string & begin, const string & end, const string & mask) const noexcept {
	// Выполняем проверку вхождения IP-адреса в диапазон адресов
	return this->range(begin, end, mask, this->_type);
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin начало диапазона адресов
 * @param end   конец диапазона адресов
 * @param mask  маска сети для перевода
 * @param type  тип адреса аппаратного или интернет подключения
 * @return      результат првоерки
 */
bool awh::Net::range(const string & begin, const string & end, const string & mask, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует и маска передана
	if(!this->_buffer.empty() && !mask.empty()){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask, type);
		// Если префикс сети получен, выполняем проверку вхождения адреса в диапазон адресов
		if(prefix > 0)
			// Выполняем получение результата
			result = this->range(begin, end, prefix, type);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @return       результат првоерки
 */
bool awh::Net::range(const string & begin, const string & end, const uint8_t prefix) const noexcept {
	// Выполняем проверку вхождения IP-адреса в диапазон адресов
	return this->range(begin, end, prefix, this->_type);
}
/**
 * @brief Метод проверки вхождения IP-адреса в диапазон адресов
 *
 * @param begin  начало диапазона адресов
 * @param end    конец диапазона адресов
 * @param prefix префикс адреса для преобразования
 * @param type   тип адреса аппаратного или интернет подключения
 * @return       результат првоерки
 */
bool awh::Net::range(const string & begin, const string & end, const uint8_t prefix, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty() && !begin.empty() && !end.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объекты сетевых модулей
			net_t net1(this->_exp, this->_fmk, this->_log),
			      net2(this->_exp, this->_fmk, this->_log),
			      net3(this->_exp, this->_fmk, this->_log);
			// Устанавливаем новое значение адреса для начала и конца диапазона адресов
			net2 = begin; net3 = end;
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Устанавливаем новое значение адреса для первого элемента
					net1 = this->v4();
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6):
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
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(begin, end, static_cast <uint16_t> (prefix), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network) const noexcept {
	// Выполняем проверку соответствия IP-адреса указанной сети
	return this->mapping(network, this->_type);
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param type    тип адреса аппаратного или интернет подключения
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес сети передан
	if((result = !network.empty())){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_t net(this->_exp, this->_fmk, this->_log);
			// Если парсинг адреса сети выполнен
			if((result = net.parse(network))){
				// Если сеть и IP-адрес принадлежат одной версии сети
				if((result = (type == net.type()))){
					/**
					 * Определяем тип IP-адреса
					 */
					switch(static_cast <uint8_t> (type)){
						// Если IP-адрес определён как IPv4
						case static_cast <uint8_t> (type_t::IPV4): {
							// Буфер данных текущего адреса
							std::array <uint8_t, 4> nwk, addr;
							// Получаем значение адреса сети
							const uint32_t ip1 = net.v4();
							// Получаем значение текущего адреса
							const uint32_t ip2 = this->v4();
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&nwk[0], &ip1, sizeof(ip1));
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&addr[0], &ip2, sizeof(ip2));
							// Выполняем сравнение двух массивов
							for(uint8_t i = 0; i < 4; i++){
								// Если октет адреса соответствует октету сети
								result = ((addr[i] == nwk[i]) || (nwk[i] == 0));
								// Если проверка не вышла
								if(!result)
									// Выходим из цикла
									break;
							}
						} break;
						// Если IP-адрес определён как IPv6
						case static_cast <uint8_t> (type_t::IPV6): {
							// Буфер данных текущего адреса
							std::array <uint16_t, 8> nwk, addr;
							// Получаем значение адреса сети
							const std::array <uint8_t, 16> & ip1 = net.v6();
							// Получаем значение текущего адреса
							const std::array <uint8_t, 16> & ip2 = this->v6();
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&nwk[0], &ip1[0], sizeof(ip1));
							// Выполняем копирование данных текущего адреса в буфер
							::memcpy(&addr[0], &ip2[0], sizeof(ip2));
							// Выполняем сравнение двух массивов
							for(uint8_t i = 0; i < 8; i++){
								// Если хексет адреса соответствует хексет сети
								result = ((addr[i] == nwk[i]) || (nwk[i] == 0));
								// Если проверка не вышла
								if(!result)
									// Выходим из цикла
									break;
							}
						} break;
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
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(network, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param mask    маска сети для наложения
 * @param addr    тип получаемого адреса
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const string & mask, const addr_t addr) const noexcept {
	// Выполняем проверку соответствия IP-адреса указанной сети
	return this->mapping(network, mask, addr, this->_type);
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param mask    маска сети для наложения
 * @param addr    тип получаемого адреса
 * @param type    тип адреса аппаратного или интернет подключения
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const string & mask, const addr_t addr, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес сети передан
	if((result = (!network.empty() && !mask.empty()))){
		// Получаем префикс сети
		const uint8_t prefix = this->mask2Prefix(mask, type);
		// Если префикс сети получен, выполняем проверку адреса соответствию сети
		if(prefix > 0)
			// Выполняем получение результата
			result = this->mapping(network, prefix, addr, type);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param prefix  префикс для наложения
 * @param addr    тип получаемого адреса
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const uint8_t prefix, const addr_t addr) const noexcept {
	// Выполняем проверку соответствия IP-адреса указанной сети
	return this->mapping(network, prefix, addr, this->_type);
}
/**
 * @brief Метод проверки соотвествия IP-адреса указанной сети
 *
 * @param network сеть для проверки соответствия
 * @param prefix  префикс для наложения
 * @param addr    тип получаемого адреса
 * @param type    тип адреса аппаратного или интернет подключения
 * @return        результат проверки
 */
bool awh::Net::mapping(const string & network, const uint8_t prefix, const addr_t addr, const type_t type) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес сети передан
	if((result = (!network.empty() && (prefix > 0)))){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_t net(this->_exp, this->_fmk, this->_log);
			// Если парсинг адреса сети выполнен
			if((result = net.parse(network))){
				// Если сеть и IP-адрес принадлежат одной версии сети
				if((result = (type == net.type()))){
					/**
					 * Определяем тип IP-адреса
					 */
					switch(static_cast <uint8_t> (type)){
						// Если IP-адрес определён как IPv4
						case static_cast <uint8_t> (type_t::IPV4): {
							// Копируем текущий IP-адрес
							net = this->v4();
							// Накладываем префикс сети
							net.impose(prefix, addr);
							// Получаем данные IPv4 текущего адреса
							const uint32_t ip = net.v4();
							// Устанавливаем данные сети
							net = network;
							// Накладываем префикс сети
							net.impose(prefix, addr);
							// Выполняем получение данных IPv4 сетевого адреса
							const uint32_t nwk = net.v4();
							// Выводим результат проверки
							return (ip == nwk);
						} break;
						// Если IP-адрес определён как IPv6
						case static_cast <uint8_t> (type_t::IPV6): {
							// Копируем текущий IP-адрес
							net = this->v6();
							// Накладываем префикс сети
							net.impose(prefix, addr);
							// Получаем данные IPv6 текущего адреса
							const auto & ip = net.v6();
							// Устанавливаем данные сети
							net = network;
							// Накладываем префикс сети
							net.impose(prefix, addr);
							// Выполняем получение данных IPv6 сетевого адреса
							const auto & nwk = net.v6();
							// Выводим результат проверки
							return (::memcmp(&ip[0], &nwk[0], sizeof(ip)) == 0);
						} break;
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
				// Выводим сообщение об ошибке
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(network, prefix, static_cast <uint16_t> (addr), static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод определения режима дислокации IP-адреса
 *
 * @return режим дислокации
 */
awh::Net::mode_t awh::Net::mode() const noexcept {
	// Результат работы функции
	mode_t result = mode_t::NONE;
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объкт для работы с адресами
			net_t net(this->_exp, this->_fmk, this->_log);
			// Выполняем инициализацию списка локальных адресов
			const_cast <net_t *> (this)->initLocalNet();
			// Выполняем группировку нужного нам вида адресов
			auto ret = this->_localsNet.equal_range(this->_type);
			// Перебираем все локальные адреса
			for(auto i = ret.first; i != ret.second; ++i){
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_type)){
					// Если IP-адрес определён как IPv4
					case static_cast <uint8_t> (type_t::IPV4): {
						// Устанавливаем IP-адрес
						net = this->v4();
						// Если получен диапазон IP-адресов
						if(i->second.end->type() == type_t::IPV4){
							// Если адрес входит в диапазон адресов
							if((net >= (* i->second.begin.get())) && (net <= * (i->second.end.get()))){
								// Если адрес зарезервирован
								if(i->second.reserved)
									// Устанавливаем результат
									return mode_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return mode_t::LAN;
							}
						// Если диапазон адресов для этой проверки не установлен
						} else {
							// Устанавливаем префикс сети
							net.impose(i->second.prefix, net_t::addr_t::NETWORK);
							// Если проверяемые сети совпадают
							if(net.v4() == i->second.begin->v4()){
								// Если адрес зарезервирован
								if(i->second.reserved)
									// Устанавливаем результат
									return mode_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return mode_t::LAN;
							}
						}
					} break;
					// Если IP-адрес определён как IPv6
					case static_cast <uint8_t> (type_t::IPV6): {
						// Устанавливаем IP-адрес
						net = this->v6();
						// Если получен диапазон IP-адресов
						if(i->second.end->type() == type_t::IPV6){
							// Если адрес входит в диапазон адресов
							if((net >= (* i->second.begin.get())) && (net <= (* i->second.end.get()))){
								// Если адрес зарезервирован
								if(i->second.reserved)
									// Устанавливаем результат
									return mode_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return mode_t::LAN;
							}
						// Если диапазон адресов для этой проверки не установлен
						} else {
							// Устанавливаем префикс сети
							net.impose(i->second.prefix, net_t::addr_t::NETWORK);
							// Если проверяемые сети совпадают
							if(::memcmp(&net.v6()[0], &i->second.begin->v6()[0], (sizeof(uint64_t) * 2)) == 0){
								// Если адрес зарезервирован
								if(i->second.reserved)
									// Устанавливаем результат
									return mode_t::SYS;
								// Иначе устанавливаем, что адрес локальный
								else return mode_t::LAN;
							}
						}
					} break;
				}
			}
			// Если результат не определён
			if(result == mode_t::NONE)
				// Устанавливаем, что файл ялвяется глобальным
				result = mode_t::WAN;
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
 * @brief Получение записи в формате ARPA
 *
 * @return запись в формате ARPA
 */
string awh::Net::arpa() const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип IP-адреса
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если IP-адрес определён как IPv4
			case static_cast <uint8_t> (type_t::IPV4): {
				// Переходим по всему массиву
				for(int8_t i = (static_cast <int8_t> (this->_buffer.size()) - 1); i > -1; i--){
					// Если строка уже существует, добавляем разделитель
					if(!result.empty())
						// Добавляем разделитель
						result.append(1, '.');
					// Добавляем текущий октет в результат
					result.append(std::to_string(this->_buffer[i]));
				}
				// Добавляем запись ARPA
				result.append(".in-addr.arpa");
			} break;
			// Если IP-адрес определён как IPv6
			case static_cast <uint8_t> (type_t::IPV6): {
				// Значение хексета
				uint16_t num = 0;
				// Переходим по всему массиву
				for(uint8_t i = 0; i < static_cast <uint8_t> (this->_buffer.size()); i += 2){
					// Выполняем получение значение числа
					::memcpy(&num, &this->_buffer[0] + i, sizeof(num));
					// Если строка уже существует, добавляем разделитель
					if(!result.empty())
						// Добавляем разделитель
						result.insert(result.begin(), '.');
					// Выполняем перебор полученного хексета
					for(auto & item : this->zerro(this->_fmk->itoa <uint16_t> (htons(num), 16), 4)){
						// Если последний символ не является точкой
						if(!result.empty() && (result.front() != '.'))
							// Добавляем разделитель
							result.insert(result.begin(), '.');
						// Добавляем хексет в версию
						result.insert(result.begin(), tolower(item));
					}
				}
				// Добавляем запись ARPA
				result.append(".ip6.arpa");
			} break;
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
 * @brief Метод установки записи в формате ARPA
 *
 * @param addr адрес в формате ARPA (1.0.168.192.in-addr.arpa)
 * @return     результат установки записи
 */
bool awh::Net::arpa(const string & addr) noexcept {
	// Результат работы функции
	bool result = false;
	// Если запись передана
	if(!addr.empty() && (addr.length() > 13)){
		// Если адрес является адресом IPv4
		if((result = (addr.substr(addr.length() - 13).compare(".in-addr.arpa") == 0))){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Выполняем очистку буфера данных
				this->_buffer.clear();
				// Выполняем инициализацию буфера
				this->_buffer.resize(4);
				// Устанавливаем тип адреса
				this->_type = type_t::IPV4;
				// Позиция разделителя
				size_t start = 0, stop = 0, index = 3;
				// Получаем адрес для парсинга
				const string ip = addr.substr(0, addr.length() - 13);
				/**
				 * Выполняем поиск разделителя
				 */
				while((stop = ip.find('.', start)) != string::npos){
					// Извлекаем полученное число
					this->_buffer[index] = this->_fmk->atoi <uint8_t> (ip.c_str() + start, stop - start);
					// Выполняем смещение
					start = (stop + 1);
					// Уменьшаем смещение индекса
					index--;
				}
				// Выполняем установку последнего октета
				this->_buffer[index] = this->_fmk->atoi <uint8_t> (ip.c_str() + start, ip.length() - start);
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
				#endif
			}
		// Если адрес является адресом IPv6
		} else if((result = (addr.substr(addr.length() - 9).compare(".ip6.arpa") == 0))) {
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				/**
				 * @brief Структура бинарного буфера
				 *
				 */
				struct Buffer {
					// Временный буфер хексета
					uint8_t hexset[4];
					// Результирующий буфер данных
					uint16_t address[8];
					/**
					 * @brief Конструктор
					 *
					 */
					Buffer() noexcept : hexset{0,0,0,0}, address{0,0,0,0,0,0,0,0} {}
				} __attribute__((packed)) buffer;
				// Выполняем очистку буфера данных
				this->_buffer.clear();
				// Выполняем инициализацию буфера
				this->_buffer.resize(16);
				// Устанавливаем тип адреса
				this->_type = type_t::IPV6;
				// Позиция разделителя
				size_t start = 0, stop = 0;
				// Устанавливаем индекс последнего элемента
				uint8_t index1 = 4, index2 = 8;
				// Получаем адрес для парсинга
				const string & ip = addr.substr(0, addr.length() - 9);
				/**
				 * Выполняем поиск разделителя
				 */
				while((stop = ip.find('.', start)) != string::npos){
					// Выполняем установку хексета
					buffer.hexset[--index1] = static_cast <uint8_t> (ip[start]);
					// Если хексет полностью заполнен
					if(index1 == 0){
						// Добавляем хексет в список
						buffer.address[--index2] = ntohs(this->_fmk->atoi <uint16_t> (reinterpret_cast <const char *> (buffer.hexset), 4, static_cast <uint8_t> (16)));
						// Выполняем сброс индекса
						index1 = 4;
					}
					// Выполняем смещение
					start = (stop + 1);
				}
				// Выполняем установку хексета
				buffer.hexset[--index1] = static_cast <uint8_t> (ip[start]);
				// Если хексет полностью заполнен
				if(index1 == 0)
					// Добавляем хексет в список
					buffer.address[--index2] = ntohs(this->_fmk->atoi <uint16_t> (reinterpret_cast <const char *> (buffer.hexset), 4, static_cast <uint8_t> (16)));
				// Выполняем копирование бинарных данных в буфер
				::memcpy(&this->_buffer[0], buffer.address, sizeof(buffer.address));
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(addr), log_t::flag_t::CRITICAL, error.what());
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
	// Выводим результат
	return result;
}
/**
 * @brief Метод парсинга адреса
 *
 * @param addr адрес аппаратный или интернет подключения для парсинга
 * @return     результат работы парсинга
 */
bool awh::Net::parse(const string & addr) noexcept {
	// Выполняем парсинг переданного адреса
	return this->parse(addr, this->host(addr));
}
/**
 * @brief Метод парсинга адреса
 *
 * @param addr адрес аппаратный или интернет подключения для парсинга
 * @param type тип адреса аппаратного или интернет подключения для парсинга
 * @return     результат работы парсинга
 */
bool awh::Net::parse(const string & addr, const type_t type) noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес аппаратный или интернет подключения передан
	if((result = !addr.empty() && ((type == type_t::MAC) || (type == type_t::IPV4) || (type == type_t::IPV6)))){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Устанавливаем тип адреса
			this->_type = type;
			/**
			 * Определяем тип переданного адреса
			 */
			switch(static_cast <uint8_t> (type)){
				// Если - это не IP-адрес, а MAC-адрес
				case static_cast <uint8_t> (type_t::MAC): {
					// Значение последнего символа
					int32_t last = -1;
					// Бинарный буфер адреса
					uint8_t buffer[6];
					// Выполняем очистку буфера данных
					this->_buffer.clear();
					// Выполняем инициализацию буфера
					this->_buffer.resize(6);
					// Выполняем парсинг MAC адреса
					const int32_t rc = sscanf(
						addr.c_str(),
						"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
						buffer + 0, buffer + 1, buffer + 2,
						buffer + 3, buffer + 4, buffer + 5, &last
					);
					// Если MAC адрес удано распарсен
					if((result = ((rc == 6) && (static_cast <int32_t> (addr.size()) == last))))
						// Выполняем копирование бинарных данных MAC-адреса в буфер
						::memcpy(&this->_buffer[0], buffer, sizeof(buffer));
				} break;
				// Если IP-адрес является адресом IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Выполняем очистку буфера данных
					this->_buffer.clear();
					// Выполняем инициализацию буфера
					this->_buffer.resize(4);
					// Выполняем парсинг IPv4 адреса
					::inet_pton(AF_INET, addr.c_str(), &this->_buffer[0]);
				} break;
				// Если IP-адрес является адресом IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Выполняем очистку буфера данных
					this->_buffer.clear();
					// Выполняем инициализацию буфера
					this->_buffer.resize(16);
					// Если найдено экранирование адреса
					if((addr.front() == '[') && (addr.back() == ']'))
						// Выполняем парсинг IPv6 адреса без экранирования
						::inet_pton(AF_INET6, addr.substr(1, addr.size() - 2).c_str(), &this->_buffer[0]);
					// Выполняем парсинг IPv6 адреса
					else ::inet_pton(AF_INET6, addr.c_str(), &this->_buffer[0]);
				} break;
				// Все остальные варианты мы пропускаем
				default: result = false;
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
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(addr, static_cast <uint16_t> (type)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Метод извлечения данных IP-адреса
 *
 * @param format формат формирования IP-адреса
 * @return       сформированная строка IP-адреса
 */
string awh::Net::get(const format_t format) const noexcept {
	// Результат работы функции
	string result = "";
	// Если бинарный буфер данных существует
	if(!this->_buffer.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если - это не IP-адрес, а MAC-адрес
				case static_cast <uint8_t> (type_t::MAC): {
					// Если размера данных достаточно
					if(this->_buffer.size() >= 6){
						// Перераспределяем объект результата
						result.resize(17);
						// Выполняем получение MAC адреса
						::sprintf(
							&result[0],
							"%02X:%02X:%02X:%02X:%02X:%02X",
							this->_buffer[0], this->_buffer[1],
							this->_buffer[2], this->_buffer[3],
							this->_buffer[4], this->_buffer[5]
						);
					}
				} break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4): {
					// Если формат адреса не принадлежит к IPv6
					if((format != format_t::LONG_IPV6) && (format != format_t::MIDDLE_IPV6) && (format != format_t::SHORT_IPV6)){
						/**
						 * Определяем формат вывода IPv4 адреса
						 */
						switch(static_cast <uint8_t> (format)){
							// Если необходимо вывести адрес в 16-м формате
							case static_cast <uint8_t> (format_t::HEX_IPV4): {
								// Перераспределяем объект результата
								result.resize(11);
								// Выполняем получение IPv4 адреса в 16-м формате
								::sprintf(
									&result[0],
									"%02X.%02X.%02X.%02X",
									this->_buffer[0], this->_buffer[1],
									this->_buffer[2], this->_buffer[3]
								);
							} break;
							// Если необходимо вывести адрес в 8-м формате
							case static_cast <uint8_t> (format_t::OCTAL_IPV4): {
								// Перераспределяем объект результата
								result.resize(16);
								// Выполняем получение IPv4 адреса в 8-м формате
								::sprintf(
									&result[0],
									"%03o.%03o.%03o.%03o",
									this->_buffer[0], this->_buffer[1],
									this->_buffer[2], this->_buffer[3]
								);
							} break;
							// Во всех остальных случаях выполняем стандартный вывод
							default: {
								// Переходим по всему массиву
								for(uint8_t i = 0; i < static_cast <uint8_t> (this->_buffer.size()); i++){
									// Если строка уже существует, добавляем разделитель
									if(!result.empty())
										// Добавляем разделитель
										result.append(1, '.');
									// Добавляем текущий октет в результат
									result.append(
										(format == format_t::LONG) || (format == format_t::LONG_IPV4) ?
										this->zerro(std::to_string(this->_buffer[i])) : std::to_string(this->_buffer[i])
									);
								}
							}
						}
					// Если формат адреса принадлежит к IPv6
					} else {
						// Значение хексета
						uint16_t num = 0;
						// Количество разделителей и количество хексетов в буфере
						uint8_t separators = 0, count = static_cast <uint8_t> (this->_buffer.size());
						// Добавляем в результат начальный разделитель
						result.append(1, ':');
						// Переходим по всему массиву
						for(uint8_t i = 0; i < count; i += 2){
							// Выполняем получение значение числа
							::memcpy(&num, &this->_buffer[0] + i, sizeof(num));
							// Если нужно выводить в кратком виде
							if(format == format_t::SHORT_IPV6){
								// Если Число установлено
								if(num > 0){
									// Добавляем разделитель
									if(!result.empty()) result.append(1, ':');
									// Добавляем хексет в версию
									result.append(this->_fmk->itoa <uint16_t> (htons(num), 16));
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
								// Добавляем хексет в версию
								result.append(
									format == format_t::LONG_IPV6 ?
									this->zerro(this->_fmk->itoa <uint16_t> (htons(num), 16), 4) :
									this->_fmk->itoa <uint16_t> (htons(num), 16)
								);
							}
						}
					}
				} break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Значение хексета
					uint16_t num = 0;
					// Количество разделителей и количество хексетов в буфере
					uint8_t separators = 0, count = static_cast <uint8_t> (this->_buffer.size());
					// Создаём временный буфер данных для сравнения
					vector <uint16_t> buffer(6);
					// Устанавливаем хексет маски
					buffer[5] = 0xFFFF;
					// Флаг зеркального вещания IPv6 => IPv4
					bool broadcast = false;
					// Если буфер данных принадлежит к вещанию IPv6 => IPv4
					if((broadcast = (::memcmp(&buffer[0], &this->_buffer[0], (buffer.size() * 2)) == 0)))
						// Уменьшаем количество итераций в буфере
						count -= 4;
					// Если режим зеркала IPv6 => IPv4 не активен, но установлен формат IPv4, активируем зеркало
					else if(!broadcast && (broadcast = (
						(format == format_t::LONG_IPV4) ||
						(format == format_t::MIDDLE_IPV4) ||
						(format == format_t::SHORT_IPV4) ||
						(format == format_t::HEX_IPV4) ||
						(format == format_t::OCTAL_IPV4))))
						// Уменьшаем количество итераций в буфере
						count -= 4;
					// Переходим по всему массиву
					for(uint8_t i = 0; i < count; i += 2){
						// Выполняем получение значение числа
						::memcpy(&num, &this->_buffer[0] + i, sizeof(num));
						// Если нужно выводить в кратком виде
						if((format == format_t::SHORT) || (format == format_t::SHORT_IPV4)){
							// Если Число установлено
							if(num > 0){
								// Добавляем разделитель
								if(!result.empty())
									// Добавляем разделитель
									result.append(1, ':');
								// Добавляем хексет в версию
								result.append(this->_fmk->itoa <uint16_t> (htons(num), 16));
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
							// Добавляем хексет в версию
							result.append(
								(format == format_t::LONG) || (format == format_t::LONG_IPV4) ?
								this->zerro(this->_fmk->itoa <uint16_t> (htons(num), 16), 4) :
								this->_fmk->itoa <uint16_t> (htons(num), 16)
							);
						}
					}
					// Если активирован флаг зеркального вещания IPv6 => IPv4
					if(broadcast){
						// Если предыдущий символ не является разделителем
						if(result.back() != ':')
							// Добавляем разделитель
							result.append(1, ':');
						/**
						 * Определяем формат вывода IPv4 адреса
						 */
						switch(static_cast <uint8_t> (format)){
							// Если необходимо вывести адрес в 16-м формате
							case static_cast <uint8_t> (format_t::HEX_IPV4): {
								// Определяем текущую позицию результата
								const size_t pos = result.size();
								// Перераспределяем объект результата
								result.resize(pos + 11);
								// Выполняем получение IPv4 адреса в 16-м формате
								::sprintf(
									&result[pos],
									"%02X.%02X.%02X.%02X",
									this->_buffer[count + 0], this->_buffer[count + 1],
									this->_buffer[count + 2], this->_buffer[count + 3]
								);
							} break;
							// Если необходимо вывести адрес в 8-м формате
							case static_cast <uint8_t> (format_t::OCTAL_IPV4): {
								// Определяем текущую позицию результата
								const size_t pos = result.size();
								// Перераспределяем объект результата
								result.resize(pos + 16);
								// Выполняем получение IPv4 адреса в 8-м формате
								::sprintf(
									&result[pos],
									"%03o.%03o.%03o.%03o",
									this->_buffer[count + 0], this->_buffer[count + 1],
									this->_buffer[count + 2], this->_buffer[count + 3]
								);
							} break;
							// Во всех остальных случаях выполняем стандартный вывод
							default: {
								// Переходим по всему массиву
								for(uint8_t i = 0; i < (static_cast <uint8_t> (this->_buffer.size()) - count); i++){
									// Если строка уже существует, добавляем разделитель
									if(i > 0)
										// Добавляем разделитель
										result.append(1, '.');
									// Добавляем текущий октет в результат
									result.append(
										(format == format_t::LONG) || (format == format_t::LONG_IPV4) ?
										this->zerro(std::to_string(this->_buffer[count + i])) : std::to_string(this->_buffer[count + i])
									);
								}
							}
						}
					}
				} break;
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
				this->_log->debug(
					"%s", __PRETTY_FUNCTION__,
					std::make_tuple(static_cast <uint16_t> (format)),
					log_t::flag_t::CRITICAL, error.what()
				);
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
 * @brief Оператор вывода IP-адреса в качестве строки
 *
 * @return IP-адрес в качестве строки
 */
awh::Net::operator string() const noexcept {
	// Выводим данные IP-адреса в виде строки
	return this->get();
}
/**
 * @brief Оператор [<] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator < (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP-адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если MAC-адрес определён
				case static_cast <uint8_t> (type_t::MAC):
					// Выполняем сравнение адресов
					result = this->_fmk->isGreater(&addr.mac()[0], &this->mac()[0], 6);
				break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Выполняем сравнение адресов
					result = (this->v4(endian_t::BIG) < addr.v4(endian_t::BIG));
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Получаем данные текущего адреса IPv6
					const auto & first = this->v6(endian_t::BIG);
					// Получаем данные сравниваемого адреса IPv6
					const auto & second = addr.v6(endian_t::BIG);
					// Выполняем бинарное сравнение
					result = this->_fmk->isGreater(&second[0], &first[0], 16);
				} break;
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
 * @brief Оператор [>] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator > (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP-адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если MAC-адрес определён
				case static_cast <uint8_t> (type_t::MAC):
					// Выполняем сравнение адресов
					result = this->_fmk->isGreater(&this->mac()[0], &addr.mac()[0], 6);
				break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Выполняем сравнение адресов
					result = (this->v4(endian_t::BIG) > addr.v4(endian_t::BIG));
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Получаем данные текущего адреса IPv6
					const auto & first = this->v6(endian_t::BIG);
					// Получаем данные сравниваемого адреса IPv6
					const auto & second = addr.v6(endian_t::BIG);
					// Выполняем бинарное сравнение
					result = this->_fmk->isGreater(&first[0], &second[0], 16);
				} break;
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
 * @brief Оператор [<=] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator <= (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP-адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если MAC-адрес определён
				case static_cast <uint8_t> (type_t::MAC): {
					// Получаем данные текущего адреса MAC
					const auto & first = this->mac();
					// Получаем данные сравниваемого адреса MAC
					const auto & second = addr.mac();
					// Выполняем проверку совпадают ли адреса
					result = (::memcmp(&first[0], &second[0], 6) == 0);
					// Если адреса не совпадают
					if(!result)
						// Выполняем сравнение адресов
						result = this->_fmk->isGreater(&second[0], &first[0], 6);
				} break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Выполняем сравнение адресов
					result = (this->v4(endian_t::BIG) <= addr.v4(endian_t::BIG));
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Получаем данные текущего адреса IPv6
					const auto & first = this->v6(endian_t::BIG);
					// Получаем данные сравниваемого адреса IPv6
					const auto & second = addr.v6(endian_t::BIG);
					// Выполняем проверку совпадают ли адреса
					result = (::memcmp(&first[0], &second[0], 16) == 0);
					// Если адреса не совпадают
					if(!result)
						// Выполняем сравнение адресов
						result = this->_fmk->isGreater(&second[0], &first[0], 16);
				} break;
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
 * @brief Оператор [>=] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator >= (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP-адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если MAC-адрес определён
				case static_cast <uint8_t> (type_t::MAC): {
					// Получаем данные текущего адреса MAC
					const auto & first = this->mac();
					// Получаем данные сравниваемого адреса MAC
					const auto & second = addr.mac();
					// Выполняем проверку совпадают ли адреса
					result = (::memcmp(&first[0], &second[0], 6) == 0);
					// Если адреса не совпадают
					if(!result)
						// Выполняем сравнение адресов
						result = this->_fmk->isGreater(&first[0], &second[0], 6);
				} break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Выполняем сравнение адресов
					result = (this->v4(endian_t::BIG) >= addr.v4(endian_t::BIG));
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6): {
					// Получаем данные текущего адреса IPv6
					const auto & first = this->v6(endian_t::BIG);
					// Получаем данные сравниваемого адреса IPv6
					const auto & second = addr.v6(endian_t::BIG);
					// Выполняем проверку совпадают ли адреса
					result = (::memcmp(&first[0], &second[0], 16) == 0);
					// Если адреса не совпадают
					if(!result)
						// Выполняем сравнение адресов
						result = this->_fmk->isGreater(&first[0], &second[0], 16);
				} break;
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
 * @brief Оператор [!=] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator != (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип IP-адреса
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если MAC-адрес определён
			case static_cast <uint8_t> (type_t::MAC):
				// Выполняем сравнение адресов
				result = (::memcmp(&this->mac()[0], &addr.mac()[0], 6) != 0);
			break;
			// Если IP-адрес определён как IPv4
			case static_cast <uint8_t> (type_t::IPV4):
				// Выполняем сравнение адресов
				result = (this->v4() != addr.v4());
			break;
			// Если IP-адрес определён как IPv6
			case static_cast <uint8_t> (type_t::IPV6):
				// Выполняем сравнение адресов
				result = (::memcmp(&this->v6()[0], &addr.v6()[0], 16) != 0);
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
	// Выводим результат
	return result;
}
/**
 * @brief Оператор [==] сравнения IP-адреса
 *
 * @param addr адрес для сравнения
 * @return     результат сравнения
 */
bool awh::Net::operator == (const net_t & addr) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если IP-адреса принадлежат одному типу адресов
	if(this->type() == addr.type()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если MAC-адрес определён
				case static_cast <uint8_t> (type_t::MAC):
					// Выполняем сравнение адресов
					result = (::memcmp(&this->mac()[0], &addr.mac()[0], 6) == 0);
				break;
				// Если IP-адрес определён как IPv4
				case static_cast <uint8_t> (type_t::IPV4):
					// Выполняем сравнение адресов
					result = (this->v4() == addr.v4());
				break;
				// Если IP-адрес определён как IPv6
				case static_cast <uint8_t> (type_t::IPV6):
					// Выполняем сравнение адресов
					result = (::memcmp(&this->v6()[0], &addr.v6()[0], 16) == 0);
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
 * @brief Оператор [=] присвоения IP-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const net_t & addr) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип IP-адреса
		 */
		switch(static_cast <uint8_t> (addr.type())){
			// Если MAC-адрес определён
			case static_cast <uint8_t> (type_t::MAC):
				// Устанавливаем MAC-адресе
				this->mac(addr.mac());
			break;
			// Если IP-адрес определён как IPv4
			case static_cast <uint8_t> (type_t::IPV4):
				// Устанавливаем IPv4 адресе
				this->v4(addr.v4());
			break;
			// Если IP-адрес определён как IPv6
			case static_cast <uint8_t> (type_t::IPV6):
				// Устанавливаем IPv6 адресе
				this->v6(addr.v6());
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
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присвоения IP-адреса
 *
 * @param ip адрес для присвоения
 * @return   текущий объект
 */
awh::Net & awh::Net::operator = (const string & ip) noexcept {
	// Выполняем установку IP-адреса
	this->parse(ip);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] установки типа IP-адреса
 *
 * @param type тип IP-адреса для установки
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const type_t type) noexcept {
	// Устанавливаем тип IP-адреса
	this->type(type);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присвоения IP-адреса
 *
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
 * @brief Оператор [=] присвоения MAC-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const std::array <uint8_t, 6> & addr) noexcept {
	// Устанавливаем MAC-адрес
	this->mac(addr);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief Оператор [=] присвоения IP-адреса
 *
 * @param addr адрес для присвоения
 * @return     текущий объект
 */
awh::Net & awh::Net::operator = (const std::array <uint8_t, 16> & addr) noexcept {
	// Устанавливаем IPv4
	this->v6(addr);
	// Выводим текущий объект
	return (* this);
}
/**
 * @brief конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Net::Net(const fmk_t * fmk, const log_t * log) noexcept :
 _type(type_t::NONE), _regexp(log), _fmk(fmk), _log(log) {
	// Устанавливаем регулярное выражение для проверки адреса
	this->_exp = this->_regexp.build(
		// Определение MAC адреса
		"^(?:([a-f\\d]{2}(?:\\:[a-f\\d]{2}){5})|"
		// Определение IPv4 адреса
		"(?:(\\d{1,3}(?:\\.\\d{1,3}){3})(?:\\:\\d+){0,1})|"
		// Определение IPv6 адреса
		"(?:\\[?(\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:\\:\\:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){1,7})?)|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))(?:\\](?:\\:\\d+){0,1})?)|"
		// Определение сети
		"((?:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\/(?:\\d{1,3}(?:\\.\\d{1,3}){3}|\\d+))|"
		// Определение домена
		"(?:((?:\\*\\.)?[\\w\\-\\.\\*]+\\.(?:xn\\-{2})?[a-z1]+)(?:\\:\\d+){0,1})|"
		// Определение HTTP адреса
		"(https?\\:\\/\\/[^\\r\\n\\t\\s]+(?:\\/(?:[^\\r\\n\\t\\s]+)?)?)|"
		// Определение адреса файловой системы
		"(\\.{0,2}\\/\\w+(?:\\/[\\w\\.\\-]+)*))$", {regexp_t::option_t::UTF8, regexp_t::option_t::CASELESS}
	);
}
/**
 * @brief конструктор
 *
 * @param exp регулярное выражение для установки
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Net::Net(const regexp_t::exp_t & exp, const fmk_t * fmk, const log_t * log) noexcept :
 _type(type_t::NONE), _regexp(log), _fmk(fmk), _log(log) {
	// Устанавливаем регулярное выражение для проверки адреса
	this->_exp = exp;
}
/**
 * @brief Оператор [>>] чтения из потока IP-адреса
 *
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
		// Устанавливаем IP-адрес
		addr.parse(ip);
	// Выводим результат
	return is;
}
/**
 * @brief Оператор [<<] вывода в поток IP-адреса
 *
 * @param os   поток куда нужно вывести данные
 * @param addr адрес для присвоения
 */
ostream & awh::operator << (ostream & os, const net_t & addr) noexcept {
	// Записываем в поток IP-адрес
	os << addr.get();
	// Выводим результат
	return os;
}
