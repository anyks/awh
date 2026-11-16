/**
 * @file xdp-hw.cpp
 * @date 2026-11-16
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
 * @brief Проверка датапути AF_XDP нулевого копирования на физической сетевой карте
 *
 * @details Набор xdp.cpp доказывает датапуть на паре veth, но нулевое копирование
 *          там не доказать вовсе: драйвер veth не несёт XSK zero-copy (замер
 *          10.11.2026 на ядре 6.12), и проверка IoXdpZeroCopyDatapathTest честно
 *          пропускается на всякой машине. Этот набор привязывается к настоящей
 *          карте, названной переменной среды, и потому доказывает нативный режим
 *          с нулевым копированием там, где он вообще есть
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Проверка собирается только под Linux с включённым спутником AF_XDP
 *
 * @details Вне Linux либо при выключенном AWH_XDP единица трансляции пуста:
 *          спутник engine::xdp объявлен только там, и звать его неоткуда
 *
 */
#if defined(__linux__) && defined(AWH_XDP)
	/**
	 * Подключаем заголовочный файл тестовой фикстуры
	 */
	#include "io.hpp"
	/**
	 * Стандартные заголовочные файлы
	 */
	#include <cstring>
	#include <cstdlib>
	#include <cstdint>
	#include <string>
	#include <chrono>
	#include <algorithm>
	#include <unistd.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>
	#include <net/if.h>
	#include <linux/if_ether.h>
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;
	/**
	 * Пространство имён помощников проверки датапути на физической карте
	 *
	 * @note Имена держатся в безымянной области намеренно: набор net собирается
	 *       единой программой, и вынос их наружу столкнул бы одинаковые
	 *       помощники разных единиц трансляции
	 *
	 */
	namespace {
		/**
		 * @brief Функция чтения строковой переменной среды проверки
		 *
		 * @param name название переменной среды
		 * @return     значение переменной, либо пустая строка когда она не задана
		 *
		 */
		static std::string xdpHwEnv(const char * name) noexcept {
			// Снимаем значение переменной среды
			const char * value = ::getenv(name);
			// Возвращаем значение, либо пустую строку когда переменная не задана
			return ((value != nullptr) ? std::string(value) : std::string());
		}
		/**
		 * @brief Функция чтения числовой переменной среды проверки
		 *
		 * @details Переменная среды - вход извне, потому разбирается со сверкой:
		 *          строка без единой цифры числа не даёт, и молча принятый ноль
		 *          подменил бы названную очередь нулём, а названный срок - умолчанием
		 *
		 * @param name     название переменной среды
		 * @param fallback значение, когда переменная не задана либо числа не несёт
		 * @return         значение переменной
		 *
		 */
		static uint32_t xdpHwNum(const char * name, const uint32_t fallback) noexcept {
			// Снимаем строковое значение переменной среды
			const std::string value = xdpHwEnv(name);
			// Если переменная не задана, остаёмся при значении по умолчанию
			if(value.empty())
				// Возвращаем значение по умолчанию
				return fallback;
			// Указатель на первый символ, не вошедший в число
			char * end = nullptr;
			// Разбираем строку как число без знака
			const unsigned long parsed = ::strtoul(value.c_str(), &end, 10);
			// Если цифр не было вовсе либо число вышло нулём, берём умолчание
			if((end == value.c_str()) || (parsed == 0))
				// Возвращаем значение по умолчанию
				return fallback;
			// Возвращаем разобранное число
			return static_cast <uint32_t> (parsed);
		}
		/**
		 * @brief Функция извлечения канального адреса физической карты
		 *
		 * @param iface название сетевого устройства
		 * @param mac   буфер приёма канального адреса
		 * @return      результат извлечения адреса
		 *
		 */
		static bool xdpHwMac(const char * iface, uint8_t * mac) noexcept {
			// Заводим сокет для запроса параметров устройства
			const int query = ::socket(AF_INET, SOCK_DGRAM, 0);
			// Если сокет не создан, адрес не получить
			if(query < 0)
				// Сообщаем об отказе извлечения
				return false;
			// Объект запроса параметров устройства
			struct ifreq ifr{};
			// Устанавливаем название устройства в запрос
			::strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
			// Спрашиваем канальный адрес устройства вызовом SIOCGIFHWADDR
			const bool ok = (::ioctl(query, SIOCGIFHWADDR, &ifr) == 0);
			// Если адрес получен, переносим его в буфер приёмника
			if(ok)
				// Копируем канальный адрес
				::memcpy(mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
			// Закрываем сокет запроса
			::close(query);
			// Возвращаем результат извлечения
			return ok;
		}
		// Число кадров, принятых колбэком чтения узла
		static uint64_t __awh_xdp_hw_frames__(0);
		// Число байт, принятых колбэком чтения узла
		static uint64_t __awh_xdp_hw_bytes__(0);
		// Признак приёма кадра с меткой внешнего впрыскивателя
		static bool __awh_xdp_hw_mark__(false);
		// Метка, какую несёт кадр внешнего впрыскивателя
		static std::string __awh_xdp_hw_marker__;
	}
	/**
	 * @brief Проверка узла AF_XDP нулевого копирования на физической карте
	 *
	 * @details Заводит узел движка в нативном режиме с нулевым копированием на
	 *          названной средой карте и очереди, сверяет режим заведённого узла,
	 *          отправляет кадр через TX-кольцо и упражняет ручной приём. Приёма
	 *          постороннего трафика не требует: очередь может быть и тихой, а
	 *          отказ узла завестись либо отказ отправки от тишины не зависят
	 *
	 * @note Карта называется переменной среды AWH_XDP_IFACE, очередь - AWH_XDP_QUEUE
	 *       (умолчание ноль). Без названной карты проверка честно пропускается:
	 *       выбирать устройство самому нельзя, на живом аплинке перенаправление
	 *       забирает трафик очереди в кольцо, и очередь управления теряет связь
	 *       (замер 16.11.2026 на Intel I210/igb: сеанс ssh, попавший в очередь 3,
	 *       был перенаправлен в кольцо и связь оборвалась). Прогон требует root и
	 *       сторожа, снимающего программу XDP при обрыве связи
	 *
	 */
	TEST_F(IoFixture, IoXdpHardwareNodeTest){
		// Если проверка запущена не от root, датапуть AF_XDP недоступен
		if(::geteuid() != 0)
			// Пропускаем проверку без прав администратора
			GTEST_SKIP() << "IoXdpHw: root (CAP_NET_ADMIN) required";
		// Если ядро не поддерживает AF_XDP, проверять нечего
		if(!engine::xdp::supported())
			// Пропускаем проверку без поддержки ядром
			GTEST_SKIP() << "IoXdpHw: AF_XDP not supported by kernel";
		// Снимаем название физической карты из переменной среды
		const std::string iface = xdpHwEnv("AWH_XDP_IFACE");
		// Если карта не названа, проверять не на чем
		if(iface.empty())
			// Пропускаем проверку без названной карты
			GTEST_SKIP() << "IoXdpHw: AWH_XDP_IFACE not set";
		// Снимаем номер очереди устройства из переменной среды
		const uint32_t queue = xdpHwNum("AWH_XDP_QUEUE", 0);
		// Если драйвер карты не несёт нулевого копирования, нативный датапуть не доказать
		if(!engine::xdp::zerocopySupported(iface, queue))
			// Пропускаем проверку без поддержки нулевого копирования устройством
			GTEST_SKIP() << "IoXdpHw: device does not support AF_XDP zero-copy";
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(engine::io::initialize());
		// Заводим узел датапути AF_XDP в нативном режиме с нулевым копированием
		const event::id_t id = engine::xdp::create(iface, queue, true, net::xdp::mode_t::NATIVE);
		// Проверяем, что нативный узел с нулевым копированием заведён
		ASSERT_NE(id, 0u);
		// Проверяем, что узел заведён именно в нативном режиме, без отката к обобщённому
		ASSERT_EQ(engine::xdp::mode(id), net::xdp::mode_t::NATIVE);
		// Канальный адрес карты: кадр отправки адресуем себе, чтобы не тревожить сеть
		uint8_t mac[ETH_ALEN];
		// Проверяем, что канальный адрес карты получен
		ASSERT_TRUE(xdpHwMac(iface.c_str(), mac));
		// Буфер кадра отправки наименьшей длины
		uint8_t frame[64];
		// Обнуляем кадр
		::memset(frame, 0, sizeof(frame));
		// Копируем адрес карты в поле получателя кадра
		::memcpy(frame, mac, ETH_ALEN);
		// Копируем адрес карты в поле отправителя кадра
		::memcpy(frame + ETH_ALEN, mac, ETH_ALEN);
		// Устанавливаем старший октет типа протокола IPv4
		frame[12] = 0x08;
		// Устанавливаем младший октет типа протокола
		frame[13] = 0x00;
		// Полезная нагрузка проверочного кадра отправки
		const char payload[] = "AWH-XDP-HW-TX";
		// Копируем полезную нагрузку в кадр
		::memcpy(frame + 14, payload, sizeof(payload));
		// Отправляем кадр через узел (TX-кольцо нулевого копирования)
		const bool txok = engine::xdp::send(id, frame, static_cast <uint32_t> (14 + sizeof(payload)));
		// Буфер ручного приёма через узел
		uint8_t mbuf[2048];
		// Принимаем кадр вручную через узел: упражнение API, тихая очередь допустима
		const int32_t manual = engine::xdp::receive(id, mbuf, static_cast <uint32_t> (sizeof(mbuf)));
		// Сносим узел датапути AF_XDP
		engine::xdp::destroy(id);
		// Утверждаем успех отправки кадра через TX-кольцо нулевого копирования
		EXPECT_TRUE(txok);
		// Ручной приём не должен отвечать ошибкой (пустой буфер допустим)
		EXPECT_GE(manual, 0);
		// Запрос режима снесённого узла отвечает обобщённым, а не падает
		EXPECT_EQ(engine::xdp::mode(id), net::xdp::mode_t::GENERIC);
	}
	/**
	 * @brief Проверка приёма кадров колбэком чтения узла нулевого копирования
	 *
	 * @details Доказывает сквозной приём на физической карте: кадр внешнего
	 *          впрыскивателя проходит нативную программу XDP, перенаправление в
	 *          сокет AF_XDP и кольца RX/FILL нулевого копирования, и доставляется
	 *          штатным колбэком чтения узла движка. Сверяется не просто приход
	 *          каких-то кадров, а приход именно отмеченного кадра: посторонний
	 *          трафик очереди тоже попадает в кольцо, и счётчик кадров сам по
	 *          себе приём своего кадра не доказывает
	 *
	 * @note Впрыск с самой машины через AF_PACKET в кольцо приёма НЕ возвращается:
	 *       кадр уходит на провод, а коммутатор не шлёт его обратно в тот же порт
	 *       (замер 16.11.2026 на Intel I210/igb: отправлено 300 кадров, в кольце
	 *       45 чужих, своих ноль). Потому проверка требует внешнего впрыскивателя
	 *       и метки AWH_XDP_MARKER, какую он кладёт в полезную нагрузку; без метки
	 *       честно пропускается. Срок ожидания - AWH_XDP_SECONDS (умолчание 10)
	 *
	 */
	TEST_F(IoFixture, IoXdpHardwareDatapathTest){
		// Если проверка запущена не от root, датапуть AF_XDP недоступен
		if(::geteuid() != 0)
			// Пропускаем проверку без прав администратора
			GTEST_SKIP() << "IoXdpHw: root (CAP_NET_ADMIN) required";
		// Если ядро не поддерживает AF_XDP, проверять нечего
		if(!engine::xdp::supported())
			// Пропускаем проверку без поддержки ядром
			GTEST_SKIP() << "IoXdpHw: AF_XDP not supported by kernel";
		// Снимаем название физической карты из переменной среды
		const std::string iface = xdpHwEnv("AWH_XDP_IFACE");
		// Если карта не названа, проверять не на чем
		if(iface.empty())
			// Пропускаем проверку без названной карты
			GTEST_SKIP() << "IoXdpHw: AWH_XDP_IFACE not set";
		// Снимаем метку внешнего впрыскивателя из переменной среды
		const std::string marker = xdpHwEnv("AWH_XDP_MARKER");
		// Если метка не названа, внешний впрыскиватель не согласован
		if(marker.empty())
			// Пропускаем проверку без согласованного впрыскивателя
			GTEST_SKIP() << "IoXdpHw: AWH_XDP_MARKER not set (external injector required)";
		// Снимаем номер очереди устройства из переменной среды
		const uint32_t queue = xdpHwNum("AWH_XDP_QUEUE", 0);
		// Снимаем срок ожидания приёма из переменной среды
		const uint32_t seconds = xdpHwNum("AWH_XDP_SECONDS", 10);
		// Если драйвер карты не несёт нулевого копирования, нативный датапуть не доказать
		if(!engine::xdp::zerocopySupported(iface, queue))
			// Пропускаем проверку без поддержки нулевого копирования устройством
			GTEST_SKIP() << "IoXdpHw: device does not support AF_XDP zero-copy";
		// Запоминаем метку для колбэка чтения: он без захвата, иначе не приводится к указателю
		__awh_xdp_hw_marker__ = marker;
		// Сбрасываем признак приёма отмеченного кадра
		__awh_xdp_hw_mark__ = false;
		// Обнуляем число принятых кадров
		__awh_xdp_hw_frames__ = 0;
		// Обнуляем число принятых байт
		__awh_xdp_hw_bytes__ = 0;
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(engine::io::initialize());
		// Заводим узел датапути AF_XDP в нативном режиме с нулевым копированием
		const event::id_t id = engine::xdp::create(iface, queue, true, net::xdp::mode_t::NATIVE);
		// Проверяем, что нативный узел с нулевым копированием заведён
		ASSERT_NE(id, 0u);
		// Проверяем, что узел заведён именно в нативном режиме
		ASSERT_EQ(engine::xdp::mode(id), net::xdp::mode_t::NATIVE);
		// Подписываемся на чтение узла: кадры приходят штатным колбэком
		engine::io::on(id, static_cast <engine::callback::read_t> ([](const event::id_t, const uint8_t * buffer, const size_t size) noexcept -> void {
			// Учитываем принятый кадр
			__awh_xdp_hw_frames__++;
			// Учитываем принятые байты
			__awh_xdp_hw_bytes__ += size;
			// Если метка ещё не найдена и кадр не короче метки
			if(!__awh_xdp_hw_mark__ && (size >= __awh_xdp_hw_marker__.size())){
				// Ищем метку впрыскивателя в кадре
				const uint8_t * found = std::search(buffer, buffer + size, __awh_xdp_hw_marker__.begin(), __awh_xdp_hw_marker__.end());
				// Если метка найдена, отмечаем приём своего кадра
				if(found != (buffer + size))
					// Отмечаем приём отмеченного кадра
					__awh_xdp_hw_mark__ = true;
			}
		}));
		// Определяем срок ожидания приёма отмеченного кадра
		const auto deadline = (std::chrono::steady_clock::now() + std::chrono::seconds(seconds));
		/**
		 * Крутим цикл событий до срока либо до приёма отмеченного кадра
		 */
		while(!__awh_xdp_hw_mark__ && (std::chrono::steady_clock::now() < deadline) && engine::io::poll(__AWH_TEST_POLL_SLICE__));
		// Снимаем число принятых кадров до сноса узла
		const uint64_t frames = __awh_xdp_hw_frames__;
		// Снимаем признак приёма отмеченного кадра до сноса узла
		const bool mark = __awh_xdp_hw_mark__;
		// Сносим узел датапути AF_XDP
		engine::xdp::destroy(id);
		// Утверждаем, что колбэк чтения узла вообще вызывался
		EXPECT_GT(frames, 0u) << "read callback never fired: RX datapath is idle";
		// Утверждаем приём именно отмеченного кадра внешнего впрыскивателя
		EXPECT_TRUE(mark) << "marked frame not received within " << seconds << "s";
	}
#endif