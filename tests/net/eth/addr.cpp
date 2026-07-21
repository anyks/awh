/**
 * @file: addr.cpp
 * @date: 2026-02-06
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем системные заголовочные файлы
 */
#include <vector>
#include <cstddef>
#include <cstring>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Инкапсулируем вспомогательные функции тестов в анонимное пространство имён
 *
 */
namespace {
	/**
	 * @brief Эталонное вычисление контрольной суммы (стандартный «интернет-чексум»)
	 *
	 * @param data   указатель на данные
	 * @param length длина данных
	 * @return       инвертированная контрольная сумма
	 */
	uint16_t sum16(const uint8_t * data, size_t length) noexcept {
		// Получаем нужного вида буфер входящих данных
		const uint16_t * buffer = reinterpret_cast <const uint16_t *> (data);
		// Инициализируем сумму
		uint32_t sum = 0;
		/**
		 *  Пока есть данные для обработки
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
		 *  Складываем старшие 16 бит суммы с младшими 16 битами суммы
		 */
		while(sum >> 16)
			// Выполняем свёртку суммы
			sum = ((sum & 0xFFFF) + (sum >> 16));
		// Возвращаем инвертированную сумму
		return static_cast <uint16_t> (~sum);
	}
	/**
	 * @brief Эталонное вычисление контрольной суммы транспортного уровня
	 *
	 * @param family    семейство протоколов (IPv4 или IPv6)
	 * @param protocol  протокол транспортного уровня
	 * @param src       указатель на источник данных
	 * @param dst       указатель на приёмник данных
	 * @param transport указатель на данные транспортного уровня
	 * @param length    длина данных транспортного уровня
	 * @param applyUdp  применять ли правило UDP (нулевая сумма → 0xFFFF)
	 * @return          эталонная контрольная сумма
	 */
	uint16_t refChecksum(const awh::event::family_t family, const awh::event::protocol_t protocol, const void * src, const void * dst, const void * transport, const size_t length, const bool applyUdp = true) noexcept {
		// Смещение поля контрольной суммы в транспортном заголовке
		size_t checksumOffset = 0;
		/**
		 * Определяем смещение поля контрольной суммы
		 */
		switch(static_cast <uint8_t> (protocol)){
			// Если протокол определён как TCP
			case static_cast <uint8_t> (awh::event::protocol_t::TCP):
				checksumOffset = offsetof(struct tcphdr, th_sum);
			break;
			// Если протокол определён как UDP
			case static_cast <uint8_t> (awh::event::protocol_t::UDP):
				checksumOffset = offsetof(struct udphdr, uh_sum);
			break;
			// Для неподдерживаемого протокола
			default: return 0;
		}
		// Буфер псевдозаголовка + транспортных данных
		std::vector <uint8_t> buffer;
		// Размер псевдозаголовка
		size_t pseudoSize = 0;
		/**
		 * Определяем семейство протоколов
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (awh::event::family_t::IPV4): {
				// Псевдозаголовок IPv4
				struct {
					uint32_t src;
					uint32_t dst;
					uint8_t zero;
					uint8_t proto;
					uint16_t length;
				} hdr;
				// Заполняем псевдозаголовок
				hdr.zero = 0;
				hdr.proto = ((protocol == awh::event::protocol_t::TCP) ? IPPROTO_TCP : IPPROTO_UDP);
				hdr.src = (* reinterpret_cast <const uint32_t *> (src));
				hdr.dst = (* reinterpret_cast <const uint32_t *> (dst));
				hdr.length = htons(static_cast <uint16_t> (length));
				// Вычисляем размер псевдозаголовка
				pseudoSize = sizeof(hdr);
				// Резервируем память под весь буфер
				buffer.resize(pseudoSize + length);
				// Копируем псевдозаголовок
				::memcpy(buffer.data(), &hdr, pseudoSize);
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (awh::event::family_t::IPV6): {
				// Псевдозаголовок IPv6
				struct {
					struct in6_addr src;
					struct in6_addr dst;
					uint32_t length;
					uint8_t zero[3];
					uint8_t next_hdr;
				} hdr;
				// Заполняем псевдозаголовок
				hdr.zero[0] = hdr.zero[1] = hdr.zero[2] = 0;
				hdr.next_hdr = ((protocol == awh::event::protocol_t::TCP) ? IPPROTO_TCP : IPPROTO_UDP);
				hdr.src = (* reinterpret_cast <const struct in6_addr *> (src));
				hdr.dst = (* reinterpret_cast <const struct in6_addr *> (dst));
				hdr.length = htonl(static_cast <uint32_t> (length));
				// Вычисляем размер псевдозаголовка
				pseudoSize = sizeof(hdr);
				// Резервируем память под весь буфер
				buffer.resize(pseudoSize + length);
				// Копируем псевдозаголовок
				::memcpy(buffer.data(), &hdr, pseudoSize);
			} break;
			// Для неподдерживаемого семейства
			default: return 0;
		}
		// Копируем транспортный заголовок + данные
		::memcpy(buffer.data() + pseudoSize, transport, length);
		// Обнуляем контрольную сумму в копии транспортного заголовка
		if((checksumOffset + sizeof(uint16_t)) <= length)
			// Зануляем поле контрольной суммы
			::memset(buffer.data() + pseudoSize + checksumOffset, 0, sizeof(uint16_t));
		// Вычисляем контрольную сумму
		uint16_t result = sum16(buffer.data(), buffer.size());
		// Для UDP нулевая контрольная сумма передаётся как 0xFFFF (RFC 768)
		if(applyUdp && (protocol == awh::event::protocol_t::UDP) && (result == 0))
			// Корректируем нулевую контрольную сумму UDP
			result = 0xFFFF;
		// Возвращаем результат
		return result;
	}
}

/**
 * ==========================================================================
 * Тесты метода isInSubnet
 * ==========================================================================
 */

/**
 * @brief Тест проверки принадлежности к подсети
 *
 */
TEST_F(EthFixture, AddressIsInSubnetTest){
	// 192.168.1.10
	uint32_t ip = 0xC0A8010A; 
	// 192.168.1.0
	uint32_t net = 0xC0A80100;
	// /24
	uint8_t prefix = 24;

	// Выполняем проверку принадлежности IP-адреса подсети
	ASSERT_TRUE(this->_eth->addr.isInSubnet(ip, net, prefix));

	// Неверная подсеть 192.168.2.0
	uint32_t net2 = 0xC0A80200;
	// Если адрес соответствует подсети
	ASSERT_FALSE(this->_eth->addr.isInSubnet(ip, net2, prefix));
}

/**
 * @brief Тест проверки принадлежности к подсети с нулевым префиксом
 *
 */
TEST_F(EthFixture, AddressIsInSubnetPrefixZeroTest){
	// При нулевом префиксе любой IP-адрес принадлежит подсети
	ASSERT_TRUE(this->_eth->addr.isInSubnet(0xC0A8010A, 0x0A000001, 0));
	// Проверяем для совершенно произвольных адресов
	ASSERT_TRUE(this->_eth->addr.isInSubnet(0xFFFFFFFF, 0x00000000, 0));
}

/**
 * @brief Тест проверки принадлежности к подсети с префиксом /32
 *
 */
TEST_F(EthFixture, AddressIsInSubnetPrefix32Test){
	// При префиксе /32 адрес должен совпадать полностью
	ASSERT_TRUE(this->_eth->addr.isInSubnet(0xC0A8010A, 0xC0A8010A, 32));
	// Если адрес отличается хотя бы на единицу
	ASSERT_FALSE(this->_eth->addr.isInSubnet(0xC0A8010A, 0xC0A8010B, 32));
}

/**
 * ==========================================================================
 * Тесты метода ipv6PrefixEqual
 * ==========================================================================
 */

/**
 * @brief Тест сравнения префиксов IPv6
 *
 */
TEST_F(EthFixture, AddressIpv6PrefixEqualTest){
	// Создаём два IPv6-адреса
	uint8_t a[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
	uint8_t b[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};

	// /64 совпадают
	ASSERT_TRUE(this->_eth->addr.ipv6PrefixEqual(a, b, 64));

	// /128 не совпадают
	ASSERT_FALSE(this->_eth->addr.ipv6PrefixEqual(a, b, 128));
}

/**
 * @brief Тест сравнения префиксов IPv6 с нулевой длиной
 *
 */
TEST_F(EthFixture, AddressIpv6PrefixEqualZeroTest){
	// Создаём два полностью различных IPv6-адреса
	uint8_t a[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
	uint8_t b[16] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
	// При нулевой длине префикса адреса считаются равными
	ASSERT_TRUE(this->_eth->addr.ipv6PrefixEqual(a, b, 0));
}

/**
 * @brief Тест сравнения префиксов IPv6 с неполным байтом
 *
 */
TEST_F(EthFixture, AddressIpv6PrefixEqualPartialBitTest){
	// Создаём базовый IPv6-адрес
	uint8_t base[16] = {0x20, 0x01, 0x0d, 0xb8, 0xF0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	{
		// Адрес отличается в младших битах 5-го байта (0xF0 -> 0xF8)
		uint8_t other[16];
		::memcpy(other, base, 16);
		// Меняем младшие биты последнего значимого байта
		other[4] = 0xF8;
		// Префикс /36 (4 полных байта + 4 старших бита) — старшие 4 бита 0xF совпадают
		ASSERT_TRUE(this->_eth->addr.ipv6PrefixEqual(base, other, 36));
		// Префикс /37 — 5-й бит отличается (0xF0 против 0xF8)
		ASSERT_FALSE(this->_eth->addr.ipv6PrefixEqual(base, other, 37));
	}{
		// Адрес отличается в третьем бите первого байта (0x20 -> 0x00, бит со значением 0x20)
		uint8_t other[16];
		::memcpy(other, base, 16);
		// Обнуляем первый байт (0x20 -> 0x00): первые два бита (00) совпадают, третий бит отличается
		other[0] = 0x00;
		// Префикс /2 — первые два бита (00) совпадают
		ASSERT_TRUE(this->_eth->addr.ipv6PrefixEqual(base, other, 2));
		// Префикс /3 — третий бит отличается (001 против 000)
		ASSERT_FALSE(this->_eth->addr.ipv6PrefixEqual(base, other, 3));
	}
}

/**
 * ==========================================================================
 * Тесты метода checksum
 * ==========================================================================
 */

/**
 * @brief Тест вычисления контрольной суммы с некорректными аргументами
 *
 */
TEST_F(EthFixture, AddressChecksumInvalidArgsTest){
	// Полезная нагрузка транспортного уровня
	uint8_t data[20] = {0};
	// IP-адреса источника и назначения
	uint32_t src = htonl(0x0A000001);
	uint32_t dst = htonl(0x0A000002);
	// Нулевые указатели должны давать нулевую контрольную сумму
	ASSERT_EQ(0, this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, nullptr, nullptr, nullptr, 0));
	// Если транспортные данные отсутствуют
	ASSERT_EQ(0, this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, &src, &dst, nullptr, 10));
	// Если длина данных нулевая
	ASSERT_EQ(0, this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, &src, &dst, data, 0));
}

/**
 * @brief Тест вычисления контрольной суммы с неподдерживаемым протоколом
 *
 */
TEST_F(EthFixture, AddressChecksumUnsupportedProtocolTest){
	// Полезная нагрузка транспортного уровня
	uint8_t data[20] = {0};
	// IP-адреса источника и назначения
	uint32_t src = htonl(0x0A000001);
	uint32_t dst = htonl(0x0A000002);
	// Для неподдерживаемого протокола контрольная сумма должна быть нулевой
	ASSERT_EQ(0, this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::ICMP, &src, &dst, data, sizeof(data)));
	// Для RAW-протокола результат также нулевой
	ASSERT_EQ(0, this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::RAW, &src, &dst, data, sizeof(data)));
}

