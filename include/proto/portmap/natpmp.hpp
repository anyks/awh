/**
 * @file: natpmp.hpp
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
 * @brief Заголовочный файл кодека договора NAT-PMP (RFC 6886) — сборка запросов внешнего адреса
 *        и перенаправления порта, разбор ответов маршрутизатора и коды итога
 *
 * \~english
 * @brief Header file of the codec of the NAT-PMP protocol (RFC 6886) — the assembly of the requests of the external address
 *        and of a port forwarding, the parsing of the answers of the router and the result codes
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROTO_PORTMAP_NATPMP__
#define __AWH_PROTO_PORTMAP_NATPMP__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

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
			 * @brief Класс кодека договора NAT-PMP
			 *
			 * @details Собирает и разбирает сообщения договора NAT-PMP (RFC 6886), которым
			 * машина просит маршрутизатор пропускать к ней подключения извне. Кодек обмена
			 * не ведёт: он лишь превращает намерение в последовательность октетов и обратно,
			 * а отправкой и повторами занимается вызывающий
			 *
			 * @note Договор древний и простой: сообщения не длиннее шестнадцати октетов,
			 * состояния между ними не хранится. Возможностей у него меньше, чем у PCP,
			 * но поддерживают его почти все маршрутизаторы
			 *
			 * @warning Договор работает лишь с адресами IPv4: сеть IPv6 преобразования
			 * адресов не требует, и перенаправлять там нечего
			 *
			 * \~english
			 * @brief Class of the codec of the NAT-PMP protocol
			 * @details Assembles and parses the messages of the NAT-PMP protocol (RFC 6886), by which
			 * a machine asks the router to let the connections from the outside through to it. The codec does not conduct
			 * the exchange: it only turns an intention into a sequence of octets and back,
			 * while the sending and the repetitions are the business of the caller
			 * @note The protocol is ancient and simple: the messages are no longer than sixteen octets,
			 * no state is stored between them. It has fewer capabilities than PCP,
			 * but almost all the routers support it
			 * @warning The protocol works only with the IPv4 addresses: an IPv6 network does not require an address
			 * translation, and there is nothing to forward there
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ NAT_PMP {
				public:
					/**
					 * \~russian
					 * @brief Издание договора, поддерживаемое кодеком
					 *
					 * \~english
					 * @brief Edition of the protocol supported by the codec
					 *
					 * \~
					 */
					static constexpr uint8_t VERSION = 0x00;
					/**
					 * \~russian
					 * @brief Порт маршрутизатора, принимающий запросы
					 *
					 * \~english
					 * @brief Port of the router accepting the requests
					 *
					 * \~
					 */
					static constexpr uint16_t PORT = 0x14E7;
					/**
					 * \~russian
					 * @brief Порт машины, принимающий уведомления о смене внешнего адреса
					 *
					 * @note Уведомления рассылаются маршрутизатором на групповой адрес
					 * 224.0.0.1 и приходят без запроса: так машина узнаёт, что внешний
					 * адрес сменился и перенаправления следует завести заново
					 *
					 * \~english
					 * @brief Port of the machine accepting the notifications about a change of the external address
					 * @note The notifications are multicast by the router to the group address
					 * 224.0.0.1 and arrive without a request: that way the machine learns that the external
					 * address has changed and that the forwardings should be created anew
					 *
					 * \~
					 */
					static constexpr uint16_t ANNOUNCE_PORT = 0x14E6;
					/**
					 * \~russian
					 * @brief Групповой адрес, на который рассылаются уведомления
					 *
					 * @note Адрес этот принадлежит всем машинам сети, и уведомление
					 * доходит до каждой без подписки на отдельную группу. Договор описан
					 * только для IPv4, и разновидности IPv6 у этого адреса нет
					 *
					 * \~english
					 * @brief Group address to which the notifications are multicast
					 * @note This address belongs to all the machines of the network, and a notification
					 * reaches each of them without a subscription to a separate group. The protocol is described
					 * only for IPv4, and this address has no IPv6 variety
					 *
					 * \~
					 */
					static constexpr const char * ANNOUNCE_ADDRESS = "224.0.0.1";
					/**
					 * \~russian
					 * @brief Наибольший размер сообщения договора
					 *
					 * \~english
					 * @brief Largest size of a message of the protocol
					 *
					 * \~
					 */
					static constexpr size_t MAX_MESSAGE_SIZE = 0x10;
					/**
					 * \~russian
					 * @brief Наибольшее количество попыток обращения к маршрутизатору
					 *
					 * \~english
					 * @brief Largest number of the attempts of a call to the router
					 *
					 * \~
					 */
					static constexpr uint8_t MAX_ATTEMPTS = 0x09;
					/**
					 * \~russian
					 * @brief Срок ожидания ответа на первую попытку в миллисекундах
					 *
					 * \~english
					 * @brief Term of waiting for an answer to the first attempt in milliseconds
					 *
					 * \~
					 */
					static constexpr uint32_t INITIAL_TIMEOUT = 0xFA;
				public:
					/**
					 * \~russian
					 * @brief Договоры перенаправления порта
					 *
					 * @note Значения совпадают с кодами действий договора намеренно: код
					 * действия запроса перенаправления и есть обозначение договора
					 *
					 * \~english
					 * @brief Protocols of a port forwarding
					 * @note The values coincide with the codes of the actions of the protocol deliberately: the code
					 * of the action of a forwarding request is the designation of the protocol
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
					 * @brief Виды сообщений договора
					 *
					 * \~english
					 * @brief Kinds of the messages of the protocol
					 *
					 * \~
					 */
					enum class kind_t : uint8_t {
						NONE    = 0x00, // Вид сообщения не определён
						ADDRESS = 0x01, // Внешний адрес маршрутизатора
						MAPPING = 0x02  // Перенаправление порта
					};
					/**
					 * \~russian
					 * @brief Коды итога, выдаваемые маршрутизатором
					 *
					 * @note Отказ здесь обычное дело, а не сбой: перенаправление у
					 * маршрутизатора нередко отключено настройкой
					 *
					 * \~english
					 * @brief Result codes issued by the router
					 * @note A refusal here is an ordinary matter rather than a failure: a forwarding at a
					 * router is not infrequently disabled by a setting
					 *
					 * \~
					 */
					enum class result_t : uint16_t {
						SUCCESS              = 0x00, // Просьба выполнена
						UNSUPPORTED_VERSION  = 0x01, // Издание договора не поддерживается
						NOT_AUTHORIZED       = 0x02, // Просьба отвергнута настройкой маршрутизатора
						NETWORK_FAILURE      = 0x03, // Маршрутизатор не имеет связи с внешней сетью
						OUT_OF_RESOURCES     = 0x04, // У маршрутизатора не осталось места под перенаправления
						UNSUPPORTED_OPCODE   = 0x05  // Действие не поддерживается
					};
					/**
					 * \~russian
					 * @brief Коды причины отказа кодека
					 *
					 * @note Отличать отказ кодека от отказа маршрутизатора необходимо:
					 * первый означает испорченное сообщение, второй - осмысленный ответ
					 *
					 * \~english
					 * @brief Codes of the reason of a refusal of the codec
					 * @note It is necessary to distinguish a refusal of the codec from a refusal of the router:
					 * the first means a spoiled message, the second — a meaningful answer
					 *
					 * \~
					 */
					enum class error_t : uint8_t {
						NONE             = 0x00, // Ошибки нет
						TRUNCATED        = 0x01, // Сообщение короче положенного
						BUFFER_TOO_SMALL = 0x02, // Отведённого места не хватает под сообщение
						INVALID_VERSION  = 0x03, // Издание договора в сообщении неизвестно
						INVALID_OPCODE   = 0x04, // Код действия в сообщении неизвестен
						NOT_A_RESPONSE   = 0x05  // Сообщение ответом не является
					};
				public:
					/**
					 * \~russian
					 * @brief Структура ответа маршрутизатора
					 *
					 * @details Оба вида ответа сведены в одну запись: разбор один и тот же,
					 * а поля, к виду ответа не относящиеся, остаются пустыми. Вид ответа
					 * проверяется прежде обращения к полям
					 *
					 * \~english
					 * @brief Structure of an answer of the router
					 * @details Both kinds of the answer are brought into a single record: the parsing is one and the same,
					 * while the fields not relating to the kind of the answer remain empty. The kind of the answer
					 * is checked before an address to the fields
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Answer {
						// Вид полученного сообщения
						kind_t kind;
						// Код итога, выданный маршрутизатором
						result_t result;
						// Договор перенаправления порта
						proto_t proto;
						/**
						 * \~russian
						 * Время работы маршрутизатора в секундах
						 *
						 * @note Убывание этого счётчика означает перезапуск маршрутизатора:
						 * заведённые перенаправления он забыл, и завести их следует заново
						 *
						 * \~english
						 * Working time of the router in seconds
						 * @note A decrease of this counter means a restart of the router:
						 * it has forgotten the created forwardings, and they should be created anew
						 *
						 * \~
						 */
						uint32_t epoch;
						// Внешний адрес маршрутизатора в порядке октетов машины
						uint32_t address;
						// Срок жизни перенаправления в секундах
						uint32_t lifeTime;
						// Внутренний порт перенаправления
						uint16_t internalPort;
						/**
						 * \~russian
						 * Внешний порт, назначенный маршрутизатором
						 *
						 * @warning Запрошенный порт для маршрутизатора лишь пожелание:
						 * занятый порт он заменит другим по своему выбору, и объявлять
						 * другим следует именно назначенный
						 *
						 * \~english
						 * External port assigned by the router
						 * @warning The requested port is for the router only a wish:
						 * an occupied port it will replace by another one of its own choice, and what should be announced to
						 * the others is exactly the assigned one
						 *
						 * \~
						 */
						uint16_t externalPort;
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
					/**
					 * \~russian
					 * @brief Структура просьбы о перенаправлении порта
					 *
					 * @note Убирается перенаправление той же просьбой с нулевым сроком
					 * жизни и нулевым внешним портом: отдельного действия договор не имеет
					 *
					 * \~english
					 * @brief Structure of a request for a port forwarding
					 * @note A forwarding is removed by the same request with a zero lifetime
					 * and a zero external port: the protocol has no separate action
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Request {
						// Договор перенаправления порта
						proto_t proto;
						// Внутренний порт перенаправления
						uint16_t internalPort;
						// Желаемый внешний порт перенаправления
						uint16_t externalPort;
						// Запрашиваемый срок жизни перенаправления в секундах
						uint32_t lifeTime;
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
						Request() noexcept;
					} request_t;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * \~russian
					 * @brief Метод сборки запроса внешнего адреса маршрутизатора
					 *
					 * @param buffer место под собираемое сообщение
					 * @param size   размер отведённого места
					 * @param error  ссылка на код причины отказа
					 * @return       размер собранного сообщения
					 *
					 * \~english
					 * @brief Method of assembling a request of the external address of the router
					 * @param buffer place for the message being assembled
					 * @param size   size of the allotted place
					 * @param error  reference to the code of the reason of the refusal
					 * @return       size of the assembled message
					 *
					 * \~
					 */
					size_t address(void * buffer, const size_t size, error_t & error) const noexcept;
					/**
					 * \~russian
					 * @brief Метод сборки просьбы о перенаправлении порта
					 *
					 * @param buffer  место под собираемое сообщение
					 * @param size    размер отведённого места
					 * @param request параметры просьбы о перенаправлении порта
					 * @param error   ссылка на код причины отказа
					 * @return        размер собранного сообщения
					 *
					 * \~english
					 * @brief Method of assembling a request for a port forwarding
					 * @param buffer  place for the message being assembled
					 * @param size    size of the allotted place
					 * @param request parameters of the request for a port forwarding
					 * @param error   reference to the code of the reason of the refusal
					 * @return        size of the assembled message
					 *
					 * \~
					 */
					size_t mapping(void * buffer, const size_t size, const request_t & request, error_t & error) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод разбора ответа маршрутизатора
					 *
					 * @details Разбирает оба вида ответа: вид определяется кодом действия и
					 * записывается в разобранный ответ
					 *
					 * @warning Успешный разбор означает лишь то, что сообщение построено
					 * верно. Выполнена ли просьба, показывает код итога в разобранном ответе
					 *
					 * @param buffer разбираемое сообщение
					 * @param size   размер разбираемого сообщения
					 * @param answer ссылка на разобранный ответ
					 * @param error  ссылка на код причины отказа
					 * @return       признак успешного разбора
					 *
					 * \~english
					 * @brief Method of parsing an answer of the router
					 * @details Parses both kinds of the answer: the kind is determined by the code of the action and
					 * is written into the parsed answer
					 * @warning A successful parsing means only that the message is constructed
					 * correctly. Whether the request has been performed is shown by the result code in the parsed answer
					 * @param buffer message being parsed
					 * @param size   size of the message being parsed
					 * @param answer reference to the parsed answer
					 * @param error  reference to the code of the reason of the refusal
					 * @return       flag of a successful parsing
					 *
					 * \~
					 */
					bool parse(const void * buffer, const size_t size, answer_t & answer, error_t & error) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения срока ожидания ответа на очередную попытку
					 *
					 * @details Договор велит удваивать срок ожидания с каждой попыткой,
					 * начиная с четверти секунды. Девять попыток укладываются примерно
					 * в минуту, после чего маршрутизатор считается договора не понимающим
					 *
					 * @param attempt порядковый номер попытки, считая с нуля
					 * @return        срок ожидания ответа в миллисекундах
					 *
					 * \~english
					 * @brief Method of getting the term of waiting for an answer to the next attempt
					 * @details The protocol orders to double the term of the waiting with every attempt,
					 * beginning with a quarter of a second. Nine attempts fit approximately
					 * into a minute, after which the router is considered not to understand the protocol
					 * @param attempt ordinal number of the attempt, counting from zero
					 * @return        term of waiting for an answer in milliseconds
					 *
					 * \~
					 */
					static uint32_t timeout(const uint8_t attempt) noexcept;
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
					NAT_PMP(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
			} natpmp_t;

			/**
			 * \~russian
			 * @brief Метод получения описания кода итога договора NAT-PMP
			 *
			 * @param result код итога, выданный маршрутизатором
			 * @return       описание кода итога на английском языке
			 *
			 * \~english
			 * @brief Method of getting the description of a result code of the NAT-PMP protocol
			 * @param result result code issued by the router
			 * @return       description of the result code in the English language
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const natpmp_t::result_t result) noexcept;

			/**
			 * \~russian
			 * @brief Метод получения описания кода причины отказа кодека NAT-PMP
			 *
			 * @param error код причины отказа кодека
			 * @return      описание кода причины отказа на английском языке
			 *
			 * \~english
			 * @brief Method of getting the description of a code of the reason of a refusal of the NAT-PMP codec
			 * @param error code of the reason of the refusal of the codec
			 * @return      description of the code of the reason of the refusal in the English language
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const natpmp_t::error_t error) noexcept;
		};
	};
};

#endif // __AWH_PROTO_PORTMAP_NATPMP__
