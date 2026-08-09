/**
 * @file: socket.cpp
 * @date: 2026-08-07
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация бэкенда низкоуровневой работы с сокетами для MS Windows —
 *        сроки ожидания, размеры накопителей, поддержание связи, число переходов,
 *        обнаружение MTU, групповая рассылка и заведение сокетов
 *
 * @details Слой этот отвечает эталонным backend/bsd/socket.cpp и backend/gnu/socket.cpp.
 *          Средства те же самые - setsockopt и getsockopt, - но набор имён опций у
 *          MS Windows свой, а часть их несёт иное устройство доводов: срок ожидания
 *          задаётся числом миллисекунд, а не структурой timeval, накопители же
 *          принимаются указателем на char вместо void
 *
 * @note Расхождения, какие снисходительностью не лечатся, помечены `@note` у самого
 *       метода: скрывать их за общим видом нельзя, вызывающая сторона вправе знать,
 *       чего система не умеет вовсе
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <vector>
#include <cstring>
#include <atomic>
#include <string>
#include <shared_mutex>

/**
 * Подключаем единую точку подключения системных заголовков MS Windows
 */
#include <sys/win32.hpp>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/eth/eth.hpp>
#include <net/backend/win/qos.hpp>

/**
 * @brief Средства перевода названия сетевого устройства в его номер
 *
 * @details Объявлены они у MS Windows в отдельном заголовке netioapi.h, а не там же,
 *          где прочие средства сокетов, как то заведено у систем POSIX заголовком
 *          net/if.h. Подключается он здесь, а не через единую точку win32.hpp:
 *          средства эти нужны одному лишь этому файлу
 *
 */
#include <netioapi.h>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Название бэкенда для записей в журнале
 *
 */
static constexpr const char * __AWH_SOCKET_BACKEND__ = "MS Windows socket backend";

/**
 * @brief Инкапсулируем состояние слоя в пространство имён
 *
 */
namespace {
	// Режим безопасности работы потоков
	awh::event::mode_t __awh_socket_thread_safety__ = awh::event::mode_t::DISABLED;
	// Замок согласования доступа к настройкам сокетов
	std::shared_mutex __awh_socket_mutex__;

	/**
	 * @brief Функция получения описания последней ошибки сокета
	 *
	 * @details Соответствия strerror у MS Windows для ошибок сокетов нет: коды их лежат
	 *          в отдельном пространстве WSA, а сообщения достаются FormatMessageW.
	 *          Сообщение выводится на языке системы, оттого код приводится рядом с ним -
	 *          по коду и следует искать, читая чужой журнал
	 *
	 * @return описание последней ошибки сокета вместе с её кодом
	 *
	 */
	std::string __awh_socket_error__() noexcept {
		// Получаем код последней ошибки сокета
		const int32_t code = ::WSAGetLastError();
		// Буфер сообщения, отводимый самой системой
		wchar_t * buffer = nullptr;
		// Получаем описание ошибки сокета
		const DWORD size = ::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, static_cast <DWORD> (code), 0, reinterpret_cast <wchar_t *> (&buffer), 0, nullptr
		);
		// Собираемое описание ошибки сокета
		std::string result = ("error " + std::to_string(code));
		// Если описание ошибки получено
		if((size > 0) && (buffer != nullptr)){
			// Отводим место под описание ошибки в однобайтовой записи
			std::vector <char> text(static_cast <size_t> (size) * 4 + 1, 0);
			// Переводим описание ошибки в однобайтовую запись
			const int32_t length = ::WideCharToMultiByte(CP_UTF8, 0, buffer, static_cast <int32_t> (size), text.data(), static_cast <int32_t> (text.size() - 1), nullptr, nullptr);
			// Если перевод описания ошибки выполнен
			if(length > 0){
				// Собираем описание ошибки
				std::string message(text.data(), static_cast <size_t> (length));
				/**
				 * Снимаем перевод строки, каким система оканчивает описание
				 */
				while(!message.empty() && ((message.back() == '\r') || (message.back() == '\n')))
					// Снимаем последний знак описания
					message.pop_back();
				// Если описание не опустело - дополняем им результат
				if(!message.empty())
					// Дополняем результат описанием ошибки
					result.append(": ").append(message);
			}
		}
		// Если буфер сообщения системой отведён
		if(buffer != nullptr)
			// Освобождаем буфер сообщения
			::LocalFree(buffer);
		// Возвращаем собранное описание ошибки сокета
		return result;
	}
};

/**
 * @brief Метод установки режима безопасной работы с потоками
 *
 * @param mode режим безопасной работы с потоками
 *
 */
void awh::eth::Socket::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасной работы с потоками
	::__awh_socket_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
}

/**
 * @brief Метод получения кода ошибки сокета
 *
 * @param sock сокет для получения кода ошибки
 * @return     код ошибки сокета
 *
 */
int32_t awh::eth::Socket::getError(const net::socket_t sock) const noexcept {
	// Переменная результата
	int32_t result = -1;
	// Размер кода ошибки
	int32_t size = static_cast <int32_t> (sizeof(result));
	// Если код ошибки сокета получить не удалось
	if(::getsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, SO_ERROR, reinterpret_cast <char *> (&result), &size) != 0)
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	// Возвращаем результат
	return result;
}

/**
 * @brief Метод получения срока ожидания сокета
 *
 * @param sock  сокет для получения срока ожидания
 * @param event событие сокета
 * @return      срок ожидания в миллисекундах
 *
 * @note Срок ожидания у MS Windows задаётся числом миллисекунд типа DWORD, а не
 *       структурой timeval, как то предписано POSIX. Расхождение это уровня самой
 *       системы, и приведение к общему виду ведётся здесь
 *
 */
uint32_t awh::eth::Socket::getTimeout(const net::socket_t sock, const net::socket_event_t event) const noexcept {
	// Срок ожидания, снятый у системы
	DWORD msec = 0;
	// Размер срока ожидания
	int32_t size = static_cast <int32_t> (sizeof(msec));
	// Имя опции срока ожидания
	const int32_t option = ((event == net::socket_event_t::READ) ? SO_RCVTIMEO : SO_SNDTIMEO);
	// Если срок ожидания получить не удалось
	if(::getsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, option, reinterpret_cast <char *> (&msec), &size) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем пустой срок ожидания
		return 0;
	}
	// Возвращаем снятый срок ожидания
	return static_cast <uint32_t> (msec);
}

/**
 * @brief Метод установки срока ожидания сокета
 *
 * @param sock  сокет для установки срока ожидания
 * @param event событие сокета
 * @param msec  срок ожидания в миллисекундах
 * @return      результат выполнения установки
 *
 * @note Нулевой срок означает ожидание бессрочное - так же, как и у систем POSIX
 *
 */
bool awh::eth::Socket::setTimeout(const net::socket_t sock, const net::socket_event_t event, const uint32_t msec) const noexcept {
	// Срок ожидания в понимании системы
	DWORD timeout = static_cast <DWORD> (msec);
	// Имя опции срока ожидания
	const int32_t option = ((event == net::socket_event_t::READ) ? SO_RCVTIMEO : SO_SNDTIMEO);
	// Если срок ожидания установить не удалось
	if(::setsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, option, reinterpret_cast <const char *> (&timeout), static_cast <int32_t> (sizeof(timeout))) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод получения размера накопителя сокета
 *
 * @param sock  сокет для получения размера накопителя
 * @param event событие сокета
 * @return      размер накопителя сокета
 *
 */
int32_t awh::eth::Socket::getBufferSize(const net::socket_t sock, const net::socket_event_t event) const noexcept {
	// Размер накопителя сокета
	int32_t result = 0;
	// Размер значения размера накопителя
	int32_t size = static_cast <int32_t> (sizeof(result));
	// Имя опции размера накопителя
	const int32_t option = ((event == net::socket_event_t::READ) ? SO_RCVBUF : SO_SNDBUF);
	// Если размер накопителя получить не удалось
	if(::getsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, option, reinterpret_cast <char *> (&result), &size) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем пустой размер накопителя
		return 0;
	}
	// Возвращаем снятый размер накопителя
	return result;
}

