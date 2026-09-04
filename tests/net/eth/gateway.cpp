/**
 * @file gateway.cpp
 * @date 2026-02-06
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
 * @brief Тесты работы со шлюзами — проверка чтения таблицы маршрутизации,
 *        определения шлюза по умолчанию и разбора параметров маршрута
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем системные заголовочные файлы
 */
/**
 * Для операционной системы MS Windows
 *
 * @note Заголовки эти принадлежат POSIX и у MS Windows отсутствуют:
 *       соответствующие им объявления приходят там из winsock2.h,
 *       подключаемого через единую точку sys/win32.hpp
 *
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Системные заголовочные файлы
	 */
	#include <arpa/inet.h>
#endif

/**
 * Подключаем восполнение средств POSIX, отсутствующих у MS Windows
 */
#include "../../posix.hpp"

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * @brief Пространство имён файлового охвата
 *
 */
namespace {
	/**
	 * @brief Охранник маршрута по умолчанию
	 *
	 * @details Снимает слепок действующего маршрута по умолчанию при заведении и
	 * возвращает его при разрушении, каким бы ни был исход проверки. Проверка правит
	 * таблицу маршрутов через ASSERT_TRUE, а всякое несбывшееся утверждение выходит
	 * из тела проверки немедленно, минуя оставшиеся строки возврата, - без охранника
	 * машина остаётся без маршрута по умолчанию
	 *
	 * @note Возврат идёт в два приёма: сперва средствами самого фреймворка, а если
	 * они не справились - средствами системы. Проверяется здесь как раз тот код,
	 * которым идёт возврат, и полагаться на него одного нельзя
	 *
	 */
	class RouteGuard {
		private:
			// Признак наличия слепка
			bool _saved;
			// Адрес подменного шлюза, поставленного проверкой
			uint32_t _substitute;
			// Префикс сети маршрута
			uint8_t _prefix;
			// Адрес шлюза маршрута
			uint32_t _gateway;
			// Адрес назначения маршрута
			uint32_t _destination;
			// Название сетевого интерфейса
			std::string _ifname;
			// Объект работы с Ethernet
			const awh::eth_t * _eth;
			// Объект фреймворка
			const awh::fmk_t * _fmk;
			// Объект работы с логами
			const awh::log_t * _log;
		private:
			/**
			 * @brief Метод сборки объекта маршрута из слепка
			 *
			 * @param route объект маршрута для заполнения
			 */
			void restore(awh::eth::gateway_t::route_t & route) const noexcept {
				// Инициализируем объект адреса шлюза в маршруте
				route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
				// Инициализируем объект адреса назначения в маршруте
				route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
				// Устанавливаем название сетевого интерфейса
				route.ifname = this->_ifname;
				// Устанавливаем префикс сети
				route.prefix = this->_prefix;
				// Устанавливаем адрес шлюза маршрута
				awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address = this->_gateway;
				// Устанавливаем адрес назначения маршрута
				awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address = this->_destination;
			}
			/**
			 * @brief Метод определения действующего шлюза по умолчанию
			 *
			 * @return адрес действующего шлюза по умолчанию либо ноль
			 */
			uint32_t current() const noexcept {
				// Структура маршрута
				awh::eth::gateway_t::route_t route{};
				// Инициализируем объект адреса шлюза в маршруте
				route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
				// Инициализируем объект адреса назначения в маршруте
				route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
				// Если маршрут по умолчанию получить не удалось
				if(!this->_eth->gateway.get(route))
					// Выводим пустой результат
					return 0;
				// Выводим адрес действующего шлюза по умолчанию
				return awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address;
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param eth объект работы с Ethernet
			 * @param fmk объект фреймворка
			 * @param log объект работы с логами
			 */
			explicit RouteGuard(const awh::eth_t * eth, const awh::fmk_t * fmk, const awh::log_t * log) noexcept :
			 _saved(false), _substitute(0), _prefix(0), _gateway(0), _destination(0), _ifname{}, _eth(eth), _fmk(fmk), _log(log) {
				// Структура маршрута
				awh::eth::gateway_t::route_t route{};
				// Инициализируем объект адреса шлюза в маршруте
				route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
				// Инициализируем объект адреса назначения в маршруте
				route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
				// Если маршрут по умолчанию получить не удалось
				if(!this->_eth->gateway.get(route))
					// Выходим из конструктора
					return;
				// Запоминаем название сетевого интерфейса
				this->_ifname = route.ifname;
				// Запоминаем префикс сети
				this->_prefix = route.prefix;
				// Запоминаем адрес шлюза маршрута
				this->_gateway = awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address;
				// Запоминаем адрес назначения маршрута
				this->_destination = awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address;
				// Запоминаем, что слепок снят
				this->_saved = (this->_gateway > 0);
			}
			/**
			 * @brief Метод установки адреса подменного шлюза
			 *
			 * @details Проверка сообщает охраннику адрес шлюза, который ставит сама.
			 * Без этого сорванное утверждение оставляло бы в таблице второй маршрут
			 * по умолчанию: возврат исходного проходит и при нём, а расхождения
			 * охранник не видит - действующим шлюзом система отвечает исходный
			 *
			 * @param substitute адрес подменного шлюза
			 */
			void substitute(const uint32_t substitute) noexcept {
				// Запоминаем адрес подменного шлюза
				this->_substitute = substitute;
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~RouteGuard() noexcept {
				// Если слепок не снят или прав на правку таблицы маршрутов нет
				if(!this->_saved || (::getuid() != 0))
					// Выходим из деструктора
					return;
				// Если проверка ставила свой шлюз и он отличается от исходного
				if((this->_substitute > 0) && (this->_substitute != this->_gateway)){
					// Структура маршрута
					awh::eth::gateway_t::route_t route{};
					// Выполняем сборку объекта маршрута из слепка
					this->restore(route);
					// Устанавливаем адрес подменного шлюза маршрута
					awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address = this->_substitute;
					// Выполняем снос маршрута по умолчанию через подменный шлюз
					this->_eth->gateway.remove(route);
				}
				// Если маршрут по умолчанию на месте
				if(this->current() == this->_gateway)
					// Выходим из деструктора
					return;
				// Структура маршрута
				awh::eth::gateway_t::route_t route{};
				// Выполняем сборку объекта маршрута из слепка
				this->restore(route);
				// Выполняем снятие того, что осталось от маршрута по умолчанию
				this->_eth->gateway.remove(route);
				// Выполняем возврат маршрута по умолчанию средствами фреймворка
				this->_eth->gateway.add(route);
				// Если маршрут по умолчанию вернулся
				if(this->current() == this->_gateway){
					// Выводим сообщение о возврате маршрута
					std::cout << "RouteGuard: default route restored by the framework" << std::endl;
					// Выходим из деструктора
					return;
				}
				// Объект адреса шлюза
				awh::net_addr_t addr(this->_fmk, this->_log);
				// Устанавливаем адрес шлюза по умолчанию
				addr.v4(this->_gateway, awh::net_addr_t::endian_t::LITTLE);
				// Получаем адрес шлюза по умолчанию в виде строки
				const std::string & gateway = static_cast <std::string> (addr);
				// Команда возврата маршрута по умолчанию средствами системы
				std::string command{};
				/**
				 * Для операционной системы Linux
				 */
				#if __linux__
					// Формируем команду возврата маршрута по умолчанию
					command = ("ip route replace default via " + gateway);
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Формируем команду возврата маршрута по умолчанию
					command = ("route -n add default " + gateway);
				#endif
				// Выполняем возврат маршрута по умолчанию средствами системы
				const int32_t status = ::system(command.c_str());
				// Выводим сообщение о возврате маршрута средствами системы
				std::cout << "RouteGuard: default route restored by the system (" << command
				          << "), status=" << status << std::endl;
			}
	};
}

/**
 * @brief Тест получения маршрута
 *
 */
TEST_F(EthFixture, GatewayGetTest){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Если получаем маршрут для указанного адреса
	ASSERT_TRUE(this->_eth->gateway.get(route));
	/**
	 * Содержимое полученного маршрута утверждается, а не печатается
	 *
	 * @warning Прежде здесь стоял ОДИН признак - что вызов ответил истиной, - а имя
	 *          устройства и адрес шлюза лишь печатались. Вернись `get` истину с
	 *          обнулённым маршрутом, проверка прошла бы зелёной: признак успеха о
	 *          содержимом не говорит ничего
	 */
	ASSERT_FALSE(route.ifname.empty()) << "маршрут по умолчанию получен без имени сетевого устройства";
	// Адрес шлюза по умолчанию нулевым быть не может
	ASSERT_NE(0u, awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address) << "маршрут по умолчанию получен с нулевым адресом шлюза";
	// Длина префикса у маршрута по умолчанию предела разрядности превышать не может
	ASSERT_LE(route.prefix, 32) << "длина префикса маршрута IPv4 превышает предел разрядности";
	// Устанавливаем полученный IP-адрес
	this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address, awh::net_addr_t::endian_t::LITTLE);
	// Записываем в лог сведения о найденном маршруте
	std::cout << "Gateway found: interface " << route.ifname << ", default gateway " << static_cast <std::string> (* this->_addr.get()) << std::endl;
}

/**
 * @brief Тест подмены маршрута по умолчанию и его восстановления
 *
 * @details Отделён от `GatewayGetTest` намеренно. Прежде это была одна проверка,
 *          и все её содержательные шаги - снос маршрута, подмена шлюза,
 *          восстановление - стояли за `if(::getuid() == 0)`. Под обычным
 *          пользователем она утверждала ровно ОДИН вызов из шести и была при этом
 *          зелёной, ничем не выдавая, что прошла в усечённом виде. Теперь
 *          нехватка полномочий отвечает ПРОПУСКОМ с доводом
 *
 * @warning Проверка правит таблицу маршрутизации и восстанавливает её охранником
 *          `RouteGuard`. Без полномочий она не выполняется вовсе
 */
TEST_F(EthFixture, GatewayRouteSubstitutionTest){
	// Если полномочий недостаточно, править таблицу маршрутизации нечем
	if(::getuid() != 0)
		// Пропускаем проверку с указанием причины
		GTEST_SKIP() << "подмена маршрута по умолчанию требует полномочий суперпользователя";
	// Заводим охранника маршрута по умолчанию
	RouteGuard guard(this->_eth.get(), this->_fmk.get(), this->_log.get());
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Если получаем маршрут для указанного адреса
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Устанавливаем полученный IP-адрес
	this->_addr->v4(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address, awh::net_addr_t::endian_t::LITTLE);
	// Получаем IP-адрес текущего шлюза по умолчанию
	const std::string gateway = static_cast <std::string> (* this->_addr.get());
	// Удаляем маршрут по указанному адресу
	ASSERT_TRUE(this->_eth->gateway.remove(route));
	/**
	 * @note Подменный шлюз берётся соседом действующего, а не задаётся числом:
	 *       ядро отвергает маршрут через шлюз, до которого не достаёт напрямую,
	 *       и зашитый адрес чужой сети валил проверку на всяком стенде кодом ESRCH
	 */
	// Получаем приставку сети действующего шлюза
	const std::string & prefix = gateway.substr(0, gateway.rfind('.') + 1);
	// Получаем адрес подменного шлюза
	const std::string & substitute = (prefix + (gateway.compare(prefix + "131") != 0 ? "131" : "132"));
	// Выполняем парсинг адреса нового шлюза
	(* this->_addr.get()) = substitute;
	// Устанавливаем адрес шлюза в маршрут
	awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Сообщаем охраннику адрес подменного шлюза
	guard.substitute(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address);
	// Добавляем маршрут с новым шлюзом
	ASSERT_TRUE(this->_eth->gateway.add(route));
	// Получаем маршрут заново: подмена обязана быть видна
	ASSERT_TRUE(this->_eth->gateway.get(route));
	/**
	 * Подмена обязана быть УТВЕРЖДЕНА, а не только выполнена
	 *
	 * @warning Прежде за добавлением следовало лишь получение маршрута с признаком
	 *          успеха, а сам подменный адрес только печатался. Не встань подмена
	 *          вовсе - проверка прошла бы: `get` вернул бы прежний маршрут и ту же
	 *          истину
	 */
	ASSERT_EQ(this->_addr->v4(awh::net_addr_t::endian_t::LITTLE), awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address) << "подменный шлюз не встал: маршрут отдаёт прежний адрес";
	// Удаляем маршрут по указанному адресу
	ASSERT_TRUE(this->_eth->gateway.remove(route));
	// Выполняем парсинг адреса прежнего шлюза
	(* this->_addr.get()) = gateway;
	// Устанавливаем адрес шлюза в маршрут
	awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Возвращаем маршрут по умолчанию на место
	ASSERT_TRUE(this->_eth->gateway.add(route));
}
/**
 * @brief Тест получения маршрута по умолчанию IPv4 и разрешения имени интерфейса
 *
 */
TEST_F(EthFixture, GatewayGetDefaultIPv4){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Имя сетевого интерфейса обязано быть определено при любом виде маршрута
	ASSERT_FALSE(route.ifname.empty());
	/**
	 * Проверяем адрес шлюза по умолчанию
	 *
	 * @note Шлюз нужен не всякому маршруту, и договор модуля говорит о том прямо:
	 *       сеть, до которой машина достаёт напрямую, задаётся одним устройством.
	 *       Маршрут по умолчанию через устройство точка-точка (VPN, туннель) шлюза
	 *       не имеет вовсе, и нуль здесь - верный ответ системы, а не отказ модуля.
	 *       Проверка потому спрашивает не «шлюз ненулевой», а «нуль оправдан видом
	 *       устройства»: на обычной машине шлюз по-прежнему обязан быть
	 */
	if(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address == 0U){
		// Получаем флаги сетевого интерфейса, которым маршрут задан
		const auto flags = this->_eth->iface.flags(route.ifname);
		// Нулевой шлюз оправдан только устройством точка-точка
		ASSERT_TRUE(flags.count(awh::event::eth_flag_t::POINTTOPOINT) > 0)
		 << "Шлюз маршрута по умолчанию нулевой, а интерфейс " << route.ifname << " не является устройством точка-точка";
	}
	// Адрес назначения маршрута по умолчанию должен быть нулевым
	ASSERT_EQ(awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address, 0U);
	// Префикс маршрута по умолчанию должен быть нулевым
	ASSERT_EQ(static_cast <uint32_t> (route.prefix), 0U);
	// Имя сетевого интерфейса должно быть определено (регрессия на разрешение через if_indextoname)
	ASSERT_FALSE(route.ifname.empty());
}
/**
 * @brief Тест получения маршрута по умолчанию IPv4 без инициализации адреса назначения
 *
 */
TEST_F(EthFixture, GatewayGetDefaultIPv4NullDestination){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем только объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Объект адреса назначения намеренно не инициализируем (должен создаться автоматически)
	ASSERT_EQ(route.destination, nullptr);
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Объект адреса назначения должен быть создан автоматически
	ASSERT_NE(route.destination, nullptr);
	// Имя сетевого интерфейса обязано быть определено при любом виде маршрута
	ASSERT_FALSE(route.ifname.empty());
	/**
	 * Проверяем адрес шлюза по умолчанию
	 *
	 * @note Нулевой шлюз законен у маршрута через устройство точка-точка,
	 *       см. пояснение у теста GatewayGetDefaultIPv4
	 */
	if(awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address == 0U){
		// Получаем флаги сетевого интерфейса, которым маршрут задан
		const auto flags = this->_eth->iface.flags(route.ifname);
		// Нулевой шлюз оправдан только устройством точка-точка
		ASSERT_TRUE(flags.count(awh::event::eth_flag_t::POINTTOPOINT) > 0)
		 << "Шлюз маршрута по умолчанию нулевой, а интерфейс " << route.ifname << " не является устройством точка-точка";
	}
}
/**
 * @brief Тест поиска маршрута по адресу шлюза IPv4 (без привилегий)
 *
 */
TEST_F(EthFixture, GatewayGetByGatewayIPv4){
	// Структура маршрута для поиска шлюза по умолчанию
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Запоминаем найденный адрес шлюза по умолчанию
	const uint32_t gateway = awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address;
	// Структура маршрута для поиска по адресу шлюза
	awh::eth::gateway_t::route_t search{};
	// Инициализируем объект адреса шлюза в маршруте
	search.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	search.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес шлюза для поиска
	awh_cast <awh::net::addr_net_ipv4_t *> (search.gateway.get())->address = gateway;
	// Выполняем поиск маршрута по адресу шлюза
	ASSERT_TRUE(this->_eth->gateway.get(search));
	// Найденный адрес шлюза должен совпадать с искомым
	ASSERT_EQ(awh_cast <awh::net::addr_net_ipv4_t *> (search.gateway.get())->address, gateway);
}
/**
 * @brief Тест поиска маршрута по адресу шлюза вместе с устройством IPv4 (без привилегий)
 *
 * @details Закрепляет разбор, при котором заданы ОБА условия разом. У систем Sun
 *          записи таблицы, снятой через mib2, имя устройства несут не всегда: у
 *          маршрута по умолчанию поле ipRouteIfIndex пусто, и сличение по устройству
 *          отвергало запись, которая условию отвечает. Порознь ни поиск по шлюзу, ни
 *          поиск по устройству дефекта не показывали
 *
 */
TEST_F(EthFixture, GatewayGetByGatewayAndInterfaceIPv4){
	// Структура маршрута для поиска шлюза по умолчанию
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Запоминаем найденный адрес шлюза по умолчанию
	const uint32_t gateway = awh_cast <awh::net::addr_net_ipv4_t *> (route.gateway.get())->address;
	// Запоминаем название сетевого интерфейса маршрута по умолчанию
	const std::string & ifname = route.ifname;
	// Название сетевого интерфейса обязано быть определено
	ASSERT_FALSE(ifname.empty());
	/**
	 * @note Нулевой шлюз законен у маршрута через устройство точка-точка, и искать
	 *       по нему нечего: условие поиска вырождается в маршрут по умолчанию
	 */
	if(gateway == 0U)
		// Пропускаем проверку
		GTEST_SKIP() << "Маршрут по умолчанию задан устройством " << ifname << " без шлюза";
	// Структура маршрута для поиска по адресу шлюза вместе с устройством
	awh::eth::gateway_t::route_t search{};
	// Инициализируем объект адреса шлюза в маршруте
	search.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	search.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес шлюза для поиска
	awh_cast <awh::net::addr_net_ipv4_t *> (search.gateway.get())->address = gateway;
	// Устанавливаем название сетевого интерфейса для поиска
	search.ifname = ifname;
	// Выполняем поиск маршрута по адресу шлюза вместе с устройством
	ASSERT_TRUE(this->_eth->gateway.get(search))
	 << "Маршрут через шлюз на устройстве " << ifname << " не найден, хотя порознь оба условия ему отвечают";
	// Найденный адрес шлюза должен совпадать с искомым
	ASSERT_EQ(awh_cast <awh::net::addr_net_ipv4_t *> (search.gateway.get())->address, gateway);
	// Найденное название сетевого интерфейса должно совпадать с искомым
	ASSERT_EQ(search.ifname, ifname);
}
/**
 * @brief Тест сноса маршрута, заданного одним устройством, без шлюза IPv4
 *
 * @details Маршрут без шлюза сносится ЕДИНСТВЕННЫМ условием - совпадением устройства,
 *          и снимок таблицы поля RTA_IFP не несёт. Требование этого поля отвергало
 *          любую запись, и такой маршрут не сносился вовсе. Так заданы маршруты через
 *          устройства точка-точка: туннели и VPN
 *
 * @note Трогается только испытательная сеть 192.0.2.0/24 (TEST-NET-1), маршрут по
 *       умолчанию не затрагивается
 *
 */
TEST_F(EthFixture, GatewayRemoveByInterfaceIPv4){
	// Если пользователь не является привилигированным
	if(::getuid() != 0)
		// Пропускаем проверку
		GTEST_SKIP() << "Для правки таблицы маршрутов нужны права суперпользователя";
	// Структура маршрута по умолчанию
	awh::eth::gateway_t::route_t origin{};
	// Инициализируем объект адреса шлюза в маршруте
	origin.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	origin.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(origin));
	// Название сетевого интерфейса обязано быть определено
	ASSERT_FALSE(origin.ifname.empty());
	// Структура маршрута до испытательной сети
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Выполняем парсинг адреса испытательной сети
	(* this->_addr.get()) = "192.0.2.0";
	// Устанавливаем адрес назначения маршрута
	awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Устанавливаем префикс сети маршрута
	route.prefix = 24;
	// Устанавливаем название сетевого интерфейса маршрута
	route.ifname = origin.ifname;
	// Добавляем маршрут, заданный одним устройством
	ASSERT_TRUE(this->_eth->gateway.add(route))
	 << "Маршрут до испытательной сети через устройство " << route.ifname << " не добавлен";
	// Сносим маршрут, заданный одним устройством
	ASSERT_TRUE(this->_eth->gateway.remove(route))
	 << "Маршрут, заданный одним устройством " << route.ifname << ", снести не удалось";
	// Структура маршрута для проверки того, что снос состоялся
	awh::eth::gateway_t::route_t search{};
	// Инициализируем объект адреса шлюза в маршруте
	search.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	search.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем адрес назначения маршрута
	awh_cast <awh::net::addr_net_ipv4_t *> (search.destination.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Устанавливаем название сетевого интерфейса маршрута
	search.ifname = origin.ifname;
	// Снесённый маршрут находиться больше не должен
	ASSERT_FALSE(this->_eth->gateway.get(search))
	 << "Маршрут до испытательной сети найден после сноса";
}
/**
 * @brief Тест поиска маршрута по имени сетевого интерфейса IPv4 (без привилегий)
 *
 */
TEST_F(EthFixture, GatewayGetByInterfaceIPv4){
	// Структура маршрута для поиска шлюза по умолчанию
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(route));
	// Если имя сетевого интерфейса не определено, пропускаем тест
	if(route.ifname.empty())
		// Завершаем тест успешно
		return;
	// Запоминаем имя сетевого интерфейса маршрута по умолчанию
	const std::string ifname = route.ifname;
	// Структура маршрута для поиска по имени интерфейса
	awh::eth::gateway_t::route_t search{};
	// Инициализируем объект адреса шлюза в маршруте
	search.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	search.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Устанавливаем имя сетевого интерфейса для поиска
	search.ifname = ifname;
	// Выполняем поиск маршрута по имени сетевого интерфейса
	ASSERT_TRUE(this->_eth->gateway.get(search));
	// Найденное имя сетевого интерфейса должно совпадать с искомым
	ASSERT_EQ(search.ifname, ifname);
}
/**
 * @brief Тест поиска несуществующего конкретного маршрута IPv4 (ветка точного совпадения)
 *
 */
