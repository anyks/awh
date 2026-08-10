/**
 * @file: socket.hpp
 * @date: 2026-01-28
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля низкоуровневой работы с сокетами — класс eth::Socket для установки опций сокета:
 *        неблокирующего режима, таймаутов, размеров буферов, keep-alive, TCP_NODELAY, TOS/DSCP,
 *        multicast и параметров переиспользования адреса
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SOCKET__
#define __AWH_SOCKET__

/**
 * Наши модули
 */
#include "../net.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён Ethernet протоколов
	 *
	 */
	namespace eth {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс для работы с сокетами
		 *
		 * @details Собирает в одном месте настройки сокета - те самые свойства,
		 * что задаются вызовом с длинным списком доводов и разнятся от системы
		 * к системе. Здесь они прикрыты понятными именами, а различия систем
		 * спрятаны внутрь
		 *
		 * Настройки делятся на несколько groups: пределы ожидания и размеры
		 * накопителей, пометки качества обслуживания, пределы числа переходов,
		 * участие в рассылке на группу, обнаружение наибольшего размера пакета
		 *
		 * @note Поддержка настроек **зависит от системы**: часть из них есть не
		 * всюду, и отрицательный итог нередко означает не сбой, а отсутствие
		 * такой возможности. Итог проверять следует всегда
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Socket {
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @warning Настройка эта **общая на весь процесс**, а не своя у каждого
				 * объекта. По умолчанию защита выключена - в расчёте на однопоточную
				 * работу, - и включать её следует до запуска второго потока
				 *
				 * @param mode флаг режима безопасности потоков
				 *
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод получения кода ошибки
				 *
				 * @details Забирает у сокета код последней ошибки, попутно его сбрасывая
				 *
				 * @note Нужно это при неблокирующем подключении: сам вызов
				 * подключения там возвращается сразу, а удалось оно или нет,
				 * выясняется потом - как раз этим способом
				 *
				 * @param sock сетевой сокет
				 * @return     код ошибки на сокете если присутствует
				 *
				 */
				int32_t getError(const net::socket_t sock) const noexcept;
			public:
				/**
				 * @brief Метод получения таймаута сокета
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @return      время таймаута в миллисекундах
				 *
				 */
				uint32_t getTimeout(const net::socket_t sock, const net::socket_event_t event) const noexcept;
				/**
				 * @brief Метод установки таймаута сокета
				 *
				 * @details Задаёт, сколько ждать при чтении или отправке, прежде чем
				 * прервать вызов
				 *
				 * @warning Действует лишь на **блокирующие** сокеты. У неблокирующих
				 * вызовы и без того возвращаются сразу, а ожиданием ведает цикл
				 * событий - и там пределы задаются движком, а не здесь
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @param msec  время таймаута в миллисекундах
				 * @return      результат установки таймаута
				 *
				 */
				bool setTimeout(const net::socket_t sock, const net::socket_event_t event, const uint32_t msec) const noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @return      размер буфера сокета
				 *
				 */
				int32_t getBufferSize(const net::socket_t sock, const net::socket_event_t event) const noexcept;
				/**
				 * @brief Метод установки размеров буфера
				 *
				 * @details Задаёт, сколько данных ядро придержит у себя - порознь на
				 * приём и на отправку
				 *
				 * @note Система вправе выдать не то, что просят: запрошенный размер
				 * обычно удваивается под служебные нужды, а сверх общесистемного
				 * предела и вовсе урезается. Итог стоит перечитать
				 *
				 * @param sock  сетевой сокет
				 * @param event событие сокета
				 * @param size  размер буфера сокета
				 * @return      установленный размер буфера сокета
				 *
				 */
				int32_t setBufferSize(const net::socket_t sock, const net::socket_event_t event, const int32_t size) const noexcept;
			public:
				/**
				 * @brief Метод установки сетевого интерфейса для multicast пакетов
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param ifname имя сетевого интерфейса
				 * @return       результат работы функции
				 *
				 */
				bool setMulticastIface(const net::socket_t sock, const event::family_t family, string_view ifname) const noexcept;
			public:
				/**
				 * @brief Метод устанавливает постоянное подключение на сокет
				 *
				 * @details Включает проверку живости: подключение, по которому долго
				 * нет обмена, будет опробовано служебными пакетами, и молчащий конец
				 * обнаружится сам
				 *
				 * @note Без этого оборванное подключение может не обнаруживаться
				 * сколь угодно долго - обрыв в сети ничем себя не выдаёт, пока по
				 * подключению не пойдут данные
				 *
				 * @param sock  сетевой сокет
				 * @param cnt   максимальное количество попыток
				 * @param idle  время через которое происходит проверка подключения
				 * @param intvl время между попытками
				 * @return      результат работы функции
				 *
				 */
				bool setKeepalive(const net::socket_t sock, int32_t cnt, int32_t idle, int32_t intvl) const noexcept;
			public:
				/**
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       значение DSCP
				 *
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const net::socket_t sock, const event::family_t family) const noexcept;
				/**
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param dscp   значение DSCP
				 * @return       результат работы функции
				 *
				 */
				bool setDifferentiatedServicesCodePoint(const net::socket_t sock, const event::family_t family, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * @brief Метод получения значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
				 *
				 * @note Выдаёт значение, устанавливаемое на исходящих пакетах. Признак
				 *       перегрузки принятых пакетов приходит отдельно для каждой
				 *       датаграммы в метаданных дейтаграммного пакета
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       значение ECN
				 *
				 */
				event::ecn_t getExplicitCongestionNotification(const net::socket_t sock, const event::family_t family) const noexcept;
				/**
				 * @brief Метод установки значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
				 *
				 * @note Класс обслуживания (DSCP) сохраняется: оба поля занимают один
				 *       октет заголовка, поэтому установка выполняется чтением текущего
				 *       значения с последующей заменой только младших двух бит
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param ecn    значение ECN
				 * @return       результат работы функции
				 *
				 */
				bool setExplicitCongestionNotification(const net::socket_t sock, const event::family_t family, const event::ecn_t ecn) const noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации генерации информации о трафике
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим активации или деактивации
				 * @return       результат работы функции
				 *
				 */
				bool trafficInfoGeneration(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode) const noexcept;
			public:
				/**
				 * @brief Метод переключения опции сокета
				 *
				 * @details Протокол принимается доводом, а не разыскивается у самого сокета.
				 *          Часть опций приложима лишь к одному протоколу - `TCP_NO_DELAY`
				 *          к TCP, - и прежде протокол читался настройкой `SO_PROTOCOL`,
				 *          то есть обращением к ядру ради того, что вызывающему и без
				 *          того известно: он этот сокет сам и заводил
				 *
				 * @note Довод необязателен, и значение `NONE` означает «протокол не
				 *       назван». Тогда он разыскивается у сокета по-прежнему: обращений
				 *       к методу много, и обязать все их назвать протокол значило бы
				 *       править места, которым он безразличен
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим активации или деактивации
				 * @param option опция сокета
				 * @param proto  протокол сокета, `NONE` - протокол не назван
				 * @return       результат работы функции
				 *
				 */
				bool switchOption(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode, const uint16_t option, const event::protocol_t proto = event::protocol_t::NONE) const noexcept;
			public:
				/**
				 * @brief Метод получения обнаружения максимального размера пакета (MTU)
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       режим обнаружения максимального размера пакета (MTU)
				 *
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family) const noexcept;
				/**
				 * @brief Метод установки обнаружения максимального размера пакета (MTU)
				 *
				 * @param sock   сетевой сокет
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим обнаружения максимального размера пакета (MTU)
				 * @return       результат работы функции
				 *
				 */
				bool setMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param sock     сетевой сокет
				 * @param family   семейство протоколов (IPv4 или IPv6)
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         максимальное количество хопов
				 *
				 */
				uint8_t getHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery) const noexcept;
				/**
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param sock     сетевой сокет
				 * @param family   семейство протоколов (IPv4 или IPv6)
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @param hops     максимальное количество хопов
				 * @return         результат работы функции
				 *
				 */
				bool setHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery, const uint8_t hops) const noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации мультикаст группы события
				 *
				 * @details Записывает сокет в группу рассылки или выписывает из неё.
				 * Пакеты, разосланные на группу, доходят лишь до вписавшихся
				 *
				 * @note Вписываться следует с указанием устройства: машина с
				 * несколькими устройствами иначе выберет его сама, и рассылка может
				 * прийти не с той стороны
				 *
				 * @param sock   сетевой сокет
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @return       результат работы функции
				 *
				 */
				bool membership(const net::socket_t sock, const net::socket_mode_t mode, const net::addr_net_t * group, const net::addr_net_t * source) const noexcept;
			public:
				/**
				 * @brief Метод проверки готовности средств сокетов системы
				 *
				 * @details Отвечает, годна ли система заводить сокеты вообще. Средства
				 * эти у отдельных систем требуют подъёма на процесс, и подъём этот
				 * вправе не удаться: у MS Windows такое обращение (`WSAStartup`)
				 * отвечает отказом при недоступной версии либо нехватке средств, и
				 * всякое обращение к сокетам после этого отвечает отказом 10093
				 *
				 * @details Спрашивают об этом на заведении движка, а не при выдаче
				 * сокета. Довод в том, КОГДА вызывающая сторона просит сеть: заводя
				 * движок, она сеть просит явно - и отказ обязан прийти прямо там, а не
				 * всплыть позже отдельными отказами каждого сокета, оставив приложение
				 * заведённым, но неработоспособным
				 *
				 * @note У систем, подъёма не требующих, ответ утвердителен всегда:
				 * средства сокетов там принадлежат ядру и в подъёме не нуждаются
				 *
				 * @return результат проверки готовности средств сокетов системы
				 *
				 */
				bool ready() const noexcept;
			public:
				/**
				 * @brief Метод выдачи нового сокета
				 *
				 * @details Часть опций события ядро принимает прямо при создании
				 * сокета, не требуя отдельных обращений: неблокирующий режим и
				 * закрытие при запуске стороннего образа. Набор опций передаётся
				 * сюда, чтобы этой возможностью воспользоваться
				 *
				 * @note Опции, которые ядро при создании не принимает, метод
				 * пропускает молча - их накладывает вызывающая сторона обычным
				 * путём. На системах без такой возможности пропускаются все
				 *
				 * @param family  семейство протоколов сокета
				 * @param type    тип сокета
				 * @param proto   протокол сокета
				 * @param options набор опций события
				 * @return        созданный сокет
				 *
				 */
				net::socket_t issue(const event::family_t family, const event::type_t type, const event::protocol_t proto, const uint16_t options = event::options::NONE) const noexcept;
			public:
				/**
				 * @brief Метод получения опций, принимаемых при создании сокета
				 *
				 * @details Отвечает, какие из переданных опций ядро этой системы
				 * принимает прямо при создании сокета - то есть какие из них метод
				 * выдачи сокета уже наложил и накладывать повторно не следует
				 *
				 * @note Знание это держится здесь намеренно: оно зависит от системы
				 * и от условной сборки, и повторять его на стороне движка означало бы
				 * завести второй источник правды, который разойдётся с первым
				 *
				 * @param options набор опций события
				 * @return        подмножество опций, наложенных при создании сокета
				 *
				 */
				uint16_t inborn(const uint16_t options) const noexcept;
			public:
				/**
				 * @brief Метод создания пары сокетов для межпроцессного взаимодействия (IPC)
				 *
				 * @details Заводит два связанных сокета: записанное в один
				 * вычитывается из другого. Обмен идёт внутри ядра, минуя сеть
				 *
				 * @note Обычное применение - разделение процесса: пара заводится до
				 * разделения, после чего каждая сторона закрывает свой конец и
				 * остаётся связь между родителем и потомком
				 *
				 * @param family семейство протоколов сокета
				 * @param type   тип сокета
				 * @param proto  протокол сокета
				 * @return       созданный сокет
				 *
				 */
				array <net::socket_t, 2> ipc(const event::family_t family, const event::type_t type, const event::protocol_t proto) const noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 *
				 */
				explicit Socket(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Socket() noexcept;
		} socket_t;
	};
};

#endif // __AWH_SOCKET__
