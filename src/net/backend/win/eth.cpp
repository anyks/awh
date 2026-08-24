/**
 * @file eth.cpp
 * @date 2026-08-05
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация системного сетевого слоя для MS Windows —
 *        разбор адресов, подсетей и контрольных сумм
 *
 * @details Слой этот лежит ниже отдельного подключения и касается машины целиком.
 *          Поначалу он был собран здесь целиком, поскольку тел почти не имел; сейчас
 *          разложен по образцу эталонных бэкендов, и в этом файле остался лишь
 *          `Network_Address`: принадлежность подсети, контрольные суммы заголовков,
 *          сличение префиксов IPv6, привязка шлюза и защита от одновременного доступа
 *
 *          Прочее разошлось по своим файлам каталога: `socket.cpp`, `iface.cpp`,
 *          `gateway.cpp`, `tunnel.cpp`, `qos.cpp` и `addr.cpp`
 *
 * @note Заглушек в слое НЕ ОСТАЛОСЬ: метка `@todo Windows`, которой они помечались,
 *       не стоит больше ни у одного метода, и поиск по ней выдаёт лишь это описание.
 *       Тела опираются на GetAdaptersAddresses для устройств, GetIpForwardTable2 для
 *       маршрутов и на те же setsockopt/getsockopt, что и прочие системы, с иным
 *       набором имён опций
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <mutex>
#include <memory>

/**
 * Подключаем единую точку подключения системных заголовков MS Windows
 */
#include <sys/win32.hpp>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/eth/eth.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Название бэкенда для записей в журнале
 *
 */
static constexpr const char * __AWH_ETH_BACKEND__ = "MS Windows ETH backend";

/**
 * @brief Инкапсулируем состояние слоя в пространство имён
 *
 */
namespace {
	// Режим безопасности работы потоков
	awh::event::mode_t __awh_thread_safety__ = awh::event::mode_t::DISABLED;

	/**
	 * @brief Заголовок сегмента TCP в наименьшем виде
	 *
	 * @details Объявляется он здесь оттого, что заголовков «netinet/tcp.h» и
	 *          «netinet/udp.h» у MS Windows нет вовсе, а нужно от них одно - смещение
	 *          поля контрольной суммы, заданное стандартом (RFC 9293 §3.1 для TCP,
	 *          RFC 768 для UDP) и от системы не зависящее
	 *
	 * @note Объявление помечено плотным размещением: без того выравнивание развело бы
	 *       поля и сместило искомое
	 *
	 */
	#pragma pack(push, 1)
		struct tcp_hdr_min {
			uint16_t sport;  // Порт источника
			uint16_t dport;  // Порт назначения
			uint32_t seq;    // Порядковый номер
			uint32_t ack;    // Номер подтверждения
			uint8_t  offset; // Смещение данных и зарезервированные разряды
			uint8_t  flags;  // Разряды управления
			uint16_t window; // Размер окна
			uint16_t sum;    // Контрольная сумма
			uint16_t urgent; // Указатель важности
		};
		/**
		 * @brief Заголовок датаграммы UDP
		 *
		 */
		struct udp_hdr_min {
			uint16_t sport;  // Порт источника
			uint16_t dport;  // Порт назначения
			uint16_t length; // Длина датаграммы
			uint16_t sum;    // Контрольная сумма
		};
	#pragma pack(pop)

	/**
	 * @brief Функция вычисления контрольной суммы по правилу сети интернет
	 *
	 * @param data   указатель на данные
	 * @param length длина данных
	 * @return       инвертированная контрольная сумма
	 *
	 * @note Правило это описано RFC 1071 и от системы не зависит вовсе
	 *
	 */
	uint16_t checksum16(const void * data, size_t length) noexcept {
		// Получаем нужного вида буфер входящих данных
		const uint16_t * buffer = reinterpret_cast <const uint16_t *> (data);
		// Инициализируем сумму
		uint32_t sum = 0;
		/**
		 * Пока есть данные для обработки
		 */
		while(length > 1){
			// Добавляем к сумме очередные два байта данных
			sum += (* buffer++);
			// Уменьшаем длину данных на два байта
			length -= 2;
		}
		// Если остался один байт данных
		if(length == 1)
			// Добавляем к сумме последний байт данных
			sum += (* reinterpret_cast <const uint8_t *> (buffer));
		/**
		 * Складываем старшие 16 бит суммы с младшими 16 битами суммы
		 */
		while(sum >> 16)
			// Складываем старшие 16 бит суммы с младшими 16 битами суммы
			sum = ((sum & 0xFFFF) + (sum >> 16));
		// Возвращаем инвертированную сумму
		return static_cast <uint16_t> (~sum);
	}
};