TEST_F(EthFixture, GatewayGetSpecificDestinationNotFoundIPv4){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Выполняем парсинг тестового адреса назначения (TEST-NET-3, RFC 5737)
	(* this->_addr.get()) = "203.0.113.123";
	// Устанавливаем конкретный адрес назначения для поиска
	awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address = this->_addr->v4(awh::net_addr_t::endian_t::LITTLE);
	// Устанавливаем префикс маршрута на хостовый
	route.prefix = 32;
	/**
	 * Адрес, у которого своей записи в таблице нет, разрешается маршрутом по
	 * умолчанию: метод отвечает на вопрос «каким путём система отправит пакет»,
	 * а не «есть ли запись ровно с таким назначением». Прежде здесь закреплялось
	 * обратное - точное сличение поля назначения, - отчего недостижимым
	 * объявлялся и адрес своей же сети
	 */
	const bool result = this->_eth->gateway.get(route);
	// Если маршрут до тестового адреса найден
	if(result){
		// Устройство, которым система отправит пакет, обязано быть названо
		ASSERT_FALSE(route.ifname.empty());
		// Запрошенный адрес назначения ответом затираться не должен
		ASSERT_EQ(awh_cast <awh::net::addr_net_ipv4_t *> (route.destination.get())->address, this->_addr->v4(awh::net_addr_t::endian_t::LITTLE));
	/**
	 * Отсутствие маршрута обязано быть ПРОПУСКОМ, а не печатью
	 *
	 * @warning Прежде здесь стояла печать в поток вывода: проверка проходила
	 *          зелёной, не утвердив ничего, и узнать об этом можно было только
	 *          читая вывод глазами. Отчёт о прогоне такой случай не показывал
	 */
	// Если маршрута нет вовсе, у машины нет и выхода наружу
	} else GTEST_SKIP() << "маршрута до тестового адреса нет: выхода наружу у машины нет";
}
/**
 * @brief Тест получения маршрута по умолчанию IPv6 (толерантный к отсутствию IPv6)
 *
 */
