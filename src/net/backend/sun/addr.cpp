/**
 * @file addr.cpp
 * @date 2026-01-28
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
 * @brief Реализация бэкенда адресов канального уровня — получение MAC-адресов и адресов сетевых интерфейсов машины,
 *        а также списка системных DNS-серверов средствами конкретной операционной системы
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Единица выравнивания адресов в сообщениях маршрутизации
 *
 * @note Ядра выравнивают адреса внутри сообщения маршрутизации по-разному:
 *       Apple округляет длину структуры адреса до четырёх октетов, остальные
 *       системы BSD - до размера длинного целого. Неверная единица сдвигает
 *       чтение всех адресов, кроме первого: они читаются с середины соседней
 *       структуры и выглядят пустыми.
 *
 */
#if __APPLE__
	/**
	 * Макрос выравнивания структуры (для Apple)
	 */
	#define AWH_SA_ALIGN sizeof(int32_t)
/**
 * Если не Apple, то считаем, что это любая BSD-система
 */
#else
	/**
	 * Макрос выравнивания структуры (для BSD)
	 */
	#define AWH_SA_ALIGN sizeof(long)
#endif

/**
 * Макрос выравнивания структуры
 */
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (AWH_SA_ALIGN - 1))) : AWH_SA_ALIGN)

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <cerrno>
#include <chrono>
#include <memory>
#include <vector>
#include <random>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <shared_mutex>

/**
 * Системные заголовочные файлы
 */
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
/**
 * Заголовки снятия таблиц ядра через интерфейс mib2
 *
 * @details Снимка таблиц через sysctl у Sun Solaris и illumos нет вовсе: заголовка
 *          `sys/sysctl.h` там не существует, а признаков `NET_RT_DUMP` и
 *          `NET_RT_FLAGS` нет в природе. Таблицы ядра эти системы отдают потоковым
 *          интерфейсом mib2 - тем самым, каким работают `netstat` и `arp`
 *
 * @note Одним запросом снимаются СРАЗУ ЧЕТЫРЕ таблицы: соседи IPv4 и IPv6, маршруты
 *       IPv4 и IPv6. Проверено опытом 11.08.2026 на обоих стендах
 *
 */
#include <fcntl.h>
#include <stropts.h>
#include <sys/stream.h>
#include <sys/tihdr.h>
#include <inet/mib2.h>
#include <sys/un.h>
#include <net/if.h>
#include <net/if_dl.h>
/**
 * Заголовок сведений об устройстве есть не во всех системах: NetBSD его не держит,
 * а нужные объявления раскрывает через прочие заголовки сети
 */
#if __has_include(<net/if_var.h>)
	#include <net/if_var.h>
