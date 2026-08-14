/**
 * @file device.hpp
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
 * @brief Заголовочный файл кодека описания устройства UPnP — разбор описания, полученного
 *        по адресу из обнаружения, перечень служб устройства и сборка адресов управления
 *
 * \~english
 * @brief Header file of the codec of the description of a UPnP device — the parsing of the description obtained
 *        at the address from the discovery, the list of the services of the device and the assembly of the control addresses
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROTO_PORTMAP_DEVICE__
#define __AWH_PROTO_PORTMAP_DEVICE__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../net/uri.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
#include "../../codec/xml/document.hpp"

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
			 * @brief Класс кодека описания устройства UPnP
			 *
			 * @details Разбирает описание устройства, полученное по адресу из обнаружения,
			 * и выдаёт перечень его служб вместе с адресами управления ими. Обмена кодек
			 * не ведёт: получить описание обязан вызывающий
			 *
			 * @note Устройство описывает себя деревом: внутри корневого устройства заведены
			 * вложенные, и нужная служба перенаправления лежит не в корне, а двумя уровнями
			 * ниже. Кодек обходит дерево целиком и сводит все службы в один перечень
			 *
			 * \~english
			 * @brief Class of the codec of the description of a UPnP device
			 * @details Parses the description of a device obtained at the address from the discovery,
			 * and issues the list of its services together with the addresses of the control of them. The codec does not
			 * conduct the exchange: the caller is obliged to obtain the description
			 * @note A device describes itself as a tree: inside the root device the nested ones are created,
			 * and the needed forwarding service lies not in the root but two levels
			 * lower. The codec traverses the whole tree and brings all the services into a single list
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ Device {
				public:
					/**
					 * \~russian
					 * @brief Обозначение пространства имён описания устройства
					 *
					 * \~english
					 * @brief Designation of the namespace of the description of a device
					 *
					 * \~
					 */
					static constexpr const char * NAMESPACE = "urn:schemas-upnp-org:device-1-0";
					/**
					 * \~russian
					 * @brief Наибольший размер разбираемого описания устройства
					 *
					 * @note Предел обязателен: описание берётся у устройства, о котором
					 * заранее ничего не известно, и доверять его размеру нельзя
					 *
					 * \~english
					 * @brief Largest size of the description of a device being parsed
					 * @note The limit is obligatory: the description is taken from a device about which
					 * nothing is known beforehand, and its size cannot be trusted
					 *
					 * \~
					 */
					static constexpr size_t MAX_DESCRIPTION_SIZE = 0x100000;
					/**
					 * \~russian
					 * @brief Обозначение искомой службы соединения по адресу IP
					 *
					 * \~english
					 * @brief Designation of the sought service of the connection by an IP address
					 *
					 * \~
					 */
					static constexpr const char * SERVICE_WAN_IP = "urn:schemas-upnp-org:service:WANIPConnection:1";
					/**
					 * \~russian
					 * @brief Обозначение искомой службы соединения по адресу IP второго издания
					 *
					 * \~english
					 * @brief Designation of the sought service of the connection by an IP address of the second edition
					 *
					 * \~
					 */
					static constexpr const char * SERVICE_WAN_IP2 = "urn:schemas-upnp-org:service:WANIPConnection:2";
					/**
					 * \~russian
					 * @brief Обозначение искомой службы соединения по договору PPP
					 *
					 * \~english
					 * @brief Designation of the sought service of the connection by the PPP protocol
					 *
					 * \~
					 */
					static constexpr const char * SERVICE_WAN_PPP = "urn:schemas-upnp-org:service:WANPPPConnection:1";
					/**
					 * \~russian
					 * @brief Обозначение вида службы заслона IPv6
					 *
					 * @details Служба эта заведована пробоями заслона IPv6, а не
					 * перенаправлениями: преобразования адресов в сети IPv6 нет, и
					 * подключения сквозь заслон разрешает она
					 *
					 * @note Служба необязательна, и выдаёт её далеко не всякое устройство:
					 * договором UPnP IGD она заведена лишь во второй его редакции
					 *
					 * \~english
					 * @brief Designation of the kind of the IPv6 firewall service
					 * @details This service is in charge of the IPv6 firewall pinholes rather than of the
					 * forwardings: there is no address translation in an IPv6 network, and
					 * it is what permits the connections through the firewall
					 * @note The service is optional, and far from every device provides it:
					 * by the UPnP IGD protocol it has been introduced only in its second edition
					 *
					 * \~
					 */
					static constexpr const char * SERVICE_WAN_IPV6 = "urn:schemas-upnp-org:service:WANIPv6FirewallControl:1";
				public:
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
						NONE          = 0x00, // Ошибки нет
						EMPTY         = 0x01, // Разбираемое описание пусто
						TOO_LARGE     = 0x02, // Описание длиннее допустимого
						MALFORMED     = 0x03, // Описание построено ошибочно
						MISSING_ROOT  = 0x04, // В описании нет корневого узла устройства
						MISSING_SPEC  = 0x05, // В описании нет самого устройства
						MISSING_UDN   = 0x06, // У устройства нет обозначения
						EMPTY_SERVICE = 0x07  // Ни одной пригодной службы устройство не имеет
					};
				public:
					/**
					 * \~russian
					 * @brief Структура службы устройства
					 *
					 * \~english
					 * @brief Structure of a service of a device
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Service {
						// Обозначение вида службы
						string type;
						// Обозначение самой службы
						string id;
						// Адрес управления службой
						string control;
						// Адрес подписки на события службы
						string event;
						// Адрес описания действий службы
						string spec;
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
						Service() noexcept {}
					} service_t;
					/**
					 * \~russian
					 * @brief Структура описания устройства
					 *
					 * \~english
					 * @brief Structure of the description of a device
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Description {
						// Обозначение вида устройства
						string type;
						// Понятное человеку название устройства
						string name;
						// Изготовитель устройства
						string manufacturer;
						// Обозначение изделия
						string model;
						/**
						 * \~russian
						 * Обозначение самого устройства
						 *
						 * @note Обозначение это у устройства единственное и неизменное:
						 * им устройство и опознаётся между обнаружениями
						 *
						 * \~english
						 * Designation of the device itself
						 * @note This designation of a device is a single and unchanging one:
						 * it is by it that the device is recognized between the discoveries
						 *
						 * \~
						 */
						string udn;
						/**
						 * \~russian
						 * Основание для сборки относительных адресов
						 *
						 * @warning Объявляется устройством не всегда, и полагаться на него
						 * нельзя: при отсутствии основанием служит сам адрес описания
						 *
						 * \~english
						 * Base for the assembly of the relative addresses
						 * @warning It is not always announced by a device, and it cannot be relied
						 * upon: in its absence the address of the description itself serves as the base
						 *
						 * \~
						 */
						string base;
						// Перечень всех служб устройства, включая вложенные устройства
						vector <service_t> services;
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
						Description() noexcept {}
					} description_t;
				private:
					// Объект работы с адресами ресурсов
					mutable uri_t _uri;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора описания устройства
					 *
					 * @details Обходит дерево описания целиком и сводит службы всех вложенных
					 * устройств в один перечень: нужная служба перенаправления лежит не в
					 * корне, а двумя уровнями ниже
					 *
					 * @param text        разбираемое описание устройства
					 * @param description ссылка на разобранное описание устройства
					 * @param error       ссылка на код причины отказа
					 * @return            признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing the description of a device
					 * @details Traverses the whole tree of the description and brings the services of all the nested
					 * devices into a single list: the needed forwarding service lies not in the
					 * root but two levels lower
					 * @param text        description of the device being parsed
					 * @param description reference to the parsed description of the device
					 * @param error       reference to the code of the reason of the refusal
					 * @return            flag of a successful parsing
					 *
					 * \~
					 */
					bool parse(const string_view text, description_t & description, error_t & error) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод поиска службы устройства по обозначению вида
					 *
					 * @param description разобранное описание устройства
					 * @param type        обозначение искомого вида службы
					 * @return            найденная служба устройства либо пустой указатель
					 *
					 * \~english
					 * @brief Method of searching for a service of a device by the designation of the kind
					 * @param description parsed description of the device
					 * @param type        designation of the sought kind of the service
					 * @return            found service of the device or an empty pointer
					 *
					 * \~
					 */
					const service_t * service(const description_t & description, const string_view type) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод сборки полного адреса управления службой
					 *
					 * @details Адрес управления устройства записывают путём, а не полным
					 * адресом. Собирается он относительно объявленного основания, а при его
					 * отсутствии - относительно самого адреса описания
					 *
					 * @param description разобранное описание устройства
					 * @param location    адрес, по которому получено описание устройства
					 * @param address     собираемый адрес управления службой
					 * @return            собранный полный адрес управления службой
					 *
					 * \~english
					 * @brief Method of assembling the full address of the control of a service
					 * @details The control address of a device is written as a path rather than as a full
					 * address. It is assembled relative to the announced base, and in its
					 * absence — relative to the address of the description itself
					 * @param description parsed description of the device
					 * @param location    address at which the description of the device has been obtained
					 * @param address     address of the control of the service being assembled
					 * @return            assembled full address of the control of the service
					 *
					 * \~
					 */
					string address(const description_t & description, const string_view location, const string_view address) const noexcept;
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
					Device(const fmk_t * fmk, const log_t * log) noexcept : _uri(fmk, log), _fmk(fmk), _log(log) {}
			} device_t;

			/**
			 * \~russian
			 * @brief Метод получения описания кода причины отказа кодека описания устройства
			 *
			 * @param error код причины отказа кодека
			 * @return      описание кода причины отказа на английском языке
			 *
			 * \~english
			 * @brief Method of getting the description of a code of the reason of a refusal of the codec of the description of a device
			 * @param error code of the reason of the refusal of the codec
			 * @return      description of the code of the reason of the refusal in the English language
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const device_t::error_t error) noexcept;
		};
	};
};

#endif // __AWH_PROTO_PORTMAP_DEVICE__