TEST_F(EthFixture, GatewayGetDefaultIPv6){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Выполняем получение маршрута по умолчанию IPv6 (может отсутствовать)
	const bool result = this->_eth->gateway.get(route);
	// Если маршрут по умолчанию IPv6 найден
	if(result){
		// Нулевой IPv6 адрес для сравнения
		const std::array <uint8_t, 16> zero{0};
		// Адрес шлюза по умолчанию IPv6 должен быть ненулевым
		ASSERT_NE(awh_cast <awh::net::addr_net_ipv6_t *> (route.gateway.get())->address, zero);
		// Имя сетевого интерфейса должно быть определено
		ASSERT_FALSE(route.ifname.empty());
	/**
	 * Отсутствие маршрута IPv6 обязано быть ПРОПУСКОМ с доводом
	 *
	 * @warning Прежде ветви «иначе» не было вовсе: на машине без IPv6 проверка
	 *          проходила зелёной при нуле утверждений. Сеть IPv6 есть не у всякой
	 *          машины, и это законная обстановка, - но названа она должна быть
	 */
	} else GTEST_SKIP() << "маршрута по умолчанию IPv6 у машины нет";
}
/**
 * @brief Тест полного круга маршрута IPv6 через устройство: заведение, поиск, снос
 *
 * @details Ветви IPv6 у `get`, `add` и `remove` не проверял никто: маршрута по умолчанию
 *          IPv6 нет ни на одном стенде, и `GatewayGetDefaultIPv6` проходит вхолостую -
 *          при отсутствии маршрута она не утверждает ничего. Круг ставится на местной
 *          сети fd7a:1c2e:3f4b::/48 из области ULA (RFC 4193), маршрут по умолчанию не
 *          трогается
 *
 * @note Документационная сеть 2001:db8::/32 для этого не годится: NetBSD держит на неё
 *       свой отвергающий маршрут через ::1, и заведение отвечает отказом «уже есть»
 *
 */
