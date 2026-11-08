/**
 * @file xdp.cpp
 * @date 2026-11-08
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
 * @brief Функциональная проверка датапути AF_XDP через узлы движка
 *
 * @details Обычный набор net не доказывает работу XDP: датапуть под AWH_XDP в
 *          нём выключен. Проверка заводит настоящий узел protocol_t::XDP на
 *          одном конце veth-пары, впрыскивает кадр с соседнего конца через
 *          AF_PACKET и принимает его штатным колбэком чтения узла, затем
 *          отправляет кадр через TX-кольцо узла. Требует root (CAP_NET_ADMIN)
 *          и поддержки AF_XDP ядром; без того честно пропускается
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
	#include <string>
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>
	#include <net/if.h>
	#include <netpacket/packet.h>
	#include <linux/if_ether.h>
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;
	/**
	 * Пространство имён помощников проверки датапути AF_XDP
	 *
	 * @note Имена держатся в безымянной области намеренно: набор net собирается
	 *       единой программой, и вынос их наружу столкнул бы одинаковые
	 *       помощники разных единиц трансляции
	 *
	 */
	namespace {
		/**
		 * @brief Функция выполнения служебной команды iproute2
		 *
		 * @param cmd команда для исполнения
		 * @return   результат выполнения команды
		 *
		 */
		static bool xdpCmd(const std::string & cmd) noexcept {
			// Выполняем команду, отведя её вывод, чтобы не засорять журнал проверки
			return ::system((cmd + " >/dev/null 2>&1").c_str()) == 0;
		}
		/**
		 * @brief Функция извлечения MAC-адреса сетевого интерфейса
		 *
		 * @param iface название сетевого интерфейса
		 * @param mac   буфер приёма канального адреса
		 * @return      результат извлечения адреса
		 *
		 */
		static bool xdpMac(const char * iface, uint8_t * mac) noexcept {
			// Заводим сокет для запроса параметров интерфейса
			const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
			// Если сокет не создан, адрес не получить
			if(fd < 0)
				// Сообщаем об отказе извлечения
				return false;
			// Объект запроса параметров интерфейса
			struct ifreq ifr{};
			// Устанавливаем название интерфейса в запрос
			::strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
			// Выполняем запрос канального адреса
			const bool ok = (::ioctl(fd, SIOCGIFHWADDR, &ifr) == 0);
			// Если адрес получен, переносим его в буфер приёмника
			if(ok)
				// Копируем канальный адрес
				::memcpy(mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
			// Закрываем сокет запроса
			::close(fd);
			// Возвращаем результат извлечения
			return ok;
		}
		/**
		 * @brief Класс пары виртуальных интерфейсов veth для проверки
		 *
		 * @details Пара заводится в конструкторе и снимается в деструкторе, чтобы
		 *          ранний выход проверки по ASSERT не оставлял интерфейсы в системе
		 *
		 */
		class XdpVeth {
			// Название первого конца пары (приёмник XDP)
			std::string a;
			// Название второго конца пары (впрыскиватель кадров)
			std::string b;
			// Признак того, что пара заведена
			bool created;
			public:
				/**
				 * @brief Метод получения признака заведения пары
				 *
				 * @return признак того, что пара заведена
				 *
				 */
				bool ok() const noexcept {
					// Возвращаем признак заведения
					return created;
				}
				/**
				 * @brief Метод получения названия первого конца пары
				 *
				 * @return название приёмника XDP
				 *
				 */
				const std::string & rx() const noexcept {
					// Возвращаем название приёмника
					return a;
				}
				/**
				 * @brief Метод получения названия второго конца пары
				 *
				 * @return название впрыскивателя кадров
				 *
				 */
				const std::string & tx() const noexcept {
					// Возвращаем название впрыскивателя
					return b;
				}
				/**
				 * @brief Конструктор пары виртуальных интерфейсов
				 *
				 * @param xa название первого конца пары
				 * @param xb название второго конца пары
				 *
				 */
				explicit XdpVeth(std::string xa, std::string xb) noexcept : a(std::move(xa)), b(std::move(xb)), created(false) {
					// Заводим пару veth и поднимаем оба конца
					created = xdpCmd("ip link add " + a + " type veth peer name " + b) &&
						xdpCmd("ip link set " + a + " up") && xdpCmd("ip link set " + b + " up");
				}
				/**
				 * @brief Деструктор пары виртуальных интерфейсов
				 *
				 */
				~XdpVeth() noexcept {
					// Если пара заведена, снимаем её (удаление одного конца снимает пару)
					if(created)
						// Удаляем первый конец пары
						xdpCmd("ip link del " + a);
				}
		};
		// Признак приёма кадра колбэком чтения узла
		static bool __awh_xdp_got__(false);
		// Длина принятого кадра
		static size_t __awh_xdp_len__(0);
		// Буфер принятого кадра
		static uint8_t __awh_xdp_buf__[2048];
	}
	/**
	 * @brief Проверка датапути AF_XDP через узел движка
	 *
	 * @details Заводит узел XDP, принимает впрыснутый кадр колбэком чтения и
	 *          отправляет кадр через узел; утверждает совпадение полезной
	 *          нагрузки принятого кадра и успех отправки
	 *
	 */
	TEST_F(IoFixture, IoXdpNodeDatapathTest){
		// Если проверка запущена не от root, датапуть AF_XDP недоступен
		if(::geteuid() != 0)
			// Пропускаем проверку без прав администратора
			GTEST_SKIP() << "IoXdp: root (CAP_NET_ADMIN) required";
		// Если ядро не поддерживает AF_XDP, проверять нечего
		if(!engine::xdp::supported())
			// Пропускаем проверку без поддержки ядром
			GTEST_SKIP() << "IoXdp: AF_XDP not supported by kernel";
		// Заводим пару виртуальных интерфейсов для проверки
		XdpVeth veth("awhxdp0", "awhxdp1");
		// Если пара не заведена, проверять не на чем
		if(!veth.ok())
			// Пропускаем проверку без veth-пары
			GTEST_SKIP() << "IoXdp: cannot create veth pair";
		// Сбрасываем признаки приёма кадра
		__awh_xdp_got__ = false;
		// Обнуляем длину принятого кадра
		__awh_xdp_len__ = 0;
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(engine::initialize());
		// Заводим узел датапути AF_XDP на приёмнике (redirect, generic SKB)
		const event::id_t id = engine::xdp::create(veth.rx().c_str(), 0, true);
		// Проверяем, что узел заведён
		ASSERT_NE(id, 0u);
		// Подписываемся на чтение узла: кадры приходят штатным колбэком
		engine::on(id, static_cast <engine::callback::read_t> ([](const event::id_t, const uint8_t * buffer, const size_t size) noexcept -> void {
			// Отмечаем приём кадра
			__awh_xdp_got__ = true;
			// Запоминаем длину принятого кадра с ограничением буфера
			__awh_xdp_len__ = (size < sizeof(__awh_xdp_buf__)) ? size : sizeof(__awh_xdp_buf__);
			// Копируем принятый кадр в буфер проверки
			::memcpy(__awh_xdp_buf__, buffer, __awh_xdp_len__);
		}));
		// Канальные адреса приёмника и впрыскивателя
		uint8_t dmac[ETH_ALEN], smac[ETH_ALEN];
		// Проверяем, что канальные адреса обоих концов получены
		ASSERT_TRUE(xdpMac(veth.rx().c_str(), dmac));
		// Извлекаем адрес впрыскивателя
		ASSERT_TRUE(xdpMac(veth.tx().c_str(), smac));
		// Заводим сокет впрыска кадров AF_PACKET
		const int inj = ::socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
		// Проверяем, что сокет впрыска создан
		ASSERT_GE(inj, 0);
		// Объект адреса канального уровня для впрыска
		struct sockaddr_ll sll{};
		// Устанавливаем семейство адресов
		sll.sll_family = AF_PACKET;
		// Устанавливаем индекс интерфейса впрыскивателя
		sll.sll_ifindex = static_cast <int> (::if_nametoindex(veth.tx().c_str()));
		// Устанавливаем длину канального адреса
		sll.sll_halen = ETH_ALEN;
		// Копируем адрес приёмника в адрес назначения
		::memcpy(sll.sll_addr, dmac, ETH_ALEN);
		// Буфер кадра с полезной нагрузкой
		uint8_t frame[64];
		// Обнуляем кадр
		::memset(frame, 0, sizeof(frame));
		// Копируем адрес приёмника в заголовок кадра
		::memcpy(frame, dmac, ETH_ALEN);
		// Копируем адрес отправителя в заголовок кадра
		::memcpy(frame + ETH_ALEN, smac, ETH_ALEN);
		// Устанавливаем тип протокола IPv4 в заголовке
		frame[12] = 0x08;
		// Устанавливаем младший октет типа протокола
		frame[13] = 0x00;
		// Полезная нагрузка проверочного кадра
		const char payload[] = "XDP-NODE-DATAPATH";
		// Копируем полезную нагрузку в кадр
		::memcpy(frame + 14, payload, sizeof(payload));
		// Впрыскиваем кадр с соседнего конца veth-пары
		const ssize_t sent = ::sendto(inj, frame, 14 + sizeof(payload), 0, reinterpret_cast <struct sockaddr *> (&sll), sizeof(sll));
		// Проверяем, что кадр впрыснут
		ASSERT_GT(sent, 0);
		// Крутим цикл событий: готовность узла -> readXdp -> колбэк чтения
		engine::poll(2000);
		// Признак совпадения полезной нагрузки принятого кадра
		bool match = false;
		// Если кадр принят целиком, сверяем полезную нагрузку
		if(__awh_xdp_got__ && (__awh_xdp_len__ >= (14 + sizeof(payload))))
			// Сравниваем полезную нагрузку принятого кадра с отправленной
			match = (::memcmp(__awh_xdp_buf__ + 14, payload, sizeof(payload)) == 0);
		// Отправляем кадр через узел (TX-кольцо)
		const bool txok = engine::xdp::send(id, frame, static_cast <uint32_t> (14 + sizeof(payload)));
		// Буфер ручного приёма через узел
		uint8_t mbuf[2048];
		// Выполняем ручной приём через узел (упражнение API, кадр уже принят опросом)
		const int32_t mn = engine::xdp::receive(id, mbuf, static_cast <uint32_t> (sizeof(mbuf)));
		// Закрываем сокет впрыска
		::close(inj);
		// Сносим узел датапути AF_XDP
		engine::xdp::destroy(id);
		// Утверждаем, что кадр принят колбэком чтения узла
		EXPECT_TRUE(__awh_xdp_got__);
		// Утверждаем совпадение полезной нагрузки принятого кадра
		EXPECT_TRUE(match);
		// Утверждаем успех отправки кадра через узел
		EXPECT_TRUE(txok);
		// Ручной приём не должен отвечать ошибкой (пустой буфер допустим)
		EXPECT_GE(mn, 0);
	}
#endif