#endif
#include <net/if_types.h>
#include <netinet/in.h> 
#include <net/route.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/locker.hpp>
#include <net/eth/addr.hpp>
#include <net/eth/gateway.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Пространство снятия таблиц ядра через интерфейс mib2
	 *
	 * @details Заменяет собой снимок таблиц через `sysctl`, какого у Sun Solaris и
	 *          illumos нет. Устроен так же, как это делают `netstat` и `arp`:
	 *          открывается потоковое устройство `/dev/arp`, поверх него надеваются
	 *          модули `tcp` и `udp`, затем подаётся запрос `T_SVR4_OPTMGMT_REQ`, и
	 *          ядро отдаёт таблицы одну за другой, всякую своей парой сообщений -
	 *          заголовком с описанием и телом с записями
	 *
	 * @note Одним запросом снимаются ВСЕ таблицы разом: соседи IPv4 и IPv6, маршруты
	 *       IPv4 и IPv6. Отбор нужной идёт по паре (уровень, название) заголовка
	 *
	 */
	namespace mib {
		/**
		 * @brief Функция снятия одной таблицы ядра
		 *
		 * @details Запрос подаётся один, а таблицы ядро отдаёт все подряд - оттого
		 *          выдача разбирается до конца даже после того, как искомая таблица
		 *          найдена: недочитанный поток оставил бы устройство с непрочитанными
		 *          сообщениями, а закрытие его посреди выдачи ядро расценивает отказом
		 *
		 * @param level  уровень искомой таблицы
		 * @param name   название искомой таблицы
		 * @param buffer буфер, в который складываются записи таблицы
		 * @return       признак того, что таблица снята
		 *
		 */
		static bool table(const int32_t level, const int32_t name, vector <char> & buffer) noexcept {
			// Результат работы функции
			bool result = false;
			// Очищаем буфер записей
			buffer.clear();
			// Выполняем открытие потокового устройства
			const int32_t fd = ::open("/dev/arp", O_RDWR);
			// Если устройство открыть не удалось
			if(fd < 0)
				// Выводим результат с ошибкой
				return result;
			/**
			 * Надеваем модули поверх устройства
			 *
			 * @note Без них ядро отдаёт лишь таблицы самого ARP, а нужны сведения
			 *       уровня IP - их выдают надетые модули
			 */
			if((::ioctl(fd, I_PUSH, "tcp") < 0) || (::ioctl(fd, I_PUSH, "udp") < 0)){
				// Выполняем закрытие потокового устройства
				::close(fd);
				// Выводим результат с ошибкой
				return result;
			}
			// Место под запрос управления
			char request[sizeof(struct T_optmgmt_req) + sizeof(struct opthdr)] = {0};
			// Получаем заголовок запроса управления
			struct T_optmgmt_req * req = reinterpret_cast <struct T_optmgmt_req *> (request);
			// Получаем описание запрашиваемой таблицы
			struct opthdr * opt = reinterpret_cast <struct opthdr *> (req + 1);
			// Устанавливаем разновидность запроса
			req->PRIM_type = T_SVR4_OPTMGMT_REQ;
			// Устанавливаем смещение описания в запросе
			req->OPT_offset = sizeof(struct T_optmgmt_req);
			// Устанавливаем размер описания в запросе
			req->OPT_length = sizeof(struct opthdr);
			// Затребуем действующие значения
			req->MGMT_flags = T_CURRENT;
			// Запрашиваем таблицы уровня IP
			opt->level = MIB2_IP;
			// Таблицы запрашиваются все разом
			opt->name = 0;
			// Тела у запроса нет
			opt->len = 0;
			// Собираем управляющую часть сообщения
			struct strbuf control = {};
			// Устанавливаем данные управляющей части
			control.buf = request;
			// Устанавливаем размер управляющей части
			control.len = static_cast <int32_t> (sizeof(request));
			// Выполняем подачу запроса ядру
			if(::putmsg(fd, &control, nullptr, 0) < 0){
				// Выполняем закрытие потокового устройства
				::close(fd);
				// Выводим результат с ошибкой
				return result;
			}
			// Место под заголовок ответа
			char answer[1024] = {0};
			// Тело очередной таблицы
			vector <char> chunk(0x10000);
			// Собираем управляющую часть ответа
			control.buf = answer;
			// Устанавливаем предельный размер управляющей части
			control.maxlen = static_cast <int32_t> (sizeof(answer));
			/**
			 * Разбираем выдачу ядра до самого конца
			 */
			for(;;){
				// Признаки полученного сообщения
				int32_t flags = 0;
				// Сбрасываем размер полученной управляющей части
				control.len = 0;
				// Выполняем получение заголовка очередной таблицы
				if(::getmsg(fd, &control, nullptr, &flags) < 0)
					// Выходим из разбора выдачи
					break;
				// Если заголовок ответа не поместился, разбирать нечего
				if(static_cast <size_t> (control.len) < sizeof(struct T_optmgmt_ack))
					// Выходим из разбора выдачи
					break;
				// Получаем заголовок ответа
				const struct T_optmgmt_ack * ack = reinterpret_cast <const struct T_optmgmt_ack *> (answer);
				// Если ядро ответило отказом либо ответ не тот, разбирать нечего
				if(ack->PRIM_type != T_OPTMGMT_ACK)
					// Выходим из разбора выдачи
					break;
				// Если описание таблицы не поместилось, разбирать нечего
				if(static_cast <size_t> (ack->OPT_length) < sizeof(struct opthdr))
					// Выходим из разбора выдачи
					break;
				// Получаем описание очередной таблицы
				const struct opthdr * head = reinterpret_cast <const struct opthdr *> (answer + ack->OPT_offset);
				// Если описание пустое, выдача окончена
				if((head->len == 0) && (head->level == 0) && (head->name == 0))
					// Выходим из разбора выдачи
					break;
				// Собираем часть сообщения с записями таблицы
				struct strbuf body = {};
				// Устанавливаем буфер записей таблицы
				body.buf = chunk.data();
				// Устанавливаем предельный размер записей таблицы
				body.maxlen = static_cast <int32_t> (chunk.size());
				// Сбрасываем размер полученных записей
				body.len = 0;
				// Сбрасываем признаки полученного сообщения
				flags = 0;
				// Выполняем получение записей очередной таблицы
				if(::getmsg(fd, nullptr, &body, &flags) < 0)
					// Выходим из разбора выдачи
					break;
				/**
				 * Если таблица оказалась искомой
				 *
				 * @note Оба поля заголовка объявлены системой БЕЗЗНАКОВЫМИ (`t_uscalar_t`),
				 *       а доводы приходят знаковыми - оттого gcc и давал `-Wsign-compare`.
				 *       Приведение здесь именно у ДОВОДОВ, а не у полей: значения эти суть
				 *       постоянные MIB, всегда положительные, и приведение отрицательного
				 *       довода к беззнаковому дало бы огромное число, какое ни с одной
				 *       таблицей не совпадёт - то есть сличение честно ответит «не она»
				 */
				if((head->level == static_cast <t_uscalar_t> (level)) && (head->name == static_cast <t_uscalar_t> (name))){
					// Запоминаем записи искомой таблицы
					buffer.assign(chunk.data(), (chunk.data() + body.len));
					// Отмечаем, что таблица снята
					result = true;
				}
			}
			// Выполняем закрытие потокового устройства
			::close(fd);
			// Выводим результат
			return result;
		}
	};

	/**
	 * @brief Генератор случайных чисел для рандомизации DNS-серверов
	 *
	 */
	random_device __awh_randev__;

	/**
	 * @brief Нулевой MAC-адрес для сравнения
	 *
	 */
	constexpr uint8_t __awh_zero_mac__[6] = {0};

	/**
	 * @brief Нулевой IPv6-адрес для сравнения
	 *
	 */
	constexpr uint8_t __awh_zero_ipv6__[16] = {0};

	/**
	 * @brief Время жизни записи об адресе с выходом во внешнюю сеть
	 *
	 * @details Адрес, с которого машина выходит наружу, меняется редко:
	 *          при смене сети, поднятии или падении туннеля. Пяти секунд
	 *          хватает, чтобы запись не разошлась с состоянием системы
	 *          заметным образом, и достаточно, чтобы массовое создание
	 *          событий обошлось одним определением вместо тысяч.
	 *
	 * @note Точным способом было бы слежение за сокетом маршрутизации,
	 *       но оно требует отдельного долгоживущего дескриптора и его
	 *       обслуживания, поэтому выбрано устаревание по времени.
	 *
	 */
	constexpr uint64_t __awh_outward_lifetime__ = 5000000000ULL;

	/**
	 * @brief Структура записи об адресе, с которого доступна внешнюю сеть
	 *
	 */
	typedef struct Outward {
		// Признак заполненности записи
		bool filled;
		// Время устаревания записи в наносекундах
		uint64_t deadline;
		// Название сетевого интерфейса
		string iface;
		// MAC-адрес сетевого интерфейса
		array <uint8_t, 6> mac;
		// Адрес сетевого интерфейса
		array <uint8_t, 16> address;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Outward() noexcept :
		 filled(false), deadline(0),
		 mac{0}, address{0} {}
	} outward_t;

	/**
	 * @brief Записи об адресах с выходом во внешнюю сеть: [0] - IPv4, [1] - IPv6
	 *
	 * @note Записи общие для всего процесса: метод определения адреса доступен
	 *       и в отрыве от сетевого движка, поэтому обращаться к нему могут из
	 *       разных потоков одновременно.
	 *
	 */
	outward_t __awh_outward__[2];

	/**
	 * @brief Флаг одноразовой инициализации мьютекса записей об адресах
	 *
	 */
	once_flag __awh_init_once__;

	/**
	 * @brief Режим безопасности работы потоков
	 *
	 */
	event::mode_t __awh_thread_safety__ = event::mode_t::DISABLED;

	/**
	 * @brief Блокировка доступа к записям об адресах с выходом во внешнюю сеть
	 *
	 * @note Чтение записи выполняется под разделённой блокировкой, а запись -
	 *       под уникальной: обращений на чтение кратно больше, они приходятся
	 *       на каждое создание события, тогда как обновление происходит раз
	 *       за время жизни записи.
	 *
	 */
	static lock_state_t <std::shared_mutex> __awh_outward_mutex__;

	/**
	 * @brief Функция получения текущего значения монотонных часов
	 *
	 * @return текущее время в наносекундах
	 *
	 */
	uint64_t nanostamp() noexcept {
		// Выводим текущее значение монотонных часов
		return static_cast <uint64_t> (chrono::duration_cast <chrono::nanoseconds> (chrono::steady_clock::now().time_since_epoch()).count());
	}

	/**
	 * @brief Функция снятия встроенной зоны с адреса, полученного от системы
	 *
	 * @details Наследие KAME хранит зону устройства **внутри самого адреса** - во
	 *          втором и третьем октетах, - оставляя поле `sin6_scope_id` нулевым.
	 *          Список устройств отдаёт канальные адреса именно в таком виде:
	 *          устройство под первым номером выдаёт `FE80:1::…` вместо `FE80::…`.
	 *          Движок же хранит адреса очищенными, а зону - отдельным полем, и
	 *          сличение очищенного адреса с сырым не совпадает никогда
	 *
	 * @note Снимать зону приходится и здесь, и в разборе таблицы маршрутов: пути
	 *       эти разные, общего места у них нет
	 *
	 * @param addr адрес, полученный от системы
	 * @return     адрес без встроенной зоны
	 *
	 */
	struct in6_addr unscope(const struct in6_addr & addr) noexcept {
		// Адрес без встроенной зоны
		struct in6_addr result = addr;
		// Если адрес в зоне не нуждается, оставляем его как есть
		if(!IN6_IS_ADDR_LINKLOCAL(&result) && !IN6_IS_ADDR_MC_LINKLOCAL(&result))
			// Выводим адрес без изменений
			return result;
		// Обнуляем старший октет второго хекстета, снимая встроенную зону
		result.s6_addr[2] = 0;
		// Обнуляем младший октет второго хекстета, снимая встроенную зону
		result.s6_addr[3] = 0;
		// Выводим адрес без встроенной зоны
		return result;
	}
	/**
	 * @brief Функция проставления зоны адресу IPv6, которому она нужна
	 *
	 * @details Зона нужна адресам канальной связи (`FE80::/10`) и групповым адресам
	 *          связи (`FF02::/16`): без неё такой адрес неоднозначен - машина держит их
	 *          по одному на каждом устройстве разом, и без указания устройства не знает,
	 *          которым его достигать. Прочим адресам зона не нужна, и у них поле
	 *          остаётся нулевым
	 *
	 * @note Зона берётся из названия устройства, которым адрес и был получен: иного
	 *       источника у неё нет, а само по себе значение адреса устройства не называет
	 *
	 * @warning Обратная сторона переноса зоны в сторону системы. Там зона уходит в поле
	 *          `sin6_scope_id`, здесь же приходит от устройства, у которого адрес взят.
	 *          Пропусти её здесь - и адрес канальной связи, добытый движком, окажется
	 *          негодным к обмену, хотя система его выдала полным
	 *
	 * @param source объект источника сетевых адресов
	 *
	 */
	void scope(net::src_t & source) noexcept {
		// Если источник хранит адрес не того семейства
		if((source.ip == nullptr) || (source.ip->size != 16))
			// Выходим из функции, так-как проставлять зону нечему
			return;
		// Получаем объект адреса источника обмена
		net::addr_net_ipv6_t * ip = awh_cast <net::addr_net_ipv6_t *> (source.ip.get());
		// Получаем адрес источника обмена
		struct in6_addr addr{};
		// Выполняем копирование адреса источника обмена
		::memcpy(&addr, &ip->address[0], sizeof(addr));
		// Если адрес в зоне не нуждается, оставляем поле нулевым
		if(!IN6_IS_ADDR_LINKLOCAL(&addr) && !IN6_IS_ADDR_MC_LINKLOCAL(&addr))
			// Выходим из функции, так-как зона такому адресу не нужна
			return;
		// Зона адреса, снятая с самого адреса
		uint32_t zone = 0;
		/**
		 * Снимаем зону, встроенную в третий хекстет адреса
		 *
		 * @details Наследие KAME хранит зону **внутри самого адреса** - в третьем
		 *          хекстете, - оставляя поле `sin6_scope_id` нулевым. Так поступают
		 *          NetBSD и OpenBSD: устройство под первым номером отдаёт свой канальный
		 *          адрес записью `FE80:1::…` вместо `FE80::…`. Оставь мы хекстет как
		 *          есть - и адрес ушёл бы в сеть искажённым, а напечатан был бы записью,
		 *          которой у него нет
		 *
		 * @note FreeBSD и macOS той же родословной, но встраивания не делают: у них
		 *       хекстет нулевой, а зона лежит в `sin6_scope_id`. Оттого проверять
		 *       приходится сам хекстет, а не систему сборки: `__KAME__` объявлен у всех
		 *       четырёх, а поступают они по-разному
		 *
		 * @warning Обход таблицы маршрутов снимает встроенную зону сам, у себя по месту.
		 *          Здесь же разбор идёт по устройствам, и снимать её приходится заново -
		 *          пути эти разные
		 *
		 */
		if(ip->address[2] || ip->address[3]){
			// Снимаем встроенную в адрес зону
			zone = static_cast <uint32_t> ((static_cast <uint16_t> (ip->address[2]) << 8) | ip->address[3]);
			// Обнуляем третий хекстет, возвращая адресу его истинный вид
			ip->address[2] = 0;
			// Обнуляем младший октет третьего хекстета
			ip->address[3] = 0;
		}
		/**
		 * Зону, не встроенную в адрес, выводим из названия устройства, которым адрес и
		 * был получен: иного источника у неё нет, а само значение адреса устройства не
		 * называет
		 */
		if((zone == 0) && !source.iface.empty())
			// Получаем зону адреса номером устройства
			zone = ::if_nametoindex(source.iface.c_str());
		// Устанавливаем зону адреса
		ip->zone = zone;
	}

	/**
	 * @brief Функция восстановления адреса из запомненной записи
	 *
	 * @param source объект сетевых адресов текущей машины
	 * @return       признак того, что запись найдена и ещё не устарела
	 *
	 */
	bool restore(net::src_t & source) noexcept {
		// Если размер адреса не соответствует ни одному из семейств
		if((source.ip->size != 4) && (source.ip->size != 16))
			// Сообщаем, что адрес нужно определять заново
			return false;
		// Блокируем доступ к записям об адресах с выходом во внешнюю сеть на чтение
		const locker_t <std::shared_mutex> lock(::__awh_outward_mutex__, locker_t <std::shared_mutex>::mode_t::SHARED);
		// Получаем запись, соответствующую семейству адресов
		const outward_t & outward = __awh_outward__[source.ip->size == 16];
		// Если запись не заполнена или уже устарела
		if(!outward.filled || (::nanostamp() >= outward.deadline))
			// Сообщаем, что адрес нужно определять заново
			return false;
		// Устанавливаем название сетевого интерфейса
		source.iface = outward.iface;
		// Если адрес является IPv4
		if(source.ip->size == 4)
			// Устанавливаем адрес сетевого интерфейса
			::memcpy(&awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address, &outward.address[0], 4);
		// Если адрес является IPv6
		else ::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &outward.address[0], 16);
		// Проставляем зону адресу, которому она нужна
		::scope(source);
		// Устанавливаем MAC-адрес сетевого интерфейса
		::memcpy(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], &outward.mac[0], 6);
		// Сообщаем, что адрес восстановлен
		return true;
	}

	/**
	 * @brief Функция запоминания определённого адреса
	 *
	 * @param source объект сетевых адресов текущей машины
	 *
	 */
	void remember(const net::src_t & source) noexcept {
		// Если размер адреса не соответствует ни одному из семейств
		if((source.ip->size != 4) && (source.ip->size != 16))
			// Выходим из функции
			return;
		// Блокируем доступ к записям об адресах с выходом во внешнюю сеть на запись
		const locker_t <std::shared_mutex> lock(::__awh_outward_mutex__, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Получаем запись, соответствующую семейству адресов
		outward_t & outward = __awh_outward__[source.ip->size == 16];
		// Запоминаем название сетевого интерфейса
		outward.iface = source.iface;
		// Если адрес является IPv4
		if(source.ip->size == 4)
			// Запоминаем адрес сетевого интерфейса
			::memcpy(&outward.address[0], &awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address, 4);
		// Если адрес является IPv6
		else ::memcpy(&outward.address[0], &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], 16);
		// Запоминаем MAC-адрес сетевого интерфейса
		::memcpy(&outward.mac[0], &awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], 6);
		// Устанавливаем время устаревания записи
		outward.deadline = (::nanostamp() + __awh_outward_lifetime__);
		// Отмечаем запись заполненной
		outward.filled = true;
	}

	/**
	 * @brief Функция проверки годности адреса IPv6 источником обмена
	 *
	 * @details Годным считается любой адрес IPv6, кроме канального и петли. Канальный
	 *          (`FE80::/10`) источником не годится оттого, что без зоны устройства
	 *          бессмыслен, а зоны структура адреса не хранит; петля же (`::1`) не выходит
	 *          за пределы самой машины. Нулевой адрес негоден и подавно
	 *
	 * @param source объект источника сетевых адресов
	 * @return       результат проверки годности адреса
	 *
	 */
	bool routable(const net::src_t & source) noexcept {
		// Если источник хранит адрес не того семейства, годным он быть не может
		if((source.ip == nullptr) || (source.ip->size != 16))
			// Выводим результат
			return false;
		// Получаем адрес источника обмена
		struct in6_addr addr{};
		// Выполняем копирование адреса источника обмена
		::memcpy(&addr, &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], sizeof(addr));
		// Выводим годность адреса источником обмена
		return (!IN6_IS_ADDR_UNSPECIFIED(&addr) && !IN6_IS_ADDR_LINKLOCAL(&addr) && !IN6_IS_ADDR_LOOPBACK(&addr));
	}
	/**
	 * @brief Функция поиска годного адреса IPv6 среди устройств машины
	 *
	 * @details Перебирает устройства и берёт первый годный адрес, попутно запоминая его
	 *          устройство. Нужна там, где устройство пути по умолчанию годного адреса не
	 *          дало: на машине без пути IPv6 наружу путём оказывается туннель, держащий
	 *          один лишь канальный адрес
	 *
	 * @note Не нашлось ни одного годного - источник остаётся нетронутым: своего адреса
	 *       IPv6 у машины нет, и выдумывать его неоткуда
	 *
	 * @param source объект источника сетевых адресов
	 * @return       результат поиска годного адреса
	 *
	 */
	bool discover(net::src_t & source) noexcept {
		// Если источник хранит адрес не того семейства, искать нечего
		if((source.ip == nullptr) || (source.ip->size != 16))
			// Выводим результат
			return false;
		// Получаем список сетевых интерфейсов
		struct ifaddrs * ptr = nullptr;
		// Выполняем получение списка сетевых интерфейсов
		if(::getifaddrs(&ptr) != 0)
			// Выводим результат
			return false;
		// Результат работы функции
		bool result = false;
		/**
		 * Перебираем все сетевые интерфейсы
		 */
		for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
			// Пропускаем не IPv6-интерфейсы
			if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
				// Переходим к следующему сетевому интерфейсу
				continue;
			// Пропускаем выключенные интерфейсы
			if(!(ifa->ifa_flags & IFF_UP))
				// Переходим к следующему сетевому интерфейсу
				continue;
			// Получаем адрес сетевого интерфейса
			const struct sockaddr_in6 * addr = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
			// Пропускаем адреса, негодные источником обмена
			if(IN6_IS_ADDR_UNSPECIFIED(&addr->sin6_addr) || IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr) || IN6_IS_ADDR_LOOPBACK(&addr->sin6_addr))
				// Переходим к следующему сетевому интерфейсу
				continue;
			// Копируем найденный адрес в источник обмена
			::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &addr->sin6_addr, sizeof(addr->sin6_addr));
			// Проставляем зону адресу, которому она нужна
			::scope(source);
			// Запоминаем устройство, которому принадлежит найденный адрес
			if(ifa->ifa_name != nullptr)
				// Устанавливаем название сетевого интерфейса
				source.iface = ifa->ifa_name;
			// Отмечаем что годный адрес найден
			result = true;
			// Выходим из цикла
			break;
		}
		// Освобождаем память списка сетевых интерфейсов
		::freeifaddrs(ptr);
		// Выводим результат
		return result;
	}

	/**
	 * @brief Функция проверки источника на отсутствие примет для подбора устройства
	 *
	 * @param source объект источника сетевых адресов
	 * @return       результат проверки
	 *
	 * @details Источник считается лишённым примет, когда нулевые разом и адрес, и
	 *          аппаратный адрес. Запись эта означает не сбой определения, а согласие
	 *          отдать выбор устройства ядру: INADDR_ANY либо IN6ADDR_ANY
	 *
	 */
	bool zero(const net::src_t & source) noexcept {
		// Если адрес либо аппаратный адрес не заданы, примет для подбора нет
		if((source.ip == nullptr) || (source.mac == nullptr))
			// Выводим результат проверки
			return true;
		// Если аппаратный адрес задан, примета для подбора устройства есть
		if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], __awh_zero_mac__, 6) != 0)
			// Выводим результат проверки
			return false;
		/**
		 * Определяем тип адреса
		 */
		switch(source.ip->size){
			// Если адрес является IPv4
			case 4: return (awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address == 0);
			// Если адрес является IPv6
			case 16: return (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], __awh_zero_ipv6__, 16) == 0);
		}
		// Выводим результат проверки
		return true;
	}

	/**
	 * @brief Функция получения аппаратного адреса сетевого интерфейса
	 *
	 * @details Ищет канальную запись устройства в уже полученном списке, а не найдя её,
	 *          отыскивает собственный адрес устройства в кэше соседей
	 *
	 * @warning Sun Solaris НЕ ОТДАЁТ канальных записей в списке сетевых интерфейсов
	 *          вовсе - проверено пробой: из четырёх записей ни одной с семейством
	 *          AF_LINK. illumos их отдаёт, и здесь системы расходятся, причём в пользу
	 *          illumos. Оттого поиск и двухступенчат: без отступного пути аппаратный
	 *          адрес у Solaris не добывался бы никогда, а признаком этого служил бы
	 *          нуль - неотличимый от настоящего незаполненного адреса
	 *
	 * @note Собственный адрес машины лежит в кэше соседей постоянной записью -
	 *       «arp -a» показывает её с признаками SPLA, - и аппаратный адрес в ней тот
	 *       самый. Кэш этот модуль и без того снимает через mib2, так что нового пути
	 *       к ядру не заводится
	 *
	 * @note Прямой путь через DLPI - открыть устройство и запросить DL_PHYS_ADDR_REQ -
	 *       проверен и ОТВЕРГНУТ: открытие «/dev/net/<имя>» требует полномочий и
	 *       обычному приложению отвечает отказом в доступе. Требовать полномочий ради
	 *       чтения собственного аппаратного адреса нельзя
	 *
	 * @param list  список сетевых интерфейсов
	 * @param iface название искомого сетевого интерфейса
	 * @param mac   буфер аппаратного адреса на шесть октетов
	 * @return      результат поиска аппаратного адреса
	 *
	 */
	bool hwaddr(struct ifaddrs * list, const char * iface, uint8_t * mac) noexcept {
		// Если доводы поиска не заданы, искать нечего
		if((list == nullptr) || (iface == nullptr) || (mac == nullptr))
			// Выводим результат поиска
			return false;
		// Собственный адрес IPv4 сетевого интерфейса
		uint32_t own = 0;
		/**
		 * Перебираем все сетевые интерфейсы
		 */
		for(struct ifaddrs * ifa = list; ifa != nullptr; ifa = ifa->ifa_next){
			// Пропускаем не совпадающие имена интерфейсов
			if((ifa->ifa_name == nullptr) || (ifa->ifa_addr == nullptr) || (::strcmp(ifa->ifa_name, iface) != 0))
				// Переходим к интерфейсу следующему
				continue;
			// Запоминаем собственный адрес устройства для поиска по кэшу соседей
			if(ifa->ifa_addr->sa_family == AF_INET)
				// Запоминаем адрес устройства
				own = reinterpret_cast <const struct sockaddr_in *> (ifa->ifa_addr)->sin_addr.s_addr;
			// Если запись является канальной
			else if(ifa->ifa_addr->sa_family == AF_LINK) {
				// Получаем текущее значение аппаратного сетевого адреса
				struct sockaddr_dl * sdl = reinterpret_cast <struct sockaddr_dl *> (ifa->ifa_addr);
				// Если длина аппаратного адреса верна
				if(sdl->sdl_alen == 6){
					// Копируем аппаратный адрес в результат
					::memcpy(mac, LLADDR(sdl), 6);
					// Выводим результат поиска
					return true;
				}
			}
		}
		// Если собственного адреса устройства нет, искать в кэше соседей нечего
		if(own == 0)
			// Выводим результат поиска
			return false;
		// Записи кэша соседей IPv4
		vector <char> records;
		// Выполняем снятие кэша соседей IPv4
		if(!::mib::table(MIB2_IP, MIB2_IP_MEDIA, records))
			// Выводим результат поиска
			return false;
		// Получаем количество записей кэша соседей
		const size_t count = (records.size() / sizeof(mib2_ipNetToMediaEntry_t));
		// Получаем записи кэша соседей
		const mib2_ipNetToMediaEntry_t * entries = reinterpret_cast <const mib2_ipNetToMediaEntry_t *> (records.data());
		/**
		 * Переходим по всем записям кэша соседей
		 */
		for(size_t i = 0; i < count; i++){
			// Получаем текущую запись кэша соседей
			const mib2_ipNetToMediaEntry_t & entry = entries[i];
			// Если аппаратный адрес записи не заполнен, запись не годится
			if(entry.ipNetToMediaPhysAddress.o_length < 6)
				// Выполняем пропуск
				continue;
			// Если запись не о собственном адресе устройства, пропускаем
			if(own != static_cast <uint32_t> (entry.ipNetToMediaNetAddress))
				// Выполняем пропуск
				continue;
			// Копируем аппаратный адрес в результат
			::memcpy(mac, entry.ipNetToMediaPhysAddress.o_bytes, 6);
			// Выводим результат поиска
			return true;
		}
		// Выводим результат поиска
		return false;
	}

	/**
	 * @brief Функция вычисления контрольной суммы
	 *
	 * @param data   указатель на данные
	 * @param length длина данных
	 * @return       вычисленная контрольная сумма
	 *
	 */
	uint16_t checksum(const void * data, size_t length) noexcept {
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
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::eth::Network_Address::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
	// Активируем работу мьютекса блокировки потока при работе с записями об адресах
	::__awh_outward_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
}
/**
 * @brief Метод заполнения источника сетевых адресов по имени сетевого интерфейса
 *
 * @param source объект источника сетевых адресов
 *
 */