/**
 * @brief Метод проверки принадлежности IP-адреса подсети
 *
 * @param ip     проверяемый IP-адрес в хостовом порядке
 * @param net    сетевой адрес подсети в хостовом порядке
 * @param prefix префикс подсети
 * @return       результат проверки
 *
 *
 * @note Вычисление это от системы не зависит вовсе, оттого и повторяет ответ прочих
 *       систем в точности
 *
 */
bool awh::eth::Network_Address::isInSubnet(const uint32_t ip, const uint32_t net, const uint8_t prefix) const noexcept {
	// Если префикс равен нулю, то любой IP-адрес принадлежит подсети
	if(prefix == 0)
		// Возвращаем результат проверки
		return true;
	// Вычисляем маску подсети
	const uint32_t mask = (~((1U << (32 - prefix)) - 1));
	// Проверяем принадлежность IP-адреса подсети
	return ((ip & mask) == (net & mask));
}

/**
 * @brief Метод вычисления контрольной суммы транспортного уровня
 *
 * @param family    семейство протоколов (IPv4 или IPv6)
 * @param protocol  протокол транспортного уровня
 * @param src       указатель на источник данных
 * @param dst       указатель на приёмник данных
 * @param transport указатель на данные транспортного уровня
 * @param length    длина данных транспортного уровня
 * @return          вычисленная контрольная сумма
 *
 *
 * @note Вычисление это от системы не зависит вовсе - псевдозаголовок предписан
 *       RFC 9293 §3.1 для TCP и RFC 768 для UDP, - оттого и повторяет ответ прочих
 *       систем в точности. Разнятся лишь объявления заголовков транспортного уровня,
 *       каких у MS Windows нет: они заведены выше в наименьшем виде
 *
 */
