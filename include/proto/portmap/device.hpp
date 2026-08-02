/**
 * @file: device.hpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл кодека описания устройства UPnP — разбор описания, полученного
 *        по адресу из обнаружения, перечень служб устройства и сборка адресов управления
 *
 * @copyright: Copyright © 2026
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
			 */
			typedef class __AWH_SHARED_EXPORT__ Device {
				public:
					/**
					 * @brief Обозначение пространства имён описания устройства
					 *
					 */
					static constexpr const char * NAMESPACE = "urn:schemas-upnp-org:device-1-0";
					/**
					 * @brief Наибольший размер разбираемого описания устройства
					 *
					 * @note Предел обязателен: описание берётся у устройства, о котором
					 * заранее ничего не известно, и доверять его размеру нельзя
					 *
					 */
					static constexpr size_t MAX_DESCRIPTION_SIZE = 0x100000;
					/**
					 * @brief Обозначение искомой службы соединения по адресу IP
					 *
					 */
					static constexpr const char * SERVICE_WAN_IP = "urn:schemas-upnp-org:service:WANIPConnection:1";
					/**
					 * @brief Обозначение искомой службы соединения по адресу IP второго издания
					 *
					 */
					static constexpr const char * SERVICE_WAN_IP2 = "urn:schemas-upnp-org:service:WANIPConnection:2";
					/**
					 * @brief Обозначение искомой службы соединения по договору PPP
					 *
					 */
					static constexpr const char * SERVICE_WAN_PPP = "urn:schemas-upnp-org:service:WANPPPConnection:1";
				public:
					/**
					 * @brief Коды причины отказа кодека
					 *
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
					 * @brief Структура службы устройства
					 *
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
						 * @brief Конструктор
						 *
						 */
						Service() noexcept {}
					} service_t;
					/**
					 * @brief Структура описания устройства
					 *
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
						 * Обозначение самого устройства
						 *
						 * @note Обозначение это у устройства единственное и неизменное:
						 * им устройство и опознаётся между обнаружениями
						 */
						string udn;
						/**
						 * Основание для сборки относительных адресов
						 *
						 * @warning Объявляется устройством не всегда, и полагаться на него
						 * нельзя: при отсутствии основанием служит сам адрес описания
						 */
						string base;
						// Перечень всех служб устройства, включая вложенные устройства
						vector <service_t> services;
						/**
						 * @brief Конструктор
						 *
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
					 */
					bool parse(const string_view text, description_t & description, error_t & error) const noexcept;
				public:
					/**
					 * @brief Метод поиска службы устройства по обозначению вида
					 *
					 * @param description разобранное описание устройства
					 * @param type        обозначение искомого вида службы
					 * @return            найденная служба устройства либо пустой указатель
					 *
					 */
					const service_t * service(const description_t & description, const string_view type) const noexcept;
				public:
					/**
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
					 */
					string address(const description_t & description, const string_view location, const string_view address) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект работы с логами
					 *
					 */
					Device(const fmk_t * fmk, const log_t * log) noexcept : _uri(fmk, log), _fmk(fmk), _log(log) {}
			} device_t;

			/**
			 * @brief Метод получения описания кода причины отказа кодека описания устройства
			 *
			 * @param error код причины отказа кодека
			 * @return      описание кода причины отказа на английском языке
			 *
			 */
			__AWH_SHARED_EXPORT__ const char * message(const device_t::error_t error) noexcept;
		};
	};
};

#endif // __AWH_PROTO_PORTMAP_DEVICE__