/**
 * @brief Метод установки размера накопителя сокета
 *
 * @param sock  сокет для установки размера накопителя
 * @param event событие сокета
 * @param size  размер накопителя сокета
 * @return      установленный размер накопителя сокета
 *
 * @note Ответ снимается у системы повторным опросом, а не берётся из просьбы: система
 *       вправе отвести накопитель иного размера, и знать надлежит отведённый
 *
 */
int32_t awh::eth::Socket::setBufferSize(const net::socket_t sock, const net::socket_event_t event, const int32_t size) const noexcept {
	// Имя опции размера накопителя
	const int32_t option = ((event == net::socket_event_t::READ) ? SO_RCVBUF : SO_SNDBUF);
	// Если размер накопителя установить не удалось
	if(::setsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, option, reinterpret_cast <const char *> (&size), static_cast <int32_t> (sizeof(size))) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем пустой размер накопителя
		return 0;
	}
	// Возвращаем размер накопителя, отведённый системой
	return this->getBufferSize(sock, event);
}

/**
 * @brief Метод установки числа переходов пакета
 *
 * @param sock     сокет для установки числа переходов
 * @param family   семейство адресов
 * @param delivery способ доставки
 * @param hops     число переходов
 * @return         результат выполнения установки
 *
 */
bool awh::eth::Socket::setHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery, const uint8_t hops) const noexcept {
	// Уровень опции числа переходов
	int32_t level = 0;
	// Имя опции числа переходов
	int32_t option = 0;
	/**
	 * Определяем семейство адресов сокета
	 */
	switch(static_cast <uint8_t> (family)){
		// Если семейство адресов является IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			// Устанавливаем уровень опции числа переходов
			level = IPPROTO_IP;
			// Устанавливаем имя опции числа переходов
			option = ((delivery == event::delivery_mode_t::MULTICAST) ? IP_MULTICAST_TTL : IP_TTL);
		} break;
		// Если семейство адресов является IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Устанавливаем уровень опции числа переходов
			level = IPPROTO_IPV6;
			// Устанавливаем имя опции числа переходов
			option = ((delivery == event::delivery_mode_t::MULTICAST) ? IPV6_MULTICAST_HOPS : IPV6_UNICAST_HOPS);
		} break;
		// Если семейство адресов неизвестно
		default: return false;
	}
	// Число переходов в понимании системы
	DWORD value = static_cast <DWORD> (hops);
	// Если число переходов установить не удалось
	if(::setsockopt(static_cast <SOCKET> (sock), level, option, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод получения числа переходов пакета
 *
 * @param sock     сокет для получения числа переходов
 * @param family   семейство адресов
 * @param delivery способ доставки
 * @return         число переходов пакета
 *
 */
uint8_t awh::eth::Socket::getHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery) const noexcept {
	// Уровень опции числа переходов
	int32_t level = 0;
	// Имя опции числа переходов
	int32_t option = 0;
	/**
	 * Определяем семейство адресов сокета
	 */
	switch(static_cast <uint8_t> (family)){
		// Если семейство адресов является IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			// Устанавливаем уровень опции числа переходов
			level = IPPROTO_IP;
			// Устанавливаем имя опции числа переходов
			option = ((delivery == event::delivery_mode_t::MULTICAST) ? IP_MULTICAST_TTL : IP_TTL);
		} break;
		// Если семейство адресов является IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Устанавливаем уровень опции числа переходов
			level = IPPROTO_IPV6;
			// Устанавливаем имя опции числа переходов
			option = ((delivery == event::delivery_mode_t::MULTICAST) ? IPV6_MULTICAST_HOPS : IPV6_UNICAST_HOPS);
		} break;
		// Если семейство адресов неизвестно
		default: return 0;
	}
	// Число переходов, снятое у системы
	DWORD value = 0;
	// Размер числа переходов
	int32_t size = static_cast <int32_t> (sizeof(value));
	// Если число переходов получить не удалось
	if(::getsockopt(static_cast <SOCKET> (sock), level, option, reinterpret_cast <char *> (&value), &size) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем пустое число переходов
		return 0;
	}
	// Возвращаем снятое число переходов
	return static_cast <uint8_t> (value);
}

/**
 * @brief Метод установки поддержания подключения в живых
 *
 * @param sock  сокет для установки поддержания подключения
 * @param cnt   максимальное количество попыток
 * @param idle  время простоя подключения в секундах
 * @param intvl интервал времени в секундах между попытками
 * @return      результат выполнения установки
 *
 * @note Число попыток у MS Windows задать нельзя вовсе: SIO_KEEPALIVE_VALS принимает
 *       лишь время простоя и промежуток между попытками, а число их система держит
 *       своё. Довод cnt потому не отбрасывается молча - о нём заносится
 *       предупреждение, когда он задан
 *
 */