uint16_t awh::eth::Network_Address::checksum(const event::family_t family, const event::protocol_t protocol, const void * src, const void * dst, const void * transport, const size_t length) const noexcept {
	// Переменная результата
	uint16_t result = 0;
	// Если входные данные негодны, выводим пустой результат
	if((src == nullptr) || (dst == nullptr) || (transport == nullptr) || (length == 0))
		// Выводим пустой результат
		return result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Общий размер данных
		size_t totalSize = 0;
		// Размер псевдозаголовка
		size_t pseudoSize = 0;
		// Смещение поля контрольной суммы в транспортном заголовке
		size_t checksumOffset = 0;
		// Псевдозаголовок
		unique_ptr <uint8_t []> pseudo = nullptr;
		/**
		 * Определяем протокол транспортного уровня
		 */
		switch(static_cast <uint8_t> (protocol)){
			// Если протокол определён как TCP
			case static_cast <uint8_t> (event::protocol_t::TCP):
				// Запоминаем смещение поля контрольной суммы в заголовке TCP
				checksumOffset = offsetof(struct tcp_hdr_min, sum);
			break;
			// Если протокол определён как UDP
			case static_cast <uint8_t> (event::protocol_t::UDP):
				// Запоминаем смещение поля контрольной суммы в заголовке UDP
				checksumOffset = offsetof(struct udp_hdr_min, sum);
			break;
			// Для неподдерживаемого протокола
			default: {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Unsupported protocol for checksum calculation", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (protocol), src, dst, transport, length), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Unsupported protocol for checksum calculation", log_t::flag_t::CRITICAL);
				#endif
				// Выводим пустой результат
				return result;
			}
		}
		/**
		 * Определяем семейство адресов
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				/**
				 * @brief Структура псевдозаголовка IPv4
				 *
				 */
				struct {
					uint32_t src;    // IP-адрес источника
					uint32_t dst;    // IP-адрес назначения
					uint8_t zero;    // Зарезервировано, должно быть равно нулю
					uint8_t proto;   // Протокол транспортного уровня
					uint16_t length; // Длина транспортного уровня
				} hdr;
				// Устанавливаем ноль в зарезервированное поле
				hdr.zero = 0;
				// Устанавливаем протокол транспортного уровня
				hdr.proto = ((static_cast <uint8_t> (protocol) == static_cast <uint8_t> (event::protocol_t::TCP)) ? IPPROTO_TCP : IPPROTO_UDP);
				// Устанавливаем IP-адрес источника
				hdr.src = (* reinterpret_cast <const uint32_t *> (src));
				// Устанавливаем IP-адрес назначения
				hdr.dst = (* reinterpret_cast <const uint32_t *> (dst));
				// Устанавливаем длину транспортного уровня
				hdr.length = htons(static_cast <uint16_t> (length));
				// Вычисляем размер псевдозаголовка
				pseudoSize = sizeof(hdr);
				// Вычисляем общий размер данных
				totalSize = (pseudoSize + length);
				// Выделяем память под псевдозаголовок
				pseudo = make_unique <uint8_t []> (totalSize);
				// Формируем псевдозаголовок
				::memcpy(pseudo.get(), &hdr, pseudoSize);
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				/**
				 * @brief Структура псевдозаголовка IPv6
				 *
				 */
				struct {
					struct in6_addr src; // IP-адрес источника
					struct in6_addr dst; // IP-адрес назначения
					uint32_t length;     // Длина транспортного уровня
					uint8_t zero[3];     // Зарезервировано, должно быть равно нулю
					uint8_t next_hdr;    // Следующий заголовок
				} hdr;
				// Устанавливаем протокол транспортного уровня
				hdr.next_hdr = ((static_cast <uint8_t> (protocol) == static_cast <uint8_t> (event::protocol_t::TCP)) ? IPPROTO_TCP : IPPROTO_UDP);
				// Устанавливаем нули в зарезервированное поле
				hdr.zero[0] = hdr.zero[1] = hdr.zero[2] = 0;
				// Устанавливаем IP-адрес источника
				hdr.src = (* reinterpret_cast <const struct in6_addr *> (src));
				// Устанавливаем IP-адрес назначения
				hdr.dst = (* reinterpret_cast <const struct in6_addr *> (dst));
				// Устанавливаем длину транспортного уровня
				hdr.length = htonl(static_cast <uint32_t> (length));
				// Вычисляем размер псевдозаголовка
				pseudoSize = sizeof(hdr);
				// Вычисляем общий размер данных
				totalSize = (pseudoSize + length);
				// Выделяем память под псевдозаголовок
				pseudo = make_unique <uint8_t []> (totalSize);
				// Формируем псевдозаголовок
				::memcpy(pseudo.get(), &hdr, pseudoSize);
			} break;
		}
		// Если семейство адресов не поддержано, псевдозаголовок не сформирован
		if(pseudo == nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Unsupported address family for checksum calculation", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (protocol), src, dst, transport, length), log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Unsupported address family for checksum calculation", log_t::flag_t::CRITICAL);
			#endif
			// Выводим пустой результат
			return result;
		}
		// Копируем транспортный заголовок вместе с данными
		::memcpy(pseudo.get() + pseudoSize, transport, length);
		/**
		 * Поле контрольной суммы гасится в копии, а не в поданном буфере: вызывающий
		 * отдаёт его на чтение и вправе ждать, что тот останется нетронутым
		 */
		if((checksumOffset + sizeof(uint16_t)) <= length)
			// Зануляем поле контрольной суммы в копии
			::memset(pseudo.get() + pseudoSize + checksumOffset, 0, sizeof(uint16_t));
		// Вычисляем контрольную сумму
		result = checksum16(pseudo.get(), totalSize);
		// Для UDP нулевая контрольная сумма передаётся как 0xFFFF (RFC 768)
		if((static_cast <uint8_t> (protocol) == static_cast <uint8_t> (event::protocol_t::UDP)) && (result == 0))
			// Корректируем нулевую контрольную сумму UDP
			result = 0xFFFF;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), static_cast <uint16_t> (protocol), src, dst, transport, length), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}

