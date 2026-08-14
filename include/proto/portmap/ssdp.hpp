/**
 * @file ssdp.hpp
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
 * @brief Заголовочный файл кодека договора SSDP (UPnP Device Architecture) — сборка запросов
 *        обнаружения устройств, разбор ответов и оповещений, обозначения искомых служб
 *
 * \~english
 * @brief Header file of the codec of the SSDP protocol (UPnP Device Architecture) — the assembly of the requests
 *        of the discovery of the devices, the parsing of the answers and of the announcements, the designations of the sought services
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROTO_PORTMAP_SSDP__
#define __AWH_PROTO_PORTMAP_SSDP__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstddef>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
#include "../http/parser/http1/http.hpp"

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
			 * @brief Класс кодека договора SSDP
			 *
			 * @details Собирает запросы обнаружения устройств и разбирает ответы на них, а
			 * также оповещения, которые устройства рассылают по сети сами. Договор описан
			 * в «UPnP Device Architecture» и построен на видоизменённом HTTP, передаваемом
			 * по UDP на групповой адрес
			 *
			 * @note Обнаружением дело не кончается: ответ содержит лишь адрес описания
			 * устройства, а сами службы перечислены уже в нём. Разбором описания занят
			 * отдельный кодек
			 *
			 * @warning Ответ приходит от всякого устройства в сети, а не только от
			 * маршрутизатора: обозначение искомой службы обязано быть сличено, а не
			 * принято на веру
			 *
			 * \~english
			 * @brief Class of the codec of the SSDP protocol
			 * @details Assembles the requests of the discovery of the devices and parses the answers to them, and
			 * also the announcements which the devices multicast over the network themselves. The protocol is described
			 * in the «UPnP Device Architecture» and is built on a modified HTTP transmitted
			 * over UDP to a group address
			 * @note The matter does not end with the discovery: the answer contains only the address of the description
			 * of the device, while the services themselves are listed already in it. The parsing of the description is done by
			 * a separate codec
			 * @warning The answer comes from every device in the network rather than only from the
			 * router: the designation of the sought service is obliged to be compared rather than
			 * taken on trust
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ SSDP {
				public:
					/**
					 * \~russian
					 * @brief Групповой адрес обнаружения устройств для IPv4
					 *
					 * \~english
					 * @brief Group address of the discovery of the devices for IPv4
					 *
					 * \~
					 */
					static constexpr const char * MULTICAST_ADDRESS = "239.255.255.250";
					/**
					 * \~russian
					 * @brief Групповой адрес обнаружения устройств в пределах связи для IPv6
					 *
					 * \~english
					 * @brief Group address of the discovery of the devices within the limits of the link for IPv6
					 *
					 * \~
					 */
					static constexpr const char * MULTICAST_ADDRESS6 = "FF02::C";
					/**
					 * \~russian
					 * @brief Групповой адрес обнаружения устройств в пределах места для IPv6
					 *
					 * \~english
					 * @brief Group address of the discovery of the devices within the limits of the site for IPv6
					 *
					 * \~
					 */
					static constexpr const char * MULTICAST_ADDRESS6_SITE = "FF05::C";
					/**
					 * \~russian
					 * @brief Порт обнаружения устройств
					 *
					 * \~english
					 * @brief Port of the discovery of the devices
					 *
					 * \~
					 */
					static constexpr uint16_t PORT = 0x76C;
					/**
					 * \~russian
					 * @brief Наибольший размер сообщения договора
					 *
					 * \~english
					 * @brief Largest size of a message of the protocol
					 *
					 * \~
					 */
					static constexpr size_t MAX_MESSAGE_SIZE = 0x800;
					/**
					 * \~russian
					 * @brief Отведённый устройству срок на ответ в секундах
					 *
					 * @note Устройство отвечает не сразу, а спустя случайное время в
					 * пределах этого срока: так ответы многих устройств разносятся во
					 * времени и не сталкиваются в сети
					 *
					 * \~english
					 * @brief Term allotted to a device for an answer in seconds
					 * @note A device answers not at once but after a random time within
					 * the limits of this term: that way the answers of many devices are spread out in
					 * time and do not collide in the network
					 *
					 * \~
					 */
					static constexpr uint8_t DEFAULT_DELAY = 0x02;
					/**
					 * \~russian
					 * @brief Обозначение искомого устройства доступа в сеть
					 *
					 * \~english
					 * @brief Designation of the sought network access device
					 *
					 * \~
					 */
					static constexpr const char * TARGET_GATEWAY = "urn:schemas-upnp-org:device:InternetGatewayDevice:1";
					/**
					 * \~russian
					 * @brief Обозначение искомой службы соединения по адресу IP
					 *
					 * \~english
					 * @brief Designation of the sought service of the connection by an IP address
					 *
					 * \~
					 */
					static constexpr const char * TARGET_WAN_IP = "urn:schemas-upnp-org:service:WANIPConnection:1";
					/**
					 * \~russian
					 * @brief Обозначение искомой службы соединения по договору PPP
					 *
					 * \~english
					 * @brief Designation of the sought service of the connection by the PPP protocol
					 *
					 * \~
					 */
					static constexpr const char * TARGET_WAN_PPP = "urn:schemas-upnp-org:service:WANPPPConnection:1";
					/**
					 * \~russian
					 * @brief Обозначение поиска всех устройств сети
					 *
					 * \~english
					 * @brief Designation of the search of all the devices of the network
					 *
					 * \~
					 */
					static constexpr const char * TARGET_ALL = "ssdp:all";
				public:
					/**
					 * \~russian
					 * @brief Виды сообщений договора
					 *
					 * \~english
					 * @brief Kinds of the messages of the protocol
					 *
					 * \~
					 */
					enum class kind_t : uint8_t {
						NONE     = 0x00, // Вид сообщения не определён
						SEARCH   = 0x01, // Запрос обнаружения устройств
						RESPONSE = 0x02, // Ответ устройства на запрос обнаружения
						NOTIFY   = 0x03  // Оповещение, разосланное устройством само
					};
					/**
					 * \~russian
					 * @brief Виды оповещений устройства
					 *
					 * \~english
					 * @brief Kinds of the announcements of a device
					 *
					 * \~
					 */
					enum class notice_t : uint8_t {
						NONE   = 0x00, // Вид оповещения не определён
						ALIVE  = 0x01, // Устройство объявилось в сети
						BYEBYE = 0x02, // Устройство покидает сеть
						UPDATE = 0x03  // Устройство сменило свои сведения
					};
					/**
					 * \~russian
					 * @brief Коды причины отказа кодека
					 *
					 * \~english
					 * @brief Codes of the reason of a refusal of the codec
					 *
					 * \~
					 */
					enum class error_t : uint8_t {
						NONE             = 0x00, // Ошибки нет
						EMPTY            = 0x01, // Разбираемое сообщение пусто
						TOO_LARGE        = 0x02, // Сообщение длиннее допустимого договором
						MALFORMED        = 0x03, // Сообщение построено ошибочно
						UNKNOWN_METHOD   = 0x04, // Действие в сообщении договору не принадлежит
						BAD_STATUS       = 0x05, // Устройство ответило отказом
						MISSING_TARGET   = 0x06, // В сообщении нет обозначения службы
						MISSING_LOCATION = 0x07  // В сообщении нет адреса описания устройства
					};
				public:
					/**
					 * \~russian
					 * @brief Структура разобранного сообщения договора
					 *
					 * \~english
					 * @brief Structure of a parsed message of the protocol
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Answer {
						// Вид полученного сообщения
						kind_t kind;
						// Вид оповещения устройства
						notice_t notice;
						/**
						 * \~russian
						 * Срок годности полученных сведений в секундах
						 *
						 * @note По истечении срока устройство следует считать пропавшим,
						 * если оно не объявилось снова: оповещение об уходе доходит не всегда
						 *
						 * \~english
						 * Validity term of the obtained information in seconds
						 * @note Upon the expiration of the term the device should be considered gone,
						 * if it has not announced itself again: an announcement about a departure does not always arrive
						 *
						 * \~
						 */
						uint32_t maxAge;
						// Обозначение службы, о которой сообщает устройство
						string target;
						// Обозначение самого устройства и его службы
						string usn;
						// Адрес описания устройства
						string location;
						// Сведения об устройстве и его встроенной программе
						string server;
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
						Answer() noexcept;
					} answer_t;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * \~russian
					 * @brief Метод сборки запроса обнаружения устройств
					 *
					 * @details Запрос рассылается на групповой адрес, и отвечает на него
					 * всякое устройство, чья служба обозначению отвечает
					 *
					 * @warning Групповой адрес передаётся тот же, на который запрос и
					 * рассылается: договор велит устройству сличать его с полем `HOST`,
					 * отвергая запрос с чужим адресом. Групп в сети IPv6 две, и признаком
					 * разновидности они не различаются
					 *
					 * @note Зона адреса в поле не записывается: она принадлежит машине
					 * отправителя, а не сети, и для устройства смысла не имеет
					 *
					 * @param target обозначение искомой службы
					 * @param delay  отведённый устройству срок на ответ в секундах
					 * @param group  групповой адрес, на который рассылается запрос
					 * @return       собранный текст запроса
					 *
					 * \~english
					 * @brief Method of assembling a request of the discovery of the devices
					 * @details The request is multicast to a group address, and every device whose service
					 * corresponds to the designation answers it
					 * @warning The group address is passed as the same one to which the request is
					 * multicast: the protocol orders a device to compare it against the `HOST` field,
					 * rejecting a request with a foreign address. There are two groups in an IPv6 network, and they are not distinguished
					 * by a flag of the variety
					 * @note The zone of the address is not written into the field: it belongs to the machine of the
					 * sender rather than to the network, and it has no meaning for a device
					 * @param target designation of the sought service
					 * @param delay  term allotted to a device for an answer in seconds
					 * @param group  group address to which the request is multicast
					 * @return       assembled text of the request
					 *
					 * \~
					 */
					string search(const string_view target, const uint8_t delay = DEFAULT_DELAY, const string_view group = MULTICAST_ADDRESS) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора сообщения договора
					 *
					 * @details Разбирает и ответ на запрос обнаружения, и оповещение,
					 * разосланное устройством само: вид сообщения записывается в разбор
					 *
					 * @warning Успешный разбор о пригодности устройства не говорит:
					 * обозначение службы обязано быть сличено с искомым
					 *
					 * @param text   разбираемое сообщение
					 * @param answer ссылка на разобранное сообщение
					 * @param error  ссылка на код причины отказа
					 * @return       признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing a message of the protocol
					 * @details Parses both an answer to a request of the discovery and an announcement
					 * multicast by a device itself: the kind of the message is written into the parsing
					 * @warning A successful parsing says nothing about the suitability of a device:
					 * the designation of the service is obliged to be compared against the sought one
					 * @param text   message being parsed
					 * @param answer reference to the parsed message
					 * @param error  reference to the code of the reason of the refusal
					 * @return       flag of a successful parsing
					 *
					 * \~
					 */
					bool parse(const string_view text, answer_t & answer, error_t & error) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод проверки пригодности обнаруженного устройства
					 *
					 * @details Пригодным считается устройство, чьё обозначение службы
					 * совпадает с искомым либо чей поиск вёлся по всем устройствам сети
					 *
					 * @param answer разобранное сообщение договора
					 * @param target обозначение искомой службы
					 * @return       признак пригодности обнаруженного устройства
					 *
					 * \~english
					 * @brief Method of checking the suitability of a discovered device
					 * @details A device is considered suitable whose designation of the service
					 * coincides with the sought one or whose search has been conducted over all the devices of the network
					 * @param answer parsed message of the protocol
					 * @param target designation of the sought service
					 * @return       flag of the suitability of the discovered device
					 *
					 * \~
					 */
					bool suitable(const answer_t & answer, const string_view target) const noexcept;
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
					SSDP(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
			} ssdp_t;

			/**
			 * \~russian
			 * @brief Метод получения описания кода причины отказа кодека SSDP
			 *
			 * @param error код причины отказа кодека
			 * @return      описание кода причины отказа на английском языке
			 *
			 * \~english
			 * @brief Method of getting the description of a code of the reason of a refusal of the SSDP codec
			 * @param error code of the reason of the refusal of the codec
			 * @return      description of the code of the reason of the refusal in the English language
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const ssdp_t::error_t error) noexcept;
		};
	};
};

#endif // __AWH_PROTO_PORTMAP_SSDP__