/**
 * @brief Тест вычисления контрольной суммы с неподдерживаемым семейством
 *
 */
TEST_F(EthFixture, AddressChecksumUnsupportedFamilyTest){
	// Полезная нагрузка транспортного уровня
	uint8_t data[20] = {0};
	// IP-адреса источника и назначения
	uint32_t src = htonl(0x0A000001);
	uint32_t dst = htonl(0x0A000002);
	// Для неподдерживаемого семейства контрольная сумма должна быть нулевой (без падения)
	ASSERT_EQ(0, this->_eth->addr.checksum(awh::event::family_t::NONE, awh::event::protocol_t::TCP, &src, &dst, data, sizeof(data)));
}

/**
 * @brief Тест вычисления контрольной суммы для IPv4/TCP
 *
 */
TEST_F(EthFixture, AddressChecksumIPv4TcpTest){
	// IP-адреса источника и назначения в сетевом порядке байт
	uint32_t src = 0, dst = 0;
	::inet_pton(AF_INET, "192.168.1.10", &src);
	::inet_pton(AF_INET, "192.168.1.20", &dst);
	// Транспортный сегмент TCP (заголовок 20 байт + полезная нагрузка)
	std::vector <uint8_t> data(20 + 16);
	/**
	 * Заполняем сегмент произвольными данными
	 */
	for(size_t i = 0; i < data.size(); ++i)
		data[i] = static_cast <uint8_t> (i + 1);
	// Записываем заведомо ненулевое значение в поле контрольной суммы
	data[offsetof(struct tcphdr, th_sum)] = 0xAB;
	data[offsetof(struct tcphdr, th_sum) + 1] = 0xCD;
	// Сравниваем результат с эталоном
	ASSERT_EQ(
		refChecksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, &src, &dst, data.data(), data.size()),
		this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, &src, &dst, data.data(), data.size())
	);
}