/**
 * @brief Метод сравнения двух адресов IPv6 по префиксу
 *
 * @param first  первый адрес IPv6
 * @param second второй адрес IPv6
 * @param length длина префикса в разрядах
 * @return       результат сравнения
 *
 * @note Вычисление это от системы не зависит вовсе, оттого и повторяет ответ прочих
 *       систем в точности
 *
 */
bool awh::eth::Network_Address::ipv6PrefixEqual(const uint8_t * first, const uint8_t * second, const uint8_t length) const noexcept {
	// Если длина префикса равна нулю, адреса считаются равными
	if(length == 0)
		// Возвращаем результат сравнения
		return true;
	// Вычисляем количество полных байтов префикса
	const size_t fullBytes = (length / 8);
	// Вычисляем количество разрядов в последнем байте префикса
	const uint8_t bitsInLast = (length % 8);
	// Сравниваем полные байты префикса
	if(::memcmp(first, second, fullBytes) != 0)
		// Возвращаем результат сравнения
		return false;
	// Если разрядов в последнем байте нет, адреса равны
	if(bitsInLast == 0)
		// Возвращаем результат сравнения
		return true;
	// Формируем маску разрядов последнего байта префикса
	const uint8_t mask = ((0xFF << (8 - bitsInLast)) & 0xFF);
	// Возвращаем результат сравнения
	return ((first[fullBytes] & mask) == (second[fullBytes] & mask));
}











































/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::eth::Network_Address::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
}


/**
 * @brief Метод установки объекта управления шлюзами
 *
 * @param gateway объект управления шлюзами для установки
 *
 */
void awh::eth::Network_Address::gateway(const Gateway * gateway) noexcept {
	// Выполняем установку объекта управления шлюзами
	this->_gateway = gateway;
}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 *
 */
awh::eth::Network_Address::Network_Address(const fmk_t * fmk, const log_t * log) noexcept :
 _iface(fmk, log), _gateway(nullptr), _fmk(fmk), _log(log) {}

/**
 * @brief Деструктор
 *
 */
awh::eth::Network_Address::~Network_Address() noexcept {}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 *
 */
awh::eth::Interface::Interface(const fmk_t * fmk, const log_t * log) noexcept :
 _fmk(fmk), _log(log), _driver(driver_t::AUTO) {}

/**
 * @brief Деструктор
 *
 */
awh::eth::Interface::~Interface() noexcept {}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 *
 */
awh::eth::Socket::Socket(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}

/**
 * @brief Деструктор
 *
 */
awh::eth::Socket::~Socket() noexcept {}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 *
 */
/**
 * @brief Конструктор записи маршрута
 *
 * @note Тело у него общее со всеми системами: поля задаются пустыми, и системного в
 *       том нет ничего. Лежит оно здесь лишь оттого, что слой этот заменяет собой весь
 *       backend/bsd целиком, а не отдельные его части
 *
 */
awh::eth::Gateway::Route::Route() noexcept :
 ifname{""}, prefix(0),
 gateway(nullptr), destination(nullptr) {}

awh::eth::Gateway::Gateway(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}

/**
 * @brief Деструктор
 *
 */
awh::eth::Gateway::~Gateway() noexcept {}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 *
 * @note Средств протокола с управлением потоком у MS Windows нет вовсе, поэтому
 *       объекта sctp здесь, в отличие от FreeBSD, не заводится
 *
 */
awh::Ethernet::Ethernet(const fmk_t * fmk, const log_t * log) noexcept :
 addr(fmk, log), iface(fmk, log), socket(fmk, log),
 gateway(fmk, log), _fmk(fmk), _log(log) {
	/**
	 * Связываем объект работы с адресами с объектом управления шлюзами: исходящий
	 * адрес определяется подбором маршрута, а подбор ведёт объект шлюзов
	 */
	this->addr.gateway(&this->gateway);
}

/**
 * @brief Деструктор
 *
 */
awh::Ethernet::~Ethernet() noexcept {}
