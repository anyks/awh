/**
 * @file: soap.hpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл кодека договора SOAP для управления службами UPnP — сборка вызова
 *        действия службы, разбор ответа и разбор отказа с кодом ошибки UPnP
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROTO_PORTMAP_SOAP__
#define __AWH_PROTO_PORTMAP_SOAP__

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
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
#include "../../codec/xml/writer.hpp"
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
			 * @brief Класс кодека договора SOAP для управления службами UPnP
			 *
			 * @details Собирает вызов действия службы и разбирает ответ на него. Кодек
			 * обмена не ведёт: отправить собранный вызов и получить ответ обязан вызывающий
			 *
			 * @note Кодек охватывает лишь ту часть договора SOAP, которой пользуется UPnP:
			 * вызов действия с простыми доводами и ответ с простыми значениями. Разбора
			 * заголовка конверта, сложных видов и ссылок на общие узлы здесь нет - UPnP их
			 * не применяет
			 *
			 * @warning Отказ службы сообщением построен верно и разбирается успешно: код
			 * ошибки лежит в разобранном ответе. Отличать отказ службы от испорченного
			 * ответа необходимо
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ SOAP {
				public:
					/**
					 * @brief Обозначение пространства имён конверта договора SOAP
					 *
					 */
					static constexpr const char * NAMESPACE = "http://schemas.xmlsoap.org/soap/envelope/";
					/**
					 * @brief Обозначение правил записи содержимого конверта
					 *
					 */
					static constexpr const char * ENCODING = "http://schemas.xmlsoap.org/soap/encoding/";
					/**
					 * @brief Обозначение пространства имён управления службами UPnP
					 *
					 * @note В этом пространстве имён записан отказ службы с кодом ошибки:
					 * сам договор SOAP видов отказа не задаёт
					 *
					 */
					static constexpr const char * CONTROL_NAMESPACE = "urn:schemas-upnp-org:control-1-0";
					/**
					 * @brief Наибольший размер разбираемого ответа службы
					 *
					 */
					static constexpr size_t MAX_ANSWER_SIZE = 0x100000;
				public:
					/**
					 * @brief Коды причины отказа кодека
					 *
					 */
					enum class error_t : uint8_t {
						NONE             = 0x00, // Ошибки нет
						EMPTY            = 0x01, // Разбираемый ответ пуст
						TOO_LARGE        = 0x02, // Ответ длиннее допустимого
						MALFORMED        = 0x03, // Ответ построен ошибочно
						MISSING_ENVELOPE = 0x04, // В ответе нет конверта договора
						MISSING_BODY     = 0x05, // В конверте ответа нет тела
						MISSING_ANSWER   = 0x06, // В теле ответа нет ни ответа службы, ни отказа
						INVALID_ACTION   = 0x07  // Название действия построено ошибочно
					};
				public:
					/**
					 * @brief Структура довода вызова либо значения ответа
					 *
					 */
					typedef struct __AWH_SHARED_EXPORT__ Argument {
						// Название довода
						string name;
						// Значение довода
						string value;
						/**
						 * @brief Конструктор
						 *
						 */
						Argument() noexcept {}
						/**
						 * @brief Конструктор
						 *
						 * @param name  название довода
						 * @param value значение довода
						 *
						 */
						Argument(const string_view name, const string_view value) noexcept : name(name), value(value) {}
					} argument_t;
					/**
					 * @brief Структура разобранного ответа службы
					 *
					 */
					typedef struct __AWH_SHARED_EXPORT__ Answer {
						/**
						 * Признак того, что служба ответила отказом
						 *
						 * @note Отказ разбирается успешно: сообщение построено верно, а
						 * причина отказа лежит в коде ошибки
						 */
						bool fault;
						// Код ошибки, выданный службой
						uint32_t code;
						// Название действия, на которое получен ответ
						string action;
						// Описание ошибки, выданное службой
						string description;
						// Перечень значений, выданных службой
						vector <argument_t> arguments;
						/**
						 * @brief Конструктор
						 *
						 */
						Answer() noexcept : fault(false), code(0) {}
					} answer_t;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * @brief Метод сборки вызова действия службы
					 *
					 * @param service   обозначение вида службы, у которой вызывается действие
					 * @param action    название вызываемого действия службы
					 * @param arguments перечень доводов вызова действия
					 * @return          собранный текст вызова действия службы
					 *
					 */
					string request(const string_view service, const string_view action, const vector <argument_t> & arguments = vector <argument_t> ()) const noexcept;
					/**
					 * @brief Метод сборки обозначения вызываемого действия службы
					 *
					 * @details Обозначение передаётся отдельным полем заголовка запроса и
					 * заключается в кавычки, как того требует договор
					 *
					 * @param service обозначение вида службы, у которой вызывается действие
					 * @param action  название вызываемого действия службы
					 * @return        собранное обозначение вызываемого действия службы
					 *
					 */
					string action(const string_view service, const string_view action) const noexcept;
				public:
					/**
					 * @brief Метод разбора ответа службы
					 *
					 * @warning Успешный разбор о выполнении действия не говорит: отказ службы
					 * разбирается успешно, и признак отказа обязан быть проверен
					 *
					 * @param text   разбираемый ответ службы
					 * @param answer ссылка на разобранный ответ службы
					 * @param error  ссылка на код причины отказа
					 * @return       признак успешного разбора
					 *
					 */
					bool parse(const string_view text, answer_t & answer, error_t & error) const noexcept;
				public:
					/**
					 * @brief Метод получения значения из разобранного ответа службы
					 *
					 * @param answer разобранный ответ службы
					 * @param name   название искомого значения
					 * @return       значение из ответа службы либо пустая последовательность
					 *
					 */
					string_view value(const answer_t & answer, const string_view name) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект работы с логами
					 *
					 */
					SOAP(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
			} soap_t;

			/**
			 * @brief Метод получения описания кода причины отказа кодека SOAP
			 *
			 * @param error код причины отказа кодека
			 * @return      описание кода причины отказа на английском языке
			 *
			 */
			__AWH_SHARED_EXPORT__ const char * message(const soap_t::error_t error) noexcept;
		};
	};
};

#endif // __AWH_PROTO_PORTMAP_SOAP__