/**
 * @brief Тест вычисления контрольной суммы для IPv4/UDP
 *
 */
TEST_F(EthFixture, AddressChecksumIPv4UdpTest){
	// IP-адреса источника и назначения в сетевом порядке байт
	uint32_t src = 0, dst = 0;
	::inet_pton(AF_INET, "10.0.0.1", &src);
	::inet_pton(AF_INET, "10.0.0.2", &dst);
	// Транспортная датаграмма UDP (заголовок 8 байт + полезная нагрузка)
	std::vector <uint8_t> data(8 + 13);
	/**
	 * Заполняем датаграмму произвольными данными
	 */
	for(size_t i = 0; i < data.size(); ++i)
		data[i] = static_cast <uint8_t> (0xFF - i);
	// Сравниваем результат с эталоном
	ASSERT_EQ(
		refChecksum(awh::event::family_t::IPV4, awh::event::protocol_t::UDP, &src, &dst, data.data(), data.size()),
		this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::UDP, &src, &dst, data.data(), data.size())
	);
}

/**
 * @brief Тест вычисления контрольной суммы для IPv6/TCP
 *
 */
TEST_F(EthFixture, AddressChecksumIPv6TcpTest){
	// IP-адреса источника и назначения
	struct in6_addr src, dst;
	::inet_pton(AF_INET6, "2001:db8::1", &src);
	::inet_pton(AF_INET6, "2001:db8::2", &dst);
	// Транспортный сегмент TCP
	std::vector <uint8_t> data(20 + 32);
	/**
	 * Заполняем сегмент произвольными данными
	 */
	for(size_t i = 0; i < data.size(); ++i)
		data[i] = static_cast <uint8_t> ((i * 7) & 0xFF);
	// Сравниваем результат с эталоном
	ASSERT_EQ(
		refChecksum(awh::event::family_t::IPV6, awh::event::protocol_t::TCP, &src, &dst, data.data(), data.size()),
		this->_eth->addr.checksum(awh::event::family_t::IPV6, awh::event::protocol_t::TCP, &src, &dst, data.data(), data.size())
	);
}