TEST_F(EthFixture, GatewayRouteLifecycleIPv6){
	// Если пользователь не является привилигированным
	if(::getuid() != 0)
		// Пропускаем проверку
		GTEST_SKIP() << "Для правки таблицы маршрутов нужны права суперпользователя";
	// Структура маршрута по умолчанию IPv4, откуда берётся действующее устройство
	awh::eth::gateway_t::route_t origin{};
	// Инициализируем объект адреса шлюза в маршруте
	origin.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Инициализируем объект адреса назначения в маршруте
	origin.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
	// Получаем маршрут по умолчанию
	ASSERT_TRUE(this->_eth->gateway.get(origin));
	// Название сетевого интерфейса обязано быть определено
	ASSERT_FALSE(origin.ifname.empty());
	// Если у сетевого интерфейса нет адреса IPv6
	if(this->_eth->iface.getAddress(origin.ifname, awh::event::family_t::IPV6) == nullptr)
		// Пропускаем проверку
		GTEST_SKIP() << "У интерфейса " << origin.ifname << " нет адреса IPv6";
	// Структура маршрута до испытательной сети IPv6
	awh::eth::gateway_t::route_t route{};
	// Инициализируем объект адреса шлюза в маршруте
	route.gateway = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Инициализируем объект адреса назначения в маршруте
	route.destination = std::make_unique <awh::net::addr_net_ipv6_t> ();
	// Выполняем парсинг адреса испытательной сети
	(* this->_addr.get()) = "fd7a:1c2e:3f4b::";
	// Получаем адрес назначения маршрута
	const auto destination = this->_addr->v6();
	// Устанавливаем адрес назначения маршрута
	awh_cast <awh::net::addr_net_ipv6_t *> (route.destination.get())->address = destination;
	// Устанавливаем префикс сети маршрута
	route.prefix = 48;
	// Устанавливаем название сетевого интерфейса маршрута
	route.ifname = origin.ifname;
	// Добавляем маршрут, заданный одним устройством
	ASSERT_TRUE(this->_eth->gateway.add(route))
	 << "Маршрут IPv6 до испытательной сети через устройство " << route.ifname << " не добавлен";
	{
		// Структура маршрута для поиска заведённого маршрута
		awh::eth::gateway_t::route_t search{};
		// Инициализируем объект адреса шлюза в маршруте
		search.gateway = std::make_unique <awh::net::addr_net_ipv6_t> ();
		// Инициализируем объект адреса назначения в маршруте
		search.destination = std::make_unique <awh::net::addr_net_ipv6_t> ();
		// Устанавливаем адрес назначения маршрута
		awh_cast <awh::net::addr_net_ipv6_t *> (search.destination.get())->address = destination;
		// Заведённый маршрут обязан находиться
		ASSERT_TRUE(this->_eth->gateway.get(search))
		 << "Заведённый маршрут IPv6 не найден";
		// Название сетевого интерфейса обязано быть определено
		ASSERT_FALSE(search.ifname.empty())
		 << "У найденного маршрута IPv6 не определено устройство";
	}
	// Сносим маршрут, заданный одним устройством
	ASSERT_TRUE(this->_eth->gateway.remove(route))
	 << "Маршрут IPv6, заданный одним устройством " << route.ifname << ", снести не удалось";
	{
		// Структура маршрута для проверки того, что снос состоялся
		awh::eth::gateway_t::route_t search{};
		// Инициализируем объект адреса шлюза в маршруте
		search.gateway = std::make_unique <awh::net::addr_net_ipv6_t> ();
		// Инициализируем объект адреса назначения в маршруте
		search.destination = std::make_unique <awh::net::addr_net_ipv6_t> ();
		// Устанавливаем адрес назначения маршрута
		awh_cast <awh::net::addr_net_ipv6_t *> (search.destination.get())->address = destination;
		// Устанавливаем название сетевого интерфейса маршрута
		search.ifname = origin.ifname;
		// Снесённый маршрут находиться больше не должен
		ASSERT_FALSE(this->_eth->gateway.get(search))
		 << "Маршрут IPv6 до испытательной сети найден после сноса";
	}
}
/**
 * @brief Тест получения маршрута с неинициализированным адресом шлюза
 *
 */
