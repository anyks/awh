/**
 * @file upnp.hpp
 * @date 2026-08-02
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
 * \~russian
 * @brief Заголовочный файл кодека действий службы перенаправления UPnP — сборка вызовов
 *        заведения, снятия и перечисления перенаправлений, разбор ответов и коды ошибок службы
 *
 * \~english
 * @brief Header file of the codec of the actions of the UPnP forwarding service — the assembly of the calls
 *        of the creation, of the removal and of the enumeration of the forwardings, the parsing of the answers and the error codes of the service
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROTO_PORTMAP_UPNP__
#define __AWH_PROTO_PORTMAP_UPNP__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "soap.hpp"
#include "device.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../sys/macro_push.hpp"

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён протоколов
	 *
	 *
	 * \~english
	 * @brief Protocols namespace
	 *
	 * \~
	 */
	namespace proto {
		/**
		 * \~russian
		 * @brief Пространство имён договоров перенаправления портов
		 *
		 *
		 * \~english
		 * @brief Port forwarding protocols namespace
		 *
		 * \~
		 */
		namespace portmap {
			/**
			 * \~russian
			 * @brief Класс кодека действий службы перенаправления UPnP
			 *
			 * @details Собирает вызовы действий службы соединения и разбирает ответы на них.
			 * Кодек обмена не ведёт: отправить собранный вызов и получить ответ обязан
			 * вызывающий
			 *
			 * @note Кодек охватывает действия службы соединения, нужные для перенаправления
			 * портов: заведение и снятие перенаправления, чтение внешнего адреса и обход
			 * заведённых перенаправлений
			 *
			 * \~english
			 * @brief Class of the codec of the actions of the UPnP forwarding service
			 * @details Assembles the calls of the actions of the connection service and parses the answers to them.
			 * The codec does not conduct the exchange: the caller is obliged to send the assembled call and to obtain the answer
			 * @note The codec covers the actions of the connection service needed for a forwarding
			 * of the ports: the creation and the removal of a forwarding, the reading of the external address and the traversal
			 * of the created forwardings
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ UPnP {
				public:
					/**
					 * \~russian
					 * @brief Наибольшая допустимая длина описания перенаправления
					 *
					 * @note Предел взят с запасом от принятого службами: описание длиннее
					 * часть маршрутизаторов обрезает, а часть отвергает вызов целиком
					 *
					 * \~english
					 * @brief Largest admissible length of the description of a forwarding
					 * @note The limit is taken with a margin from the one accepted by the services: a longer description
					 * a part of the routers truncates, while a part rejects the call in full
					 *
					 * \~
					 */
					static constexpr size_t MAX_DESCRIPTION = 0x40;
					/**
					 * \~russian
					 * @brief Наибольший допустимый срок жизни пробоя заслона IPv6
					 *
					 * @note Предел задан договором: сроку отведён промежуток от одной
					 * секунды до суток, и просьба за его пределами устройством отвергается
					 *
					 * \~english
					 * @brief Largest admissible lifetime of an IPv6 firewall pinhole
					 * @note The limit is given by the protocol: an interval from one
					 * second to a day is allotted to the term, and a request beyond its limits is rejected by the device
					 *
					 * \~
					 */
					static constexpr uint32_t MAX_LIFETIME = 0x15180;
				public:
					/**
					 * \~russian
					 * @brief Договоры перенаправления порта
					 *
					 * \~english
					 * @brief Protocols of a port forwarding
					 *
					 * \~
					 */
					enum class proto_t : uint8_t {
						NONE = 0x00, // Договор не определён
						UDP  = 0x01, // Перенаправление порта UDP
						TCP  = 0x02  // Перенаправление порта TCP
					};
					/**
					 * \~russian
					 * @brief Действия службы соединения
					 *
					 * \~english
					 * @brief Actions of the connection service
					 *
					 * \~
					 */
					enum class action_t : uint8_t {
						NONE      = 0x00, // Действие не определено
						ADD       = 0x01, // Завести перенаправление порта
						DELETE    = 0x02, // Снять перенаправление порта
						EXTERNAL  = 0x03, // Узнать внешний адрес маршрутизатора
						ENTRY     = 0x04, // Прочитать перенаправление по порядковому номеру
						SPECIFIC  = 0x05, // Прочитать перенаправление по внешнему порту
						STATUS    = 0x06, // Узнать состояние соединения маршрутизатора
						PINHOLE   = 0x07, // Проделать пробой заслона IPv6
						UNPINHOLE = 0x08, // Заделать проделанный пробой заслона IPv6
						FIREWALL  = 0x09, // Узнать состояние заслона IPv6 маршрутизатора
						REPINHOLE = 0x0A  // Продлить срок проделанного пробоя заслона IPv6
					};
					/**
					 * \~russian
					 * @brief Коды ошибок, выдаваемые службой
					 *
					 * @note Коды заданы договором UPnP IGD и приходят в подробностях отказа.
					 * Различать их необходимо: одни означают, что просить бесполезно, другие -
					 * что следует просить иначе
					 *
					 * \~english
					 * @brief Error codes issued by the service
					 * @note The codes are given by the UPnP IGD protocol and arrive in the details of a refusal.
					 * It is necessary to distinguish them: some mean that it is useless to ask, others —
					 * that one should ask differently
					 *
					 * \~
					 */
					enum class result_t : uint32_t {
						SUCCESS               = 0x000, // Просьба выполнена
						INVALID_ACTION        = 0x191, // Действие службе неизвестно
						INVALID_ARGS          = 0x192, // Доводы вызова построены ошибочно
						ACTION_FAILED         = 0x1F5, // Действие выполнить не удалось
						NOT_AUTHORIZED        = 0x25E, // Действие отвергнуто настройкой маршрутизатора
						PINHOLE_EXHAUSTED     = 0x2BD, // У маршрутизатора не осталось места под пробои
						FIREWALL_DISABLED     = 0x2BE, // Заслон IPv6 у маршрутизатора отключён
						PINHOLE_NOT_ALLOWED   = 0x2BF, // Пробои заслона IPv6 маршрутизатором запрещены
						NO_SUCH_PINHOLE       = 0x2C0, // Пробоя с таким опознавателем нет
						PROTO_NOT_SUPPORTED   = 0x2C1, // Договор пробоя маршрутизатором не поддерживается
						INT_PORT_NOT_WILDCARD = 0x2C2, // Пустой внутренний порт пробоя не допускается
						PROTO_NOT_WILDCARD    = 0x2C3, // Пустой договор пробоя не допускается
						SRC_NOT_WILDCARD      = 0x2C4, // Пустой внешний узел пробоя не допускается
						NO_TRAFFIC_RECEIVED   = 0x2C5, // Через пробой движения не проходило
						INDEX_INVALID         = 0x2C9, // Порядковый номер перенаправления вне перечня
						NO_SUCH_ENTRY         = 0x2CA, // Перенаправления с такими признаками нет
						WILDCARD_NOT_SRC      = 0x2CB, // Пустой внешний узел здесь не допускается
						WILDCARD_NOT_EXT      = 0x2CC, // Пустой внешний порт здесь не допускается
						CONFLICT              = 0x2CE, // Перенаправление занято другой машиной
						SAME_PORT_REQUIRED    = 0x2D4, // Маршрутизатор требует равенства портов
						ONLY_PERMANENT_LEASES = 0x2D5, // Маршрутизатор заводит лишь бессрочные перенаправления
						REMOTE_ONLY_WILDCARD  = 0x2D6, // Маршрутизатор принимает лишь пустой внешний узел
						EXT_ONLY_WILDCARD     = 0x2D7, // Маршрутизатор принимает лишь пустой внешний порт
						NO_PORT_MAPS          = 0x2D8, // У маршрутизатора не осталось места под перенаправления
						CONFLICT_MECHANISM    = 0x2D9, // Перенаправление занято иным средством
						WILDCARD_NOT_INT      = 0x2DC  // Пустой внутренний порт здесь не допускается
					};
				public:
					/**
					 * \~russian
					 * @brief Структура перенаправления порта
					 *
					 * \~english
					 * @brief Structure of a port forwarding
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Mapping {
						/**
						 * \~russian
						 * Признак включения перенаправления
						 *
						 * @note Перенаправление заводят выключенным редко, однако при обходе
						 * заведённых такие встречаются: они место занимают, а подключений
						 * не пропускают
						 *
						 * \~english
						 * Flag of the enabling of the forwarding
						 * @note A forwarding is rarely created disabled, however at a traversal of the
						 * created ones such ones are met: they take up a place but do not let the connections
						 * through
						 *
						 * \~
						 */
						bool enabled;
						// Договор перенаправления порта
						proto_t proto;
						// Внешний порт перенаправления
						uint16_t externalPort;
						// Внутренний порт перенаправления
						uint16_t internalPort;
						/**
						 * \~russian
						 * Срок жизни перенаправления в секундах
						 *
						 * @note Нулевой срок означает перенаправление бессрочное: часть
						 * маршрутизаторов иных не заводит вовсе
						 *
						 * \~english
						 * Lifetime of the forwarding in seconds
						 * @note A zero term means an indefinite forwarding: a part of the
						 * routers creates no other ones at all
						 *
						 * \~
						 */
						uint32_t lifeTime;
						/**
						 * \~russian
						 * Внешний узел, подключения с которого пропускаются
						 *
						 * @note Пустое значение означает «с любого узла», и именно его
						 * принимает подавляющее большинство маршрутизаторов
						 *
						 * \~english
						 * External node the connections from which are let through
						 * @note An empty value means «from any node», and it is exactly it that
						 * the overwhelming majority of the routers accepts
						 *
						 * \~
						 */
						string remoteHost;
						// Внутренний адрес машины, которой отдаются подключения
						string internalClient;
						// Описание перенаправления, видное в настройках маршрутизатора
						string description;
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
						Mapping() noexcept :
						 enabled(true), proto(proto_t::NONE), externalPort(0),
						 internalPort(0), lifeTime(0) {}
					} mapping_t;
					/**
					 * \~russian
					 * @brief Структура пробоя заслона IPv6
					 *
					 * @details Пробой - это не перенаправление: преобразования адресов в сети
					 * IPv6 нет, и просьба означает разрешение подключений сквозь заслон
					 * маршрутизатора к машине, у которой адрес и так свой. Оттого внешнего
					 * порта у пробоя нет вовсе - есть порт того узла, откуда подключения
					 * пропускаются
					 *
					 * @note Собрано по договору UPnP IGD:2 (WANIPv6FirewallControl:1) и
					 * проверено на живом устройстве: MiniUPnPd 2.3.7 с заслоном IPv6 на
					 * netfilter. Пробой заводится и заделывается, а заведённое разрешение
					 * отвечает переданным признакам знак в знак
					 *
					 * \~english
					 * @brief Structure of an IPv6 firewall pinhole
					 * @details A pinhole is not a forwarding: there is no address translation in an IPv6
					 * network, and the request means a permission of the connections through the firewall
					 * of the router to a machine which has an address of its own anyway. Because of that a pinhole has no external
					 * port at all — there is the port of that node from which the connections
					 * are let through
					 * @note Assembled by the UPnP IGD:2 protocol (WANIPv6FirewallControl:1) and
					 * verified on a live device: MiniUPnPd 2.3.7 with an IPv6 firewall on
					 * netfilter. A pinhole is made and patched up, while the created permission
					 * corresponds to the passed attributes character by character
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Pinhole {
						// Договор пробоя заслона
						proto_t proto;
						/**
						 * \~russian
						 * Порт узла, с которого пропускаются подключения
						 *
						 * @note Нулевой порт означает «с любого порта»
						 *
						 * \~english
						 * Port of the node from which the connections are let through
						 * @note A zero port means «from any port»
						 *
						 * \~
						 */
						uint16_t remotePort;
						// Внутренний порт машины, которой отдаются подключения
						uint16_t internalPort;
						/**
						 * \~russian
						 * Срок жизни пробоя в секундах
						 *
						 * @note Бессрочных пробоев договор не заводит: срок обязателен, и
						 * пробой следует продлевать до его истечения
						 *
						 * \~english
						 * Lifetime of the pinhole in seconds
						 * @note The protocol does not create indefinite pinholes: the term is obligatory, and
						 * a pinhole should be prolonged before its expiration
						 *
						 * \~
						 */
						uint32_t lifeTime;
						/**
						 * \~russian
						 * Внешний узел, подключения с которого пропускаются
						 *
						 * @note Пустое значение означает «с любого узла», однако принимает
						 * его не всякое устройство: часть отвечает отказом `SRC_NOT_WILDCARD`
						 *
						 * \~english
						 * External node the connections from which are let through
						 * @note An empty value means «from any node», however not every device accepts
						 * it: a part answers with a `SRC_NOT_WILDCARD` refusal
						 *
						 * \~
						 */
						string remoteHost;
						// Внутренний адрес машины, которой отдаются подключения
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
						Pinhole() noexcept :
						 proto(proto_t::NONE), remotePort(0),
						 internalPort(0), lifeTime(0) {}
					} pinhole_t;
					/**
					 * \~russian
					 * @brief Структура собранного вызова действия службы
					 *
					 * \~english
					 * @brief Structure of an assembled call of an action of a service
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Request {
						// Название вызываемого действия службы
						string action;
						// Обозначение вызываемого действия для поля заголовка запроса
						string header;
						// Собранный текст вызова действия службы
						string body;
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
						Request() noexcept {}
						/**
						 * \~russian
						 * @brief Метод проверки собранного вызова на пригодность
						 *
						 * @return результат проверки
						 *
						 * \~english
						 * @brief Method of checking an assembled call for its validity
						 * @return result of the check
						 *
						 * \~
						 */
						bool valid() const noexcept {
							// Выводим итог проверки собранного вызова на пригодность
							return (!this->action.empty() && !this->header.empty() && !this->body.empty());
						}
					} request_t;
				private:
					// Объект кодека договора SOAP
					soap_t _soap;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * \~russian
					 * @brief Метод сборки вызова заведения перенаправления порта
					 *
					 * @details Тем же вызовом перенаправление и продлевают: повторная просьба
					 * отсчитывает срок жизни заново
					 *
					 * @param service обозначение вида службы соединения
					 * @param mapping параметры заводимого перенаправления порта
					 * @return        собранный вызов действия службы
					 *
					 * \~english
					 * @brief Method of assembling a call of the creation of a port forwarding
					 * @details A forwarding is prolonged by the same call: a repeated request
					 * counts the lifetime anew
					 * @param service designation of the kind of the connection service
					 * @param mapping parameters of the port forwarding being created
					 * @return        assembled call of the action of the service
					 *
					 * \~
					 */
					request_t add(const string_view service, const mapping_t & mapping) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки вызова снятия перенаправления порта
					 *
					 * @param service      обозначение вида службы соединения
					 * @param proto        договор снимаемого перенаправления порта
					 * @param externalPort внешний порт снимаемого перенаправления
					 * @param remoteHost   внешний узел снимаемого перенаправления
					 * @return             собранный вызов действия службы
					 *
					 * \~english
					 * @brief Method of assembling a call of the removal of a port forwarding
					 * @param service      designation of the kind of the connection service
					 * @param proto        protocol of the port forwarding being removed
					 * @param externalPort external port of the forwarding being removed
					 * @param remoteHost   external node of the forwarding being removed
					 * @return             assembled call of the action of the service
					 *
					 * \~
					 */
					request_t remove(const string_view service, const proto_t proto, const uint16_t externalPort, const string_view remoteHost = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки вызова чтения внешнего адреса маршрутизатора
					 *
					 * @param service обозначение вида службы соединения
					 * @return        собранный вызов действия службы
					 *
					 * \~english
					 * @brief Method of assembling a call of the reading of the external address of the router
					 * @param service designation of the kind of the connection service
					 * @return        assembled call of the action of the service
					 *
					 * \~
					 */
					request_t external(const string_view service) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки вызова чтения перенаправления по порядковому номеру
					 *
					 * @details Перечня перенаправлений служба не выдаёт: обходить их следует
					 * по одному, наращивая номер, пока служба не ответит отказом о выходе за
					 * перечень
					 *
					 * @param service обозначение вида службы соединения
					 * @param index   порядковый номер читаемого перенаправления
					 * @return        собранный вызов действия службы
					 *
					 * \~english
					 * @brief Method of assembling a call of the reading of a forwarding by an ordinal number
					 * @details The service does not provide a list of the forwardings: they should be traversed
					 * one by one, incrementing the number, until the service answers with a refusal about a going beyond
					 * the list
					 * @param service designation of the kind of the connection service
					 * @param index   ordinal number of the forwarding being read
					 * @return        assembled call of the action of the service
					 *
					 * \~
					 */
					request_t entry(const string_view service, const uint32_t index) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки вызова чтения перенаправления по внешнему порту
					 *
					 * @param service      обозначение вида службы соединения
					 * @param proto        договор читаемого перенаправления порта
					 * @param externalPort внешний порт читаемого перенаправления
					 * @param remoteHost   внешний узел читаемого перенаправления
					 * @return             собранный вызов действия службы
					 *
					 * \~english
					 * @brief Method of assembling a call of the reading of a forwarding by an external port
					 * @param service      designation of the kind of the connection service
					 * @param proto        protocol of the port forwarding being read
					 * @param externalPort external port of the forwarding being read
					 * @param remoteHost   external node of the forwarding being read
					 * @return             assembled call of the action of the service
					 *
					 * \~
					 */
					request_t specific(const string_view service, const proto_t proto, const uint16_t externalPort, const string_view remoteHost = "") const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки вызова чтения состояния соединения маршрутизатора
					 *
					 * @param service обозначение вида службы соединения
					 * @return        собранный вызов действия службы
					 *
					 * \~english
					 * @brief Method of assembling a call of the reading of the state of the connection of the router
					 * @param service designation of the kind of the connection service
					 * @return        assembled call of the action of the service
					 *
					 * \~
					 */
					request_t status(const string_view service) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки вызова проделывания пробоя заслона IPv6
					 *
					 * @details Тем же вызовом пробой и продлевают: повторная просьба с теми же
					 * признаками отсчитывает срок жизни заново
					 *
					 * @note Вызов проверен на живом устройстве - см. замечание к структуре пробоя
					 *
					 * @param service обозначение вида службы заслона IPv6
					 * @param pinhole параметры проделываемого пробоя заслона
					 * @return        собранный вызов действия службы
					 *
					 * \~english
					 * @brief Method of assembling a call of the making of an IPv6 firewall pinhole
					 * @details A pinhole is prolonged by the same call: a repeated request with the same
					 * attributes counts the lifetime anew
					 * @note The call has been verified on a live device — see the note to the structure of a pinhole
					 * @param service designation of the kind of the IPv6 firewall service
					 * @param pinhole parameters of the firewall pinhole being made
					 * @return        assembled call of the action of the service
					 *
					 * \~
					 */
					request_t pinhole(const string_view service, const pinhole_t & pinhole) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки вызова заделывания пробоя заслона IPv6
					 *
					 * @details Пробой заделывается по опознавателю, выданному при его
					 * проделывании, а не по признакам: одним и тем же признакам отвечает
					 * несколько пробоев, и снимать их следует поимённо
					 *
					 * @note Вызов проверен на живом устройстве - см. замечание к структуре пробоя
					 *
					 * @param service обозначение вида службы заслона IPv6
					 * @param unique  опознаватель заделываемого пробоя заслона
					 * @return        собранный вызов действия службы
					 *
					 * \~english
					 * @brief Method of assembling a call of the patching up of an IPv6 firewall pinhole
					 * @details A pinhole is patched up by the identifier issued at its
					 * making rather than by the attributes: one and the same attributes are matched by
					 * several pinholes, and they should be removed by name
					 * @note The call has been verified on a live device — see the note to the structure of a pinhole
					 * @param service designation of the kind of the IPv6 firewall service
					 * @param unique  identifier of the firewall pinhole being patched up
					 * @return        assembled call of the action of the service
					 *
					 * \~
					 */
					request_t unpinhole(const string_view service, const uint16_t unique) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки вызова продления срока пробоя заслона IPv6
					 *
					 * @details Продление снимать и заводить пробой заново не требует:
					 * опознаватель остаётся прежним, и разрешение в заслоне не пропадает
					 * даже на миг. Заведение заново выдало бы новый опознаватель, а старый
					 * пришлось бы забыть
					 *
					 * @warning Довод срока именуется в этом вызове с приставкой - `NewLeaseTime`,
					 * а не `LeaseTime`, как у проделывания пробоя. Приставку объявляет описание
					 * службы, и без неё устройство срока попросту не увидит
					 *
					 * @note Вызов проверен на живом устройстве - см. замечание к структуре пробоя
					 *
					 * @param service  обозначение вида службы заслона IPv6
					 * @param unique   опознаватель продлеваемого пробоя заслона
					 * @param lifeTime запрашиваемый срок жизни пробоя заслона
					 * @return         собранный вызов действия службы
					 *
					 * \~english
					 * @brief Method of assembling a call of the prolongation of the term of an IPv6 firewall pinhole
					 * @details The prolongation does not require removing and making the pinhole anew:
					 * the identifier remains the previous one, and the permission in the firewall does not disappear
					 * even for an instant. A creation anew would issue a new identifier, while the old one
					 * would have to be forgotten
					 * @warning The argument of the term is named in this call with a prefix — `NewLeaseTime`
					 * rather than `LeaseTime`, as at the making of a pinhole. The prefix is declared by the description
					 * of the service, and without it the device will simply not see the term
					 * @note The call has been verified on a live device — see the note to the structure of a pinhole
					 * @param service  designation of the kind of the IPv6 firewall service
					 * @param unique   identifier of the firewall pinhole being prolonged
					 * @param lifeTime requested lifetime of the firewall pinhole
					 * @return         assembled call of the action of the service
					 *
					 * \~
					 */
					request_t repinhole(const string_view service, const uint16_t unique, const uint32_t lifeTime) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки вызова чтения состояния заслона IPv6
					 *
					 * @details Служба заслона выдаётся устройством и тогда, когда заслон
					 * отключён либо пробои им запрещены: спрашивать состояние следует прежде,
					 * чем просить пробой
					 *
					 * @note Вызов проверен на живом устройстве - см. замечание к структуре пробоя
					 *
					 * @param service обозначение вида службы заслона IPv6
					 * @return        собранный вызов действия службы
					 *
					 * \~english
					 * @brief Method of assembling a call of the reading of the state of the IPv6 firewall
					 * @details The firewall service is provided by a device even when the firewall
					 * is disabled or the pinholes are prohibited by it: the state should be asked for before
					 * asking for a pinhole
					 * @note The call has been verified on a live device — see the note to the structure of a pinhole
					 * @param service designation of the kind of the IPv6 firewall service
					 * @return        assembled call of the action of the service
					 *
					 * \~
					 */
					request_t firewall(const string_view service) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод извлечения внешнего адреса маршрутизатора из ответа службы
					 *
					 * @param answer  разобранный ответ службы
					 * @param address ссылка на извлечённый внешний адрес маршрутизатора
					 * @return        признак успешного извлечения
					 *
					 * \~english
					 * @brief Method of extracting the external address of the router from an answer of the service
					 * @param answer  parsed answer of the service
					 * @param address reference to the extracted external address of the router
					 * @return        flag of a successful extraction
					 *
					 * \~
					 */
					bool address(const soap_t::answer_t & answer, string & address) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения перенаправления порта из ответа службы
					 *
					 * @warning Ответ на чтение по порядковому номеру внешний порт содержит, а
					 * ответ на чтение по внешнему порту - нет: спрашивавший его и без того
					 * знает. Незаполненные поля вызывающий обязан дополнить сам
					 *
					 * @param answer  разобранный ответ службы
					 * @param mapping ссылка на извлечённое перенаправление порта
					 * @return        признак успешного извлечения
					 *
					 * \~english
					 * @brief Method of extracting a port forwarding from an answer of the service
					 * @warning The answer to a reading by an ordinal number contains the external port, while the
					 * answer to a reading by an external port does not: the one who has asked knows it anyway.
					 * The caller is obliged to fill in the unfilled fields himself
					 * @param answer  parsed answer of the service
					 * @param mapping reference to the extracted port forwarding
					 * @return        flag of a successful extraction
					 *
					 * \~
					 */
					bool mapping(const soap_t::answer_t & answer, mapping_t & mapping) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения опознавателя пробоя из ответа службы
					 *
					 * @note Извлечение проверено на живом устройстве - см. замечание к
					 * структуре пробоя
					 *
					 * @param answer разобранный ответ службы
					 * @param unique ссылка на извлечённый опознаватель пробоя заслона
					 * @return       признак успешного извлечения
					 *
					 * \~english
					 * @brief Method of extracting the identifier of a pinhole from an answer of the service
					 * @note The extraction has been verified on a live device — see the note to
					 * the structure of a pinhole
					 * @param answer parsed answer of the service
					 * @param unique reference to the extracted identifier of the firewall pinhole
					 * @return       flag of a successful extraction
					 *
					 * \~
					 */
					bool unique(const soap_t::answer_t & answer, uint16_t & unique) const noexcept;
					/**
					 * \~russian
					 * @brief Метод извлечения состояния заслона IPv6 из ответа службы
					 *
					 * @note Извлечение проверено на живом устройстве - см. замечание к
					 * структуре пробоя
					 *
					 * @param answer  разобранный ответ службы
					 * @param enabled ссылка на признак того, что заслон включён
					 * @param allowed ссылка на признак того, что пробои заслона разрешены
					 * @return        признак успешного извлечения
					 *
					 * \~english
					 * @brief Method of extracting the state of the IPv6 firewall from an answer of the service
					 * @note The extraction has been verified on a live device — see the note to
					 * the structure of a pinhole
					 * @param answer  parsed answer of the service
					 * @param enabled reference to the flag of the firewall being enabled
					 * @param allowed reference to the flag of the pinholes of the firewall being permitted
					 * @return        flag of a successful extraction
					 *
					 * \~
					 */
					bool firewall(const soap_t::answer_t & answer, bool & enabled, bool & allowed) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения кода ошибки, выданного службой
					 *
					 * @param answer разобранный ответ службы
					 * @return       код ошибки, выданный службой
					 *
					 * \~english
					 * @brief Method of getting the error code issued by the service
					 * @param answer parsed answer of the service
					 * @return       error code issued by the service
					 *
					 * \~
					 */
					result_t result(const soap_t::answer_t & answer) const noexcept;
					/**
					 * \~russian
					 * @brief Метод проверки осмысленности повторной просьбы с иным портом
					 *
					 * @details Часть отказов означает, что просить бесполезно, а часть - что
					 * следует просить иначе. Занятый порт стоит попросить заново другим,
					 * а отказ настройки повторять незачем
					 *
					 * @param result код ошибки, выданный службой
					 * @return       признак осмысленности повторной просьбы с иным портом
					 *
					 * \~english
					 * @brief Method of checking the meaningfulness of a repeated request with another port
					 * @details A part of the refusals means that it is useless to ask, while a part — that
					 * one should ask differently. An occupied port is worth asking for anew with another one,
					 * while there is no point in repeating a refusal of the settings
					 * @param result error code issued by the service
					 * @return       flag of the meaningfulness of a repeated request with another port
					 *
					 * \~
					 */
					bool retriable(const result_t result) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения обозначения договора перенаправления
					 *
					 * @param proto договор перенаправления порта
					 * @return      обозначение договора, отведённое договором UPnP
					 *
					 * \~english
					 * @brief Method of getting the designation of a forwarding protocol
					 * @param proto protocol of a port forwarding
					 * @return      designation of the protocol allotted by the UPnP protocol
					 *
					 * \~
					 */
					static const char * name(const proto_t proto) noexcept;
					/**
					 * \~russian
					 * @brief Метод определения договора перенаправления по обозначению
					 *
					 * @param text обозначение договора перенаправления порта
					 * @return     определённый договор перенаправления порта
					 *
					 * \~english
					 * @brief Method of determining a forwarding protocol by a designation
					 * @param text designation of the protocol of a port forwarding
					 * @return     determined protocol of a port forwarding
					 *
					 * \~
					 */
					proto_t proto(const string_view text) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект работы с логами
					 *
					 *
					 * \~english
					 * @brief Constructor
					 * @param fmk framework object
					 * @param log object for working with logs
					 *
					 * \~
					 */
					UPnP(const fmk_t * fmk, const log_t * log) noexcept : _soap(fmk, log), _fmk(fmk), _log(log) {}
			} upnp_t;

			/**
			 * \~russian
			 * @brief Метод получения описания кода ошибки службы перенаправления UPnP
			 *
			 * @param result код ошибки, выданный службой
			 * @return       описание кода ошибки на английском языке
			 *
			 * \~english
			 * @brief Method of getting the description of an error code of the UPnP forwarding service
			 * @param result error code issued by the service
			 * @return       description of the error code in the English language
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const upnp_t::result_t result) noexcept;
		};
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_PROTO_PORTMAP_UPNP__