/**
 * @brief Тест вычисления контрольной суммы для IPv6/UDP
 *
 */
TEST_F(EthFixture, AddressChecksumIPv6UdpTest){
	// IP-адреса источника и назначения
	struct in6_addr src, dst;
	::inet_pton(AF_INET6, "fe80::1", &src);
	::inet_pton(AF_INET6, "fe80::2", &dst);
	// Транспортная датаграмма UDP
	std::vector <uint8_t> data(8 + 20);
	/**
	 * Заполняем датаграмму произвольными данными
	 */
	for(size_t i = 0; i < data.size(); ++i)
		data[i] = static_cast <uint8_t> ((i * 3 + 5) & 0xFF);
	// Сравниваем результат с эталоном
	ASSERT_EQ(
		refChecksum(awh::event::family_t::IPV6, awh::event::protocol_t::UDP, &src, &dst, data.data(), data.size()),
		this->_eth->addr.checksum(awh::event::family_t::IPV6, awh::event::protocol_t::UDP, &src, &dst, data.data(), data.size())
	);
}

/**
 * @brief Тест вычисления контрольной суммы для данных нечётной длины
 *
 */
TEST_F(EthFixture, AddressChecksumOddLengthTest){
	// IP-адреса источника и назначения
	uint32_t src = 0, dst = 0;
	::inet_pton(AF_INET, "172.16.0.1", &src);
	::inet_pton(AF_INET, "172.16.0.2", &dst);
	// Транспортные данные нечётной длины (заголовок 20 байт + 7 байт полезной нагрузки)
	std::vector <uint8_t> data(20 + 7);
	/**
	 * Заполняем данные произвольными значениями
	 */
	for(size_t i = 0; i < data.size(); ++i)
		data[i] = static_cast <uint8_t> (0x10 + i);
	// Сравниваем результат с эталоном
	ASSERT_EQ(
		refChecksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, &src, &dst, data.data(), data.size()),
		this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, &src, &dst, data.data(), data.size())
	);
}