void awh::eth::Network_Address::fillSource(net::src_t & source) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если название сетевого интерфейса передано
		if(!source.iface.empty()){
			// Если MAC-адрес ещё не заполнен
			if((::strncmp("lo", source.iface.c_str(), 2) != 0) &&
			   (::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], __awh_zero_mac__, 6) == 0)){
				// Получаем список сетевых интерфейсов
				struct ifaddrs * ptr = nullptr;
				// Выполняем получение списка сетевых интерфейсов
				if(::getifaddrs(&ptr) != 0)
					// Выходим из функции
					return;
				// Выполняем поиск аппаратного адреса сетевого интерфейса
				const bool result = ::hwaddr(ptr, source.iface.c_str(), &awh_cast <net::addr_mac_t *> (source.mac.get())->address[0]);
				// Освобождаем память списка сетевых интерфейсов
				::freeifaddrs(ptr);
				/**
				 * Отсутствие аппаратного адреса заполнению адреса IP не помеха
				 *
				 * @details Аппаратного адреса нет у устройств без канального слоя -
				 *          туннелей, к примеру: `utun0` держит адрес IP, но канальной
				 *          записи не имеет вовсе. Прежде поиск аппаратного адреса на
				 *          таком устройстве не удавался и уводил из функции, не дав
				 *          заполнить и адрес IP, отчего своим адресом объявлялся нуль
				 *
				 * @warning Тем самым туннель оставался без своего адреса целиком, и
				 *          признак этого был обманчив: нуль неотличим от настоящего
				 *          неопределённого адреса
				 *
				 */
				(void) result;
			}
			/**
			 * Определяем тип адреса
			 */
			switch(source.ip->size){
				// Если адрес является IPv4
				case 4: {
					// Если IP-адрес ещё не заполнен
					if(awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address == 0){
						// Получаем список сетевых интерфейсов
						struct ifaddrs * ptr = nullptr;
						// Выполняем получение списка сетевых интерфейсов
						if(::getifaddrs(&ptr) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (source.ip->size)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						/**
						 * Перебираем все сетевые интерфейсы
						 */
						for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
							// Пропускаем не IPv4-интерфейсы
							if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET))
								// Пропускаем интерфейсы, которые не являются IPv4
								continue;
							// Если интерфейс не активен
							if(!(ifa->ifa_flags & IFF_UP))
								// Пропускаем неактивные интерфейсы
								continue;
							// Если имя интерфейса совпадает
							if(this->_fmk->compare(ifa->ifa_name, source.iface)){
								// Копируем IP-адрес в результат
								awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr)->sin_addr.s_addr;
								// Выходим из цикла
								break;
							}
						}
						// Освобождаем память от списка сетевых интерфейсов
						::freeifaddrs(ptr);
					}
				} break;
				// Если адрес является IPv6
				case 16: {
					// Если IPv6-адрес ещё не заполнен
					if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], __awh_zero_ipv6__, 16) == 0){
						// Получаем список сетевых интерфейсов
						struct ifaddrs * ptr = nullptr;
						// Выполняем получение списка сетевых интерфейсов
						if(::getifaddrs(&ptr) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (source.ip->size)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
							/**
							 * Канальный адрес берётся лишь за неимением иного
							 *
							 * @details Устройство держит по нескольку адресов IPv6 разом, и первым в
							 *          списке идёт канальный (`FE80::/10`). Источником он не годится:
							 *          без зоны устройства он бессмыслен, а зоны структура адреса не
							 *          хранит. Годным считается любой иной - глобальный либо местного
							 *          назначения
							 *
							 * @note Прежде брался первый попавшийся, и своим адресом объявлялся
							 *       канальный, хотя рядом на том же устройстве лежал годный
							 *
							 */
							// Канальный адрес устройства, отложенный на крайний случай
							const struct sockaddr_in6 * linked = nullptr;
						/**
						 * Перебираем все сетевые интерфейсы
						 */
						for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
							// Пропускаем не IPv6-интерфейсы
							if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
								// Пропускаем интерфейсы, которые не являются IPv6
								continue;
							// Пропускаем выключенные интерфейсы
							if(!(ifa->ifa_flags & IFF_UP))
								// Пропускаем неактивные интерфейсы
								continue;
							// Если имя интерфейса совпадает
							if(this->_fmk->compare(ifa->ifa_name, source.iface)){
								// Получаем адрес сетевого интерфейса
								const struct sockaddr_in6 * addr = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
								// Если адрес является канальным, откладываем его на крайний случай
								if(IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr)){
									// Запоминаем первый встреченный канальный адрес устройства
									if(linked == nullptr)
										// Откладываем канальный адрес до конца перебора
										linked = addr;
									// Переходим к следующему адресу устройства
									continue;
								}
								// Копируем IP-адрес в результат
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &addr->sin6_addr, sizeof(in6_addr));
								// Проставляем зону адресу, которому она нужна
								::scope(source);
								// Выходим из цикла
								break;
							}
						}
						/**
						 * Если годного адреса не нашлось, берём отложенный канальный
						 *
						 * @note Скобки здесь обязательны. Прежде их не было, и под условие
						 *       попадало одно лишь копирование адреса, тогда как проставление
						 *       зоны шло всегда - в том числе поверх годного адреса, найденного
						 *       перебором, которому зона не нужна. Отступ при этом обещал
						 *       обратное
						 *
						 */
						if((linked != nullptr) && (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], __awh_zero_ipv6__, 16) == 0)){
							// Копируем канальный адрес устройства в результат
							::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &linked->sin6_addr, sizeof(in6_addr));
							// Проставляем зону адресу, которому она нужна
							::scope(source);
						}
						// Освобождаем память списка сетевых интерфейсов
						::freeifaddrs(ptr);
					}
				} break;
			}
		// Загружаем данные по умолчанию
		} else this->fillSource(event::node_t::NONE, source);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (source.ip->size)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод заполнения источника сетевых адресов по заданной сети
 *
 * @param net    сетевой адрес подсети (IP-адрес в сетевом порядке байт)
 * @param source объект источника сетевых адресов
 *
 */
