/**
 * @file soap.hpp
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
 * @brief Заголовочный файл кодека договора SOAP для управления службами UPnP — сборка вызова
 *        действия службы, разбор ответа и разбор отказа с кодом ошибки UPnP
 *
 * \~english
 * @brief Header file of the codec of the SOAP protocol for the control of the UPnP services — the assembly of a call
 *        of an action of a service, the parsing of the response and the parsing of a refusal with a UPnP error code
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

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
#include "../../sys/macro/global.hpp"
#include "../../codec/xml/writer.hpp"
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
		 * \~english
		 * @brief Port forwarding protocols namespace
		 *
		 * \~
		 */
		namespace portmap {
			/**
			 * \~russian
			 * @brief Пространство имён кодека договора SOAP для управления службами UPnP
			 *
			 * @details Собирает вызов действия службы и разбирает ответ на него. Кодек
			 *          обмена не ведёт: отправить собранный вызов и получить ответ обязан вызывающий.
			 *
			 * @note Кодек охватывает лишь ту часть договора SOAP, которой пользуется UPnP:
			 *       вызов действия с простыми доводами и ответ с простыми значениями. Разбора
			 *       заголовка конверта, сложных видов и ссылок на общие узлы здесь нет - UPnP их
			 *       не применяет
			 *
			 * @warning Отказ службы сообщением построен верно и разбирается успешно: код
			 *          ошибки лежит в разобранном ответе. Отличать отказ службы от испорченного
			 *          ответа необходимо
			 *
			 * \~english
			 * @brief Namespace of the codec of the SOAP protocol for the control of the UPnP services
			 *
			 * @details Assembles a call of an action of a service and parses the response to it. The codec
			 *          does not conduct the exchange: the caller is obliged to send the assembled call and to obtain the response.
			 *
			 * @note The codec covers only that part of the SOAP protocol which UPnP makes use of:
			 *       a call of an action with simple arguments and an response with simple values. There is no parsing
			 *       of the header of the envelope, of the complex kinds and of the references to the common nodes here — UPnP does not
			 *       apply them
			 *
			 * @warning A refusal of a service is a correctly constructed message and is parsed successfully: the error
			 *          code lies in the parsed response. It is necessary to distinguish a refusal of a service from a spoiled response
			 *
			 * \~
			 */
			namespace soap {
				/**
				 * \~russian
				 * @brief Наибольший размер разбираемого ответа службы
				 *
				 * \~english
				 * @brief Largest size of the response of a service being parsed
				 *
				 * \~
				 */
				constexpr size_t MAX_ANSWER_SIZE = 0x100000;

				/**
				 * \~russian
				 * @brief Обозначение пространства имён управления службами UPnP
				 *
				 * @note В этом пространстве имён записан отказ службы с кодом ошибки:
				 * сам договор SOAP видов отказа не задаёт
				 *
				 * \~english
				 * @brief Designation of the namespace of the control of the UPnP services
				 *
				 * @note In this namespace a refusal of a service with an error code is written:
				 * the SOAP protocol itself does not give the kinds of the refusals
				 *
				 * \~
				 */
				constexpr const char * CONTROL_NAMESPACE = "urn:schemas-upnp-org:control-1-0";
				/**
				 * \~russian
				 * @brief Обозначение правил записи содержимого конверта
				 *
				 * \~english
				 * @brief Designation of the rules of the notation of the content of the envelope
				 *
				 * \~
				 */
				constexpr const char * ENCODING = "http://schemas.xmlsoap.org/soap/encoding/";
				/**
				 * \~russian
				 * @brief Обозначение пространства имён конверта договора SOAP
				 *
				 * \~english
				 * @brief Designation of the namespace of the envelope of the SOAP protocol
				 *
				 * \~
				 */
				constexpr const char * NAMESPACE = "http://schemas.xmlsoap.org/soap/envelope/";

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
					EMPTY            = 0x01, // Разбираемый ответ пуст
					TOO_LARGE        = 0x02, // Ответ длиннее допустимого
					MALFORMED        = 0x03, // Ответ построен ошибочно
					MISSING_ENVELOPE = 0x04, // В ответе нет конверта договора
					MISSING_BODY     = 0x05, // В конверте ответа нет тела
					MISSING_ANSWER   = 0x06, // В теле ответа нет ни ответа службы, ни отказа
					INVALID_ACTION   = 0x07  // Название действия построено ошибочно
				};

				/**
				 * \~russian
				 * @brief Структура довода вызова либо значения ответа
				 *
				 * \~english
				 * @brief Structure of an argument of a call or of a value of an response
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Argument {
					// Название довода
					string name;
					// Значение довода
					string value;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Argument() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param name  название довода
					 * @param value значение довода
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * @param name  name of the argument
					 * @param value value of the argument
					 *
					 * \~
					 */
					Argument(const string_view name, const string_view value) noexcept;
				} argument_t;
				/**
				 * \~russian
				 * @brief Структура разобранного ответа службы
				 *
				 * \~english
				 * @brief Structure of a parsed response of a service
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Response {
					/**
					 * \~russian
					 * Признак того, что служба ответила отказом
					 *
					 * @note Отказ разбирается успешно: сообщение построено верно, а
					 *       причина отказа лежит в коде ошибки
					 *
					 * \~english
					 * Flag of the service having responseed with a refusal
					 *
					 * @note A refusal is parsed successfully: the message is constructed correctly, while
					 *       the reason of the refusal lies in the error code
					 *
					 * \~
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
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Response() noexcept;
				} response_t;

				/**
				 * \~russian
				 * @brief Функция сборки обозначения вызываемого действия службы
				 *
				 * @details Обозначение передаётся отдельным полем заголовка запроса и
				 *          заключается в кавычки, как того требует договор.
				 *
				 * @param service обозначение вида службы, у которой вызывается действие
				 * @param action  название вызываемого действия службы
				 * @return        собранное обозначение вызываемого действия службы
				 *
				 * \~english
				 * @brief Function of assembling the designation of the action of the service being called
				 *
				 * @details The designation is passed as a separate field of the header of the request and
				 *          is enclosed in quotes, as the protocol requires.
				 *
				 * @param service designation of the kind of the service at which the action is called
				 * @param action  name of the action of the service being called
				 * @return        assembled designation of the action of the service being called
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ string action(const string_view service, const string_view action) noexcept;
				/**
				 * \~russian
				 * @brief Функция получения значения из разобранного ответа службы
				 *
				 * @param response разобранный ответ службы
				 * @param name     название искомого значения
				 * @return         значение из ответа службы либо пустая последовательность
				 *
				 * \~english
				 * @brief Function of getting a value from a parsed response of a service
				 *
				 * @param response parsed response of the service
				 * @param name     name of the sought value
				 * @return         value from the response of the service or an empty sequence
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ string_view value(const response_t & response, const string_view name) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора ответа службы
				 *
				 * @warning Успешный разбор о выполнении действия не говорит: отказ службы
				 *          разбирается успешно, и признак отказа обязан быть проверен
				 *
				 * @param text     разбираемый ответ службы
				 * @param response ссылка на разобранный ответ службы
				 * @param error    ссылка на код причины отказа
				 * @return         признак успешного разбора
				 *
				 * \~english
				 * @brief Function of parsing the response of a service
				 *
				 * @warning A successful parsing says nothing about the performance of the action: a refusal of a service
				 *          is parsed successfully, and the flag of the refusal is obliged to be checked
				 *
				 * @param text     response of the service being parsed
				 * @param response reference to the parsed response of the service
				 * @param error    reference to the code of the reason of the refusal
				 * @return         flag of a successful parsing
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool parse(const string_view text, response_t & response, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки вызова действия службы
				 *
				 * @param service   обозначение вида службы, у которой вызывается действие
				 * @param action    название вызываемого действия службы
				 * @param arguments перечень доводов вызова действия
				 * @return          собранный текст вызова действия службы
				 *
				 * \~english
				 * @brief Function of assembling a call of an action of a service
				 *
				 * @param service   designation of the kind of the service at which the action is called
				 * @param action    name of the action of the service being called
				 * @param arguments list of the arguments of the call of the action
				 * @return          assembled text of the call of the action of the service
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ string request(const string_view service, const string_view action, const vector <argument_t> & arguments = vector <argument_t> ()) noexcept;
			};

			/**
			 * \~russian
			 * @brief Функция получения описания кода причины отказа кодека SOAP
			 *
			 * @param error код причины отказа кодека
			 * @return      описание кода причины отказа на английском языке
			 *
			 * \~english
			 * @brief Function of getting the description of a code of the reason of a refusal of the SOAP codec
			 *
			 * @param error code of the reason of the refusal of the codec
			 * @return      description of the code of the reason of the refusal in the English language
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const soap::error_t error) noexcept;
		};
	};
};