/**
 * @brief Тест детерминированности и неизменности входного буфера
 *
 */
TEST_F(EthFixture, AddressChecksumDeterministicTest){
	// IP-адреса источника и назначения
	uint32_t src = 0, dst = 0;
	::inet_pton(AF_INET, "192.0.2.1", &src);
	::inet_pton(AF_INET, "192.0.2.2", &dst);
	// Транспортный сегмент TCP с ненулевым полем контрольной суммы
	std::vector <uint8_t> data(20 + 8);
	/**
	 * Заполняем сегмент произвольными данными
	 */
	for(size_t i = 0; i < data.size(); ++i)
		data[i] = static_cast <uint8_t> (i + 0x20);
	// Сохраняем исходную копию буфера
	std::vector <uint8_t> original = data;
	// Вычисляем контрольную сумму
	uint16_t first = this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, &src, &dst, data.data(), data.size());
	// Проверяем, что исходный буфер не был модифицирован (регрессия на правку const-буфера)
	ASSERT_EQ(0, ::memcmp(data.data(), original.data(), data.size()));
	// Вычисляем контрольную сумму повторно
	uint16_t second = this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, &src, &dst, data.data(), data.size());
	// Результат должен быть детерминированным
	ASSERT_EQ(first, second);
}

/**
 * @brief Тест правила UDP: нулевая контрольная сумма передаётся как 0xFFFF
 *
 */
TEST_F(EthFixture, AddressChecksumUdpZeroIsFFFFTest){
	// IP-адреса источника и назначения
	uint32_t src = 0, dst = 0;
	// Признак того, что подходящие данные найдены
	bool found = false;
	/**
	 * Перебираем все варианты двухбайтовой полезной нагрузки
	 */
	for(uint32_t value = 0; value <= 0xFFFF; ++value){
		// Формируем двухбайтовую полезную нагрузку
		uint16_t payload = static_cast <uint16_t> (value);
		// Вычисляем эталонную «сырую» контрольную сумму (без правила UDP)
		uint16_t raw = refChecksum(awh::event::family_t::IPV4, awh::event::protocol_t::UDP, &src, &dst, &payload, sizeof(payload), false);
		// Если «сырая» контрольная сумма равна нулю
		if(raw == 0){
			// Для UDP метод должен вернуть 0xFFFF вместо нуля
			ASSERT_EQ(0xFFFF, this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::UDP, &src, &dst, &payload, sizeof(payload)));
			// Помечаем, что подходящие данные найдены
			found = true;
			// Прерываем перебор
			break;
		}
	}
	// Убеждаемся, что подходящие данные были найдены
	ASSERT_TRUE(found);

	// Дополнительно убеждаемся, что для TCP правило 0xFFFF не применяется
	found = false;
	/**
	 * Перебираем все варианты двухбайтовой полезной нагрузки
	 */
	for(uint32_t value = 0; value <= 0xFFFF; ++value){
		// Формируем двухбайтовую полезную нагрузку
		uint16_t payload = static_cast <uint16_t> (value);
		// Вычисляем эталонную контрольную сумму для TCP
		uint16_t raw = refChecksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, &src, &dst, &payload, sizeof(payload), false);
		// Если контрольная сумма равна нулю
		if(raw == 0){
			// Для TCP метод должен вернуть ноль (правило UDP не применяется)
			ASSERT_EQ(0, this->_eth->addr.checksum(awh::event::family_t::IPV4, awh::event::protocol_t::TCP, &src, &dst, &payload, sizeof(payload)));
			// Помечаем, что подходящие данные найдены
			found = true;
			// Прерываем перебор
			break;
		}
	}
	// Убеждаемся, что подходящие данные были найдены
	ASSERT_TRUE(found);
}