void awh::eth::Network_Address::fillSource(const net::addr_t * net, net::src_t & source) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Сличаем вид поданной сети с видом адреса источника
		 *
		 * @warning Разбор ведётся по виду адреса ИСТОЧНИКА, а поданная сеть
		 *          приводилась к тому же виду БЕЗ ПРОВЕРКИ. Подай источник IPv6, а
		 *          сеть IPv4 - и чтение шло по шестнадцати октетам там, где выделено
		 *          четыре: надзиратель `address` отвечает на это
		 *          `heap-buffer-overflow` в `ipv6PrefixEqual`. Оба довода объявлены
		 *          общим основанием `net::addr_t`, и запретить такую пару языком
		 *          нечем - вид её несёт поле `size`, по нему и сличаем
		 *
		 * @note Достижимо это не одним ворошителем: движок зовёт метод с адресом,
		 *       заданным потребителем (`fillSource(source->ip.get(), src)`), и вид
		 *       его с семейством узла разойтись волен
		 */
		if((net == nullptr) || (source.ip == nullptr) || (net->size != source.ip->size))
			// Заполнять источник несличимой парой нечем
			return;
		/**
		 * Определяем тип адреса
		 */
		switch(source.ip->size){
			// Если адрес является IPv4
			case 4: {
				// Получаем сетевой адрес подсети
				const net::addr_net_ipv4_t * network = awh_cast <const net::addr_net_ipv4_t *> (net);
				/**
				 * Блокируем работу ненужной проверки (пока непонятно что с этим делать)
				 * Проверка не работает на то, соответствует ли IP-адрес 192.168.7.249 маске 255.255.255.0
				 */
				#ifdef __AWH_DISABLED__
					// Проверка выравнивания сетевого адреса по маске
					const uint32_t mask = ((network->prefix == 0) ? 0 : (~((1U << (32 - network->prefix)) - 1)));
					// Если сетевой адрес не выровнен по маске
					if((htonl(network->address) & mask) != htonl(network->address)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Записываем ошибку в лог
							this->_log->debug(
								"Network address %u is not aligned to prefix %u", __PRETTY_FUNCTION__,
								make_tuple(htonl(network->address), static_cast <uint16_t> (network->prefix)),
								log_t::flag_t::WARNING, htonl(network->address), static_cast <uint16_t> (network->prefix)
							);
						/**
						 * Если режим отладки не включён
						 */
						#else
							// Записываем ошибку в лог
							this->_log->print("Network address %u is not aligned to prefix %u", log_t::flag_t::WARNING, htonl(network->address), static_cast <uint16_t> (network->prefix));
						#endif
						// Выходим из функции
						return;
					}
				#endif
				// Получаем список сетевых интерфейсов
				struct ifaddrs * ptr = nullptr;
				// Выполняем получение списка сетевых интерфейсов
				if(::getifaddrs(&ptr) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						/**
						 * Буфер временных данных для генерации IP-адреса
						 *
						 * @note Объявлен ВНУТРИ отладочной ветви намеренно: снаружи он в
						 *       сборке без отладки не употребляется вовсе, и gcc даёт
						 *       `-Wunused-variable`. Предупреждение верное - буфер и нужен
						 *       единственно записи в журнал отладки
						 */
						char buffer[INET_ADDRSTRLEN];
						// Записываем ошибку в лог
						this->_log->debug(
							"Unable to get list of network interfaces", __PRETTY_FUNCTION__,
							make_tuple(
								::inet_ntop(AF_INET, &network->address, buffer, sizeof(buffer)),
								static_cast <uint16_t> (network->prefix)
							), log_t::flag_t::WARNING
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
					#endif
					// Выходим из функции
					return;
				}
				// Устанавливаем префикс хостового адреса
				awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->prefix = ((network->prefix > 32) ? static_cast <uint8_t> (32) : network->prefix);
				/**
				 * Перебираем все сетевые интерфейсы
				 */
				for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
					// Пропускаем не IPv4-интерфейсы
					if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET))
						// Пропускаем интерфейсы, которые не являются IPv4
						continue;
					// Пропускаем loopback и down-интерфейсы (опционально)
					// if(ifa->ifa_flags & IFF_LOOPBACK) continue;
					// Если интерфейс не активен
					if(!(ifa->ifa_flags & IFF_UP))
						// Пропускаем неактивные интерфейсы
						continue;
					// Получаем IP-адрес интерфейса
					struct sockaddr_in * sin = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr);
					// Преобразуем IP-адрес в хостовый порядок
					const uint32_t ip = sin->sin_addr.s_addr;
					/**
					 * Связь без адреса источником быть не может
					 *
					 * @details Система выдаёт нулевой адрес всякой связи, у которой адреса ещё
					 *          нет, - у систем Sun такие связи заводятся распорядителем заранее
					 *          под туннели. Нулевой же адрес подсети принадлежит ЛЮБОЙ подсети,
					 *          и такая связь становилась источником для запроса по нулевому
					 *          адресу
					 *
					 * @warning Установлено 23.08.2026 на стендах Solaris и OpenIndiana: там
					 *          заведены `awh_tun0`...`awh_tun7` без адресов, и `EthSuiteTest`
					 *          получал у нулевого адреса имя связи вместо пустого
					 */
					if(ip == 0)
						// Пропускаем связь без адреса
						continue;
					// Проверяем принадлежность IP-адреса подсети
					if(this->isInSubnet(ntohl(ip), htonl(network->address), network->prefix)){
						// Устанавливаем название сетевого интерфейса
						source.iface = ifa->ifa_name;
						// Получаем MAC-адрес сетевого интерфейса
						this->fillSource(source);
						// Устанавливаем хост сети
						awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = ip;
						// Прерываем цикл поиска
						break;
					}
				}
				// Освобождаем память от списка сетевых интерфейсов
				::freeifaddrs(ptr);
			} break;
			// Если адрес является IPv6
			case 16: {
				// Получаем сетевой адрес подсети
				const net::addr_net_ipv6_t * network = awh_cast <const net::addr_net_ipv6_t *> (net);
				// Получаем список сетевых интерфейсов
				struct ifaddrs * ptr = nullptr;
				// Выполняем получение списка сетевых интерфейсов
				if(::getifaddrs(&ptr) != 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						/**
						 * Буфер временных данных для генерации IP-адреса
						 *
						 * @note Объявлен ВНУТРИ отладочной ветви намеренно: снаружи он в
						 *       сборке без отладки не употребляется вовсе, и gcc даёт
						 *       `-Wunused-variable`. Предупреждение верное - буфер и нужен
						 *       единственно записи в журнал отладки
						 */
						char buffer[INET_ADDRSTRLEN];
						// Записываем ошибку в лог
						this->_log->debug(
							"Unable to get list of network interfaces", __PRETTY_FUNCTION__,
							make_tuple(
								::inet_ntop(AF_INET6, &network->address[0], buffer, sizeof(buffer)),
								static_cast <uint16_t> (network->prefix)
							), log_t::flag_t::WARNING
						);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
					#endif
					// Выходим из функции
					return;
				}
				// Устанавливаем префикс хостового адреса
				awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->prefix = ((network->prefix > 128) ? static_cast <uint8_t> (128) : network->prefix);
				/**
				 * Перебираем все сетевые интерфейсы
				 */
				for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
					// Пропускаем не IPv6-интерфейсы
					if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
						// Пропускаем интерфейсы, которые не являются IPv6
						continue;
					// Пропускаем выключенные интерфейсы
					if(!(ifa->ifa_flags & IFF_UP))
						// Пропускаем неактивные интерфейсы
						continue;
					// Получаем указатель на IPv6-адрес
					struct sockaddr_in6 * sin = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
					/**
					 * Получаем IP-адрес устройства со снятой встроенной зоной
					 *
					 * @note Снятие обязательно. Список устройств отдаёт канальные адреса
					 *       со встроенной зоной во втором хекстете, а движок хранит их
					 *       очищенными: устройство под первым номером выдаёт системе
					 *       `FE80:1::…`, тогда как у нас лежит `FE80::…`. Сличение
					 *       очищенного адреса с сырым не совпадало никогда, и устройство
					 *       по своему же канальному адресу оказывалось неопределимым
					 *
					 */
					const in6_addr ip = ::unscope(sin->sin6_addr);
					// Признак связи без адреса IPv6
					bool empty = true;
					/**
					 * Проходим по всем октетам адреса связи
					 */
					for(uint8_t i = 0; (i < 16) && empty; i++)
						// Снимаем признак у первого ненулевого октета
						empty = (ip.s6_addr[i] == 0);
					// Связь без адреса источником быть не может, довод тот же, что и у IPv4 выше
					if(empty)
						// Пропускаем связь без адреса
						continue;
					// Проверяем принадлежность IP-адреса подсети
					if(this->ipv6PrefixEqual(ip.s6_addr, &network->address[0], network->prefix)){
						// Устанавливаем название сетевого интерфейса
						source.iface = ifa->ifa_name;
						// Получаем MAC-адрес сетевого интерфейса
						this->fillSource(source);
						// Хост: просто копируем найденный адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], ip.s6_addr, sizeof(ip.s6_addr));
						// Проставляем зону адресу, которому она нужна
						::scope(source);
						// Прерываем цикл поиска
						break;
					}
				}
				// Освобождаем память списка сетевых интерфейсов
				::freeifaddrs(ptr);
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод заполнения источника сетевых адресов
 *
 * @param node   тип узла события
 * @param source объект источника сетевых адресов
 *
 */
