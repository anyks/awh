/**
 * @file xdp.hpp
 * @date 2026-11-06
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
 * @brief Заголовочный файл модуля датапути AF_XDP —
 *        пространство имён eth::xdp для заведения сокетов AF_XDP, приёма и отправки кадров
 *        канального уровня в обход обычного сетевого стека
 *
 * \~english
 * @brief Header file of the module of the AF_XDP datapath —
 *        the eth::xdp namespace for creating the AF_XDP sockets, receiving and sending the frames
 *        of the link level bypassing the usual network stack
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
 * Подключаем заголовочные файлы модуля
 */
#include "../net.hpp"
#include "../../sys/macro/global.hpp"

/**
 * Для операционных систем с поддержкой AF_XDP: только Linux и лишь когда
 * спутник включён сборкой (-DCMAKE_BUILD_XDP=ON)
 */
#if (defined(__linux__) && defined(AWH_XDP)) || defined(DOXYGEN)
	/**
	 * \~russian
	 * @brief Основное пространство имён
	 *
	 * \~english
	 * @brief Main namespace
	 *
	 * \~
	 */
	namespace awh {
		/**
		 * \~russian
		 * @brief Пространство имён сетевых сущностей
		 *
		 * \~english
		 * @brief Namespace of the network entities
		 *
		 * \~
		 */
		namespace net {
			/**
			 * \~russian
			 * @brief Пространство имён датапути AF_XDP
			 *
			 * \~english
			 * @brief Namespace of the AF_XDP datapath
			 *
			 * \~
			 */
			namespace xdp {
				/**
				 * \~russian
				 * @brief Предварительное объявление состояния сокета AF_XDP
				 *
				 * @details Состояние несёт указатели libxdp на сокет и область UMEM, четыре
				 *          кольца и пул свободных кадров. Полное определение живёт в единице
				 *          трансляции спутника, а здесь объявлено непрозрачно намеренно:
				 *          заголовкам проекта положено быть чистыми, и ссылка на неполный
				 *          тип не требует подключения заголовков libxdp, которого у прочих
				 *          систем и при выключенном спутнике нет
				 *
				 * \~english
				 * @brief Preliminary declaration of the state of an AF_XDP socket
				 *
				 * @details The state carries the libxdp pointers to the socket and the UMEM region, the four
				 *          rings and the pool of the free frames. The full definition lives in the translation
				 *          unit of the satellite, and here it is declared opaque deliberately: the headers of
				 *          the project are due to be clean, and a reference to an incomplete type does not require
				 *          including the libxdp headers, which the other systems and the disabled satellite do not have
				 *
				 * \~
				 */
				struct Socket;
				/**
				 * \~russian
				 * @brief Опознаватель состояния сокета AF_XDP
				 *
				 * \~english
				 * @brief AF_XDP socket state identifier
				 *
				 * \~
				 */
				typedef struct Socket socket_t;
			}
		}
		/**
		 * \~russian
		 * @brief Пространство имён Ethernet протоколов
		 *
		 * \~english
		 * @brief Namespace of the Ethernet protocols
		 *
		 * \~
		 */
		namespace eth {
			/**
			 * Используем стандартное пространство имён
			 */
			using namespace std;

			/**
			 * \~russian
			 * @brief Пространство имён датапути AF_XDP
			 *
			 * @details Низкий уровень датапути кадров канального уровня в обход сетевого
			 *          стека. Ведает заведением сокета AF_XDP с областью UMEM и кольцами,
			 *          приёмом кадров из кольца приёма и отправкой через кольцо отправки.
			 *          Вся нативная поверхность датапути сосредоточена здесь, в eth, а
			 *          публичный фасад движка работает лишь с идентификаторами узлов
			 *
			 * @note Собирается лишь на Linux и только при включённом спутнике
			 *       (-DCMAKE_BUILD_XDP=ON): прочие системы датапути AF_XDP не несут
			 *
			 * \~english
			 * @brief Namespace of the AF_XDP datapath
			 *
			 * @details The low level of the datapath of the frames of the link level bypassing the network
			 *          stack. Is in charge of creating the AF_XDP socket with the UMEM region and the rings,
			 *          of receiving the frames from the reception ring and of sending through the sending ring.
			 *          The whole native surface of the datapath is concentrated here, in eth, while
			 *          the public facade of the engine works only with the identifiers of the nodes
			 *
			 * @note Is built only on Linux and only with the satellite enabled
			 *       (-DCMAKE_BUILD_XDP=ON): the other systems do not carry the AF_XDP datapath
			 *
			 * \~
			 */
			namespace xdp {
				/**
				 * \~russian
				 * @brief Функция проверки поддержки AF_XDP ядром
				 *
				 * @details Пробует завести сокет семейства AF_XDP: успех означает, что ядро
				 *          несёт этот датапуть. Отказ по праву (EPERM/EACCES) тоже означает
				 *          поддержку: семейство ядром опознано, но заведение сокета требует
				 *          привилегии CAP_NET_ADMIN. Лишь отказ EAFNOSUPPORT значит, что ядро
				 *          не несёт AF_XDP. Проба не требует ни привязки к устройству, ни UMEM
				 *
				 * @return признак поддержки AF_XDP ядром
				 *
				 * \~english
				 * @brief Function of checking the AF_XDP support by the kernel
				 *
				 * @details Tries to create a socket of the AF_XDP family: the success means that the kernel
				 *          carries this datapath. A refusal by right (EPERM/EACCES) also means support: the family
				 *          is recognized by the kernel, but creating the socket requires the CAP_NET_ADMIN privilege.
				 *          Only an EAFNOSUPPORT refusal means that the kernel does not carry AF_XDP. The probe requires
				 *          neither the binding to a device nor the UMEM
				 *
				 * @return sign of the AF_XDP support by the kernel
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool supported() noexcept;
				/**
				 * \~russian
				 * @brief Функция получения описателя сокета AF_XDP
				 *
				 * @details Описатель пригоден для опроса готовности (poll/epoll): по нему
				 *          движок узнаёт о приходе кадров, не обращаясь к кольцам на всякой
				 *          итерации. Описатель внутренний, уровня eth, и публичному фасаду
				 *          движка не выдаётся
				 *
				 * @param state состояние сокета
				 * @return      описатель сокета, либо неверный сокет при отказе
				 *
				 * \~english
				 * @brief Function of getting the descriptor of an AF_XDP socket
				 *
				 * @details The descriptor is suitable for the readiness polling (poll/epoll): by it the engine
				 *          learns of the arrival of the frames without touching the rings on every iteration.
				 *          The descriptor is internal, of the eth level, and is not given out to the public facade
				 *          of the engine
				 *
				 * @param state state of the socket
				 * @return      descriptor of the socket, or an invalid socket on refusal
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ net::socket_t fd(const net::xdp::socket_t * state) noexcept;
				/**
				 * \~russian
				 * @brief Функция уничтожения сокета AF_XDP
				 *
				 * @details Освобождает сокет, кольца и область UMEM, заведённые create(),
				 *          и освобождает само состояние
				 *
				 * @param state состояние сокета
				 * @return      признак успешного уничтожения
				 *
				 * \~english
				 * @brief Function of destroying an AF_XDP socket
				 *
				 * @details Frees the socket, the rings and the UMEM region created by create(),
				 *          and frees the state itself
				 *
				 * @param state state of the socket
				 * @return      sign of the successful destruction
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool destroy(net::xdp::socket_t * state) noexcept;
				/**
				 * \~russian
				 * @brief Функция отправки одной датаграммы через кольцо TX
				 *
				 * @details Берёт свободный кадр из пула, копирует в него датаграмму, кладёт
				 *          дескриптор в кольцо отправки и побуждает ядро к отправке; завершённые
				 *          кадры возвращаются в пул по кольцу завершения
				 *
				 * @param state состояние сокета
				 * @param data  данные датаграммы
				 * @param size  размер датаграммы в байтах
				 * @return      признак успешной отправки
				 *
				 * \~english
				 * @brief Function of sending one datagram through the TX ring
				 *
				 * @details Takes a free frame from the pool, copies the datagram into it, lays the descriptor
				 *          into the sending ring and prompts the kernel to send; the completed frames are returned
				 *          to the pool by the completion ring
				 *
				 * @param state state of the socket
				 * @param data  data of the datagram
				 * @param size  size of the datagram in bytes
				 * @return      sign of the successful sending
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool send(net::xdp::socket_t * state, const uint8_t * data, const uint32_t size) noexcept;
				/**
				 * \~russian
				 * @brief Функция приёма одной датаграммы из кольца RX
				 *
				 * @details Снимает готовые кадры с кольца приёма, копирует первую датаграмму
				 *          в поданный буфер, возвращает кадры в пул свободных и дочитывает
				 *          кольцо наполнения, чтобы ядро могло класть новые кадры
				 *
				 * @param state  состояние сокета
				 * @param buffer буфер под принятую датаграмму
				 * @param size   размер буфера в байтах
				 * @return       число принятых байт, ноль если датаграмм нет, минус единица при отказе
				 *
				 * \~english
				 * @brief Function of receiving one datagram from the RX ring
				 *
				 * @details Takes the ready frames off the reception ring, copies the first datagram into the given
				 *          buffer, returns the frames to the free pool and refills the filling ring so that the kernel
				 *          could lay new frames
				 *
				 * @param state  state of the socket
				 * @param buffer buffer for the received datagram
				 * @param size   size of the buffer in bytes
				 * @return       number of the received bytes, zero if there are no datagrams, minus one on refusal
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ int32_t receive(net::xdp::socket_t * state, uint8_t * buffer, const uint32_t size) noexcept;
				/**
				 * \~russian
				 * @brief Функция заведения сокета AF_XDP
				 *
				 * @details Выделяет область UMEM (кадры памяти), заводит кольца приёма,
				 *          отправки, наполнения и завершения и привязывает сокет AF_XDP к
				 *          устройству и очереди. Умолчанием программа BPF НЕ крепится (признак
				 *          inhibit): заведение не перенаправляет трафик и потому безопасно;
				 *          признак redirect крепит программу перенаправления трафика в сокет.
				 *          Режим обобщённый (generic SKB). Требует привилегии CAP_NET_ADMIN
				 *
				 * @param iface    название сетевого устройства
				 * @param queue    номер очереди устройства
				 * @param redirect крепить ли программу BPF-редиректа трафика в сокет
				 * @return         состояние сокета, либо пустой указатель при отказе
				 *
				 * \~english
				 * @brief Function of creating an AF_XDP socket
				 *
				 * @details Allocates the UMEM region (the memory frames), creates the rings of the reception,
				 *          sending, filling and completion and binds the AF_XDP socket to the device and the queue.
				 *          By default the BPF program is NOT attached (the inhibit flag): the creation does not redirect
				 *          the traffic and therefore is safe; the redirect flag attaches the program of the traffic
				 *          redirection into the socket. The mode is generic (generic SKB). Requires the CAP_NET_ADMIN privilege
				 *
				 * @param iface    name of the network device
				 * @param queue    number of the device queue
				 * @param redirect whether to attach the BPF program of the traffic redirection
				 * @return         state of the socket, or an empty pointer on refusal
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ net::xdp::socket_t * create(const string & iface, const uint32_t queue = 0, const bool redirect = false) noexcept;
			}
		}
	}
#endif