TEST_F(EthFixture, GatewayGetUninitialized){
	// Структура маршрута без инициализированного адреса шлюза
	awh::eth::gateway_t::route_t route{};
	// Получение маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.get(route));
}
/**
 * @brief Тест получения маршрута с неподдерживаемым семейством адресов
 *
 */
TEST_F(EthFixture, GatewayGetUnsupportedFamily){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Устанавливаем неподдерживаемый тип адреса шлюза (MAC, размер 6)
	route.gateway = std::make_unique <awh::net::addr_mac_t> ();
	// Получение маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.get(route));
}
/**
 * @brief Тест добавления маршрута с неинициализированным адресом шлюза
 *
 */
TEST_F(EthFixture, GatewayAddUninitialized){
	// Структура маршрута без инициализированного адреса шлюза
	awh::eth::gateway_t::route_t route{};
	// Добавление маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.add(route));
}
/**
 * @brief Тест добавления маршрута с неподдерживаемым семейством адресов
 *
 */
TEST_F(EthFixture, GatewayAddUnsupportedFamily){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Устанавливаем неподдерживаемый тип адреса шлюза (MAC, размер 6)
	route.gateway = std::make_unique <awh::net::addr_mac_t> ();
	// Добавление маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.add(route));
}
/**
 * @brief Тест удаления маршрута с неинициализированным адресом шлюза
 *
 */
