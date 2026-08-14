/**
 * @file: pcp.hpp
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
 * @brief Заголовочный файл кодека договора PCP (RFC 6887) — сборка запросов перенаправления
 *        и сношения с узлом, разбор ответов маршрутизатора, дополнения запроса и коды итога
 *
 * \~english
 * @brief Header file of the codec of the PCP protocol (RFC 6887) — the assembly of the requests of a forwarding
 *        and of a peering with a node, the parsing of the answers of the router, the options of a request and the result codes
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROTO_PORTMAP_PCP__
#define __AWH_PROTO_PORTMAP_PCP__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
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
			 * @brief Класс кодека договора PCP
			 *
			 * @details Собирает и разбирает сообщения договора PCP (RFC 6887) - самого нового
			 * из трёх договоров перенаправления. Кодек обмена не ведёт: он лишь превращает
			 * намерение в последовательность октетов и обратно, а отправкой, повторами и
			 * подбором договора занимается вызывающий
			 *
			 * @note Договор наследует NAT-PMP и работает на том же порту, но умеет больше:
			 * адреса IPv6, перенаправление от имени другой машины, сношение с определённым
			 * узлом и открытие прохода без преобразования адресов
			 *
			 * @warning Все адреса договором передаются шестнадцатью октетами. Адрес IPv4
			 * записывается в них видом, отведённым под IPv4 в IPv6 - отдельного поля под
			 * него договор не имеет
			 *
			 * \~english
			 * @brief Class of the codec of the PCP protocol
			 * @details Assembles and parses the messages of the PCP protocol (RFC 6887) — the newest
			 * of the three forwarding protocols. The codec does not conduct the exchange: it only turns
			 * an intention into a sequence of octets and back, while the sending, the repetitions and
			 * the selection of the protocol are the business of the caller
			 * @note The protocol inherits NAT-PMP and works on the same port, but it can do more:
			 * the IPv6 addresses, a forwarding on behalf of another machine, a peering with a particular
			 * node and the opening of a passage without an address translation
			 * @warning All the addresses are transmitted by the protocol as sixteen octets. An IPv4 address
			 * is written in them in the form allotted for IPv4 in IPv6 — the protocol has no separate field
			 * for it
			 *
			 * \~
			 */
			typedef class __AWH_SHARED_EXPORT__ PCP {
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
					static constexpr uint8_t VERSION = 0x02;
					/**
					 * \~russian
					 * @brief Порт маршрутизатора, принимающий запросы
					 *
					 * @note Порт тот же, что и у NAT-PMP: договоры разделяются изданием в
					 * первом октете сообщения, а не портом
					 *
					 * \~english
					 * @brief Port of the router accepting the requests
					 * @note The port is the same as that of NAT-PMP: the protocols are separated by the version in
					 * the first octet of the message rather than by the port
					 *
					 * \~
					 */
					static constexpr uint16_t PORT = 0x14E7;
					/**
					 * \~russian
					 * @brief Порт машины, принимающий уведомления маршрутизатора
					 *
					 * \~english
					 * @brief Port of the machine accepting the notifications of the router
					 *
					 * \~
					 */
					static constexpr uint16_t ANNOUNCE_PORT = 0x14E6;
					/**
					 * \~russian
					 * @brief Групповой адрес, на который рассылаются уведомления в сети IPv4
					 *
					 * @note Адрес тот же, что и у договора NAT-PMP: он принадлежит всем
					 * машинам сети, и уведомление доходит до каждой без подписки на
					 * отдельную группу
					 *
					 * \~english
					 * @brief Group address to which the notifications are multicast in an IPv4 network
					 * @note The address is the same as that of the NAT-PMP protocol: it belongs to all the
					 * machines of the network, and a notification reaches each of them without a subscription to
					 * a separate group
					 *
					 * \~
					 */
					static constexpr const char * ANNOUNCE_ADDRESS = "224.0.0.1";
					/**
					 * \~russian
					 * @brief Групповой адрес, на который рассылаются уведомления в сети IPv6
					 *
					 * @note Адрес этот принадлежит всем узлам в пределах связи и отвечает
					 * адресу IPv4 всех машин сети
					 *
					 * \~english
					 * @brief Group address to which the notifications are multicast in an IPv6 network
					 * @note This address belongs to all the nodes within the limits of the link and corresponds
					 * to the IPv4 address of all the machines of the network
					 *
					 * \~
					 */
					static constexpr const char * ANNOUNCE_ADDRESS6 = "FF02::1";
					/**
					 * \~russian
					 * @brief Наибольший размер сообщения договора
					 *
					 * \~english
					 * @brief Largest size of a message of the protocol
					 *
					 * \~
					 */
					static constexpr size_t MAX_MESSAGE_SIZE = 0x44C;
					/**
					 * \~russian
					 * @brief Размер заголовка сообщения договора
					 *
					 * \~english
					 * @brief Size of the header of a message of the protocol
					 *
					 * \~
					 */
					static constexpr size_t HEADER_SIZE = 0x18;
					/**
					 * \~russian
					 * @brief Размер отличительной метки перенаправления
					 *
					 * \~english
					 * @brief Size of the distinguishing mark of a forwarding
					 *
					 * \~
					 */
					static constexpr size_t NONCE_SIZE = 0x0C;
					/**
					 * \~russian
					 * @brief Размер адреса в сообщении договора
					 *
					 * \~english
					 * @brief Size of an address in a message of the protocol
					 *
					 * \~
					 */
					static constexpr size_t ADDRESS_SIZE = 0x10;
					/**
					 * \~russian
					 * @brief Срок ожидания ответа на первую попытку в миллисекундах
					 *
					 * \~english
					 * @brief Term of waiting for an answer to the first attempt in milliseconds
					 *
					 * \~
					 */
					static constexpr uint32_t INITIAL_TIMEOUT = 0xBB8;
					/**
					 * \~russian
					 * @brief Наибольший срок ожидания ответа в миллисекундах
					 *
					 * \~english
					 * @brief Largest term of waiting for an answer in milliseconds
					 *
					 * \~
					 */
					static constexpr uint32_t MAX_TIMEOUT = 0xFA000;
				public:
					/**
					 * \~russian
					 * @brief Действия договора
					 *
					 * \~english
					 * @brief Actions of the protocol
					 *
					 * \~
					 */
					enum class opcode_t : uint8_t {
						ANNOUNCE = 0x00, // Уведомление о состоянии маршрутизатора
						MAP      = 0x01, // Перенаправление порта
						PEER     = 0x02  // Сношение с определённым узлом
					};
					/**
					 * \~russian
					 * @brief Договоры перенаправления порта
					 *
					 * @note Значения заданы числами договоров по перечню IANA, как того
					 * требует договор PCP. Значение «все договоры» допустимо лишь при
					 * нулевом внутреннем порте
					 *
					 * \~english
					 * @brief Protocols of a port forwarding
					 * @note The values are given by the numbers of the protocols from the IANA list, as the PCP
					 * protocol requires. The value «all the protocols» is admissible only at a
					 * zero internal port
					 *
					 * \~
					 */
					enum class proto_t : uint8_t {
						ALL = 0x00, // Перенаправление всех договоров разом
						TCP = 0x06, // Перенаправление порта TCP
						UDP = 0x11  // Перенаправление порта UDP
					};
					/**
					 * \~russian
					 * @brief Коды дополнений запроса
					 *
					 * \~english
					 * @brief Codes of the options of a request
					 *
					 * \~
					 */
					enum class option_t : uint8_t {
						NONE           = 0x00, // Дополнение не определено
						THIRD_PARTY    = 0x01, // Перенаправление от имени другой машины
						PREFER_FAILURE = 0x02, // Отказ вместо назначения иного внешнего порта
						FILTER         = 0x03  // Пропускать лишь подключения с указанного узла
					};
					/**
					 * \~russian
					 * @brief Коды итога, выдаваемые маршрутизатором
					 *
					 * \~english
					 * @brief Result codes issued by the router
					 *
					 * \~
					 */
					enum class result_t : uint8_t {
						SUCCESS                 = 0x00, // Просьба выполнена
						UNSUPP_VERSION          = 0x01, // Издание договора не поддерживается
						NOT_AUTHORIZED          = 0x02, // Просьба отвергнута настройкой маршрутизатора
						MALFORMED_REQUEST       = 0x03, // Запрос построен ошибочно
						UNSUPP_OPCODE           = 0x04, // Действие не поддерживается
						UNSUPP_OPTION           = 0x05, // Дополнение запроса не поддерживается
						MALFORMED_OPTION        = 0x06, // Дополнение запроса построено ошибочно
						NETWORK_FAILURE         = 0x07, // Маршрутизатор не имеет связи с внешней сетью
						NO_RESOURCES            = 0x08, // У маршрутизатора не осталось места под перенаправления
						UNSUPP_PROTOCOL         = 0x09, // Договор перенаправления не поддерживается
						USER_EX_QUOTA           = 0x0A, // Машина исчерпала отведённую ей долю перенаправлений
						CANNOT_PROVIDE_EXTERNAL = 0x0B, // Запрошенный внешний адрес выдать невозможно
						ADDRESS_MISMATCH        = 0x0C, // Адрес в запросе не совпадает с адресом отправителя
						EXCESSIVE_REMOTE_PEERS  = 0x0D  // Узлов в просьбе указано больше допустимого
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
						TRUNCATED        = 0x01, // Сообщение короче положенного
						BUFFER_TOO_SMALL = 0x02, // Отведённого места не хватает под сообщение
						INVALID_VERSION  = 0x03, // Издание договора в сообщении неизвестно
						INVALID_OPCODE   = 0x04, // Код действия в сообщении неизвестен
						NOT_A_RESPONSE   = 0x05, // Сообщение ответом не является
						MALFORMED_OPTION = 0x06, // Дополнение запроса построено ошибочно
						TOO_LARGE        = 0x07  // Сообщение длиннее допустимого договором
					};
				public:
					/**
					 * \~russian
					 * @brief Структура дополнения запроса
					 *
					 * @note Дополнения передаются кодом и содержимым как есть: разбирать
					 * содержимое обязан тот, кто дополнение и запросил
					 *
					 * \~english
					 * @brief Structure of an option of a request
					 * @note The options are transmitted as a code and a content as they are: the one who has requested the option
					 * is obliged to parse the content
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Option {
						// Код дополнения запроса
						option_t code;
						// Содержимое дополнения запроса
						vector <uint8_t> data;
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
						Option() noexcept;
					} opt_t;
					/**
					 * \~russian
					 * @brief Структура запроса к маршрутизатору
					 *
					 * @note Убирается перенаправление тем же запросом с нулевым сроком жизни:
					 * отдельного действия договор не имеет
					 *
					 * @warning Отличительная метка обязана быть случайной и одной и той же во
					 * всех повторах одной просьбы: ею маршрутизатор отличает повтор от
					 * новой просьбы, а машина - свой ответ от чужого. Кодек метку не
					 * создаёт: случайность дело вызывающего
					 *
					 * \~english
					 * @brief Structure of a request to the router
					 * @note A forwarding is removed by the same request with a zero lifetime:
					 * the protocol has no separate action
					 * @warning The distinguishing mark is obliged to be a random one and one and the same in
					 * all the repetitions of a single request: by it the router distinguishes a repetition from
					 * a new request, while the machine — its own answer from a foreign one. The codec does not create the mark:
					 * the randomness is the business of the caller
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Request {
						// Действие договора
						opcode_t opcode;
						// Договор перенаправления порта
						proto_t proto;
						// Запрашиваемый срок жизни перенаправления в секундах
						uint32_t lifeTime;
						// Внутренний порт перенаправления
						uint16_t internalPort;
						// Желаемый внешний порт перенаправления
						uint16_t externalPort;
						// Порт узла, с которым ведётся сношение
						uint16_t remotePort;
						// Отличительная метка перенаправления
						uint8_t nonce[NONCE_SIZE];
						// Адрес машины, обращающейся к маршрутизатору
						uint8_t client[ADDRESS_SIZE];
						// Желаемый внешний адрес перенаправления
						uint8_t external[ADDRESS_SIZE];
						// Адрес узла, с которым ведётся сношение
						uint8_t remote[ADDRESS_SIZE];
						// Дополнения запроса
						vector <opt_t> options;
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
					/**
					 * \~russian
					 * @brief Структура ответа маршрутизатора
					 *
					 * \~english
					 * @brief Structure of an answer of the router
					 *
					 * \~
					 */
					typedef struct __AWH_SHARED_EXPORT__ Answer {
						// Действие договора
						opcode_t opcode;
						// Код итога, выданный маршрутизатором
						result_t result;
						// Договор перенаправления порта
						proto_t proto;
						// Назначенный срок жизни перенаправления в секундах
						uint32_t lifeTime;
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
						// Внутренний порт перенаправления
						uint16_t internalPort;
						/**
						 * \~russian
						 * Внешний порт, назначенный маршрутизатором
						 *
						 * @warning Запрошенный порт для маршрутизатора лишь пожелание:
						 * занятый порт он заменит другим по своему выбору
						 *
						 * \~english
						 * External port assigned by the router
						 * @warning The requested port is for the router only a wish:
						 * an occupied port it will replace by another one of its own choice
						 *
						 * \~
						 */
						uint16_t externalPort;
						// Порт узла, с которым ведётся сношение
						uint16_t remotePort;
						// Отличительная метка перенаправления
						uint8_t nonce[NONCE_SIZE];
						// Внешний адрес, назначенный маршрутизатором
						uint8_t external[ADDRESS_SIZE];
						// Адрес узла, с которым ведётся сношение
						uint8_t remote[ADDRESS_SIZE];
						// Дополнения ответа
						vector <opt_t> options;
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
					 * @brief Метод сборки запроса к маршрутизатору
					 *
					 * @param buffer  место под собираемое сообщение
					 * @param size    размер отведённого места
					 * @param request параметры запроса к маршрутизатору
					 * @param error   ссылка на код причины отказа
					 * @return        размер собранного сообщения
					 *
					 * \~english
					 * @brief Method of assembling a request to the router
					 * @param buffer  place for the message being assembled
					 * @param size    size of the allotted place
					 * @param request parameters of the request to the router
					 * @param error   reference to the code of the reason of the refusal
					 * @return        size of the assembled message
					 *
					 * \~
					 */
					size_t request(void * buffer, const size_t size, const request_t & request, error_t & error) const noexcept;
					/**
					 * \~russian
					 * @brief Метод разбора ответа маршрутизатора
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
					 * @brief Метод проверки принадлежности ответа запросу
					 *
					 * @details Сличает отличительную метку, действие и внутренний порт. На
					 * открытый порт приходят и чужие ответы, и ответы на прежние просьбы,
					 * и принимать их за свои недопустимо
					 *
					 * @warning Проверку обязан выполнять всякий, кто ведёт обмен: без неё
					 * посторонний, угадавший порт, подменит машине внешний адрес
					 *
					 * @param answer  разобранный ответ маршрутизатора
					 * @param request отправленный запрос к маршрутизатору
					 * @return        признак принадлежности ответа запросу
					 *
					 * \~english
					 * @brief Method of checking the belonging of an answer to a request
					 * @details Compares the distinguishing mark, the action and the internal port. At an
					 * open port both foreign answers and the answers to the previous requests arrive,
					 * and it is inadmissible to take them for one's own
					 * @warning Everyone who conducts the exchange is obliged to perform this check: without it
					 * an outsider who has guessed the port will substitute the external address for the machine
					 * @param answer  parsed answer of the router
					 * @param request sent request to the router
					 * @return        flag of the belonging of the answer to the request
					 *
					 * \~
					 */
					bool belongs(const answer_t & answer, const request_t & request) const noexcept;
				public:
					/**
					 * \~russian
					 * @brief Метод получения срока ожидания ответа на очередную попытку
					 *
					 * @details Договор велит начинать с трёх секунд и удваивать срок с
					 * каждой попыткой, не превышая примерно семнадцати минут
					 *
					 * @param attempt порядковый номер попытки, считая с нуля
					 * @return        срок ожидания ответа в миллисекундах
					 *
					 * \~english
					 * @brief Method of getting the term of waiting for an answer to the next attempt
					 * @details The protocol orders to begin with three seconds and to double the term with
					 * every attempt without exceeding approximately seventeen minutes
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
					PCP(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
			} pcp_t;

			/**
			 * \~russian
			 * @brief Метод получения описания кода итога договора PCP
			 *
			 * @param result код итога, выданный маршрутизатором
			 * @return       описание кода итога на английском языке
			 *
			 * \~english
			 * @brief Method of getting the description of a result code of the PCP protocol
			 * @param result result code issued by the router
			 * @return       description of the result code in the English language
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const pcp_t::result_t result) noexcept;

			/**
			 * \~russian
			 * @brief Метод получения описания кода причины отказа кодека PCP
			 *
			 * @param error код причины отказа кодека
			 * @return      описание кода причины отказа на английском языке
			 *
			 * \~english
			 * @brief Method of getting the description of a code of the reason of a refusal of the PCP codec
			 * @param error code of the reason of the refusal of the codec
			 * @return      description of the code of the reason of the refusal in the English language
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const pcp_t::error_t error) noexcept;
		};
	};
};

#endif // __AWH_PROTO_PORTMAP_PCP__
