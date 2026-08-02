/**
 * @file: ssdp.hpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл кодека договора SSDP (UPnP Device Architecture) — сборка запросов
 *        обнаружения устройств, разбор ответов и оповещений, обозначения искомых служб
 *
 * @copyright: Copyright © 2026
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
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён протоколов
	 *
	 */
	namespace proto {
		/**
		 * @brief Пространство имён договоров перенаправления портов
		 *
		 */
		namespace portmap {
			/**
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
			 */
			typedef class __AWH_SHARED_EXPORT__ SSDP {
				public:
					/**
					 * @brief Групповой адрес обнаружения устройств для IPv4
					 *
					 */
					static constexpr const char * MULTICAST_ADDRESS = "239.255.255.250";
					/**
					 * @brief Групповой адрес обнаружения устройств в пределах связи для IPv6
					 *
					 */
					static constexpr const char * MULTICAST_ADDRESS6 = "FF02::C";
					/**
					 * @brief Групповой адрес обнаружения устройств в пределах места для IPv6
					 *
					 */
					static constexpr const char * MULTICAST_ADDRESS6_SITE = "FF05::C";
					/**
					 * @brief Порт обнаружения устройств
					 *
					 */
					static constexpr uint16_t PORT = 0x76C;
					/**
					 * @brief Наибольший размер сообщения договора
					 *
					 */
					static constexpr size_t MAX_MESSAGE_SIZE = 0x800;
					/**
					 * @brief Отведённый устройству срок на ответ в секундах
					 *
					 * @note Устройство отвечает не сразу, а спустя случайное время в
					 * пределах этого срока: так ответы многих устройств разносятся во
					 * времени и не сталкиваются в сети
					 *
					 */
					static constexpr uint8_t DEFAULT_DELAY = 0x02;
					/**
					 * @brief Обозначение искомого устройства доступа в сеть
					 *
					 */
					static constexpr const char * TARGET_GATEWAY = "urn:schemas-upnp-org:device:InternetGatewayDevice:1";
					/**
					 * @brief Обозначение искомой службы соединения по адресу IP
					 *
					 */
					static constexpr const char * TARGET_WAN_IP = "urn:schemas-upnp-org:service:WANIPConnection:1";
					/**
					 * @brief Обозначение искомой службы соединения по договору PPP
					 *
					 */
					static constexpr const char * TARGET_WAN_PPP = "urn:schemas-upnp-org:service:WANPPPConnection:1";
					/**
					 * @brief Обозначение поиска всех устройств сети
					 *
					 */
					static constexpr const char * TARGET_ALL = "ssdp:all";
				public:
					/**
					 * @brief Виды сообщений договора
					 *
					 */
					enum class kind_t : uint8_t {
						NONE     = 0x00, // Вид сообщения не определён
						SEARCH   = 0x01, // Запрос обнаружения устройств
						RESPONSE = 0x02, // Ответ устройства на запрос обнаружения
						NOTIFY   = 0x03  // Оповещение, разосланное устройством само
					};
					/**
					 * @brief Виды оповещений устройства
					 *
					 */
					enum class notice_t : uint8_t {
						NONE   = 0x00, // Вид оповещения не определён
						ALIVE  = 0x01, // Устройство объявилось в сети
						BYEBYE = 0x02, // Устройство покидает сеть
						UPDATE = 0x03  // Устройство сменило свои сведения
					};
					/**
					 * @brief Коды причины отказа кодека
					 *
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
					 * @brief Структура разобранного сообщения договора
					 *
					 */
					typedef struct __AWH_SHARED_EXPORT__ Answer {
						// Вид полученного сообщения
						kind_t kind;
						// Вид оповещения устройства
						notice_t notice;
						/**
						 * Срок годности полученных сведений в секундах
						 *
						 * @note По истечении срока устройство следует считать пропавшим,
						 * если оно не объявилось снова: оповещение об уходе доходит не всегда
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
						 * @brief Конструктор
						 *
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
					 * @brief Метод сборки запроса обнаружения устройств
					 *
					 * @details Запрос рассылается на групповой адрес, и отвечает на него
					 * всякое устройство, чья служба обозначению отвечает
					 *
					 * @param target обозначение искомой службы
					 * @param delay  отведённый устройству срок на ответ в секундах
					 * @param six    признак сборки запроса для сети IPv6
					 * @return       собранный текст запроса
					 *
					 */
					string search(const string_view target, const uint8_t delay = DEFAULT_DELAY, const bool six = false) const noexcept;
				public:
					/**
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
					 */
					bool parse(const string_view text, answer_t & answer, error_t & error) const noexcept;
				public:
					/**
					 * @brief Метод проверки пригодности обнаруженного устройства
					 *
					 * @details Пригодным считается устройство, чьё обозначение службы
					 * совпадает с искомым либо чей поиск вёлся по всем устройствам сети
					 *
					 * @param answer разобранное сообщение договора
					 * @param target обозначение искомой службы
					 * @return       признак пригодности обнаруженного устройства
					 *
					 */
					bool suitable(const answer_t & answer, const string_view target) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект работы с логами
					 *
					 */
					SSDP(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
			} ssdp_t;

			/**
			 * @brief Метод получения описания кода причины отказа кодека SSDP
			 *
			 * @param error код причины отказа кодека
			 * @return      описание кода причины отказа на английском языке
			 *
			 */
			__AWH_SHARED_EXPORT__ const char * message(const ssdp_t::error_t error) noexcept;
		};
	};
};

#endif // __AWH_PROTO_PORTMAP_SSDP__