TEST_F(EthFixture, GatewayRemoveUninitialized){
	// Структура маршрута без инициализированного адреса шлюза
	awh::eth::gateway_t::route_t route{};
	// Удаление маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.remove(route));
}
/**
 * @brief Тест удаления маршрута с неподдерживаемым семейством адресов
 *
 */
TEST_F(EthFixture, GatewayRemoveUnsupportedFamily){
	// Структура маршрута
	awh::eth::gateway_t::route_t route{};
	// Устанавливаем неподдерживаемый тип адреса шлюза (MAC, размер 6)
	route.gateway = std::make_unique <awh::net::addr_mac_t> ();
	// Удаление маршрута должно завершиться неудачей
	ASSERT_FALSE(this->_eth->gateway.remove(route));
}

/**
 * @brief Тест отказа подбора маршрута по несличимой паре видов адреса
 *
 * @details Закрепляет находку ворошителя `tools/fuzz/eth.cpp` от 29.08.2026. Разбор
 *          ведётся по виду ШЛЮЗА, а адрес назначения приводился к тому же виду БЕЗ
 *          ПРОВЕРКИ. У систем GNU заслон стоял на ЧТЕНИИ назначения, а на ЗАПИСИ
 *          найденного маршрута его не было вовсе: назначение иного вида переписывалось
 *          шестнадцатью октетами при выделенных четырёх
 *
 * @warning Запись хуже чтения: порча уходит за пределы объекта молча, и надзиратель
 *          ловит её не всегда. У MS Windows та же беда даёт не выход за границу, а
 *          мусорный ответ - `SOCKADDR_INET` там объединение, места хватает всегда
 *
 */