/**
 * ==========================================================================
 * Тесты метода fillSource
 * ==========================================================================
 */

/**
 * @brief Тест заполнения источника сетевых адресов
 *
 */
TEST_F(EthFixture, AddressFillSourceTest){
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем извлечение сетевых параметров
	this->_eth->addr.fillSource(source);
	// Проверяем, что название сетевого интерфейса получено (или хотя бы не упало, может быть пустым если сети нет)
	ASSERT_FALSE(source.iface.empty()); // Необходимо, чтобы хотя бы один сетевой интерфейс имел выход в интернет
}

/**
 * @brief Тест заполнения источника сетевых адресов по имени интерфейса
 *
 */
TEST_F(EthFixture, AddressFillSourceByIfaceTest){
	// Сначала определяем доступный сетевой интерфейс
	awh::net::src_t discovery(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем извлечение сетевых параметров
	this->_eth->addr.fillSource(discovery);
	// Проверяем, что интерфейс получен
	ASSERT_FALSE(discovery.iface.empty());
	// Создаём новый источник с явно заданным именем интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Устанавливаем имя ранее найденного интерфейса
	source.iface = discovery.iface;
	// Выполняем извлечение сетевых параметров по имени интерфейса
	this->_eth->addr.fillSource(source);
	// Имя интерфейса должно сохраниться
	ASSERT_EQ(discovery.iface, source.iface);
}

/**
 * @brief Тест заполнения источника сетевых адресов по типу узла
 *
 */
TEST_F(EthFixture, AddressFillSourceNodeTest){
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем извлечение сетевых параметров
	this->_eth->addr.fillSource(awh::event::node_t::NONE, source);
	// Проверяем результат
	ASSERT_FALSE(source.iface.empty());
}

/**
 * @brief Тест заполнения источника сетевых адресов для узла клиента
 *
 */
TEST_F(EthFixture, AddressFillSourceClientTest){
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Устанавливаем адрес loopback (он присутствует практически всегда)
	static_cast <awh::net::addr_net_ipv4_t *> (source.ip.get())->address = htonl(INADDR_LOOPBACK);
	// Выполняем извлечение сетевых параметров для узла клиента
	this->_eth->addr.fillSource(awh::event::node_t::CLIENT, source);
	// Для loopback-адреса интерфейс должен быть найден
	ASSERT_FALSE(source.iface.empty());
}

/**
 * @brief Тест заполнения источника сетевых адресов для узла сервера
 *
 */
TEST_F(EthFixture, AddressFillSourceServerTest){
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Устанавливаем адрес loopback (он присутствует практически всегда)
	static_cast <awh::net::addr_net_ipv4_t *> (source.ip.get())->address = htonl(INADDR_LOOPBACK);
	// Выполняем извлечение сетевых параметров для узла сервера
	this->_eth->addr.fillSource(awh::event::node_t::SERVER, source);
	// Для loopback-адреса интерфейс должен быть найден
	ASSERT_FALSE(source.iface.empty());
}

/**
 * @brief Тест заполнения источника сетевых адресов для однорангового узла
 *
 */
TEST_F(EthFixture, AddressFillSourcePeerTest){
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Устанавливаем адрес loopback
	static_cast <awh::net::addr_net_ipv4_t *> (source.ip.get())->address = htonl(INADDR_LOOPBACK);
	// Выполняем извлечение сетевых параметров для однорангового узла
	// Содержимое таблицы маршрутизации зависит от окружения, поэтому проверяем лишь отсутствие падения
	ASSERT_NO_THROW(this->_eth->addr.fillSource(awh::event::node_t::PEER, source));
}

/**
 * @brief Тест заполнения источника сетевых адресов для однорангового узла IPv6
 *
 */
TEST_F(EthFixture, AddressFillSourcePeerIPv6Test){
	// Временный объект для извлечения сетевого интерфейса IPv6
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv6_t> ());
	// Устанавливаем адрес loopback IPv6 (::1)
	static_cast <awh::net::addr_net_ipv6_t *> (source.ip.get())->address[15] = 1;
	// Выполняем извлечение сетевых параметров для однорангового узла
	// Проверяем устойчивость разбора таблицы маршрутизации IPv6 (регрессия на правку указателя sin)
	ASSERT_NO_THROW(this->_eth->addr.fillSource(awh::event::node_t::PEER, source));
}

