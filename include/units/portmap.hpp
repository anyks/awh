/**
 * @file: portmap.hpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля перенаправления портов — класс unit::Portmap,
 *        заводящий перенаправление на маршрутизаторе договорами NAT-PMP, PCP и UPnP,
 *        отыскивающий маршрутизатор и ведущий обмен с ним без блокировки потока
 *
 * \~english
 * @brief Header file of the port forwarding module — the unit::Portmap class,
 *        which creates a forwarding on the router by the NAT-PMP, PCP and UPnP protocols,
 *        finds the router and conducts the exchange with it without blocking the thread
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_PORTMAP__
#define __AWH_UNIT_PORTMAP__

/**
 * Подключаем заголовочные файлы проекта
 */
#include "unit.hpp"
#include "../net/uri.hpp"
#include "../net/eth/iface.hpp"
#include "../net/eth/gateway.hpp"
#include "../proto/http/parser/http1/http.hpp"
#include "../proto/portmap/pcp.hpp"
#include "../proto/portmap/ssdp.hpp"
#include "../proto/portmap/soap.hpp"
#include "../proto/portmap/upnp.hpp"
#include "../proto/portmap/device.hpp"
#include "../proto/portmap/natpmp.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Пространство имён модулей
	 *
	 *
	 * \~english
	 * @brief Modules namespace
	 *
	 * \~
	 */
	namespace unit {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс модуля перенаправления портов
		 *
		 * @details Заводит на маршрутизаторе перенаправление внешнего порта на внутренний,
		 * убирает заведённое и спрашивает внешний адрес. Обмен ведётся без блокировки
		 * потока: просьба лишь отправляется, а итог приходит функцией обратного вызова
		 *
		 * @note Договоров перенаправления три, и они друг друга не заменяют: NAT-PMP и PCP
		 * отвечают дейтаграммой напрямую маршрутизатору, UPnP же требует поиска устройства
		 * рассылкой SSDP, чтения описания устройства и обращения к нему запросом SOAP.
		 * Какой из трёх поддерживает маршрутизатор, заранее неизвестно, поэтому предусмотрен
		 * вид опроса AUTO: все три ведутся разом под общим сроком, и принимается тот ответ,
		 * что пришёл первым
		 *
		 * @warning Заведённое перенаправление переживает завершение процесса: маршрутизатор
		 * держит его до истечения срока жизни. Убирать заведённое следует самому
		 *
		 * @par Намеренные решения
		 *
		 * Решения ниже приняты обдуманно и переоткрывались разборами не по одному разу.
		 * Меняя их, следует понимать, чем именно они вызваны:
		 *
		 * - **Узел, названный именем, отвергается.** И адрес описания устройства, и адрес
		 *   управления службой проверяются на принадлежность местной сети, а имя проверить
		 *   без разрешения нельзя - само же разрешение назначает не модуль. Устройства
		 *   доступа в сеть записывают эти адреса числом, и потеря на имени невелика рядом
		 *   с уводом обращения наружу.
		 *
		 * - **Договор NAT-PMP в сети IPv6 не опрашивается.** RFC 6886 описан только для
		 *   IPv4, и разновидности IPv6 у договора нет вовсе. Молчание его лишь затянуло бы
		 *   общий срок опроса.
		 *
		 * - **Перенаправлений в сети IPv6 договор UPnP не заводит.** Преобразования адресов
		 *   там нет, и подключения сквозь заслон разрешает отдельная служба заслона IPv6,
		 *   которую выдаёт не всякое устройство. Внешний адрес и перечень служба соединения
		 *   выдаёт и по связи IPv6, и они не отсекаются.
		 *
		 * - **Отличительная метка выдаётся по отправке просьбы, а не по ответу.** Под видом
		 *   опроса AUTO обращение завершается по первому ответу, но просьба договора PCP к
		 *   этому времени уже ушла, и маршрутизатор вправе завести по ней перенаправление.
		 *   Без метки снять его было бы нечем.
		 *
		 * - **Сетевое устройство задаётся рассылке явно.** Групповой адрес в сети IPv6
		 *   принадлежит связи, и подписка на чужой связи оставила бы рассылку без ответа.
		 *   Система с исправно заведённой сетью IPv6 справляется и сама, но полагаться на её
		 *   выбор незачем, когда устройство уже назначено.
		 *
		 * - **Отказ вступления в группу обнаружения обмен не прерывает.** Ответ на просьбу
		 *   обнаружения устройство шлёт одному спросившему, а не в группу: членство нужно
		 *   лишь для объявлений, которых модуль не ждёт.
		 *
		 * - **Отброшенный ответ ожидания не прерывает.** Движок держит срок ожиданием
		 *   ответа и снимает его приходом данных, не разбирая, ответ это или чужая
		 *   дейтаграмма. Разобрать способен лишь модуль, и всюду, где ответ отбрасывается -
		 *   на общем порту дейтаграммного договора, на групповой рассылке и на недочитанном
		 *   ответе устройства, - ожидание продолжается вызовом `rearmTimeout()`. Продолжается
		 *   остатком, а не заново: чужой ответ лишнего времени просьбе не дарит, и отказ
		 *   наступает в свой черёд. Пределом простоя срок при этом не становится - на
		 *   событии, которому ничего не отправляли, он не взводится вовсе.
		 *
		 * - **Утрата состояния маршрутизатором объявляется, а не заделывается.** Договоры
		 *   NAT-PMP и PCP несут отсчёт времени работы маршрутизатора, и откат его означает,
		 *   что все заведённые перенаправления пропали. Модуль сверяет отсчёт у каждого
		 *   ответа и объявляет утрату обратным вызовом `reset`, но заводить перенаправления
		 *   заново не берётся: какие из них нужны и в каком порядке, знает лишь вызывающий,
		 *   а модуль ведёт одно обращение разом и о прежних заведениях памяти не держит.
		 *   Договор UPnP отсчёта не имеет вовсе, и утрату по нему заметить нечем.
		 *
		 * - **Приём объявлений заводится по просьбе, а не сам собой.** Он держит гнездо на
		 *   договорном порту всё время работы и принимает дейтаграммы от любого узла сети.
		 *   Приложению, которому объявления не нужны, всё это ни к чему, и включает приём
		 *   вызывающий вызовом `announce()`.
		 *
		 * - **В группу объявлений модуль не вступает.** Объявления рассылаются на всеобщие
		 *   групповые адреса - 224.0.0.1 и FF02::1, - членство в которых принадлежит всякому
		 *   узлу по устройству самой сети: рассылку по ним система доставляет без подписки,
		 *   и подписка прибавила бы ровно ничего. Отказом её не встречают - вступление
		 *   принимают и macOS, и Linux, и FreeBSD, - но и нужды в ней нет. Отличие от
		 *   рассылки обнаружения SSDP, где членство обязательно, именно в этом: там адрес
		 *   свой, договорный, и без подписки не придёт ничего.
		 *
		 * - **Отправитель объявления отбирается по внешнему адресу, а не по местной сети.**
		 *   Отбрасывается пришедшее извне, а петля пропускается: объявление по ней приходит
		 *   от службы, работающей на этой же машине, и отвергать его значило бы отвергать
		 *   своё.
		 *
		 * @warning Устройство доступа в сеть роняет часть подключений и отвечает
		 * `Connection: close`, требуя подключения на каждое перенаправление. Обмен это
		 * переживает: неудавшуюся попытку подключения повторяет само событие, а молчание в
		 * ответ на отправленный запрос модуль повторяет тем же шагом, на котором стоял
		 *
		 * \~english
		 * @brief Class of the port forwarding module
		 * @details Creates on the router a forwarding of an external port to an internal one,
		 * removes what has been created and asks for the external address. The exchange is conducted without blocking
		 * the thread: the request is merely sent, while the result arrives through a callback function
		 * @note There are three forwarding protocols, and they do not replace one another: NAT-PMP and PCP
		 * answer with a datagram directly to the router, while UPnP requires a discovery of the device
		 * by an SSDP multicast, a reading of the description of the device and a call to it with a SOAP request.
		 * Which of the three the router supports is unknown beforehand, therefore the AUTO
		 * polling kind is provided: all three are conducted at once under a common term, and the answer that
		 * has come first is accepted
		 * @warning A created forwarding outlives the termination of the process: the router
		 * holds it until its lifetime expires. What has been created should be removed by oneself
		 * @par Deliberate decisions
		 * The decisions below have been taken deliberately and have been reopened by the audits more than once.
		 * When changing them, one should understand what exactly they are caused by:
		 * - **A node named by a name is rejected.** Both the address of the description of the device and the address
		 *   of the control of the service are checked for belonging to the local network, while a name cannot be checked
		 *   without a resolution — and the resolution itself is assigned not by the module. The network access
		 *   devices write those addresses as numbers, and the loss on the name is small next to
		 *   the diversion of the call to the outside.
		 * - **The NAT-PMP protocol is not polled in an IPv6 network.** RFC 6886 is described only for
		 *   IPv4, and the protocol has no IPv6 variety at all. Its silence would only prolong
		 *   the common term of the polling.
		 * - **The UPnP protocol does not create forwardings in an IPv6 network.** There is no address translation
		 *   there, and the connections through the firewall are permitted by a separate IPv6 firewall service,
		 *   which not every device provides. The external address and the list are provided by the connection service
		 *   over an IPv6 link as well, and they are not cut off.
		 * - **The distinguishing mark is issued upon the sending of the request rather than upon the answer.** Under the AUTO
		 *   polling kind the call is completed by the first answer, but the request of the PCP protocol has
		 *   already gone out by that time, and the router has the right to create a forwarding by it.
		 *   Without the mark there would be nothing to remove it by.
		 * - **The network device is given to the multicast explicitly.** A group address in an IPv6 network
		 *   belongs to the link, and a subscription on a foreign link would leave the multicast without an answer.
		 *   A system with a properly configured IPv6 network copes by itself as well, but there is no point in relying on its
		 *   choice when the device has already been assigned.
		 * - **A refusal of joining the discovery group does not interrupt the exchange.** The answer to a discovery
		 *   request is sent by the device to the single asker rather than into the group: the membership is needed
		 *   only for the announcements, which the module does not wait for.
		 * - **A discarded answer does not interrupt the waiting.** The engine holds the term by the waiting for
		 *   an answer and removes it upon the arrival of data without distinguishing whether that is an answer or a foreign
		 *   datagram. Only the module is capable of distinguishing, and everywhere where an answer is discarded —
		 *   on the common port of a datagram protocol, on a group multicast and on an underread
		 *   answer of a device — the waiting is continued by a call to `rearmTimeout()`. It is continued
		 *   with the remainder rather than anew: a foreign answer does not gift the request any extra time, and the refusal
		 *   comes in its due turn. The term does not thereby become a limit of the idleness — on
		 *   an event to which nothing has been sent it is not armed at all.
		 * - **A loss of the state by the router is announced rather than patched up.** The NAT-PMP
		 *   and PCP protocols carry the counter of the working time of the router, and its rollback means
		 *   that all the created forwardings have disappeared. The module verifies the counter at every
		 *   answer and announces the loss by the `reset` callback, but it does not undertake to create the forwardings
		 *   anew: which of them are needed and in what order is known only to the caller,
		 *   while the module conducts one call at a time and keeps no memory of the previous creations.
		 *   The UPnP protocol has no counter at all, and there is nothing to notice a loss by there.
		 * - **The reception of the announcements is created upon a request rather than by itself.** It holds a socket on
		 *   the protocol port for the whole time of the work and accepts datagrams from any node of the network.
		 *   An application that does not need the announcements has no use for all of that, and the reception is enabled by the
		 *   caller with a call to `announce()`.
		 * - **The module does not join the announcement group.** The announcements are multicast to the all-nodes
		 *   group addresses — 224.0.0.1 and FF02::1 — the membership in which belongs to every
		 *   node by the design of the network itself: a multicast to them is delivered by the system without a subscription,
		 *   and a subscription would add exactly nothing. It is not met with a refusal — the joining
		 *   is accepted by macOS, and by Linux, and by FreeBSD — but there is no need for it either. The difference from
		 *   the SSDP discovery multicast, where the membership is obligatory, lies exactly in this: there the address
		 *   is a proper, protocol one, and without a subscription nothing will arrive.
		 * - **The sender of an announcement is selected by the external address rather than by the local network.**
		 *   What has come from the outside is discarded, while the loopback is let through: an announcement over it comes
		 *   from a service working on this same machine, and to reject it would mean to reject
		 *   one's own.
		 * @warning A network access device drops a part of the connections and answers with
		 * `Connection: close`, requiring a connection for every forwarding. The exchange
		 * survives this: an unsuccessful connection attempt is repeated by the event itself, while a silence in
		 * answer to a sent request is repeated by the module at the same step at which it stood
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Portmap : public unit_t {
			public:
				/**
				 * \~russian
				 * @brief Предельное число перенаправлений в читаемом перечне
				 *
				 * @details Перечня служба разом не выдаёт, и конец его объявляет отказом о
				 * выходе за перечень. Устройство, отвечающее успехом без конца, обход тем
				 * самым не завершает, и предел здесь обязателен: сведения берутся у
				 * устройства, о котором заранее ничего не известно
				 *
				 * \~english
				 * @brief Limit of the number of the forwardings in the list being read
				 * @details The service does not provide the list at once, and it announces its end with a refusal about
				 * going beyond the list. A device that answers with success endlessly does not thereby complete the
				 * traversal, and a limit is obligatory here: the information is taken from a
				 * device about which nothing is known beforehand
				 *
				 * \~
				 */
				static constexpr uint32_t MAX_MAPPINGS = 0x400;
				/**
				 * \~russian
				 * @brief Предельное число отправителей объявлений маршрутизатора
				 *
				 * @details Приём объявлений открыт всей сети, и своё событие заводится на
				 * каждого отправителя. Предел здесь обязателен: без него всякий узел сети
				 * прибавлял бы модулю событий, сколько ему вздумается, - довольно менять
				 * порт отправки. Дойдя до предела, самое давнее событие закрывается
				 *
				 * @note Обмену с маршрутизатором предел не мешает: маршрутизатор рассылает
				 * объявления одним и тем же гнездом, и событие у него одно
				 *
				 * \~english
				 * @brief Limit of the number of the senders of the announcements of the router
				 * @details The reception of the announcements is open to the whole network, and an own event is created for
				 * every sender. A limit is obligatory here: without it any node of the network
				 * would add as many events to the module as it likes — it is enough to change
				 * the sending port. Having reached the limit, the oldest event is closed
				 * @note The limit does not hinder the exchange with the router: the router multicasts
				 * the announcements with one and the same socket, and it has a single event
				 *
				 * \~
				 */
				static constexpr size_t MAX_ANNOUNCERS = 0x10;
			public:
				/**
				 * \~russian
				 * @brief Договоры перенаправления портов
				 *
				 * \~english
				 * @brief Port forwarding protocols
				 *
				 * \~
				 */
				enum class type_t : uint8_t {
					NONE    = 0x00, // Договор не определён
					PCP     = 0x01, // Перенаправление договором PCP
					UPNP    = 0x02, // Перенаправление договором UPnP
					NAT_PMP = 0x03, // Перенаправление договором NAT-PMP
					AUTO    = 0x04  // Опрос всеми договорами разом под общим сроком
				};
				/**
				 * \~russian
				 * @brief Разновидности сети, в которой ведётся обмен
				 *
				 * @details Обмен ведётся целиком одной разновидностью: и отыскание
				 * маршрутизатора, и просьбы к нему. Разом обеими не ведётся - маршрутизатор
				 * отвечает по той, по которой к нему обратились, и опрашивать его дважды
				 * значило бы заводить два перенаправления вместо одного
				 *
				 * @note Договор NAT-PMP разновидности IPv6 не имеет вовсе: RFC 6886 описан
				 * только для IPv4, и в сети IPv6 он не опрашивается
				 *
				 * @warning В сети IPv6 преобразования адресов нет, и перенаправление
				 * означает не подмену адреса, а разрешение подключений сквозь заслон
				 * маршрутизатора: внешним адресом выдаётся собственный адрес машины
				 *
				 * \~english
				 * @brief Varieties of the network in which the exchange is conducted
				 * @details The exchange is conducted entirely with one variety: both the discovery of the
				 * router and the requests to it. It is not conducted with both at once — the router
				 * answers over the one over which it has been addressed, and to poll it twice
				 * would mean to create two forwardings instead of one
				 * @note The NAT-PMP protocol has no IPv6 variety at all: RFC 6886 is described
				 * only for IPv4, and in an IPv6 network it is not polled
				 * @warning In an IPv6 network there is no address translation, and a forwarding
				 * means not a substitution of the address but a permission of the connections through the firewall
				 * of the router: the address of the machine itself is issued as the external address
				 *
				 * \~
				 */
				enum class family_t : uint8_t {
					IPV4 = 0x01, // Обмен ведётся сетью IPv4
					IPV6 = 0x02  // Обмен ведётся сетью IPv6
				};
				/**
				 * \~russian
				 * @brief Договоры перенаправляемого порта
				 *
				 * \~english
				 * @brief Protocols of the forwarded port
				 *
				 * \~
				 */
				enum class proto_t : uint8_t {
					NONE = 0x00, // Договор не определён
					TCP  = 0x01, // Перенаправление порта TCP
					UDP  = 0x02  // Перенаправление порта UDP
				};
				/**
				 * \~russian
				 * @brief Просьбы, с которыми модуль обращается к маршрутизатору
				 *
				 * \~english
				 * @brief Requests with which the module addresses the router
				 *
				 * \~
				 */
				enum class action_t : uint8_t {
					NONE     = 0x00, // Просьба не определена
					OPEN     = 0x01, // Завести перенаправление порта
					CLOSE    = 0x02, // Убрать заведённое перенаправление порта
					EXTERNAL = 0x03, // Выдать внешний адрес маршрутизатора
					LIST     = 0x04, // Выдать перечень заведённых перенаправлений
					RENEW    = 0x05  // Продлить срок заведённого перенаправления
				};
				/**
				 * \~russian
				 * @brief Коды причины отказа перенаправления
				 *
				 * @details Отказ маршрутизатора отделён от сбоя обмена намеренно: первый
				 * означает осмысленный ответ, второй - что ответа не получено вовсе
				 *
				 * \~english
				 * @brief Codes of the reason of a refusal of a forwarding
				 * @details A refusal of the router is separated from a failure of the exchange deliberately: the first
				 * means a meaningful answer, the second — that no answer has been received at all
				 *
				 * \~
				 */
				enum class error_t : uint8_t {
					NONE             = 0x00, // Ошибки нет
					NO_GATEWAY       = 0x01, // Маршрутизатор отыскать не удалось
					NO_RESPONSE      = 0x02, // Маршрутизатор не ответил в отведённый срок
					NOT_SUPPORTED    = 0x03, // Маршрутизатор договора не поддерживает
					NOT_AUTHORIZED   = 0x04, // Перенаправление отвергнуто настройкой маршрутизатора
					NETWORK_FAILURE  = 0x05, // Маршрутизатор не имеет связи с внешней сетью
					OUT_OF_RESOURCES = 0x06, // У маршрутизатора не осталось места под перенаправления
					MALFORMED        = 0x07, // Ответ маршрутизатора разобрать не удалось
					REFUSED          = 0x08  // Маршрутизатор отказал по иной причине
				};
			public:
				/**
				 * \~russian
				 * @brief Структура перенаправления порта
				 *
				 * @note Одна и та же запись служит и просьбой, и итогом: маршрутизатор
				 * вправе назначить внешний порт и срок жизни по своему выбору, и в итоге
				 * стоит уже назначенное, а не запрошенное
				 *
				 * \~english
				 * @brief Structure of a port forwarding
				 * @note One and the same record serves both as a request and as a result: the router
				 * has the right to assign the external port and the lifetime by its own choice, and in the result
				 * what has already been assigned stands rather than what has been requested
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Mapping {
					// Договор перенаправляемого порта
					proto_t proto;
					// Внутренний порт перенаправления
					uint16_t internalPort;
					/**
					 * \~russian
					 * Внешний порт перенаправления
					 *
					 * @warning Запрошенный порт для маршрутизатора лишь пожелание: занятый
					 * порт он заменит другим по своему выбору, и объявлять другим следует
					 * именно назначенный
					 *
					 * @note У пробоя заслона IPv6 внешнего порта нет вовсе: преобразования
					 * адресов там не происходит, и подключения приходят прямо на внутренний
					 * порт. В итоге такого обращения поле остаётся тем, каким его задал
					 * вызывающий, а опознаёт пробой поле `pinhole`
					 *
					 * \~english
					 * External port of the forwarding
					 * @warning The requested port is for the router only a wish: an occupied
					 * port it will replace by another one of its own choice, and what should be announced to the others is
					 * exactly the assigned one
					 * @note An IPv6 firewall pinhole has no external port at all: no address translation
					 * takes place there, and the connections arrive right at the internal
					 * port. In the result of such a call the field remains the one which the
					 * caller has given it, while a pinhole is recognized by the `pinhole` field
					 *
					 * \~
					 */
					uint16_t externalPort;
					// Срок жизни перенаправления в секундах
					uint32_t lifeTime;
					/**
					 * \~russian
					 * Описание перенаправления, показываемое в настройках маршрутизатора
					 *
					 * @note Описание несёт лишь договор UPnP: NAT-PMP и PCP его не имеют,
					 * и там оно пропадает
					 *
					 * \~english
					 * Description of the forwarding shown in the settings of the router
					 * @note Only the UPnP protocol carries the description: NAT-PMP and PCP do not have it,
					 * and there it disappears
					 *
					 * \~
					 */
					string description;
					/**
					 * \~russian
					 * Отличительная метка перенаправления
					 *
					 * @details Метку эту несёт лишь договор PCP, и она обязана быть одной и
					 * той же у просьбы заведения, у её повторов и у просьбы снятия: по метке
					 * маршрутизатор и опознаёт, о каком перенаправлении речь
					 *
					 * @note Заполняется модулем при заведении и выдаётся вместе с заведённым
					 * перенаправлением. Чтобы снять заведённое в другом запуске, метку
					 * следует сохранить и передать в просьбе снятия: без неё маршрутизатор
					 * снимать откажется. Договоры NAT-PMP и UPnP метки не имеют, и там поле
					 * не участвует
					 *
					 * \~english
					 * Distinguishing mark of the forwarding
					 * @details Only the PCP protocol carries this mark, and it is obliged to be one and the
					 * same at the creation request, at its repetitions and at the removal request: by the mark
					 * the router recognizes which forwarding is being spoken about
					 * @note Filled in by the module at the creation and issued together with the created
					 * forwarding. In order to remove what has been created in another run, the mark
					 * should be preserved and passed in the removal request: without it the router
					 * will refuse to remove. The NAT-PMP and UPnP protocols have no mark, and there the field
					 * does not participate
					 *
					 * \~
					 */
					vector <uint8_t> nonce;
					/**
					 * \~russian
					 * Опознаватель пробоя заслона IPv6
					 *
					 * @details Опознаватель этот несёт лишь служба заслона IPv6 договора
					 * UPnP: пробой заделывается по нему, а не по признакам - одним и тем же
					 * признакам отвечает несколько пробоев
					 *
					 * @note Заполняется модулем при проделывании пробоя и выдаётся вместе с
					 * ним. Чтобы заделать проделанное в другом запуске, опознаватель следует
					 * сохранить и передать в просьбе заделывания. В сети IPv4 и у договоров
					 * NAT-PMP и PCP поле не участвует
					 *
					 * @note Работа с опознавателем проверена на живом устройстве: MiniUPnPd
					 * 2.3.7 с заслоном IPv6 на netfilter выдаёт опознаватель проделанного
					 * пробоя, и заделывание им снимает заведённое разрешение
					 *
					 * \~english
					 * Identifier of an IPv6 firewall pinhole
					 * @details Only the IPv6 firewall service of the UPnP protocol carries this
					 * identifier: a pinhole is patched up by it rather than by the attributes — one and the same
					 * attributes are matched by several pinholes
					 * @note Filled in by the module at the making of a pinhole and issued together with
					 * it. In order to patch up what has been made in another run, the identifier should be
					 * preserved and passed in the patching request. In an IPv4 network and in the NAT-PMP
					 * and PCP protocols the field does not participate
					 * @note The work with the identifier has been verified on a live device: MiniUPnPd
					 * 2.3.7 with an IPv6 firewall on netfilter issues the identifier of a made
					 * pinhole, and the patching by it removes the created permission
					 *
					 * \~
					 */
					uint16_t pinhole;
					/**
					 * \~russian
					 * Признак того, что перенаправление подключения пропускает
					 *
					 * @note Заполняется чтением перечня: среди заведённых встречаются и
					 * отключённые - место они занимают, а подключений не пропускают.
					 * При заведении перенаправления признак не участвует
					 *
					 * \~english
					 * Flag of the forwarding letting the connections through
					 * @note Filled in by the reading of the list: among the created ones there are also
					 * disabled ones — they take up a place but do not let the connections through.
					 * At the creation of a forwarding the flag does not participate
					 *
					 * \~
					 */
					bool enabled;
					/**
					 * \~russian
					 * Внешний узел, подключения с которого пропускаются
					 *
					 * @note Заполняется чтением перечня. Пустое значение означает «с
					 * любого узла», и оно же встречается почти всегда
					 *
					 * \~english
					 * External node the connections from which are let through
					 * @note Filled in by the reading of the list. An empty value means «from
					 * any node», and it is what is met almost always
					 *
					 * \~
					 */
					string remoteHost;
					/**
					 * \~russian
					 * Внутренний адрес машины, которой отдаются подключения
					 *
					 * @note Заполняется чтением перечня и нужен затем, что перечень общий
					 * на всю сеть: по этому полю и видно, чьё перенаправление. При
					 * заведении адрес подставляет модуль сам
					 *
					 * \~english
					 * Internal address of the machine to which the connections are given
					 * @note Filled in by the reading of the list and is needed because the list is common
					 * to the whole network: by this field one sees whose forwarding it is. At the
					 * creation the address is substituted by the module itself
					 *
					 * \~
					 */
					string internalClient;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					Mapping() noexcept;
				} mapping_t;
			private:
				/**
				 * \~russian
				 * @brief Структура обмена по одному договору перенаправления
				 *
				 * @details Виды опроса ведутся одинаково: своё событие, отсчёт попыток и
				 * признак ожидания ответа. Разнится лишь сборка просьбы и разбор ответа,
				 * и потому обмены сведены в одну запись
				 *
				 * \~english
				 * @brief Structure of the exchange over a single forwarding protocol
				 * @details The polling kinds are conducted identically: an own event, a counter of the attempts and
				 * a flag of the waiting for an answer. Only the assembly of the request and the parsing of the answer differ,
				 * and therefore the exchanges are brought together into a single record
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Exchange {
					// Признак ожидания ответа маршрутизатора
					bool waiting;
					// Порядковый номер выполненной попытки
					uint8_t attempt;
					// Идентификатор события обмена
					event::id_t eid;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					Exchange() noexcept;
				} exchange_t;
				/**
				 * \~russian
				 * @brief Структура отсчёта времени работы маршрутизатора
				 *
				 * @details Договоры NAT-PMP и PCP несут в каждом ответе отсчёт секунд с
				 * той поры, как маршрутизатор завёл своё состояние перенаправлений. Откат
				 * этого отсчёта означает, что состояние утрачено - перезагрузкой ли,
				 * сбросом ли настроек, - и все заведённые перенаправления вместе с ним
				 *
				 * @note Отсчёт держится отдельно на каждый договор: маршрутизатор вправе
				 * вести их разными службами, и совпадения отсчётов договоры не требуют
				 *
				 * \~english
				 * @brief Structure of the counter of the working time of the router
				 * @details The NAT-PMP and PCP protocols carry in every answer the counter of the seconds since
				 * the moment when the router has created its forwarding state. A rollback of
				 * this counter means that the state has been lost — whether by a reboot or by
				 * a reset of the settings — and all the created forwardings along with it
				 * @note The counter is kept separately for every protocol: the router has the right to
				 * conduct them with different services, and the protocols do not require a coincidence of the counters
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Epoch {
					// Признак того, что отсчёт уже получен хотя бы раз
					bool filled;
					// Отсчёт времени работы маршрутизатора в секундах
					uint32_t value;
					// Время получения отсчёта по часам этой машины в миллисекундах
					uint64_t stamp;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					Epoch() noexcept;
				} epoch_t;
			private:
				/**
				 * \~russian
				 * @brief Шаги обмена по договору UPnP
				 *
				 * @details Договор UPnP одним обменом не обходится: сперва устройство
				 * отыскивается рассылкой SSDP, затем у него читается описание, и лишь потом
				 * вызывается действие его службы. Каждый шаг опирается на добытое предыдущим
				 *
				 * \~english
				 * @brief Steps of the exchange over the UPnP protocol
				 * @details The UPnP protocol does not make do with a single exchange: first the device is
				 * found by an SSDP multicast, then its description is read from it, and only then
				 * an action of its service is called. Every step relies on what has been obtained by the previous one
				 *
				 * \~
				 */
				enum class stage_t : uint8_t {
					NONE        = 0x00, // Обмен не ведётся
					SEARCH      = 0x01, // Отыскание устройства рассылкой SSDP
					DESCRIPTION = 0x02, // Чтение описания отысканного устройства
					CONTROL     = 0x03  // Вызов действия службы устройства
				};
			private:
				// Вид опроса маршрутизатора
				type_t _type;
				// Разновидность сети, в которой ведётся обмен
				family_t _family;
				// Просьба, с которой ведётся текущее обращение
				action_t _action;
			private:
				// Количество попыток обращения к маршрутизатору
				uint8_t _attempts;
				// Срок ожидания ответа маршрутизатора в миллисекундах
				uint32_t _delay;
			private:
				// Перенаправление, о котором ведётся текущее обращение
				mapping_t _mapping;
			private:
				/**
				 * \~russian
				 * Последняя просьба, отправленная по договору PCP
				 *
				 * @note Держится затем, что ответ сличается с просьбой средствами кодека:
				 * он сверяет и действие, и отличительную метку, и внутренний порт - то
				 * есть больше, чем модуль проверил бы сам
				 *
				 * \~english
				 * Last request sent over the PCP protocol
				 * @note It is kept because the answer is verified against the request by the means of the codec:
				 * it checks both the action, and the distinguishing mark, and the internal port — that
				 * is, more than the module would check by itself
				 *
				 * \~
				 */
				proto::portmap::pcp_t::request_t _inquiry;
				/**
				 * \~russian
				 * Отличительная метка перенаправления для договора PCP
				 *
				 * @note Метка держится обменом, а не собирается заново на каждую попытку:
				 * договор предписывает одну и ту же метку для просьбы, её повторов и
				 * снятия заведённого
				 *
				 * \~english
				 * Distinguishing mark of the forwarding for the PCP protocol
				 * @note The mark is kept by the exchange rather than assembled anew for every attempt:
				 * the protocol prescribes one and the same mark for a request, its repetitions and
				 * the removal of what has been created
				 *
				 * \~
				 */
				vector <uint8_t> _nonce;
				// Порядковый номер читаемого перенаправления при чтении перечня
				uint32_t _index;
				/**
				 * \~russian
				 * Признак того, что спрашивается состояние заслона IPv6
				 *
				 * @details Договор велит узнать состояние заслона прежде, чем просить о
				 * пробое: отключённый заслон и запрет пробоев устройство объявляет
				 * отдельным действием, а на саму просьбу отвечает отказом, по которому
				 * причина уже не видна. Спрошенное состояние отвечает на неё до просьбы
				 *
				 * @note Признак держится лишь между двумя шагами обмена: он ставится перед
				 * тем, как спросить состояние, и снимается ответом на этот вопрос. Дальше
				 * тем же ходом обмена уходит уже сама просьба о пробое
				 *
				 * \~english
				 * Flag of the state of the IPv6 firewall being asked for
				 * @details The protocol orders one to learn the state of the firewall before asking for a
				 * pinhole: a disabled firewall and a prohibition of the pinholes are announced by the device with
				 * a separate action, while to the request itself it answers with a refusal by which
				 * the reason is no longer visible. The asked-for state answers it before the request
				 * @note The flag is kept only between two steps of the exchange: it is set before
				 * the state is asked for, and it is removed by the answer to that question. Further on
				 * the request for a pinhole itself goes out by the same course of the exchange
				 *
				 * \~
				 */
				bool _probe;
				// Перечень прочитанных у маршрутизатора перенаправлений портов
				vector <mapping_t> _mappings;
			private:
				// Идентификатор события приёма объявлений маршрутизатора
				event::id_t _announcer;
				/**
				 * \~russian
				 * Идентификаторы событий отправителей объявлений маршрутизатора
				 *
				 * @note Держатся затем, что заведены они движком, а закрывать их обязан
				 * модуль: без этого приём объявлений, отключённый вызывающим, оставил бы
				 * за собой события отправителей
				 *
				 * \~english
				 * Event identifiers of the senders of the announcements of the router
				 * @note They are kept because they have been created by the engine while the module is obliged to close
				 * them: without this the reception of the announcements, disabled by the caller, would leave
				 * the events of the senders behind it
				 *
				 * \~
				 */
				vector <event::id_t> _announcers;
			private:
				// Отсчёт времени работы маршрутизатора по договору PCP
				epoch_t _epochPCP;
				// Отсчёт времени работы маршрутизатора по договору NAT-PMP
				epoch_t _epochNATPMP;
			private:
				// Обмен по договору PCP
				exchange_t _exchangePCP;
				// Обмен по договору UPnP
				exchange_t _exchangeUPNP;
				// Обмен по договору NAT-PMP
				exchange_t _exchangeNATPMP;
			private:
				// Шаг обмена по договору UPnP
				stage_t _stage;
				// Идентификатор потокового события обмена с устройством UPnP
				event::id_t _stream;
			private:
				// Адрес описания отысканного устройства UPnP
				string _location;
				// Адрес управления службой отысканного устройства UPnP
				string _control;
				// Обозначение вида службы отысканного устройства UPnP
				string _service;
			private:
				/**
				 * \~russian
				 * Объект разбора ответа устройства UPnP
				 *
				 * @note Разбор ведётся по мере поступления, а не по накопленному целиком:
				 * парсер держит своё состояние между чтениями, и подавать ему заново весь
				 * ответ на каждый пришедший кусок значило бы разбирать его столько раз,
				 * сколько кусков пришло
				 *
				 * \~english
				 * Object of the parsing of the answer of a UPnP device
				 * @note The parsing is conducted as the data arrives rather than over what has been accumulated in full:
				 * the parser keeps its state between the readings, and to feed it the whole
				 * answer anew at every arrived chunk would mean to parse it as many times as
				 * the chunks that have arrived
				 *
				 * \~
				 */
				http::parser_http_t _parser;
				// Собираемый текст запроса к устройству UPnP
				string _request;
				// Собираемое тело ответа устройства UPnP
				string _payload;
				// Признак того, что ответ устройства UPnP прочитан до конца
				bool _complete;
				/**
				 * \~russian
				 * Признак того, что потоковое событие к устройству UPnP подключено
				 *
				 * @note Соединение держится на весь обмен, а не на один запрос: чтение
				 * перечня требует обращения на каждое перенаправление, и рукопожатие на
				 * каждое из них означало бы столько же случаев его потерять
				 *
				 * \~english
				 * Flag of the stream event to the UPnP device being connected
				 * @note The connection is held for the whole exchange rather than for a single request: the reading
				 * of the list requires a call for every forwarding, and a handshake for
				 * each of them would mean as many occasions to lose it
				 *
				 * \~
				 */
				bool _connected;
			private:
				// Объект работы с адресами ресурсов
				uri_t _uri;
			private:
				// Адрес маршрутизатора, заданный настройкой
				string _router;
				/**
				 * \~russian
				 * Сетевое устройство, которым ведётся обмен
				 *
				 * @note Нужно рассылке SSDP в сети IPv6: групповой адрес там принадлежит
				 * связи, и без указания устройства он неоднозначен. Не заданное настройкой
				 * берётся из маршрута до внешней сети
				 *
				 * \~english
				 * Network device with which the exchange is conducted
				 * @note Needed by the SSDP multicast in an IPv6 network: the group address there belongs to the
				 * link, and without an indication of the device it is ambiguous. What has not been given by the setting
				 * is taken from the route to the external network
				 *
				 * \~
				 */
				string _iface;
				/**
				 * \~russian
				 * Сетевое устройство, взятое из маршрута до внешней сети
				 *
				 * @note Держится отдельно от заданного настройкой затем, что настройка
				 * старше: отыскание маршрута ведётся на каждое обращение, и затирать им
				 * заданное вызывающим неверно
				 *
				 * \~english
				 * Network device taken from the route to the external network
				 * @note It is kept separately from the one given by the setting because the setting is
				 * senior: the discovery of the route is conducted at every call, and to overwrite with it
				 * what has been given by the caller is wrong
				 *
				 * \~
				 */
				string _link;
				// Адрес маршрутизатора, отысканный по таблице маршрутов
				unique_ptr <net::addr_t> _address;
			private:
				// Объект работы с сетевыми адресами
				net_addr_t _addr;
				// Объект работы с сетевыми устройствами
				eth::iface_t _ifaces;
				// Объект получения маршрута до внешней сети
				eth::gateway_t _gateway;
			private:
				// Кодек договора PCP
				proto::portmap::pcp_t _pcp;
				// Кодек рассылки SSDP
				proto::portmap::ssdp_t _ssdp;
				// Кодек запросов SOAP
				proto::portmap::soap_t _soap;
				// Кодек службы перенаправления UPnP
				proto::portmap::upnp_t _upnp;
				// Кодек описания устройства UPnP
				proto::portmap::device_t _device;
				// Кодек договора NAT-PMP
				proto::portmap::natpmp_t _natpmp;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки ошибок событий обмена с маршрутизатором
				 *
				 * @param eid         идентификатор события обмена
				 * @param error       код ошибки события обмена
				 * @param description описание ошибки события обмена
				 *
				 * \~english
				 * @brief Method of processing the errors of the events of the exchange with the router
				 * @param eid         event identifier of the exchange
				 * @param error       error code of the event of the exchange
				 * @param description error description of the event of the exchange
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки ответов маршрутизатора
				 *
				 * @param eid  идентификатор события чтения
				 * @param data данные, полученные от маршрутизатора
				 * @param size размер полученных данных
				 *
				 * \~english
				 * @brief Method of processing the answers of the router
				 * @param eid  event identifier of the reading
				 * @param data data received from the router
				 * @param size size of the received data
				 *
				 * \~
				 */
				void response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки истечения срока ожидания ответа маршрутизатора
				 *
				 * @param eid    идентификатор события обмена
				 * @param action тип действия для истекшего срока ожидания
				 * @param delay  длительность срока ожидания в миллисекундах
				 * @return       нужно ли завершить обработчик после истечения срока
				 *
				 * \~english
				 * @brief Method of processing the expiration of the term of waiting for an answer of the router
				 * @param eid    event identifier of the exchange
				 * @param action action type for the expired waiting term
				 * @param delay  duration of the waiting term in milliseconds
				 * @return       whether the handler should be terminated after the term has expired
				 *
				 * \~
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод отыскания маршрутизатора
				 *
				 * @details Заданный настройкой адрес имеет старшинство над отысканным: сеть
				 * с несколькими маршрутизаторами таблицей маршрутов однозначно не описывается,
				 * и выбор там за приложением
				 *
				 * @return результат отыскания маршрутизатора
				 *
				 * \~english
				 * @brief Method of finding the router
				 * @details The address given by the setting has seniority over the found one: a network
				 * with several routers is not described unambiguously by the routing table,
				 * and the choice there belongs to the application
				 * @return result of finding the router
				 *
				 * \~
				 */
				bool discover() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения сетевого устройства, которым ведётся обмен
				 *
				 * @details Заданное настройкой имеет старшинство над взятым из маршрута до
				 * внешней сети
				 *
				 * @return название сетевого устройства
				 *
				 * \~english
				 * @brief Method of getting the network device with which the exchange is conducted
				 * @details What has been given by the setting has seniority over what has been taken from the route to
				 * the external network
				 * @return name of the network device
				 *
				 * \~
				 */
				const string & iface() const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки принадлежности адреса локальной сети
				 *
				 * @details Устройство доступа в сеть лежит на той же сети, и обращаться
				 * следует лишь к ней: и адрес описания, названный ответом на рассылку, и
				 * адрес управления, собранный по самому описанию, назначает не модуль, а
				 * тот, кто на рассылку ответил
				 *
				 * @warning Проверка обязательна на каждом адресе, по которому пойдёт
				 * обращение, а не на одном лишь адресе описания: основание записи
				 * устройство объявляет само, и увести обращение наружу способно им
				 *
				 * @note Полная проверка требовала бы сличения с адресом отправителя
				 * ответа, но чтение датаграммного события его не выдаёт: договор `read_t`
				 * несёт лишь данные. Оттого проверяется происхождение адреса, а не его
				 * совпадение
				 *
				 * @param host проверяемый адрес устройства
				 * @return     признак принадлежности адреса локальной сети
				 *
				 * \~english
				 * @brief Method of checking the belonging of an address to the local network
				 * @details The network access device lies on the same network, and one should address
				 * only it: both the address of the description named by the answer to the multicast and
				 * the address of the control assembled by the description itself are assigned not by the module but by
				 * the one who has answered the multicast
				 * @warning The check is obligatory at every address to which a call will go,
				 * rather than at the address of the description alone: the base of the record is announced by the
				 * device itself, and it is capable of diverting the call to the outside by it
				 * @note A full check would require a comparison with the address of the sender of the
				 * answer, but the reading of a datagram event does not provide it: the `read_t` contract
				 * carries only the data. Because of that the origin of the address is checked rather than its
				 * coincidence
				 * @param host address of the device being checked
				 * @return     flag of the belonging of the address to the local network
				 *
				 * \~
				 */
				bool local(const string & host) noexcept;
				/**
				 * \~russian
				 * @brief Метод сборки узла для заголовка запроса к устройству UPnP
				 *
				 * @details Запись IPv6 в заголовке `Host` берётся в квадратные скобки:
				 * иначе двоеточия записи неотличимы от двоеточия перед портом
				 *
				 * @param host адрес устройства, с которым ведётся обмен
				 * @param port порт устройства, с которым ведётся обмен
				 * @return     собранный узел для заголовка запроса
				 *
				 * \~english
				 * @brief Method of assembling the host for the header of a request to a UPnP device
				 * @details An IPv6 record in the `Host` header is taken into square brackets:
				 * otherwise the colons of the record are indistinguishable from the colon before the port
				 * @param host address of the device with which the exchange is conducted
				 * @param port port of the device with which the exchange is conducted
				 * @return     assembled host for the header of the request
				 *
				 * \~
				 */
				string authority(const string & host, const uint16_t port) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод заведения события обмена по дейтаграммному договору
				 *
				 * @note Событие пересоздаётся на каждую попытку: назначение получателя на
				 * запущенном событии не действует
				 *
				 * @param type договор перенаправления, по которому ведётся обмен
				 * @return     результат заведения события обмена
				 *
				 * \~english
				 * @brief Method of creating an exchange event over a datagram protocol
				 * @note The event is recreated for every attempt: the assignment of the recipient on
				 * a launched event has no effect
				 * @param type forwarding protocol over which the exchange is conducted
				 * @return     result of creating the exchange event
				 *
				 * \~
				 */
				bool datagram(const type_t type) noexcept;
				/**
				 * \~russian
				 * @brief Метод прекращения заведённого события обмена
				 *
				 * @details Событие заводится прежде, чем собирается просьба, и неудача
				 * сборки либо отправки оставила бы его жить до конца обращения: занятый
				 * порт и подписка на чтение держались бы там, где обмена уже не будет
				 *
				 * @param type договор перенаправления, по которому ведётся обмен
				 *
				 * \~english
				 * @brief Method of terminating a created exchange event
				 * @details The event is created before the request is assembled, and a failure of the
				 * assembly or of the sending would leave it alive until the end of the call: an occupied
				 * port and a read subscription would be held where there will be no exchange any more
				 * @param type forwarding protocol over which the exchange is conducted
				 *
				 * \~
				 */
				void discard(const type_t type) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки просьбы маршрутизатору по дейтаграммному договору
				 *
				 * @param type договор перенаправления, по которому ведётся обмен
				 * @return     результат отправки просьбы
				 *
				 * \~english
				 * @brief Method of sending a request to the router over a datagram protocol
				 * @param type forwarding protocol over which the exchange is conducted
				 * @return     result of sending the request
				 *
				 * \~
				 */
				bool submit(const type_t type) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод определения договора, которому принадлежит событие обмена
				 *
				 * @note Событие обмена удаляется по завершении обращения, но выданное ему
				 * чтение способно прийти уже после удаления: сличать следует, а не
				 * принимать всякое событие за оставшийся договор
				 *
				 * @param eid идентификатор события обмена
				 * @return    договор перенаправления, которому принадлежит событие
				 *
				 * \~english
				 * @brief Method of determining the protocol to which an exchange event belongs
				 * @note The exchange event is removed upon the completion of the call, but a reading issued to
				 * it is capable of arriving already after the removal: one should verify rather than
				 * take every event for the remaining protocol
				 * @param eid event identifier of the exchange
				 * @return    forwarding protocol to which the event belongs
				 *
				 * \~
				 */
				type_t belongs(const event::id_t eid) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения записи обмена по договору перенаправления
				 *
				 * @param type договор перенаправления, по которому ведётся обмен
				 * @return     запись обмена по указанному договору
				 *
				 * \~english
				 * @brief Method of getting the exchange record for a forwarding protocol
				 * @param type forwarding protocol over which the exchange is conducted
				 * @return     exchange record for the specified protocol
				 *
				 * \~
				 */
				exchange_t & exchange(const type_t type) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод приведения кода итога договора PCP к коду причины отказа
				 *
				 * @param result код итога, выданный маршрутизатором
				 * @return       код причины отказа перенаправления
				 *
				 * \~english
				 * @brief Method of bringing a result code of the PCP protocol to a refusal reason code
				 * @param result result code issued by the router
				 * @return       code of the reason of the refusal of the forwarding
				 *
				 * \~
				 */
				error_t reason(const proto::portmap::pcp_t::result_t result) const noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения кода итога договора NAT-PMP к коду причины отказа
				 *
				 * @param result код итога, выданный маршрутизатором
				 * @return       код причины отказа перенаправления
				 *
				 * \~english
				 * @brief Method of bringing a result code of the NAT-PMP protocol to a refusal reason code
				 * @param result result code issued by the router
				 * @return       code of the reason of the refusal of the forwarding
				 *
				 * \~
				 */
				error_t reason(const proto::portmap::natpmp_t::result_t result) const noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения кода итога службы UPnP к коду причины отказа
				 *
				 * @param result код итога, выданный службой устройства
				 * @return       код причины отказа перенаправления
				 *
				 * \~english
				 * @brief Method of bringing a result code of a UPnP service to a refusal reason code
				 * @param result result code issued by the service of the device
				 * @return       code of the reason of the refusal of the forwarding
				 *
				 * \~
				 */
				error_t reason(const proto::portmap::upnp_t::result_t result) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод получения внутреннего адреса машины для просьбы договора PCP
				 *
				 * @note Договор предписывает указывать в просьбе адрес обращающейся машины,
				 * и маршрутизатор сличает его с адресом отправителя дейтаграммы
				 *
				 * @param eid     идентификатор события обмена
				 * @param address место под адрес в записи договора PCP
				 * @return        результат получения адреса
				 *
				 * \~english
				 * @brief Method of getting the internal address of the machine for a request of the PCP protocol
				 * @note The protocol prescribes indicating in the request the address of the calling machine,
				 * and the router verifies it against the address of the sender of the datagram
				 * @param eid     event identifier of the exchange
				 * @param address place for the address in the record of the PCP protocol
				 * @return        result of getting the address
				 *
				 * \~
				 */
				bool client(const event::id_t eid, uint8_t * address) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод отыскания устройства UPnP рассылкой SSDP
				 *
				 * @details Просьба рассылается на групповой адрес: устройство отвечает не
				 * сразу, а спустя случайное время в пределах отведённого срока, поэтому
				 * ответ приходит обычным чтением события, а не следом за отправкой
				 *
				 * @return результат начала отыскания устройства
				 *
				 * \~english
				 * @brief Method of finding a UPnP device by an SSDP multicast
				 * @details The request is multicast to a group address: the device answers not
				 * at once but after a random time within the allotted term, therefore
				 * the answer arrives by an ordinary reading of the event rather than right after the sending
				 * @return result of the beginning of the discovery of the device
				 *
				 * \~
				 */
				bool search() noexcept;
				/**
				 * \~russian
				 * @brief Метод отыскания устройства UPnP рассылкой SSDP на заданную группу
				 *
				 * @details Групп обнаружения в сети IPv6 две - принадлежащая связи и
				 * принадлежащая площадке, - и какой из них объявляет себя устройство,
				 * заранее неизвестно
				 *
				 * @param group групповой адрес обнаружения устройств
				 * @return      результат начала отыскания устройства
				 *
				 * \~english
				 * @brief Method of finding a UPnP device by an SSDP multicast to a given group
				 * @details There are two discovery groups in an IPv6 network — the one belonging to the link and
				 * the one belonging to the site — and which of them the device announces itself in
				 * is unknown beforehand
				 * @param group group address of the discovery of the devices
				 * @return      result of the beginning of the discovery of the device
				 *
				 * \~
				 */
				bool search(string_view group) noexcept;
				/**
				 * \~russian
				 * @brief Метод вступления события рассылки в группу обнаружения устройств
				 *
				 * @details Устройство подписки задаётся тем же, которым уходит рассылка:
				 * групповой адрес в сети IPv6 принадлежит связи, и подписка на чужой связи
				 * оставила бы рассылку без ответа
				 *
				 * @param group групповой адрес обнаружения устройств
				 * @param six   признак того, что обмен ведётся сетью IPv6
				 * @return      результат вступления в группу обнаружения
				 *
				 * \~english
				 * @brief Method of the joining of the multicast event to the device discovery group
				 * @details The device of the subscription is given as the same one with which the multicast goes out:
				 * a group address in an IPv6 network belongs to the link, and a subscription on a foreign link
				 * would leave the multicast without an answer
				 * @param group group address of the discovery of the devices
				 * @param six   flag of the exchange being conducted over an IPv6 network
				 * @return      result of the joining to the discovery group
				 *
				 * \~
				 */
				bool membership(const string & group, const bool six) noexcept;
				/**
				 * \~russian
				 * @brief Метод чтения описания отысканного устройства UPnP
				 *
				 * @return результат начала чтения описания устройства
				 *
				 * \~english
				 * @brief Method of reading the description of a found UPnP device
				 * @return result of the beginning of the reading of the description of the device
				 *
				 * \~
				 */
				bool describe() noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова действия службы отысканного устройства UPnP
				 *
				 * @return результат начала вызова действия службы
				 *
				 * \~english
				 * @brief Method of calling an action of the service of a found UPnP device
				 * @return result of the beginning of the call of the action of the service
				 *
				 * \~
				 */
				bool control() noexcept;
				/**
				 * \~russian
				 * @brief Метод повторения текущего шага обмена по договору UPnP
				 *
				 * @details Устройство приняло подключение, но на запрос не ответило в
				 * отведённый срок. Обращение повторяется тем же шагом, на котором стояло:
				 * повторять весь обмен с рассылки незачем - устройство уже отыскано
				 *
				 * @note Потоковое событие здесь не уничтожается: запрос вызова действия
				 * службы собирается по внутреннему адресу подключённого события, и снять
				 * его нужно уже после сборки. Уничтожает старое событие заведение нового
				 *
				 * @return результат повторения текущего шага обмена
				 *
				 * \~english
				 * @brief Method of repeating the current step of the exchange over the UPnP protocol
				 * @details The device has accepted the connection but has not answered the request within
				 * the allotted term. The call is repeated at the same step at which it stood:
				 * there is no point in repeating the whole exchange from the multicast — the device has already been found
				 * @note The stream event is not destroyed here: the request of the call of an action
				 * of the service is assembled by the internal address of the connected event, and it should be removed
				 * already after the assembly. The old event is destroyed by the creation of a new one
				 * @return result of repeating the current step of the exchange
				 *
				 * \~
				 */
				bool repeat() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод заведения потокового события обмена с устройством UPnP
				 *
				 * @param address адрес устройства, с которым ведётся обмен
				 * @param port    порт устройства, с которым ведётся обмен
				 * @return        результат заведения потокового события обмена
				 *
				 * \~english
				 * @brief Method of creating a stream event of the exchange with a UPnP device
				 * @param address address of the device with which the exchange is conducted
				 * @param port    port of the device with which the exchange is conducted
				 * @return        result of creating the stream event of the exchange
				 *
				 * \~
				 */
				bool stream(string_view address, const uint16_t port) noexcept;
				/**
				 * \~russian
				 * @brief Метод подготовки объекта разбора к чтению ответа устройства UPnP
				 *
				 * @details Объект разбора живёт весь обмен и переиспользуется: сбрасывать его
				 * перед каждым ответом обязательно, иначе следующий ответ разбирался бы
				 * поверх состояния предыдущего
				 *
				 * \~english
				 * @brief Method of preparing the parsing object for the reading of the answer of a UPnP device
				 * @details The parsing object lives for the whole exchange and is reused: resetting it
				 * before every answer is obligatory, otherwise the next answer would be parsed
				 * on top of the state of the previous one
				 *
				 * \~
				 */
				void prepare() noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки подключения к устройству UPnP
				 *
				 * @note Запрос отправляется по подключении, а не раньше: до него отправлять
				 * потоковому событию нечего
				 *
				 * @param eid идентификатор потокового события обмена
				 * @param ok  результат подключения к устройству
				 *
				 * \~english
				 * @brief Method of processing the connection to a UPnP device
				 * @note The request is sent upon the connection rather than earlier: before it there is nothing
				 * to send to the stream event
				 * @param eid event identifier of the stream exchange event
				 * @param ok  result of the connection to the device
				 *
				 * \~
				 */
				void connected(const event::id_t eid, const bool ok) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки данных, полученных от устройства UPnP
				 *
				 * @param eid  идентификатор потокового события обмена
				 * @param data данные, полученные от устройства
				 * @param size размер полученных данных
				 *
				 * \~english
				 * @brief Method of processing the data received from a UPnP device
				 * @param eid  event identifier of the stream exchange event
				 * @param data data received from the device
				 * @param size size of the received data
				 *
				 * \~
				 */
				void incoming(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод разбора прочитанного описания устройства UPnP
				 *
				 * \~english
				 * @brief Method of parsing the read description of a UPnP device
				 *
				 * \~
				 */
				void described() noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора ответа службы устройства UPnP
				 *
				 * \~english
				 * @brief Method of parsing the answer of the service of a UPnP device
				 *
				 * \~
				 */
				void controlled() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод начала обращения к маршрутизатору
				 *
				 * @details Начатое ранее обращение прекращается: одновременно ведётся лишь
				 * одно обращение, и просьба, поданная поверх незавершённой, отменяет её
				 *
				 * @param action  просьба, с которой ведётся обращение
				 * @param mapping перенаправление, о котором ведётся обращение
				 * @return        результат начала обращения
				 *
				 * \~english
				 * @brief Method of beginning a call to the router
				 * @details A call begun earlier is terminated: only one call is conducted at a
				 * time, and a request submitted on top of an unfinished one cancels it
				 * @param action  request with which the call is conducted
				 * @param mapping forwarding about which the call is conducted
				 * @return        result of the beginning of the call
				 *
				 * \~
				 */
				bool perform(const action_t action, const mapping_t & mapping) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод завершения обмена отказом
				 *
				 * @param type  договор перенаправления, по которому вёлся обмен
				 * @param error код причины отказа
				 *
				 * \~english
				 * @brief Method of completing the exchange with a refusal
				 * @param type  forwarding protocol over which the exchange has been conducted
				 * @param error code of the reason of the refusal
				 *
				 * \~
				 */
				void failure(const type_t type, const error_t error) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод сверки отсчёта времени работы маршрутизатора
				 *
				 * @details Договоры NAT-PMP (RFC 6886, раздел 3.6) и PCP (RFC 6887, раздел
				 * 8.5) предписывают сверять этот отсчёт у каждого полученного ответа.
				 * Отсчёт обязан расти не медленнее часов этой машины: пошёл он вспять или
				 * заметно отстал - маршрутизатор своё состояние утратил, и все заведённые
				 * перенаправления пропали вместе с ним
				 *
				 * @note Допуск взят из RFC 6886: отставание прощается в одну шестнадцатую
				 * прошедшего времени и ещё две секунды сверху. Меньший допуск давал бы
				 * ложные срабатывания от расхождения часов, больший - пропускал бы
				 * настоящую потерю состояния
				 *
				 * @note Первый ответ утратой не считается вовсе: сверять его не с чем, и
				 * отсчёт лишь запоминается
				 *
				 * @param type  договор перенаправления, по которому получен ответ
				 * @param epoch отсчёт времени работы маршрутизатора из ответа
				 * @return      признак того, что маршрутизатор утратил своё состояние
				 *
				 * \~english
				 * @brief Method of verifying the counter of the working time of the router
				 * @details The NAT-PMP (RFC 6886, section 3.6) and PCP (RFC 6887, section
				 * 8.5) protocols prescribe verifying this counter at every received answer.
				 * The counter is obliged to grow no slower than the clock of this machine: has it gone backwards or
				 * has it noticeably fallen behind — the router has lost its state, and all the created
				 * forwardings have disappeared along with it
				 * @note The tolerance is taken from RFC 6886: a lag is forgiven within one sixteenth of
				 * the elapsed time and two more seconds on top. A smaller tolerance would give
				 * false triggerings from a divergence of the clocks, a larger one would let through
				 * a real loss of the state
				 * @note The first answer is not considered a loss at all: there is nothing to verify it against, and
				 * the counter is merely remembered
				 * @param type  forwarding protocol over which the answer has been received
				 * @param epoch counter of the working time of the router from the answer
				 * @return      flag of the router having lost its state
				 *
				 * \~
				 */
				bool lost(const type_t type, const uint32_t epoch) noexcept;
				/**
				 * \~russian
				 * @brief Метод приёма объявления, разосланного маршрутизатором
				 *
				 * @details Объявления обоих договоров приходят на один и тот же порт, и
				 * разделяются они изданием в первом октете сообщения. Разобранное
				 * объявление сверяется отсчётом времени работы наравне с обычным ответом:
				 * рассылается оно и по утрате состояния, и по смене внешнего адреса, а
				 * что именно случилось, видно лишь по отсчёту
				 *
				 * @note Объявление приходит от любого узла сети, а не от того, к кому
				 * обращались: отправитель проверяется на принадлежность местной сети,
				 * и объявление извне отбрасывается молча
				 *
				 * @param eid  идентификатор события приёма объявления
				 * @param data полученное объявление маршрутизатора
				 * @param size размер полученного объявления
				 *
				 * \~english
				 * @brief Method of receiving an announcement multicast by the router
				 * @details The announcements of both protocols arrive at one and the same port, and
				 * they are separated by the version in the first octet of the message. A parsed
				 * announcement is verified by the counter of the working time on a par with an ordinary answer:
				 * it is multicast both upon a loss of the state and upon a change of the external address, while
				 * what exactly has happened is visible only by the counter
				 * @note An announcement arrives from any node of the network rather than from the one that has been
				 * addressed: the sender is checked for the belonging to the local network,
				 * and an announcement from the outside is discarded silently
				 * @param eid  event identifier of the reception of the announcement
				 * @param data received announcement of the router
				 * @param size size of the received announcement
				 *
				 * \~
				 */
				void announced(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод приёма отправителя объявлений маршрутизатора
				 *
				 * @details Дейтаграммное серверное событие разбирает принятое по
				 * отправителям, и на каждого заводит своё событие. Заведённое событие
				 * подписок не несёт, и выставить их обязан принимающий, иначе
				 * принимаемое им обрабатывать будет некому
				 *
				 * @note Время жизни заведённого события движку не принадлежит, и держится
				 * оно до отключения приёма объявлений: маршрутизатор рассылает объявление
				 * не однажды, а десять раз подряд, и закрывать событие на первом же было
				 * бы заведением его заново на каждом следующем
				 *
				 * @param eid идентификатор события приёма объявлений
				 * @param oid идентификатор события отправителя объявлений
				 *
				 * \~english
				 * @brief Method of accepting a sender of the announcements of the router
				 * @details A datagram server event sorts what has been accepted by the
				 * senders and creates an own event for each of them. A created event carries no
				 * subscriptions, and the accepting side is obliged to set them, otherwise there will be no one
				 * to process what it accepts
				 * @note The lifetime of a created event does not belong to the engine, and it is kept
				 * until the reception of the announcements is disabled: the router multicasts an announcement
				 * not once but ten times in a row, and to close the event at the very first one would
				 * mean to create it anew at every following one
				 * @param eid event identifier of the reception of the announcements
				 * @param oid event identifier of the sender of the announcements
				 *
				 * \~
				 */
				void accepted(const event::id_t eid, const event::id_t oid) noexcept;
				/**
				 * \~russian
				 * @brief Метод завершения обмена по одному договору
				 *
				 * @details Обмены прочих договоров прекращаются: под видом опроса AUTO их
				 * ведётся несколько разом, и первый пришедший ответ делает остальные ненужными
				 *
				 * @param type договор перенаправления, по которому получен ответ
				 *
				 * \~english
				 * @brief Method of completing the exchange over a single protocol
				 * @details The exchanges of the other protocols are terminated: under the AUTO polling kind several of them
				 * are conducted at once, and the first answer that has arrived makes the rest unnecessary
				 * @param type forwarding protocol over which the answer has been received
				 *
				 * \~
				 */
				void complete(const type_t type) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод прекращения всех ведущихся обменов
				 *
				 * \~english
				 * @brief Method of terminating all the exchanges being conducted
				 *
				 * \~
				 */
				void cancel() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 *
				 * \~english
				 * @brief Method of setting the callback functions
				 * @param callback callback functions
				 *
				 * \~
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения вида опроса маршрутизатора
				 *
				 * @return вид опроса маршрутизатора
				 *
				 * \~english
				 * @brief Method of getting the polling kind of the router
				 * @return polling kind of the router
				 *
				 * \~
				 */
				type_t getType() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки вида опроса маршрутизатора
				 *
				 * @param type вид опроса маршрутизатора для установки
				 *
				 * \~english
				 * @brief Method of setting the polling kind of the router
				 * @param type polling kind of the router to be set
				 *
				 * \~
				 */
				void setType(const type_t type) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения разновидности сети, в которой ведётся обмен
				 *
				 * @return установленная разновидность сети
				 *
				 * \~english
				 * @brief Method of getting the variety of the network in which the exchange is conducted
				 * @return set variety of the network
				 *
				 * \~
				 */
				family_t getFamily() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки разновидности сети, в которой ведётся обмен
				 *
				 * @details Разновидность выбирается настройкой, а не отыскивается: сеть
				 * бывает и двойной, и какой из двух маршрутизатор перенаправляет порты -
				 * решает тот, кто модуль применяет
				 *
				 * @note Договор NAT-PMP разновидности IPv6 не имеет, и в сети IPv6 он не
				 * опрашивается даже видом опроса `AUTO`
				 *
				 * @param family разновидность сети для установки
				 *
				 * \~english
				 * @brief Method of setting the variety of the network in which the exchange is conducted
				 * @details The variety is chosen by the setting rather than found out: a network happens to be
				 * a dual one as well, and which of the two the router forwards the ports over is
				 * decided by the one who applies the module
				 * @note The NAT-PMP protocol has no IPv6 variety, and in an IPv6 network it is not
				 * polled even under the `AUTO` polling kind
				 * @param family variety of the network to be set
				 *
				 * \~
				 */
				void setFamily(const family_t family) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки срока ожидания ответа маршрутизатора
				 *
				 * @warning Нулевым сроком ожидание снимается вовсе, и обращение остаётся
				 * без предела: повторять просьбу становится нечему, а маршрутизатор,
				 * договора не знающий, молчит всегда. Итог такого обращения приходит
				 * лишь ответом устройства, а молчание длится, пока обмен не прекратят
				 *
				 * @param delay срок ожидания ответа маршрутизатора в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the term of waiting for an answer of the router
				 * @warning With a zero term the waiting is removed entirely, and the call remains
				 * without a limit: there becomes nothing to repeat the request by, while a router
				 * that does not know the protocol is silent always. The result of such a call arrives
				 * only by an answer of the device, while the silence lasts until the exchange is terminated
				 * @param delay term of waiting for an answer of the router in milliseconds
				 *
				 * \~
				 */
				void setTimeout(const uint32_t delay) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки количества попыток обращения к маршрутизатору
				 *
				 * @param attempts количество попыток обращения к маршрутизатору
				 *
				 * \~english
				 * @brief Method of setting the number of the attempts of a call to the router
				 * @param attempts number of the attempts of a call to the router
				 *
				 * \~
				 */
				void setAttempts(const uint8_t attempts) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса маршрутизатора
				 *
				 * @details Заданный адрес отменяет отыскание маршрутизатора по таблице
				 * маршрутов. Пустой адрес возвращает отыскание
				 *
				 * @param router адрес маршрутизатора для установки
				 *
				 * \~english
				 * @brief Method of setting the address of the router
				 * @details A given address cancels the discovery of the router by the routing
				 * table. An empty address returns the discovery
				 * @param router address of the router to be set
				 *
				 * \~
				 */
				void setRouter(string_view router) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сетевого устройства, которым ведётся обмен
				 *
				 * @details Нужно рассылке SSDP в сети IPv6: групповой адрес обнаружения
				 * принадлежит связи, и без указания устройства система разослать просьбу
				 * отказывается. Не заданное берётся из маршрута до внешней сети
				 *
				 * @note Настройка нужна лишь тогда, когда маршрут до внешней сети ведёт не
				 * тем устройством, которым машина видит маршрутизатор - к примеру, когда
				 * движение завёрнуто в туннель
				 *
				 * @param iface название сетевого устройства для установки
				 *
				 * \~english
				 * @brief Method of setting the network device with which the exchange is conducted
				 * @details Needed by the SSDP multicast in an IPv6 network: the group address of the discovery
				 * belongs to the link, and without an indication of the device the system refuses to multicast
				 * the request. What has not been given is taken from the route to the external network
				 * @note The setting is needed only when the route to the external network leads not by
				 * the device with which the machine sees the router — for example, when
				 * the traffic is wrapped into a tunnel
				 * @param iface name of the network device to be set
				 *
				 * \~
				 */
				void setIface(string_view iface) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения внешнего адреса маршрутизатора
				 *
				 * @return результат отправки просьбы
				 *
				 * \~english
				 * @brief Method of getting the external address of the router
				 * @return result of sending the request
				 *
				 * \~
				 */
				bool external() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения перечня заведённых перенаправлений портов
				 *
				 * @details Спрашивает у маршрутизатора, какие перенаправления у него
				 * заведены. Перечень выдаётся обратным вызовом `mappings`
				 *
				 * @note Перечень этот общий на всю сеть, а не свой у машины: в нём
				 * окажутся и перенаправления, заведённые другими. Чьё именно
				 * перенаправление, видно по полю `internalClient`
				 *
				 * @note Перечня служба разом не выдаёт: перенаправления читаются по
				 * одному, наращивая порядковый номер, и обращений к маршрутизатору
				 * выходит столько же, сколько перенаправлений. Мгновенным чтение
				 * поэтому не бывает
				 *
				 * @warning Перечень выдаёт лишь договор UPnP: у NAT-PMP и PCP чтения
				 * заведённых перенаправлений нет вовсе. При ином виде опроса просьба
				 * завершается отказом `NOT_SUPPORTED`
				 *
				 * @return результат отправки просьбы
				 *
				 * \~english
				 * @brief Method of getting the list of the created port forwardings
				 * @details Asks the router which forwardings are created on
				 * it. The list is issued by the `mappings` callback
				 * @note This list is common to the whole network rather than being an own one of the machine: in it
				 * there will be the forwardings created by others as well. Whose exactly a
				 * forwarding is, is visible by the `internalClient` field
				 * @note The service does not provide the list at once: the forwardings are read one by
				 * one, incrementing the ordinal number, and there turn out to be as many calls to the router
				 * as there are forwardings. The reading is therefore never
				 * instantaneous
				 * @warning Only the UPnP protocol provides the list: NAT-PMP and PCP have no reading
				 * of the created forwardings at all. Under another polling kind the request
				 * is completed with a `NOT_SUPPORTED` refusal
				 * @return result of sending the request
				 *
				 * \~
				 */
				bool list() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод заведения перенаправления порта
				 *
				 * @param mapping перенаправление порта для заведения
				 * @return        результат отправки просьбы
				 *
				 * \~english
				 * @brief Method of creating a port forwarding
				 * @param mapping port forwarding to be created
				 * @return        result of sending the request
				 *
				 * \~
				 */
				bool open(const mapping_t & mapping) noexcept;
				/**
				 * \~russian
				 * @brief Метод удаления заведённого перенаправления порта
				 *
				 * @param mapping перенаправление порта для удаления
				 * @return        результат отправки просьбы
				 *
				 * \~english
				 * @brief Method of removing a created port forwarding
				 * @param mapping port forwarding to be removed
				 * @return        result of sending the request
				 *
				 * \~
				 */
				bool close(const mapping_t & mapping) noexcept;
				/**
				 * \~russian
				 * @brief Метод продления срока заведённого перенаправления порта
				 *
				 * @details Продление снимать и заводить перенаправление заново не требует,
				 * и различие между договорами оно скрывает: службе соединения UPnP и
				 * договорам PCP с NAT-PMP продлевает повторная просьба, а службе заслона
				 * IPv6 - отдельное действие, поскольку заведение заново выдало бы новый
				 * опознаватель пробоя
				 *
				 * @note Продление пробоя заслона IPv6 требует опознавателя, выданного при
				 * его проделывании: без него просьба отвергается, не доходя до устройства.
				 * Договоры PCP и NAT-PMP опознавателя не имеют и продлеваются признаками
				 * самого перенаправления
				 *
				 * @param mapping перенаправление порта для продления
				 * @return        результат отправки просьбы
				 *
				 * \~english
				 * @brief Method of prolonging the term of a created port forwarding
				 * @details The prolongation does not require removing and creating the forwarding anew,
				 * and it hides the difference between the protocols: for the UPnP connection service and
				 * for the PCP and NAT-PMP protocols a repeated request prolongs, while for the IPv6 firewall
				 * service — a separate action, since a creation anew would issue a new
				 * identifier of the pinhole
				 * @note The prolongation of an IPv6 firewall pinhole requires the identifier issued at
				 * its making: without it the request is rejected without reaching the device.
				 * The PCP and NAT-PMP protocols have no identifier and are prolonged by the attributes
				 * of the forwarding itself
				 * @param mapping port forwarding to be prolonged
				 * @return        result of sending the request
				 *
				 * \~
				 */
				bool renew(const mapping_t & mapping) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод включения приёма объявлений маршрутизатора
				 *
				 * @details Маршрутизатор, перезагрузившись или сменив внешний адрес,
				 * рассылает об этом объявление на групповой адрес - десять раз с
				 * нарастающим промежутком (RFC 6886, раздел 3.2.1, и RFC 6887, раздел 14.1).
				 * Приём объявлений даёт узнать об утрате состояния сразу, а не при
				 * следующем обращении: без него сверка отсчёта времени работы срабатывает
				 * лишь тогда, когда вызывающий сам о чём-то спросил, а до этого
				 * перенаправление уже мертво, но числится живым
				 *
				 * @details Утрата состояния объявляется обратным вызовом `reset`, а
				 * сменившийся внешний адрес - обратным вызовом `external`: договор
				 * NAT-PMP несёт его прямо в объявлении, и это вторая половина того, ради
				 * чего объявление рассылается
				 *
				 * @note Приём заводится по просьбе, а не сам собой: он держит гнездо на
				 * договорном порту всё время работы, вступает в группу и принимает
				 * дейтаграммы от любого узла сети. Приложению, которому объявления не
				 * нужны, всё это ни к чему
				 *
				 * @note Оба договора рассылают объявления на один и тот же порт, и
				 * разделяются они изданием в первом октете сообщения, как и обычные
				 * ответы. Договор UPnP отсчёта времени работы не имеет вовсе, и утрату
				 * состояния по нему заметить нечем
				 *
				 * @warning Порт объявлений бывает занят другой службой перенаправления,
				 * работающей на той же машине: тогда приём завести не удаётся, и метод
				 * отвечает отказом. Обращений к маршрутизатору это не отменяет - они
				 * ведутся своими событиями и приёма не требуют
				 *
				 * @param mode режим приёма объявлений маршрутизатора
				 * @return     результат включения приёма объявлений
				 *
				 * \~english
				 * @brief Method of enabling the reception of the announcements of the router
				 * @details A router, having rebooted or having changed its external address,
				 * multicasts an announcement about this to a group address — ten times with
				 * a growing interval (RFC 6886, section 3.2.1, and RFC 6887, section 14.1).
				 * The reception of the announcements makes it possible to learn about a loss of the state at once rather than at the
				 * next call: without it the verification of the counter of the working time triggers
				 * only when the caller has asked about something itself, while before that
				 * the forwarding is already dead but is listed as alive
				 * @details A loss of the state is announced by the `reset` callback, while
				 * a changed external address — by the `external` callback: the NAT-PMP
				 * protocol carries it right in the announcement, and this is the second half of what
				 * the announcement is multicast for
				 * @note The reception is created upon a request rather than by itself: it holds a socket on
				 * the protocol port for the whole time of the work, joins the group and accepts
				 * datagrams from any node of the network. An application that does not need the announcements
				 * has no use for all of that
				 * @note Both protocols multicast the announcements to one and the same port, and
				 * they are separated by the version in the first octet of the message, just like the ordinary
				 * answers. The UPnP protocol has no counter of the working time at all, and there is nothing
				 * to notice a loss of the state by there
				 * @warning The port of the announcements happens to be occupied by another forwarding service
				 * working on the same machine: then the reception cannot be created, and the method
				 * answers with a refusal. This does not cancel the calls to the router — they
				 * are conducted by their own events and do not require the reception
				 * @param mode mode of the reception of the announcements of the router
				 * @return     result of enabling the reception of the announcements
				 *
				 * \~
				 */
				bool announce(const bool mode) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Конструктор копирования (запрещаем)
				 *
				 *
				 * \~english
				 * @brief Copy constructor (prohibited)
				 *
				 * \~
				 */
				Portmap(const Portmap &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 *
				 * \~english
				 * @brief Copy assignment operator (prohibited)
				 * @return current value of the object
				 *
				 * \~
				 */
				Portmap & operator = (const Portmap &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Portmap(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Portmap() noexcept;
		} portmap_t;
	};
};

#endif // __AWH_UNIT_PORTMAP__