TEST_F(EthFixture, GatewayMismatchedKindTest){
	/**
	 * Шлюз IPv4, а назначение IPv6
	 */
	{
		// Маршрут, каким ведётся проверка
		awh::eth::gateway_t::route_t route{};
		// Заводим шлюз маршрута семейства IPv4
		route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Заводим адрес назначения маршрута семейства IPv6
		route.destination = std::make_unique <awh::net::addr_net_ipv6_t> ();
		// Несличимая пара обязана отвечать отказом, а не порчей памяти
		ASSERT_NO_THROW(static_cast <void> (this->_eth->gateway.get(route)));
		// Подбор по несличимой паре обязан отвечать отказом
		ASSERT_FALSE(this->_eth->gateway.get(route));
	}
	/**
	 * Шлюз IPv6, а назначение IPv4
	 */
	{
		// Маршрут, каким ведётся проверка
		awh::eth::gateway_t::route_t route{};
		// Заводим шлюз маршрута семейства IPv6
		route.gateway = std::make_unique <awh::net::addr_net_ipv6_t> ();
		// Заводим адрес назначения маршрута семейства IPv4
		route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Несличимая пара обязана отвечать отказом, а не порчей памяти
		ASSERT_NO_THROW(static_cast <void> (this->_eth->gateway.get(route)));
		// Подбор по несличимой паре обязан отвечать отказом
		ASSERT_FALSE(this->_eth->gateway.get(route));
	}
	/**
	 * Прокладка и снос обязаны отвечать отказом по той же паре
	 *
	 * @details Проверка эта стояла у одного лишь подбора, тогда как эталонные наречия
	 *          POSIX ставят её во всех трёх обращениях. У наречия MS Windows её не было
	 *          ни у прокладки, ни у сноса: пара из назначения IPv4 и шлюза IPv6
	 *          переносилась без возражений, и несовпадение вскрывалось лишь ядром
	 *
	 */
	{
		// Маршрут, каким ведётся проверка
		awh::eth::gateway_t::route_t route{};
		// Заводим шлюз маршрута семейства IPv4
		route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Заводим адрес назначения маршрута семейства IPv6
		route.destination = std::make_unique <awh::net::addr_net_ipv6_t> ();
		// Устанавливаем название устройства маршрута
		route.ifname = "lo";
		// Прокладка по несличимой паре обязана отвечать отказом
		ASSERT_FALSE(this->_eth->gateway.add(route));
		// Снос по несличимой паре обязан отвечать отказом
		ASSERT_FALSE(this->_eth->gateway.remove(route));
	}
	{
		// Маршрут, каким ведётся проверка
		awh::eth::gateway_t::route_t route{};
		// Заводим шлюз маршрута семейства IPv6
		route.gateway = std::make_unique <awh::net::addr_net_ipv6_t> ();
		// Заводим адрес назначения маршрута семейства IPv4
		route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Устанавливаем название устройства маршрута
		route.ifname = "lo";
		// Прокладка по несличимой паре обязана отвечать отказом
		ASSERT_FALSE(this->_eth->gateway.add(route));
		// Снос по несличимой паре обязан отвечать отказом
		ASSERT_FALSE(this->_eth->gateway.remove(route));
	}
}

/**
 * @brief Тест отказа по длине префикса, разрядности адреса не отвечающей
 *
 * @par Намеренные решения
 *
 * Проверка заведена 03.09.2026 по находке сличения наречий. Поле `prefix` объявлено
 * `uint8_t` и принимает до 255, а предел зависит от ВИДА адреса: 32 у IPv4 и 128 у
 * IPv6. Договор этот не блюл НИ ОДИН слой, и каждый узнавал о нём порознь -
 * непроверенных длин в проекте насчитано четыре
 *
 * @warning У наречий BSD и Sun негодная длина давала НЕОПРЕДЕЛЁННОЕ ПОВЕДЕНИЕ, а не
 *          просто негодную маску: разность `32 - prefix` считается беззнаковой и при
 *          33 обращается в 4294967295, а сдвиг на такую величину стандартом не
 *          определён. Прежние проверки этого не ловили, потому что длину задавали
 *          всегда в пределах - то есть проверялся лишь годный случай
 *
 * @note Утверждается ОТКАЗ, а не приведение к пределу. Приведение обратило бы
 *       бессмысленную просьбу в осмысленную, но ДРУГУЮ - путь к одному узлу вместо
 *       заказанной сети, - и сделало бы это молча
 *
 * @warning ЧЕМ ЭТА ПРОВЕРКА ЗРЯЧА. Признак отказа для заслона негоден ПО УСТРОЙСТВУ:
 *          заслон лишь ОПЕРЕЖАЕТ отказ, какой пришёл бы и без него - без надзорных
 *          полномочий прокладка и снос отвечают отказом всегда. Первая редакция этой
 *          проверки утверждала `ASSERT_FALSE` и проходила при СНЯТОМ заслоне -
 *          проверено щупом 03.09.2026, то есть утверждала она нехватку полномочий,
 *          а не работу заслона
 *
 * @note Зрячей её делает разбор ДОВОДА: проверка подписывается на журнал и требует,
 *       чтобы причина отказа была названа длиной префикса. При заслоне довод есть,
 *       без заслона приходит сообщение системы о невозможности проложить путь.
 *       Щуп на снятие заслона проверку валит - утверждение двустороннее
 *
 * @note Сам дефект сдвига доказан отдельно надзирателем неопределённого поведения:
 *       в сборке `-DCMAKE_BUILD_SANITIZE=YES` со снятым заслоном приходит
 *       «runtime error: shift exponent 4294967295 is too large for 32-bit type» в
 *       `bsd/gateway.cpp:127`, а с заслоном надзиратель молчит
 *
 * @warning Правило, общее для ВСЯКОГО заслона: отказ, приходящий по двум причинам,
 *          неотличим от отказа по нужной. Оттого щуп на снятие правки у заслона
 *          обязателен всегда, а разводить надо по доводу, а не по признаку отказа
 *
 */