/**
 * @brief Тест заполнения источника сетевых адресов по сети
 *
 */
TEST_F(EthFixture, AddressFillSourceNetTest){
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Создаём объект IPv4-адреса
	std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес (например, 0.0.0.0 или localhost 127.0.0.1)
	static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->address = htonl(INADDR_LOOPBACK);

	// Выполняем извлечение сетевых параметров
	this->_eth->addr.fillSource(addr.get(), source);
	// Проверяем, что интерфейс найден (для loopback он должен быть)
	ASSERT_FALSE(source.iface.empty());
}

/**
 * @brief Тест корректировки слишком большого префикса IPv4 при заполнении по сети
 *
 */
TEST_F(EthFixture, AddressFillSourceNetPrefixClampIPv4Test){
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Создаём объект IPv4-адреса с заведомо некорректным префиксом
	std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес loopback
	static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->address = htonl(INADDR_LOOPBACK);
	// Устанавливаем некорректный префикс (> 32)
	static_cast <awh::net::addr_net_ipv4_t *> (addr.get())->prefix = 40;
	// Выполняем извлечение сетевых параметров
	this->_eth->addr.fillSource(addr.get(), source);
	// Префикс источника должен быть скорректирован до 32
	ASSERT_EQ(32, static_cast <uint16_t> (static_cast <awh::net::addr_net_ipv4_t *> (source.ip.get())->prefix));
}

/**
 * @brief Тест корректировки слишком большого префикса IPv6 при заполнении по сети
 *
 */
TEST_F(EthFixture, AddressFillSourceNetPrefixClampIPv6Test){
	// Временный объект для извлечения сетевого интерфейса IPv6
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv6_t> ());
	// Создаём объект IPv6-адреса с заведомо некорректным префиксом
	std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Устанавливаем адрес loopback IPv6 (::1)
	static_cast <awh::net::addr_net_ipv6_t *> (addr.get())->address[15] = 1;
	// Устанавливаем некорректный префикс (> 128) — раньше приводил к выходу за границы массива
	static_cast <awh::net::addr_net_ipv6_t *> (addr.get())->prefix = 200;
	// Выполняем извлечение сетевых параметров (не должно падать)
	ASSERT_NO_THROW(this->_eth->addr.fillSource(addr.get(), source));
	// Префикс источника должен быть скорректирован до 128
	ASSERT_EQ(128, static_cast <uint16_t> (static_cast <awh::net::addr_net_ipv6_t *> (source.ip.get())->prefix));
}

/**
 * @brief Тест заполнения источника сетевых адресов IPv6 по типу узла
 *
 */
TEST_F(EthFixture, AddressFillSourceNodeIPv6Test){
	// Временный объект для извлечения сетевого интерфейса IPv6
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv6_t> ());
	// Выполняем извлечение сетевых параметров
	// Наличие IPv6-маршрута зависит от окружения, поэтому проверяем лишь отсутствие падения
	ASSERT_NO_THROW(this->_eth->addr.fillSource(awh::event::node_t::NONE, source));
}

/**
 * @brief Тест заполнения источника сетевых адресов IPv6 по сети
 *
 */
TEST_F(EthFixture, AddressFillSourceNetIPv6Test){
	// Временный объект для извлечения сетевого интерфейса IPv6
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv6_t> ());
	// Создаём объект IPv6-адреса
	std::unique_ptr <awh::net::addr_t> addr = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Устанавливаем адрес loopback IPv6 (::1)
	static_cast <awh::net::addr_net_ipv6_t *> (addr.get())->address[15] = 1;
	// Устанавливаем префикс /128
	static_cast <awh::net::addr_net_ipv6_t *> (addr.get())->prefix = 128;
	// Выполняем извлечение сетевых параметров (не должно падать)
	ASSERT_NO_THROW(this->_eth->addr.fillSource(addr.get(), source));
	// Префикс источника должен быть установлен в /128
	ASSERT_EQ(128, static_cast <uint16_t> (static_cast <awh::net::addr_net_ipv6_t *> (source.ip.get())->prefix));
}