bool awh::eth::Socket::setKeepalive(const net::socket_t sock, int32_t cnt, int32_t idle, int32_t intvl) const noexcept {
	// Признак поддержания подключения в живых
	DWORD mode = 1;
	// Если признак поддержания подключения установить не удалось
	if(::setsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast <const char *> (&mode), static_cast <int32_t> (sizeof(mode))) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	// Если число попыток задано - сообщаем, что система его не принимает
	if(cnt > 0)
		// Заносим в журнал предупреждение о неприменимости числа попыток
		this->_log->print("%s: keepalive probe count is not settable on MS Windows, the value %d is ignored", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, cnt);
	// Если ни время простоя, ни промежуток между попытками не заданы
	if((idle <= 0) && (intvl <= 0))
		// Возвращаем положительный результат установки
		return true;
	// Настройки поддержания подключения в живых
	struct tcp_keepalive settings{};
	// Включаем поддержание подключения в живых
	settings.onoff = 1;
	// Устанавливаем время простоя подключения в миллисекундах
	settings.keepalivetime = static_cast <ULONG> ((idle > 0) ? (idle * 1000) : 7200000);
	// Устанавливаем промежуток между попытками в миллисекундах
	settings.keepaliveinterval = static_cast <ULONG> ((intvl > 0) ? (intvl * 1000) : 1000);
	// Количество байт, отданных управляющим вызовом
	DWORD returned = 0;
	// Если настройки поддержания подключения применить не удалось
	if(::WSAIoctl(static_cast <SOCKET> (sock), SIO_KEEPALIVE_VALS, &settings, static_cast <DWORD> (sizeof(settings)), nullptr, 0, &returned, nullptr, nullptr) == SOCKET_ERROR){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод установки сетевого устройства групповой рассылки
 *
 * @param sock   сокет для установки сетевого устройства
 * @param family семейство адресов
 * @param ifname название сетевого устройства
 * @return       результат выполнения установки
 *
 * @note Устройство задаётся номером, а не названием: у MS Windows опции с названием
 *       устройства нет вовсе, и перевод названия в номер ведётся здесь -
 *       if_nametoindex объявлен там же, где и прочие средства Winsock
 *
 */
bool awh::eth::Socket::setMulticastIface(const net::socket_t sock, const event::family_t family, string_view ifname) const noexcept {
	// Если название сетевого устройства не передано
	if(ifname.empty())
		// Возвращаем отрицательный результат установки
		return false;
	// Получаем номер сетевого устройства по его названию
	const uint32_t index = ::if_nametoindex(string(ifname).c_str());
	// Если номер сетевого устройства получить не удалось
	if(index == 0){
		// Заносим в журнал предупреждение об отсутствии сетевого устройства
		this->_log->print("%s: network interface \"%s\" could not be found", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, string(ifname).c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	/**
	 * Определяем семейство адресов сокета
	 */
	switch(static_cast <uint8_t> (family)){
		// Если семейство адресов является IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			/**
			 * Номер устройства передаётся адресом особого вида
			 *
			 * @details Опция IP_MULTICAST_IF у IPv4 принимает адрес устройства, а не его
			 *          номер. MS Windows принимает взамен номер, записанный в порядке
			 *          сети на месте адреса, - приём этот описан самой системой и иного
			 *          способа назвать устройство у IPv4 там нет
			 *
			 */
			// Номер устройства в порядке октетов сети
			const DWORD value = ::htonl(index);
			// Если сетевое устройство установить не удалось
			if(::setsockopt(static_cast <SOCKET> (sock), IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
				// Записываем ошибку в лог
				this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Возвращаем отрицательный результат установки
				return false;
			}
		} break;
		// Если семейство адресов является IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Номер устройства в понимании системы
			const DWORD value = static_cast <DWORD> (index);
			// Если сетевое устройство установить не удалось
			if(::setsockopt(static_cast <SOCKET> (sock), IPPROTO_IPV6, IPV6_MULTICAST_IF, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
				// Записываем ошибку в лог
				this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Возвращаем отрицательный результат установки
				return false;
			}
		} break;
		// Если семейство адресов неизвестно
		default: return false;
	}
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод получения способа обнаружения MTU
 *
 * @param sock   сокет для получения способа обнаружения MTU
 * @param family семейство адресов
 * @return       способ обнаружения MTU
 *
 * @note Способов у MS Windows четыре против десяти у самой библиотеки, и приводятся
 *       они к общему виду здесь: IP_PMTUDISC_DONT отвечает DONT, IP_PMTUDISC_DO - DO,
 *       IP_PMTUDISC_PROBE - PROBE. Остальным ответа у системы нет вовсе
 *
 */
awh::event::mtu_discover_t awh::eth::Socket::getMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family) const noexcept {
	// Уровень опции обнаружения MTU
	const int32_t level = ((family == event::family_t::IPV6) ? IPPROTO_IPV6 : IPPROTO_IP);
	// Имя опции обнаружения MTU
	const int32_t option = ((family == event::family_t::IPV6) ? IPV6_MTU_DISCOVER : IP_MTU_DISCOVER);
	// Способ обнаружения MTU, снятый у системы
	DWORD value = 0;
	// Размер способа обнаружения MTU
	int32_t size = static_cast <int32_t> (sizeof(value));
	// Если способ обнаружения MTU получить не удалось
	if(::getsockopt(static_cast <SOCKET> (sock), level, option, reinterpret_cast <char *> (&value), &size) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем неопределённый способ обнаружения MTU
		return event::mtu_discover_t::NONE;
	}
	/**
	 * Определяем снятый способ обнаружения MTU
	 */
	switch(value){
		// Если обнаружение MTU выполнять не следует
		case IP_PMTUDISC_DONT: return event::mtu_discover_t::DONT;
		// Если обнаружение MTU выполнять следует
		case IP_PMTUDISC_DO: return event::mtu_discover_t::DO;
		// Если следует отправлять пробные пакеты
		case IP_PMTUDISC_PROBE: return event::mtu_discover_t::PROBE;
	}
	// Возвращаем неопределённый способ обнаружения MTU
	return event::mtu_discover_t::NONE;
}

/**
 * @brief Метод установки способа обнаружения MTU
 *
 * @param sock   сокет для установки способа обнаружения MTU
 * @param family семейство адресов
 * @param mode   способ обнаружения MTU
 * @return       результат выполнения установки
 *
 * @note Способы, ответа у системы не имеющие, отказом не отвечают молча: о них
 *       заносится предупреждение, а сама установка не ведётся
 *
 */
bool awh::eth::Socket::setMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family, const event::mtu_discover_t mode) const noexcept {
	// Способ обнаружения MTU в понимании системы
	DWORD value = 0;
	/**
	 * Определяем заданный способ обнаружения MTU
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если обнаружение MTU выполнять не следует
		case static_cast <uint8_t> (event::mtu_discover_t::DONT): value = IP_PMTUDISC_DONT; break;
		// Если обнаружение MTU выполнять следует
		case static_cast <uint8_t> (event::mtu_discover_t::WANT):
		case static_cast <uint8_t> (event::mtu_discover_t::DO): value = IP_PMTUDISC_DO; break;
		// Если следует отправлять пробные пакеты
		case static_cast <uint8_t> (event::mtu_discover_t::PROBE): value = IP_PMTUDISC_PROBE; break;
		// Если способ обнаружения MTU ответа у системы не имеет
		default: {
			// Заносим в журнал предупреждение об отсутствии ответа у системы
			this->_log->print("%s: MTU discover mode %u has no counterpart on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, static_cast <uint16_t> (mode));
			// Возвращаем отрицательный результат установки
			return false;
		}
	}
	// Уровень опции обнаружения MTU
	const int32_t level = ((family == event::family_t::IPV6) ? IPPROTO_IPV6 : IPPROTO_IP);
	// Имя опции обнаружения MTU
	const int32_t option = ((family == event::family_t::IPV6) ? IPV6_MTU_DISCOVER : IP_MTU_DISCOVER);
	// Если способ обнаружения MTU установить не удалось
	if(::setsockopt(static_cast <SOCKET> (sock), level, option, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод заведения сокета
 *
 * @param family семейство адресов
 * @param type   тип сокета
 * @param proto  протокол передачи данных
 * @return       заведённый сокет
 *
 * @note Сокет заводится наложенным (WSA_FLAG_OVERLAPPED), и иначе быть не может: у
 *       дескриптора без наложения система выстраивает операции в очередь, и запись из
 *       одного потока дожидается чтения из другого. Проверено опытом на именованном
 *       канале - обмен вставал намертво. Наложение же есть то самое устройство, на
 *       каком стоит порт завершения ввода-вывода
 *
 */
awh::net::socket_t awh::eth::Socket::issue(const event::family_t family, const event::type_t type, const event::protocol_t proto, const uint16_t options) const noexcept {
	// Опции при создании сокета эта система не принимает, они накладываются отдельно
	(void) options;
	// Семейство адресов сокета в понимании системы
	int32_t domain = 0;
	/**
	 * Определяем семейство адресов сокета
	 */
	switch(static_cast <uint8_t> (family)){
		// Если семейство адресов является IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): domain = AF_INET; break;
		// Если семейство адресов является IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): domain = AF_INET6; break;
		// Если семейство адресов неизвестно
		default: {
			// Заносим в журнал предупреждение о неподдерживаемом семействе адресов
			this->_log->print("%s: address family %u is not supported", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, static_cast <uint16_t> (family));
			// Возвращаем признак отсутствия заведённого сокета
			return static_cast <net::socket_t> (INVALID_SOCKET);
		}
	}
	// Тип сокета в понимании системы
	int32_t kind = 0;
	/**
	 * Определяем тип сокета
	 */
	switch(static_cast <uint8_t> (type)){
		// Если сокет является потоковым
		case static_cast <uint8_t> (event::type_t::STREAM): kind = SOCK_STREAM; break;
		// Если сокет является дейтаграммным
		case static_cast <uint8_t> (event::type_t::DATAGRAM): kind = SOCK_DGRAM; break;
		// Если сокет является сырым
		case static_cast <uint8_t> (event::type_t::RAW): kind = SOCK_RAW; break;
		/**
		 * Сокета последовательных пакетов у MS Windows нет вовсе
		 *
		 * @details Держится он на SCTP, какого система не несёт ни в каком виде: ни
		 *          заголовков, ни поддержки в ядре. Отказ здесь потому не временный, и
		 *          заглушкой он не является
		 *
		 */
		case static_cast <uint8_t> (event::type_t::SEQPACKET): {
			// Заносим в журнал предупреждение об отсутствии типа сокета у системы
			this->_log->print("%s: sequenced packet sockets are not available on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__);
			// Возвращаем признак отсутствия заведённого сокета
			return static_cast <net::socket_t> (INVALID_SOCKET);
		}
		// Если тип сокета неизвестен
		default: {
			// Заносим в журнал предупреждение о неподдерживаемом типе сокета
			this->_log->print("%s: socket type %u is not supported", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, static_cast <uint16_t> (type));
			// Возвращаем признак отсутствия заведённого сокета
			return static_cast <net::socket_t> (INVALID_SOCKET);
		}
	}
	// Протокол передачи данных в понимании системы
	int32_t protocol = 0;
	/**
	 * Определяем протокол передачи данных
	 */
	switch(static_cast <uint8_t> (proto)){
		// Если протоколом передачи данных является TCP
		case static_cast <uint8_t> (event::protocol_t::TCP): protocol = IPPROTO_TCP; break;
		// Если протоколом передачи данных является UDP
		case static_cast <uint8_t> (event::protocol_t::UDP): protocol = IPPROTO_UDP; break;
		// Если протокол передачи данных не задан - система выбирает его сама по типу
		default: protocol = 0;
	}
	// Выполняем заведение наложенного сокета
	const SOCKET result = ::WSASocketW(domain, kind, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
	// Если сокет завести не удалось
	if(result == INVALID_SOCKET)
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	// Возвращаем заведённый сокет
	return static_cast <net::socket_t> (result);
}

/**
 * @brief Метод получения опций, принимаемых при создании сокета
 *
 * @note Эта система опций при создании сокета не принимает вовсе, оттого
 * набор всегда пуст, и опции накладываются отдельными обращениями
 *
 * @param options набор опций события
 * @return        подмножество опций, наложенных при создании сокета
 *
 */
uint16_t awh::eth::Socket::inborn(const uint16_t options) const noexcept {
	// Опции при создании сокета эта система не принимает
	(void) options;
	// Выводим пустой набор опций
	return event::options::NONE;
}


/**
 * @brief Метод заведения пары сокетов для обмена сообщениями между процессами
 *
 * @param family семейство адресов сокета
 * @param type   тип сокета
 * @param proto  протокол сокета
 * @return       пара заведённых сокетов
 *
 * @details Соответствия socketpair у MS Windows нет вовсе, и пара строится в обход:
 *          заводится слушатель на петлевом устройстве с нулевым портом, к нему
 *          выполняется подключение, а принятое подключение и даёт вторую сторону
 *          пары. Слушатель затем закрывается - он был нужен лишь для сведения сторон
 *
 * @note Порт слушателю не задаётся: система выбирает свободный сама, и узнаётся он
 *       затем через getsockname. Оттого пара заводится без всякой оглядки на то,
 *       занят ли какой-либо порт
 *
 * @details Тип пары соблюдается, а не подменяется молча:
 *
 *          | Тип | Устройство под MS Windows | Границы | Надёжность |
 *          |---|---|---|---|
 *          | `STREAM` | петлевая пара TCP | нет | да |
 *          | `DATAGRAM` | петлевая пара UDP | да | петлевая доставка |
 *          | `SEQPACKET` | именованный канал в строе сообщений | да | да |
 *
 * @warning Пара последовательных пакетов стоит **не на сокете**, и это следует держать
 *          в уме всюду, где она попадает в чужие руки:
 *
 *          - обмен ведётся ReadFile и WriteFile, а не recv и send;
 *          - закрывается дескриптор через CloseHandle, а не через closesocket;
 *          - опции сокета к нему неприменимы вовсе.
 *
 *          Различать пути обязан тот, кто дескриптором пользуется, и признака для того
 *          заводить не требуется: тип узла он уже несёт в своём состоянии, а вид пары
 *          определяется им однозначно
 *
 * @note Численно дескриптор канала в net::socket_t помещается: под MS Windows тип этот
 *       заведён как uintptr_t, а не как int32_t, каким он бывает у POSIX
 *
 * @warning Сокеты заводятся наложенными: у дескриптора без наложения система
 *          выстраивает операции в очередь, и запись из одного потока дожидается
 *          чтения из другого. Проверено опытом на именованном канале - обмен вставал
 *          намертво
 *
 */
array <awh::net::socket_t, 2> awh::eth::Socket::ipc([[maybe_unused]] const event::family_t family, const event::type_t type, [[maybe_unused]] const event::protocol_t proto) const noexcept {
	// Переменная результата
	array <net::socket_t, 2> result = {
		net::invalid_socket_t,
		net::invalid_socket_t
	};
	// Тип сокетов пары в понимании системы
	int32_t kind = 0;
	// Протокол сокетов пары в понимании системы
	int32_t protocol = 0;
	/**
	 * Определяем запрошенный тип пары
	 */
	switch(static_cast <uint8_t> (type)){
		// Если пара запрошена потоковой
		case static_cast <uint8_t> (event::type_t::NONE):
		case static_cast <uint8_t> (event::type_t::STREAM): {
			// Устанавливаем потоковый тип сокетов пары
			kind = SOCK_STREAM;
			// Устанавливаем протокол сокетов пары
			protocol = IPPROTO_TCP;
		} break;
		// Если пара запрошена дейтаграммной
		case static_cast <uint8_t> (event::type_t::DATAGRAM): {
			// Устанавливаем дейтаграммный тип сокетов пары
			kind = SOCK_DGRAM;
			// Устанавливаем протокол сокетов пары
			protocol = IPPROTO_UDP;
		} break;
		/**
		 * Если пара запрошена последовательными пакетами - строим её именованным каналом
		 *
		 * @details Сокетом последовательные пакеты у MS Windows не выходят вовсе: они
		 *          требуют границ сообщений и надёжности разом. Даёт то и другое лишь
		 *          именованный канал в строе сообщений, и пара этого вида стоит на нём.
		 *          Подставлять взамен дейтаграммную пару, как то делает эталонный
		 *          бэкенд на macOS и OpenBSD, здесь не потребовалось: там подстановка
		 *          вынужденная, а тут есть устройство с той же семантикой
		 *
		 */
		case static_cast <uint8_t> (event::type_t::SEQPACKET): {
			/**
			 * Составляем имя именованного канала
			 *
			 * @details Имя обязано быть неповторимым в пределах всей машины: пространство
			 *          имён каналов у MS Windows общее, и совпадение имён свело бы вместе
			 *          два несвязанных обмена. Неповторимость даётся парой «номер процесса
			 *          - порядковый номер пары»: первый разводит процессы между собой,
			 *          второй разводит пары внутри одного процесса
			 *
			 */
			// Порядковый номер заводимой пары
			static std::atomic_uint64_t counter{0};
			// Составляем имя именованного канала
			const std::wstring name = (L"\\\\.\\pipe\\awh-ipc-" + std::to_wstring(static_cast <uint32_t> (::GetCurrentProcessId())) + L"-" + std::to_wstring(++counter));
			/**
			 * Заводим сторону канала, ожидающую подключения
			 *
			 * @note Обмен ведётся наложенным: у дескриптора без наложения система
			 *       выстраивает операции в очередь, и запись из одного потока дожидается
			 *       чтения из другого. Проверено опытом - обмен вставал намертво
			 *
			 */
			HANDLE server = ::CreateNamedPipeW(
				name.c_str(),
				PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
				2, 65536, 65536, 0, nullptr
			);
			// Если сторону канала завести не удалось
			if(server == INVALID_HANDLE_VALUE){
				// Записываем ошибку в лог
				this->_log->print("%s: named pipe could not be created, error %lu", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::GetLastError());
				// Возвращаем пустую пару
				return result;
			}
			// Открываем встречный конец канала
			HANDLE client = ::CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
			// Если встречный конец канала открыть не удалось
			if(client == INVALID_HANDLE_VALUE){
				// Записываем ошибку в лог
				this->_log->print("%s: named pipe could not be opened, error %lu", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::GetLastError());
				// Закрываем заведённую сторону канала
				::CloseHandle(server);
				// Возвращаем пустую пару
				return result;
			}
			// Строй чтения встречного конца канала
			DWORD mode = PIPE_READMODE_MESSAGE;
			// Переводим встречный конец канала в строй сообщений
			::SetNamedPipeHandleState(client, &mode, nullptr, nullptr);
			// Запоминаем сторону канала, ожидающую подключения
			result[0] = reinterpret_cast <net::socket_t> (server);
			// Запоминаем встречный конец канала
			result[1] = reinterpret_cast <net::socket_t> (client);
			// Возвращаем заведённую пару
			return result;
		}
		// Если пара запрошена типом, какого у системы нет вовсе
		default: {
			// Заносим в журнал предупреждение об отсутствии типа пары у системы
			this->_log->print("%s: socket pair of type %u is not available on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, static_cast <uint16_t> (type));
			// Возвращаем пустую пару сокетов
			return result;
		}
	}
	/**
	 * Дейтаграммная пара строится встречной привязкой, без слушателя
	 *
	 * @details Слушателя дейтаграммам не нужно вовсе: оба сокета привязываются к
	 *          петлевому устройству с нулевым портом, узнают выбранные системой порты
	 *          друг друга и подключаются встречно. Подключение дейтаграммного сокета
	 *          обмена не заводит, а лишь закрепляет за ним собеседника - тем и
	 *          выходит пара
	 *
	 * @note Границы сообщений такая пара сохраняет, а вот доставку обещает лишь в той
	 *       мере, в какой её обещает петлевое устройство: потерь там не бывает, но
	 *       правилом это не закреплено
	 *
	 */
	if(kind == SOCK_DGRAM){
		// Адреса сторон пары
		struct sockaddr_in first{}, second{};
		// Размеры адресов сторон пары
		int32_t firstSize = static_cast <int32_t> (sizeof(first)), secondSize = static_cast <int32_t> (sizeof(second));
		// Заводим первую сторону пары наложенным сокетом
		const SOCKET one = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		// Заводим вторую сторону пары наложенным сокетом
		const SOCKET two = ((one != INVALID_SOCKET) ? ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED) : INVALID_SOCKET);
		/**
		 * Заводим дейтаграммную пару
		 *
		 * @note Цикл здесь однопроходный и заведён ради единой точки уборки
		 *
		 */
		do {
			// Если хоть одну из сторон пары завести не удалось
			if((one == INVALID_SOCKET) || (two == INVALID_SOCKET))
				// Выходим из цикла заведения пары
				break;
			// Выполняем обнуление адреса первой стороны пары
			::memset(&first, 0, sizeof(first));
			// Устанавливаем семейство адресов первой стороны пары
			first.sin_family = AF_INET;
			// Устанавливаем петлевое сетевое устройство первой стороне пары
			first.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
			// Копируем заготовку адреса второй стороне пары
			second = first;
			// Если привязать первую сторону пары не удалось
			if(::bind(one, reinterpret_cast <struct sockaddr *> (&first), static_cast <int32_t> (sizeof(first))) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			// Если привязать вторую сторону пары не удалось
			if(::bind(two, reinterpret_cast <struct sockaddr *> (&second), static_cast <int32_t> (sizeof(second))) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			// Если узнать адрес первой стороны пары не удалось
			if(::getsockname(one, reinterpret_cast <struct sockaddr *> (&first), &firstSize) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			// Если узнать адрес второй стороны пары не удалось
			if(::getsockname(two, reinterpret_cast <struct sockaddr *> (&second), &secondSize) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			/**
			 * Восполняем поля, каких система не заполняет
			 *
			 * @details Getsockname у MS Windows вправе выставить один лишь номер порта,
			 *          оставив семейство адресов и сам адрес нетронутыми
			 *
			 */
			// Устанавливаем семейство адресов первой стороны пары
			first.sin_family = AF_INET;
			// Устанавливаем петлевое сетевое устройство первой стороне пары
			first.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
			// Устанавливаем семейство адресов второй стороны пары
			second.sin_family = AF_INET;
			// Устанавливаем петлевое сетевое устройство второй стороне пары
			second.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
			// Если закрепить за первой стороной пары вторую не удалось
			if(::connect(one, reinterpret_cast <struct sockaddr *> (&second), static_cast <int32_t> (sizeof(second))) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			// Если закрепить за второй стороной пары первую не удалось
			if(::connect(two, reinterpret_cast <struct sockaddr *> (&first), static_cast <int32_t> (sizeof(first))) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			// Запоминаем первую сторону пары
			result[0] = static_cast <net::socket_t> (one);
			// Запоминаем вторую сторону пары
			result[1] = static_cast <net::socket_t> (two);
			// Возвращаем заведённую пару сокетов
			return result;
		} while(false);
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Если первая сторона пары заведена - закрываем её
		if(one != INVALID_SOCKET)
			// Закрываем первую сторону пары
			::closesocket(one);
		// Если вторая сторона пары заведена - закрываем её
		if(two != INVALID_SOCKET)
			// Закрываем вторую сторону пары
			::closesocket(two);
		// Возвращаем пустую пару сокетов
		return result;
	}

	/**
	 * Объединение видов адреса слушателя
	 *
	 * @note Объединение нужно затем, что bind и getsockname принимают адрес общего
	 *       вида, а заполняется он видом IPv4: приведение указателем обошлось бы
	 *       нарушением правил обращения к памяти
	 *
	 */
	union {
		struct sockaddr_in inaddr; // Адрес слушателя вида IPv4
		struct sockaddr addr;      // Адрес слушателя общего вида
	} address;
	// Размер адреса слушателя
	int32_t length = static_cast <int32_t> (sizeof(address.inaddr));
	// Заводим сокет слушателя
	const SOCKET listener = ::socket(AF_INET, kind, protocol);
	// Если сокет слушателя завести не удалось
	if(listener == INVALID_SOCKET){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем пустую пару сокетов
		return result;
	}
	// Выполняем обнуление адреса слушателя
	::memset(&address, 0, sizeof(address));
	// Устанавливаем семейство адресов слушателя
	address.inaddr.sin_family = AF_INET;
	// Устанавливаем петлевое сетевое устройство
	address.inaddr.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
	// Порт слушателю не задаём - система выберет свободный сама
	address.inaddr.sin_port = 0;
	// Сокеты пары, заводимые ниже
	SOCKET first = INVALID_SOCKET, second = INVALID_SOCKET;
	/**
	 * Заводим пару сокетов
	 *
	 * @note Цикл здесь однопроходный и заведён ради единой точки уборки: всякий отказ
	 *       выходит из него, а уборка ведётся один раз ниже
	 *
	 */
	do {
		// Признак разрешения повторного использования адреса
		const int32_t reuse = 1;
		// Если признак повторного использования адреса установить не удалось
		if(::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&reuse), static_cast <int32_t> (sizeof(reuse))) == SOCKET_ERROR)
			// Выходим из цикла заведения пары
			break;
		// Если привязать слушателя к адресу не удалось
		if(::bind(listener, &address.addr, static_cast <int32_t> (sizeof(address.inaddr))) == SOCKET_ERROR)
			// Выходим из цикла заведения пары
			break;
		// Выполняем обнуление адреса слушателя перед его опросом
		::memset(&address, 0, sizeof(address));
		// Если узнать адрес слушателя не удалось
		if(::getsockname(listener, &address.addr, &length) == SOCKET_ERROR)
			// Выходим из цикла заведения пары
			break;
		/**
		 * Восполняем поля, каких система не заполняет
		 *
		 * @details Getsockname у MS Windows вправе выставить один лишь номер порта,
		 *          оставив семейство адресов и сам адрес нетронутыми. Описано это самой
		 *          системой; полагаться на заполнение их нельзя
		 *
		 */
		// Устанавливаем семейство адресов слушателя
		address.inaddr.sin_family = AF_INET;
		// Устанавливаем петлевое сетевое устройство
		address.inaddr.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
		// Если запустить прослушивание не удалось
		if(::listen(listener, 1) == SOCKET_ERROR)
			// Выходим из цикла заведения пары
			break;
		// Заводим первую сторону пары наложенным сокетом
		first = ::WSASocketW(AF_INET, kind, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
		// Если первую сторону пары завести не удалось
		if(first == INVALID_SOCKET)
			// Выходим из цикла заведения пары
			break;
		// Если подключить первую сторону пары к слушателю не удалось
		if(::connect(first, &address.addr, static_cast <int32_t> (sizeof(address.inaddr))) == SOCKET_ERROR)
			// Выходим из цикла заведения пары
			break;
		// Принимаем подключение - оно и даёт вторую сторону пары
		second = ::accept(listener, nullptr, nullptr);
		// Если вторую сторону пары принять не удалось
		if(second == INVALID_SOCKET)
			// Выходим из цикла заведения пары
			break;
		// Закрываем сокет слушателя - сведя стороны, он больше не нужен
		::closesocket(listener);
		// Запоминаем первую сторону пары
		result[0] = static_cast <net::socket_t> (first);
		// Запоминаем вторую сторону пары
		result[1] = static_cast <net::socket_t> (second);
		// Возвращаем заведённую пару сокетов
		return result;
	} while(false);
	// Записываем ошибку в лог
	this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	// Закрываем сокет слушателя
	::closesocket(listener);
	// Если первая сторона пары заведена - закрываем её
	if(first != INVALID_SOCKET)
		// Закрываем первую сторону пары
		::closesocket(first);
	// Если вторая сторона пары заведена - закрываем её
	if(second != INVALID_SOCKET)
		// Закрываем вторую сторону пары
		::closesocket(second);
	// Возвращаем пустую пару сокетов
	return result;
}

/**
 * @brief Метод получения класса обслуживания в заголовке IP-пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @return       класс обслуживания
 *
 * @warning Отдаётся здесь запрошенное, а не показание системы. Настройка IP_TOS у MS
 *          Windows существует, но с версии Windows XP SP2 не исполняется: запись
 *          отвечает согласием, а в заголовок пакета не попадает ничего. Отметка
 *          ставится подсистемой качества обслуживания qWAVE, а обратного чтения она
 *          не даёт вовсе - оттого выдаётся то, что запрашивали
 *
 */
awh::event::dscp_t awh::eth::Socket::getDifferentiatedServicesCodePoint(const net::socket_t sock, [[maybe_unused]] const event::family_t family) const noexcept {
	// Выводим запрошенный для сокета класс обслуживания
	return win::qos::mark(sock);
}
/**
 * @brief Метод установки класса обслуживания в заголовке IP-пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @param dscp   устанавливаемый класс обслуживания
 * @return       результат выполнения установки
 *
 * @note Отметка ставится сразу, если сокет уже соединён - подсистема берёт назначение
 *       у него самого. Иначе она запоминается и будет применена, едва назначение
 *       станет известно: назначения требует сама подсистема, а порядок вызовов ей не
 *       подчинён
 *
 * @warning Заведённый подсистемой поток обязан быть снят прежде закрытия сокета
 *
 */
bool awh::eth::Socket::setDifferentiatedServicesCodePoint(const net::socket_t sock, [[maybe_unused]] const event::family_t family, const event::dscp_t dscp) const noexcept {
	// Выводим результат установки класса обслуживания сокету
	return win::qos::mark(sock, dscp, this->_log);
}
/**
 * @brief Метод получения признака перегрузки в заголовке IP-пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @return       признак перегрузки
 *
 * @note У систем POSIX признак этот занимает два младших разряда того же октета, где
 *       стоит класс обслуживания, и читается вместе с ним настройкой IP_TOS. MS
 *       Windows развела их порознь: признак перегрузки читается своей настройкой
 *       IP_ECN и класса обслуживания при себе не несёт - оттого прикрывать чтение
 *       разрядной маской здесь не приходится
 *
 */
awh::event::ecn_t awh::eth::Socket::getExplicitCongestionNotification(const net::socket_t sock, const event::family_t family) const noexcept {
	// Если сокет не передан
	if(sock == net::invalid_socket_t)
		// Выводим отсутствие признака перегрузки
		return event::ecn_t::NOT_ECT;
	// Считываемое значение признака перегрузки
	int32_t value = 0;
	// Размер считываемого значения
	int32_t length = static_cast <int32_t> (sizeof(value));
	// Уровень настройки и её название
	int32_t level = IPPROTO_IP, option = IP_ECN;
	// Если сокет работает по протоколу IPv6
	if(family == event::family_t::IPV6){
		// Устанавливаем уровень настройки
		level = IPPROTO_IPV6;
		// Устанавливаем название настройки
		option = IPV6_ECN;
	}
	// Если считать признак перегрузки не удалось
	if(::getsockopt(sock, level, option, reinterpret_cast <char *> (&value), &length) != 0){
		// Выводим в журнал сообщение о невозможности чтения признака перегрузки
		this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Выводим отсутствие признака перегрузки
		return event::ecn_t::NOT_ECT;
	}
	// Выводим считанный признак перегрузки
	return static_cast <event::ecn_t> (value & 0x03);
}
/**
 * @brief Метод установки признака перегрузки в заголовке IP-пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @param ecn    устанавливаемый признак перегрузки
 * @return       результат выполнения установки
 *
 * @note Читать прежнее значение перед записью, как то делают эталонные бэкенды, здесь
 *       не нужно: класс обслуживания настройкой этой не задевается вовсе
 *
 */
bool awh::eth::Socket::setExplicitCongestionNotification(const net::socket_t sock, const event::family_t family, const event::ecn_t ecn) const noexcept {
	// Если сокет не передан
	if(sock == net::invalid_socket_t)
		// Выводим отрицательный результат установки
		return false;
	// Устанавливаемое значение признака перегрузки
	const int32_t value = static_cast <int32_t> (ecn);
	// Уровень настройки и её название
	int32_t level = IPPROTO_IP, option = IP_ECN;
	// Если сокет работает по протоколу IPv6
	if(family == event::family_t::IPV6){
		// Устанавливаем уровень настройки
		level = IPPROTO_IPV6;
		// Устанавливаем название настройки
		option = IPV6_ECN;
	}
	// Если установить признак перегрузки не удалось
	if(::setsockopt(sock, level, option, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
		// Выводим в журнал сообщение о невозможности установки признака перегрузки
		this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Выводим отрицательный результат установки
		return false;
	}
	// Выводим положительный результат установки
	return true;
}
/**
 * @brief Метод включения либо выключения выдачи сведений о принятых пакетах
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @param mode   режим включения либо выключения
 * @return       результат выполнения переключения
 *
 * @details Выдача эта прикладывает к каждому принятому пакету его сопутствующие
 *          сведения - число переходов, какое пакет ещё вправе сделать, и устройство,
 *          каким он принят
 *
 * @note Обе настройки выставляются заодно, и успехом считается лишь общий их успех:
 *       выдача сведений наполовину для разбора пакета непригодна
 *
 */
bool awh::eth::Socket::trafficInfoGeneration(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode) const noexcept {
	// Если сокет не передан
	if(sock == net::invalid_socket_t)
		// Выводим отрицательный результат переключения
		return false;
	// Устанавливаемое значение настроек
	const int32_t value = (mode == net::socket_mode_t::ENABLED ? 1 : 0);
	// Уровень настроек и их названия
	int32_t level = IPPROTO_IP, hops = IP_RECVTTL, info = IP_PKTINFO;
	// Если сокет работает по протоколу IPv6
	if(family == event::family_t::IPV6){
		// Устанавливаем уровень настроек
		level = IPPROTO_IPV6;
		// Устанавливаем название настройки выдачи числа переходов
		hops = IPV6_HOPLIMIT;
		// Устанавливаем название настройки выдачи сведений об устройстве
		info = IPV6_PKTINFO;
	}
	// Результат выставления настройки выдачи числа переходов
	const bool first = (::setsockopt(sock, level, hops, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) == 0);
	// Если выставить настройку выдачи числа переходов не удалось
	if(!first)
		// Выводим в журнал сообщение о невозможности выставления настройки
		this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	// Результат выставления настройки выдачи сведений об устройстве
	const bool second = (::setsockopt(sock, level, info, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) == 0);
	// Если выставить настройку выдачи сведений об устройстве не удалось
	if(!second)
		// Выводим в журнал сообщение о невозможности выставления настройки
		this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	// Выводим общий результат переключения
	return (first && second);
}
/**
 * @brief Метод переключения настройки сокета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @param mode   режим включения либо выключения
 * @param option переключаемая настройка
 * @param proto  протокол сокета, NONE - протокол не назван
 * @return       результат выполнения переключения
 *
 * @details Часть настроек у MS Windows зовётся иначе, чем у систем POSIX, часть
 *          достигается вовсе не настройкой сокета, а часть недостижима
 *
 *          | Настройка | Чем достигается у MS Windows |
 *          |---|---|
 *          | HDRINCL | IP_HDRINCL, у IPv6 соответствия нет |
 *          | TCP_NO_DELAY | TCP_NODELAY |
 *          | IPV6_ONLY | IPV6_V6ONLY |
 *          | BROADCAST | SO_BROADCAST |
 *          | REUSE_ADDR | SO_REUSEADDR |
 *          | MULTICAST_LOOPBACK | IP_MULTICAST_LOOP либо IPV6_MULTICAST_LOOP |
 *          | HARD_CLOSE | SO_LINGER с нулевой выдержкой |
 *          | NO_IO_BLOCK, SM_IO_BLOCK | ioctlsocket с FIONBIO |
 *          | CLOSE_ON_EXEC | SetHandleInformation без наследования |
 *          | NO_SIGILL, NO_SIGPIPE | достигнуто само собой |
 *          | TCP_CORKING | недостижимо |
 *          | REUSE_PORT | недостижимо |
 *
 * @note Заглушение сигналов настройки не требует вовсе: сокеты MS Windows сигналов не
 *       поднимают, и запрошенное состояние здесь уже стоит. Отвечать отказом на то,
 *       чего добивались и что уже есть, было бы неверно, оттого настройки эти
 *       отвечают согласием, не трогая сокета
 *
 */
bool awh::eth::Socket::switchOption(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode, const uint16_t option, [[maybe_unused]] const event::protocol_t proto) const noexcept {
	// Если сокет не передан
	if(sock == net::invalid_socket_t)
		// Выводим отрицательный результат переключения
		return false;
	// Устанавливаемое значение настройки
	const int32_t value = (mode == net::socket_mode_t::ENABLED ? 1 : 0);
	// Уровень настройки и её название
	int32_t level = 0, name = 0;
	/**
	 * Определяем переключаемую настройку
	 */
	switch(option){
		// Если переключается настройка самостоятельной сборки заголовков пакета
		case event::options::HDRINCL: {
			// Если сокет работает по протоколу IPv6
			if(family == event::family_t::IPV6){
				// Выводим в журнал сообщение об отсутствии соответствия
				this->_log->print("%s: IPv6 header inclusion is not supported by MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__);
				// Выводим отрицательный результат переключения
				return false;
			}
			// Устанавливаем уровень настройки
			level = IPPROTO_IP;
			// Устанавливаем название настройки
			name = IP_HDRINCL;
		} break;
		// Если переключается настройка немедленной отправки
		case event::options::TCP_NO_DELAY: {
			// Устанавливаем уровень настройки
			level = IPPROTO_TCP;
			// Устанавливаем название настройки
			name = TCP_NODELAY;
		} break;
		// Если переключается настройка работы одним лишь IPv6
		case event::options::IPV6_ONLY: {
			// Устанавливаем уровень настройки
			level = IPPROTO_IPV6;
			// Устанавливаем название настройки
			name = IPV6_V6ONLY;
		} break;
		// Если переключается настройка широковещательной рассылки
		case event::options::BROADCAST: {
			// Устанавливаем уровень настройки
			level = SOL_SOCKET;
			// Устанавливаем название настройки
			name = SO_BROADCAST;
		} break;
		// Если переключается настройка повторного занятия адреса
		case event::options::REUSE_ADDR: {
			// Устанавливаем уровень настройки
			level = SOL_SOCKET;
			// Устанавливаем название настройки
			name = SO_REUSEADDR;
		} break;
		// Если переключается настройка возврата рассылки самому себе
		case event::options::MULTICAST_LOOPBACK: {
			// Устанавливаем уровень настройки
			level = (family == event::family_t::IPV6 ? IPPROTO_IPV6 : IPPROTO_IP);
			// Устанавливаем название настройки
			name = (family == event::family_t::IPV6 ? IPV6_MULTICAST_LOOP : IP_MULTICAST_LOOP);
		} break;
		/**
		 * Если переключается настройка обрыва связи без доводки
		 *
		 * @note Достигается она выдержкой закрытия нулевой длины: сокет закрывается
		 *       немедленно, недоставленное отбрасывается, а тому концу уходит сброс
		 *       связи взамен обыкновенного её завершения
		 *
		 */
		case event::options::HARD_CLOSE: {
			// Настройки выдержки закрытия сокета
			struct linger value{};
			// Устанавливаем признак применения выдержки
			value.l_onoff = static_cast <u_short> (mode == net::socket_mode_t::ENABLED ? 1 : 0);
			// Устанавливаем длину выдержки
			value.l_linger = 0;
			// Если выставить выдержку закрытия не удалось
			if(::setsockopt(sock, SOL_SOCKET, SO_LINGER, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
				// Выводим в журнал сообщение о невозможности выставления настройки
				this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Выводим отрицательный результат переключения
				return false;
			}
			// Выводим положительный результат переключения
			return true;
		}
		// Если переключается настройка работы сокета без задержек
		case event::options::NO_IO_BLOCK:
		// Если переключается настройка работы сокета с задержками
		case event::options::SM_IO_BLOCK: {
			/**
			 * Настройка эта у MS Windows задаётся не сокету, а его управлению
			 *
			 * @note Значение здесь обратное по смыслу: единица означает работу без
			 *       задержек, оттого настройка задержек и настройка их отсутствия
			 *       разводятся сами собой
			 *
			 */
			u_long value = static_cast <u_long> (option == event::options::NO_IO_BLOCK ? (mode == net::socket_mode_t::ENABLED ? 1 : 0) : (mode == net::socket_mode_t::ENABLED ? 0 : 1));
			// Если выставить настройку не удалось
			if(::ioctlsocket(sock, FIONBIO, &value) != 0){
				// Выводим в журнал сообщение о невозможности выставления настройки
				this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Выводим отрицательный результат переключения
				return false;
			}
			// Выводим положительный результат переключения
			return true;
		}
		/**
		 * Если переключается настройка закрытия сокета при запуске другого приложения
		 *
		 * @note Наследования у MS Windows нет вовсе - взамен него дочернему процессу
		 *       передаётся перечень описателей, помеченных наследуемыми. Снятие
		 *       пометки и есть то самое, чего добиваются закрытием при запуске
		 *
		 */
		case event::options::CLOSE_ON_EXEC: {
			// Если снять либо поставить пометку наследования не удалось
			if(!::SetHandleInformation(reinterpret_cast <HANDLE> (sock), HANDLE_FLAG_INHERIT, (mode == net::socket_mode_t::ENABLED ? 0 : HANDLE_FLAG_INHERIT))){
				// Выводим в журнал сообщение о невозможности выставления настройки
				this->_log->print("%s: handle inheritance could not be changed, error %lu", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::GetLastError());
				// Выводим отрицательный результат переключения
				return false;
			}
			// Выводим положительный результат переключения
			return true;
		}
		// Если переключается заглушение сигнала неверного действия
		case event::options::NO_SIGILL:
		// Если переключается заглушение сигнала обрыва канала
		case event::options::NO_SIGPIPE:
			/**
			 * Сигналов этих сокеты MS Windows не поднимают вовсе
			 *
			 * @note Запрошенное состояние здесь стоит само собой, и отвечать отказом
			 *       на достигнутое было бы неверно
			 *
			 */
			return true;
		// Если переключается настройка придержки отправки
		case event::options::TCP_CORKING: {
			// Выводим в журнал сообщение об отсутствии соответствия
			this->_log->print("%s: TCP output corking has no counterpart on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__);
			// Выводим отрицательный результат переключения
			return false;
		}
		/**
		 * Если переключается настройка повторного занятия порта
		 *
		 * @note Соответствия ей у MS Windows нет: SO_REUSEADDR здесь позволяет занять
		 *       уже занятый порт и без неё, отчего подменять одно другим означало бы
		 *       выдать иное поведение за запрошенное
		 *
		 */
		case event::options::REUSE_PORT: {
			// Выводим в журнал сообщение об отсутствии соответствия
			this->_log->print("%s: port reuse has no counterpart on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__);
			// Выводим отрицательный результат переключения
			return false;
		}
		// Если настройка модулю неизвестна
		default: {
			// Выводим в журнал сообщение о неизвестной настройке
			this->_log->print("%s: socket option %u is not supported", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, static_cast <uint32_t> (option));
			// Выводим отрицательный результат переключения
			return false;
		}
	}
	// Если выставить настройку сокета не удалось
	if(::setsockopt(sock, level, name, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
		// Выводим в журнал сообщение о невозможности выставления настройки
		this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Выводим отрицательный результат переключения
		return false;
	}
	// Выводим положительный результат переключения
	return true;
}
/**
 * @brief Метод входа в группу рассылки либо выхода из неё
 *
 * @param sock   сетевой сокет
 * @param mode   режим входа либо выхода
 * @param group  адрес группы рассылки
 * @param source адрес устройства, каким выполняется вход
 * @return       результат выполнения
 *
 * @note Устройство у IPv6 задаётся не адресом, а номером, и номер этот берётся из
 *       поля зоны переданного адреса. Пустая зона означает выбор устройства самой
 *       системой - тем поведение приводится к общему с эталонными бэкендами виду
 *
 */
bool awh::eth::Socket::membership(const net::socket_t sock, const net::socket_mode_t mode, const net::addr_net_t * group, const net::addr_net_t * source) const noexcept {
	// Если сокет либо адреса не переданы
	if((sock == net::invalid_socket_t) || (group == nullptr) || (source == nullptr)){
		// Выводим в журнал сообщение о непереданных адресах
		this->_log->print("%s: multicast group or source address is not initialized", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__);
		// Выводим отрицательный результат выполнения
		return false;
	}
	// Если виды переданных адресов не совпали
	if(group->size != source->size){
		// Выводим в журнал сообщение о несовпадении видов адресов
		this->_log->print("%s: multicast group and source addresses belong to different families", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__);
		// Выводим отрицательный результат выполнения
		return false;
	}
	// Признак входа в группу рассылки
	const bool join = (mode == net::socket_mode_t::ENABLED);
	/**
	 * Определяем вид переданных адресов
	 */
	switch(group->size){
		// Если адреса являются адресами IPv4
		case 4: {
			// Запрос на вход в группу рассылки
			struct ip_mreq request{};
			// Устанавливаем адрес группы рассылки
			request.imr_multiaddr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (group)->address;
			// Устанавливаем адрес устройства, каким выполняется вход
			request.imr_interface.s_addr = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
			// Если выполнить вход в группу рассылки либо выход из неё не удалось
			if(::setsockopt(sock, IPPROTO_IP, (join ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP), reinterpret_cast <const char *> (&request), static_cast <int32_t> (sizeof(request))) != 0){
				// Выводим в журнал сообщение о невозможности выполнения
				this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Выводим отрицательный результат выполнения
				return false;
			}
			// Выводим положительный результат выполнения
			return true;
		}
		// Если адреса являются адресами IPv6
		case 16: {
			// Запрос на вход в группу рассылки
			struct ipv6_mreq request{};
			// Устанавливаем адрес группы рассылки
			::memcpy(&request.ipv6mr_multiaddr, &awh_cast <const net::addr_net_ipv6_t *> (group)->address[0], sizeof(request.ipv6mr_multiaddr));
			// Устанавливаем номер устройства, каким выполняется вход
			request.ipv6mr_interface = static_cast <ULONG> (awh_cast <const net::addr_net_ipv6_t *> (source)->zone);
			// Если выполнить вход в группу рассылки либо выход из неё не удалось
			if(::setsockopt(sock, IPPROTO_IPV6, (join ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP), reinterpret_cast <const char *> (&request), static_cast <int32_t> (sizeof(request))) != 0){
				// Выводим в журнал сообщение о невозможности выполнения
				this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Выводим отрицательный результат выполнения
				return false;
			}
			// Выводим положительный результат выполнения
			return true;
		}
	}
	// Выводим в журнал сообщение о неподдерживаемом виде адресов
	this->_log->print("%s: only IPv4 and IPv6 multicast groups are supported", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__);
	// Выводим отрицательный результат выполнения
	return false;
}