void awh::eth::Network_Address::fillSource(const event::node_t node, net::src_t & source) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * Определяем тип узла
		 */
		switch(static_cast <uint8_t> (node)){
			// Если тип узла не установлен
			case static_cast <uint8_t> (event::node_t::NONE): {
				/**
				 * @note Определение адреса, с которого доступна внешняя сеть,
				 *       стоит создания сокета, подключения к нему, запроса
				 *       имени сокета и его закрытия, а следом ещё и обхода
				 *       списка сетевых интерфейсов. При массовом создании
				 *       событий эта работа повторялась бы на каждое событие,
				 *       поэтому результат запоминается на время жизни записи.
				 *
				 */
				// Если адрес удалось восстановить из запомненной записи
				if(::restore(source))
					// Выходим из функции
					return;
				/**
				 * Определяем тип адреса
				 */
				switch(source.ip->size){
					// Если адрес является IPv4
					case 4: {
						/**
						 * Исходящий адрес определяется подбором маршрута по умолчанию:
						 * у ядра спрашивается устройство, которым машина ходит наружу,
						 * и берётся его адрес
						 *
						 * @details Прежде адрес добывался подключением UDP-сокета к
						 *          случайно выбранному из шести зашитых серверов имён и
						 *          запросом имени сокета. Приём этот работал, пока путь
						 *          наружу один: на машине с раздельным туннелем все шесть
						 *          адресов уходили в туннель, и своим объявлялся адрес
						 *          туннеля, отчего отправка соседу по своей же сети
						 *          отвечала отказом в маршруте. Сверх того определение
						 *          своего адреса требовало выхода в интернет и зависело
						 *          от случая при выборе сервера
						 */
						// Объект маршрута для подбора устройства
						gateway_t::route_t route{};
						// Выполняем инициализацию адреса шлюза маршрута
						route.gateway = ::make_unique <net::addr_net_ipv4_t> ();
						// Выполняем инициализацию адреса назначения маршрута
						route.destination = ::make_unique <net::addr_net_ipv4_t> ();
						// Если объект управления шлюзами задан и маршрут по умолчанию получен
						if((this->_gateway != nullptr) && this->_gateway->get(route) && !route.ifname.empty()){
							// Устанавливаем название сетевого интерфейса
							source.iface = ::move(route.ifname);
							// Получаем адрес и MAC-адрес сетевого интерфейса
							this->fillSource(source);
							// Если адрес сетевого интерфейса получен
							if(awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address > 0)
								// Запоминаем определённый адрес до устаревания записи
								::remember(source);
						}
					} break;
					// Если адрес является IPv6
					case 16: {
						/**
						 * Исходящий адрес IPv6 определяется тем же подбором маршрута по
						 * умолчанию, что и адрес IPv4. Довод к отказу от подключения к
						 * серверам имён приведён у ветви IPv4
						 */
						// Объект маршрута для подбора устройства
						gateway_t::route_t route{};
						// Выполняем инициализацию адреса шлюза маршрута
						route.gateway = ::make_unique <net::addr_net_ipv6_t> ();
						// Выполняем инициализацию адреса назначения маршрута
						route.destination = ::make_unique <net::addr_net_ipv6_t> ();
						// Если объект управления шлюзами задан и маршрут по умолчанию получен
						if((this->_gateway != nullptr) && this->_gateway->get(route) && !route.ifname.empty()){
							// Устанавливаем название сетевого интерфейса
							source.iface = ::move(route.ifname);
							// Получаем адрес и MAC-адрес сетевого интерфейса
							this->fillSource(source);
						}
						/**
						 * Устройство пути по умолчанию годного адреса дать не смогло, и
						 * годный отыскивается среди прочих устройств машины
						 *
						 * @details Так выходит на машине без пути IPv6 наружу: путём по
						 *          умолчанию оказывается туннель, а он держит один лишь
						 *          канальный адрес. Канальный же источником не годится -
						 *          без зоны устройства он бессмыслен, - и объявлять его
						 *          своим значит обрекать собеседника на отказ
						 *
						 * @note Разбор идёт в том же порядке, что и у самого устройства:
						 *       годным считается любой адрес, кроме канального и петли.
						 *       Не нашлось и такого - остаётся то, что дало устройство
						 *       пути, вплоть до нуля: своего адреса IPv6 у машины нет
						 *
						 */
						if(!::routable(source))
							// Выполняем поиск годного адреса среди прочих устройств
							::discover(source);
						// Если адрес сетевого интерфейса получен
						if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], __awh_zero_ipv6__, 16) != 0)
							// Запоминаем определённый адрес до устаревания записи
							::remember(source);
					} break;
				}
			} break;
			// Если тип узла является одноранговым узлом
			case static_cast <uint8_t> (event::node_t::PEER): {
				/**
				 * Определяем тип адреса
				 */
				switch(source.ip->size){
					// Если адрес является IPv4
					case 4: {
						// Записи кэша соседей IPv4
						vector <char> records;
						// Выполняем снятие кэша соседей IPv4
						if(!::mib::table(MIB2_IP, MIB2_IP_MEDIA, records)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Neighbour cache retrieval", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Neighbour cache retrieval", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Получаем количество записей кэша соседей
						const size_t count = (records.size() / sizeof(mib2_ipNetToMediaEntry_t));
						// Получаем записи кэша соседей
						const mib2_ipNetToMediaEntry_t * entries = reinterpret_cast <const mib2_ipNetToMediaEntry_t *> (records.data());
						// Получаем числовое значение IP-адреса
						const uint32_t addr = awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address;
						/**
						 * Переходим по всем записям кэша соседей
						 */
						for(size_t i = 0; i < count; i++){
							// Получаем текущую запись кэша соседей
							const mib2_ipNetToMediaEntry_t & entry = entries[i];
							// Если аппаратный адрес записи не заполнен, запись не годится
							if(entry.ipNetToMediaPhysAddress.o_length < 6)
								// Выполняем пропуск
								continue;
							// Получаем аппаратный адрес записи
							const uint8_t * ptr = reinterpret_cast <const uint8_t *> (entry.ipNetToMediaPhysAddress.o_bytes);
							// Если IP-адрес установлен
							if(addr > 0){
								// Если искомый IP-адрес не совпадает, пропускаем
								if(addr != static_cast <uint32_t> (entry.ipNetToMediaNetAddress))
									// Выполняем пропуск
									continue;
								// Копируем MAC-адрес в результат
								::memcpy(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], ptr, 6);
								// Выходим из цикла
								break;
							// Если IP-адрес не установлен, ищем его по аппаратному адресу
							} else if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], ptr, 6) == 0) {
								// Копируем IP-адрес в результат
								awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = static_cast <uint32_t> (entry.ipNetToMediaNetAddress);
								// Выходим из цикла
								break;
							}
						}
					} break;
					// Если адрес является IPv6
					case 16: {
						// Записи кэша соседей IPv6
						vector <char> records;
						/**
						 * Выполняем снятие кэша соседей IPv6
						 *
						 * @note Кэш этот к ARP отношения не имеет - соседи IPv6 разыскиваются
						 *       протоколом NDP, - но отдаётся он тем же самым запросом, лишь
						 *       иной парой уровня и названия. Оттого точечного запроса вида
						 *       SIOCGXARP для IPv6 и не существует: его описание прямо требует
						 *       AF_INET
						 */
						if(!::mib::table(MIB2_IP6, MIB2_IP6_MEDIA, records)){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Neighbour cache retrieval", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Neighbour cache retrieval", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Получаем количество записей кэша соседей
						const size_t count = (records.size() / sizeof(mib2_ipv6NetToMediaEntry_t));
						// Получаем записи кэша соседей
						const mib2_ipv6NetToMediaEntry_t * entries = reinterpret_cast <const mib2_ipv6NetToMediaEntry_t *> (records.data());
						// Получаем искомый IP-адрес
						const auto * target = awh_cast <net::addr_net_ipv6_t *> (source.ip.get());
						// Признак того, что искомый IP-адрес установлен
						bool exist = false;
						/**
						 * Определяем, установлен ли искомый IP-адрес
						 */
						for(uint8_t i = 0; i < 16; i++){
							// Если очередной октет адреса заполнен
							if(reinterpret_cast <const uint8_t *> (&target->address[0])[i] != 0){
								// Отмечаем, что искомый IP-адрес установлен
								exist = true;
								// Выходим из цикла
								break;
							}
						}
						/**
						 * Переходим по всем записям кэша соседей
						 */
						for(size_t i = 0; i < count; i++){
							// Получаем текущую запись кэша соседей
							const mib2_ipv6NetToMediaEntry_t & entry = entries[i];
							// Если аппаратный адрес записи не заполнен, запись не годится
							if(entry.ipv6NetToMediaPhysAddress.o_length < 6)
								// Выполняем пропуск
								continue;
							// Получаем аппаратный адрес записи
							const uint8_t * ptr = reinterpret_cast <const uint8_t *> (entry.ipv6NetToMediaPhysAddress.o_bytes);
							// Если IP-адрес установлен
							if(exist){
								// Если искомый IP-адрес не совпадает, пропускаем
								if(::memcmp(&target->address[0], &entry.ipv6NetToMediaNetAddress, 16) != 0)
									// Выполняем пропуск
									continue;
								// Копируем MAC-адрес в результат
								::memcpy(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], ptr, 6);
								// Выходим из цикла
								break;
							// Если IP-адрес не установлен, ищем его по аппаратному адресу
							} else if(::memcmp(&awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], ptr, 6) == 0) {
								// Копируем IP-адрес в результат
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &entry.ipv6NetToMediaNetAddress, 16);
								// Выходим из цикла
								break;
							}
						}
					} break;
				}
			} break;
			// Если узел является клиентом
			case static_cast <uint8_t> (event::node_t::CLIENT):
			// Если узел является сервером
			case static_cast <uint8_t> (event::node_t::SERVER): {
				/**
				 * Подбор устройства ведётся либо по адресу, либо по аппаратному адресу:
				 * заданный адрес ищется среди адресов устройств, а при нулевом адресе
				 * устройство отбирается совпадением MAC-адреса
				 *
				 * @note Нулевые адрес и MAC-адрес разом означают не «устройство не
				 *       найдено», а осознанный отказ от выбора: запись читается как
				 *       INADDR_ANY либо IN6ADDR_ANY, устройство отбирает ядро при
				 *       привязке сокета. Искать тут нечего, и обход списка устройств
				 *       завершился бы ничем, поэтому он не начинается
				 *
				 */
				if(::zero(source))
					// Выходим из функции, оставляя выбор устройства ядру
					return;
				/**
				 * Определяем тип адреса
				 */
				switch(source.ip->size){
					// Если адрес является IPv4
					case 4: {
						// Получаем список сетевых интерфейсов
						struct ifaddrs * ptr = nullptr;
						// Выполняем получение списка сетевых интерфейсов
						if(::getifaddrs(&ptr) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Получаем числовое значение IP-адреса
						const uint32_t addr = awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address;
						/**
						 * Перебираем все сетевые интерфейсы
						 */
						for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
							// Пропускаем не IPv4-интерфейсы
							if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET))
								// Пропускаем интерфейсы, которые не являются IPv4
								continue;
							// Если интерфейс не активен
							if(!(ifa->ifa_flags & IFF_UP))
								// Пропускаем неактивные интерфейсы
								continue;
							// Получаем IP-адрес интерфейса
							struct sockaddr_in * sin = reinterpret_cast <struct sockaddr_in *> (ifa->ifa_addr);
							// Если IP-адрес установлен
							if(addr > 0){
								// Если IP-адрес совпадает с указанным IP-адресом
								if(addr == sin->sin_addr.s_addr){
									// Устанавливаем название сетевого интерфейса
									source.iface = ifa->ifa_name;
									// Получаем MAC-адрес сетевого интерфейса
									this->fillSource(source);
									// Прерываем цикл поиска
									break;
								}
							// Если IP-адрес не установлен
							} else {
								// Буфер MAC-адреса текущего интерфейса
								uint8_t mac[6] = {0};
								/**
								 * Извлекаем аппаратный адрес устройства из уже полученного списка
								 *
								 * @note Посредник обходится без повторного обращения к списку
								 *       устройств, а не найдя канальной записи - её у Sun Solaris
								 *       не бывает вовсе, - отыскивает адрес в кэше соседей
								 */
								const bool found = ::hwaddr(ptr, ifa->ifa_name, mac);
								// Сравниваем MAC-адреса
								if(found && (::memcmp(mac, &awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], 6) == 0)){
									// Устанавливаем название сетевого интерфейса
									source.iface = ifa->ifa_name;
									// Копируем IP-адрес в результат
									awh_cast <net::addr_net_ipv4_t *> (source.ip.get())->address = sin->sin_addr.s_addr;
									// Выходим из цикла
									break;
								}
							}
						}
						// Освобождаем память от списка сетевых интерфейсов
						::freeifaddrs(ptr);
					} break;
					// Если адрес является IPv6
					case 16: {
						// Получаем список сетевых интерфейсов
						struct ifaddrs * ptr = nullptr;
						// Выполняем получение списка сетевых интерфейсов
						if(::getifaddrs(&ptr) != 0){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Unable to get list of network interfaces", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node)), log_t::flag_t::WARNING);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Unable to get list of network interfaces", log_t::flag_t::WARNING);
							#endif
							// Выходим из функции
							return;
						}
						// Создаём объект подключения
						struct sockaddr_in6 addr{};
						// Копируем IP-адрес в структуру подключения
						::memcpy(&addr.sin6_addr, &awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], sizeof(addr.sin6_addr));
						/**
						 * Перебираем все сетевые интерфейсы
						 */
						for(struct ifaddrs * ifa = ptr; ifa != nullptr; ifa = ifa->ifa_next){
							// Пропускаем не IPv6-интерфейсы
							if((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET6))
								// Пропускаем интерфейсы, которые не являются IPv6
								continue;
							// Пропускаем выключенные интерфейсы
							if(!(ifa->ifa_flags & IFF_UP))
								// Пропускаем неактивные интерфейсы
								continue;
							// Получаем указатель на IPv6-адрес
							struct sockaddr_in6 * sin = reinterpret_cast <struct sockaddr_in6 *> (ifa->ifa_addr);
							/**
							 * Получаем адрес устройства со снятой встроенной зоной
							 *
							 * @note Снятие обязательно, как и в разборе по заданной сети:
							 *       список устройств отдаёт канальные адреса со встроенной
							 *       зоной, а движок хранит их очищенными, и сличение сырого
							 *       адреса с очищенным не совпадает никогда
							 *
							 */
							const struct in6_addr device = ::unscope(sin->sin6_addr);
							// Если IP-адрес установлен
							if(::memcmp(&addr.sin6_addr, __awh_zero_ipv6__, 16) != 0){
								// Если IP-адрес совпадает с указанным IP-адресом
								if(IN6_ARE_ADDR_EQUAL(&addr.sin6_addr, &device)){
									// Устанавливаем название сетевого интерфейса
									source.iface = ifa->ifa_name;
									// Получаем MAC-адрес сетевого интерфейса
									this->fillSource(source);
									// Прерываем цикл поиска
									break;
								}
							// Если IP-адрес не установлен
							} else {
								// Буфер MAC-адреса текущего интерфейса
								uint8_t mac[6] = {0};
								/**
								 * Извлекаем аппаратный адрес устройства из уже полученного списка
								 *
								 * @note Посредник обходится без повторного обращения к списку
								 *       устройств, а не найдя канальной записи - её у Sun Solaris
								 *       не бывает вовсе, - отыскивает адрес в кэше соседей
								 */
								const bool found = ::hwaddr(ptr, ifa->ifa_name, mac);
								// Сравниваем MAC-адреса
								if(found && (::memcmp(mac, &awh_cast <net::addr_mac_t *> (source.mac.get())->address[0], 6) == 0)){
									// Устанавливаем название сетевого интерфейса
									source.iface = ifa->ifa_name;
									/**
									 * Адрес берётся разбором по имени устройства, а не отсюда
									 *
									 * @details Устройство держит по нескольку адресов IPv6 разом, и
									 *          обход выдаёт их подряд: первым идёт канальный, который
									 *          источником не годится. Разбор по имени эти адреса уже
									 *          различает, и повторять здесь тот же выбор значило бы
									 *          завести второе место, которое разойдётся с первым при
									 *          первой же правке
									 *
									 * @note Разбор по имени ничего не дал - берётся то, что выдал
									 *       обход, как и прежде
									 *
									 */
									this->fillSource(source);
									// Если адрес сетевого интерфейса разбором по имени не получен
									if(::memcmp(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], __awh_zero_ipv6__, 16) == 0){
										// Копируем IP-адрес в результат
										::memcpy(&awh_cast <net::addr_net_ipv6_t *> (source.ip.get())->address[0], &sin->sin6_addr, sizeof(in6_addr));
									}
									/**
									 * Проставляем зону адресу, которому она нужна
									 *
									 * @warning Обращение это БЕЗУСЛОВНО, и прежде отступ говорил обратное:
									 *          стояло оно на уровне тела `if`, а исполнялось всегда, отчего
									 *          gcc и давал `-Wmisleading-indentation`. Отступ приведён к
									 *          исполнению, а не наоборот: у наречия BSD в том же месте
									 *          обращение тоже безусловно
									 *
									 * @note Дефекта поведения не было: зона выводится из САМОГО адреса и
									 *       названия устройства, обращение идемпотентно, а адресу, зоны не
									 *       требующему, оно ничего не проставляет. Врал отступ, не код
									 */
									::scope(source);
									// Выходим из цикла
									break;
								}
							}
						}
						// Освобождаем память списка сетевых интерфейсов
						::freeifaddrs(ptr);
					} break;
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (node), static_cast <uint16_t> (source.ip->size)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод проверки принадлежности IP-адреса подсети
 *
 * @param ip     проверяемый IP-адрес в хостовом порядке
 * @param net    сетевой адрес подсети в хостовом порядке
 * @param prefix префикс подсети
 * @return       результат проверки
 *
 */