TEST_F(EthFixture, GatewayOversizedPrefixTest){
	/**
	 * Заслон опознаётся ДОВОДОМ в журнале, а не признаком отказа
	 *
	 * @details Признак отказа для заслона негоден по устройству: заслон лишь ОПЕРЕЖАЕТ
	 *          отказ, какой пришёл бы и без него - без надзорных полномочий прокладка
	 *          отвечает отказом всегда. Оттого разводить надо по ДОВОДУ: при заслоне в
	 *          журнал уходит сообщение о длине префикса, без заслона - сообщение
	 *          системы о невозможности проложить путь
	 */
	// Признак того, что заслон назвал причину отказа
	bool named = false;
	// Подписываемся на журнал ради разбора довода отказа
	this->_log->subscribe([&named]([[maybe_unused]] const awh::log_t::flag_t flag, const std::string_view text) noexcept -> void {
		// Отмечаем, что довод отказа назвал длину префикса
		named = (named || (text.find("prefix length") != std::string_view::npos));
	});
	/**
	 * Длины, разрядности адреса IPv4 не отвечающие
	 */
	for(const uint8_t prefix : {static_cast <uint8_t> (33), static_cast <uint8_t> (64), static_cast <uint8_t> (200), static_cast <uint8_t> (255)}){
		// Маршрут, каким ведётся проверка
		awh::eth::gateway_t::route_t route{};
		// Заводим шлюз маршрута семейства IPv4
		route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Заводим адрес назначения маршрута семейства IPv4
		route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Задаём длину префикса, разрядности адреса не отвечающую
		route.prefix = prefix;
		// Прокладка по негодной длине обязана отвечать отказом, а не сдвигом за предел
		ASSERT_NO_THROW(static_cast <void> (this->_eth->gateway.add(route)));
		// Прокладка по негодной длине обязана отвечать отказом
		ASSERT_FALSE(this->_eth->gateway.add(route)) << "Движок принял длину префикса " << static_cast <uint32_t> (prefix) << " у адреса IPv4, где предел равен 32";
		// Снос по негодной длине обязан отвечать отказом
		ASSERT_FALSE(this->_eth->gateway.remove(route)) << "Движок принял длину префикса " << static_cast <uint32_t> (prefix) << " у адреса IPv4 при сносе пути";
	}
	/**
	 * Длины, разрядности адреса IPv6 не отвечающие
	 */
	for(const uint8_t prefix : {static_cast <uint8_t> (129), static_cast <uint8_t> (200), static_cast <uint8_t> (255)}){
		// Маршрут, каким ведётся проверка
		awh::eth::gateway_t::route_t route{};
		// Заводим шлюз маршрута семейства IPv6
		route.gateway = std::make_unique <awh::net::addr_net_ipv6_t> ();
		// Заводим адрес назначения маршрута семейства IPv6
		route.destination = std::make_unique <awh::net::addr_net_ipv6_t> ();
		// Задаём длину префикса, разрядности адреса не отвечающую
		route.prefix = prefix;
		// Прокладка по негодной длине обязана отвечать отказом
		ASSERT_NO_THROW(static_cast <void> (this->_eth->gateway.add(route)));
		// Прокладка по негодной длине обязана отвечать отказом
		ASSERT_FALSE(this->_eth->gateway.add(route)) << "Движок принял длину префикса " << static_cast <uint32_t> (prefix) << " у адреса IPv6, где предел равен 128";
		// Снос по негодной длине обязан отвечать отказом
		ASSERT_FALSE(this->_eth->gateway.remove(route)) << "Движок принял длину префикса " << static_cast <uint32_t> (prefix) << " у адреса IPv6 при сносе пути";
	}
	/**
	 * Предельные длины обязаны заслоном НЕ отвергаться
	 *
	 * @note Утверждение это обратное и без него проверка была бы односторонней:
	 *       заслон, отвергающий всё подряд, прошёл бы её точно так же. Здесь
	 *       закрепляется не исход прокладки - он зависит от полномочий и от
	 *       состояния машины, - а то, что заслон по длине не срабатывает
	 */
	for(const uint8_t prefix : {static_cast <uint8_t> (0), static_cast <uint8_t> (24), static_cast <uint8_t> (32)}){
		// Маршрут, каким ведётся проверка
		awh::eth::gateway_t::route_t route{};
		// Заводим шлюз маршрута семейства IPv4
		route.gateway = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Заводим адрес назначения маршрута семейства IPv4
		route.destination = std::make_unique <awh::net::addr_net_ipv4_t> ();
		// Задаём годную длину префикса
		route.prefix = prefix;
		// Годная длина обязана проходить заслон без порчи памяти
		ASSERT_NO_THROW(static_cast <void> (this->_eth->gateway.add(route)));
	}
	/**
	 * Заслон обязан был назвать причину хотя бы раз
	 *
	 * @note Утверждение это и делает проверку ЗРЯЧЕЙ: снятие заслона его валит, тогда
	 *       как утверждения об отказе выше проходят и без заслона - проверено щупом
	 */
	ASSERT_TRUE(named) << "Заслон по длине префикса причину отказа не назвал: отказ пришёл от системы, а не от заслона";
	// Снимаем подписку на журнал
	this->_log->subscribe(nullptr);
}