bool awh::eth::Network_Address::isInSubnet(const uint32_t ip, const uint32_t net, const uint8_t prefix) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если префикс равен нулю, то любой IP-адрес принадлежит подсети
		if(prefix == 0)
			// Возвращаем результат проверки
			return true;
		/**
		 * Префикс шире самого адреса означает один-единственный узел
		 *
		 * @warning Проверки на это не было вовсе, и префикс свыше тридцати двух давал
		 *          сдвиг на ОТРИЦАТЕЛЬНОЕ число разрядов - поведение неопределённое,
		 *          и надзиратель `undefined` отвечает на него прямо: «shift exponent
		 *          -2 is negative». Довод приходит снаружи полем `uint8_t`, и запретить
		 *          такое значение типом нечем
		 */
		if(prefix >= 32)
			// Сличаем адреса целиком: сеть из одного узла
			return (ip == net);
		// Вычисляем маску подсети
		uint32_t mask = (~((1U << (32 - prefix)) - 1));
		// Проверяем принадлежность IP-адреса подсети
		return ((ip & mask) == (net & mask));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(ip, net, static_cast <uint16_t> (prefix)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод сравнения двух IPv6-адресов по префиксу (в битах)
 *
 * @param first  Первый IPv6-адрес
 * @param second Второй IPv6-адрес
 * @param length Длина префикса в битах
 * @return       Результат сравнения
 *
 */
bool awh::eth::Network_Address::ipv6PrefixEqual(const uint8_t * first, const uint8_t * second, const uint8_t length) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если длина префикса равна нулю, адреса считаются равными
		if(length == 0)
			// Возвращаем результат сравнения
			return true;
		/**
		 * Ограничиваем длину префикса разрядностью адреса IPv6
		 *
		 * @warning Метод берёт ГОЛЫЕ указатели и длину полем `uint8_t`, а длина эта
		 *          доходит до 255 при отведённых адресу шестнадцати октетах. Без края
		 *          `memcmp` читает вдвое больше положенного, а взятие последнего
		 *          октета бьёт мимо вовсе - надзиратель `address` отвечает на это
		 *          `heap-buffer-overflow`
		 *
		 * @note Край стоит ИМЕННО ЗДЕСЬ, а не у вызывающих: метод объявлен открытым, и
		 *       потребитель фреймворка волен позвать его напрямую с любой длиной.
		 *       Оберегать свои края - дело самого метода, а не всякого, кто его зовёт
		 *
		 * @note Прежде край держался у единственного вызывающего правкой поля `prefix`
		 *       через `const_cast` - то есть изменением ЧУЖОГО объекта, помеченного
		 *       постоянным. Приём этот снят: вызывающий вправе не ждать, что его
		 *       объект после запроса изменится
		 */
		const uint8_t bits = ((length > 128) ? static_cast <uint8_t> (128) : length);
		// Вычисляем количество полных байтов и оставшихся битов
		size_t fullBytes = (bits / 8);
		// Вычисляем количество битов в последнем байте
		uint8_t bitsInLast = (bits % 8);
		// Сравниваем полные байты
		if(::memcmp(first, second, fullBytes) != 0)
			// Возвращаем результат сравнения
			return false;
		// Если нет оставшихся битов, адреса равны
		if(bitsInLast == 0)
			// Возвращаем результат сравнения
			return true;
		// Сравниваем оставшиеся биты в последнем байте
		const uint8_t mask = ((0xFF << (8 - bitsInLast)) & 0xFF);
		// Возвращаем результат сравнения
		return ((first[fullBytes] & mask) == (second[fullBytes] & mask));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(first, second, static_cast <uint16_t> (length)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
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
 */
uint16_t awh::eth::Network_Address::checksum(const event::family_t family, const event::protocol_t protocol, const void * src, const void * dst, const void * transport, const size_t length) const noexcept {
	// Переменная результата
	uint16_t result = 0;
	// Проверяем корректность входных данных
	if((src != nullptr) && (dst != nullptr) && (transport != nullptr) && (length > 0)){
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
			 * Определяем протокол
			 */
			switch(static_cast <uint8_t> (protocol)){
				// Если протокол определён как TCP
				case static_cast <uint8_t> (event::protocol_t::TCP):
					// Запоминаем смещение поля контрольной суммы в TCP-заголовке
					checksumOffset = offsetof(struct tcphdr, th_sum);
				break;
				// Если протокол определён как UDP
				case static_cast <uint8_t> (event::protocol_t::UDP):
					// Запоминаем смещение поля контрольной суммы в UDP-заголовке
					checksumOffset = offsetof(struct udphdr, uh_sum);
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
					// Выходим из функции
					return result;
				}
			}
			/**
			 * Определяем семейство события
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
						uint8_t zero;    // Зарезервировано, должно быть равно 0
						uint8_t proto;   // Протокол транспортного уровня
						uint16_t length; // Длина транспортного уровня
					} hdr;
					// Устанавливаем ноль в зарезервированное поле
					hdr.zero = 0;
					/**
					 * Определяем протокол
					 */
					switch(static_cast <uint8_t> (protocol)){
						// Если протокол определён как TCP
						case static_cast <uint8_t> (event::protocol_t::TCP):
							// Устанавливаем протокол транспортного уровня
							hdr.proto = IPPROTO_TCP;
						break;
						// Если протокол определён как UDP
						case static_cast <uint8_t> (event::protocol_t::UDP):
							// Устанавливаем протокол транспортного уровня
							hdr.proto = IPPROTO_UDP;
						break;
					}
					// Устанавливаем IP-адреса источника и назначения
					hdr.src = (* reinterpret_cast <const uint32_t *> (src));
					hdr.dst = (* reinterpret_cast <const uint32_t *> (dst));
					// Устанавливаем длину транспортного уровня
					hdr.length = htons(static_cast <uint16_t> (length));
					// Вычисляем размеры псевдозаголовка
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
					// IPv6
					struct {
						// IP-адрес источника
						struct in6_addr src;
						// IP-адрес назначения
						struct in6_addr dst;
						// Длина транспортного уровня
						uint32_t length;
						// Зарезервировано, должно быть равно 0
						uint8_t zero[3];
						// Следующий заголовок
						uint8_t next_hdr;
					} hdr;
					/**
					 * Определяем протокол
					 */
					switch(static_cast <uint8_t> (protocol)){
						// Если протокол определён как TCP
						case static_cast <uint8_t> (event::protocol_t::TCP):
							// Устанавливаем протокол транспортного уровня
							hdr.next_hdr = IPPROTO_TCP;
						break;
						// Если протокол определён как UDP
						case static_cast <uint8_t> (event::protocol_t::UDP):
							// Устанавливаем протокол транспортного уровня
							hdr.next_hdr = IPPROTO_UDP;
						break;
					}
					// Устанавливаем нули в зарезервированное поле
					hdr.zero[0] = hdr.zero[1] = hdr.zero[2] = 0;
					// Устанавливаем IP-адреса источника и назначения
					hdr.src = (* reinterpret_cast <const struct in6_addr *> (src));
					hdr.dst = (* reinterpret_cast <const struct in6_addr *> (dst));
					// Устанавливаем длину транспортного уровня (да, 32-bit, но значение 16-bit)
					hdr.length = htonl(static_cast <uint32_t> (length));
					// Вычисляем размеры псевдозаголовка
					pseudoSize = sizeof(hdr);
					// Вычисляем общий размер данных
					totalSize = (pseudoSize + length);
					// Выделяем память под псевдозаголовок
					pseudo = make_unique <uint8_t []> (totalSize);
					// Формируем псевдозаголовок
					::memcpy(pseudo.get(), &hdr, pseudoSize);
				} break;
			}
			// Если семейство протокола не поддержано, псевдозаголовок не сформирован
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
				// Выходим из функции
				return result;
			}
			// Копируем транспортный заголовок + данные
			::memcpy(pseudo.get() + pseudoSize, transport, length);
			// Обнуляем контрольную сумму в копии транспортного заголовка (исходный буфер не модифицируется)
			if((checksumOffset + sizeof(uint16_t)) <= length)
				// Зануляем поле контрольной суммы в копии
				::memset(pseudo.get() + pseudoSize + checksumOffset, 0, sizeof(uint16_t));
			// Вычисляем контрольную сумму
			result = ::checksum(pseudo.get(), totalSize);
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
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 *
 */
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
 _iface(fmk, log), _gateway(nullptr), _fmk(fmk), _log(log) {
	/**
	 * Выполняем одноразовую настройку блокировки для всех экземпляров класса
	 */
	std::call_once(::__awh_init_once__, []() noexcept {
		// Активируем работу мьютекса блокировки потока при работе с записями об адресах
		::__awh_outward_mutex__.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	});
}
/**
 * @brief Деструктор
 *
 */
awh::eth::Network_Address::~Network_Address() noexcept {}
