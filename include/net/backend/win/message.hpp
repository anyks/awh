/**
 * @file message.hpp
 * @date 2026-09-03
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
 * @brief Заголовочный файл строя сообщений POSIX для MS Windows — рассеянные буферы,
 *        служебные метаданные и приём сообщения вместе с ними
 *
 * @details Строй этот у MS Windows есть весь, но зовётся своими именами: `WSAMSG`
 *          взамен `msghdr`, `WSABUF` взамен `iovec`, `WSA_CMSG_*` взамен `CMSG_*`, а
 *          сам приём с метаданными - расширенным вызовом `WSARecvMsg`, какой берётся
 *          у сокета управляющим запросом, а не объявлен наперёд
 *
 *          Состав полей у двойников иной, чем у POSIX, и одним переобъявлением имени
 *          тут не обойтись: перенос состава ведётся в самих посредниках
 *
 * @par Намеренные решения
 *
 *      **Вынесено из движка НЕ ради красоты.** Прежде строй этот жил внутри единицы
 *      трансляции `iocp.cpp`, и следствий у того было два, оба вскрылись в один день
 *      03.09.2026. Первое: проверка доставки метки перегрузки на датаграмму
 *      (`SocketEcnDeliveryTest`) пропускалась под MS Windows с доводом «нет `msghdr`,
 *      доступного проверке», - движок метку доносил, а убедиться в том было нечем.
 *      Второе: щуп чтения каталога довести до движка не удалось по той же причине, и
 *      сличать пришлось дословно вынесенный текст двух редакций вместо работы самого
 *      движка. Два независимых следствия одного решения - признак того, что граница
 *      модуля проведена не там
 *
 *      **Приём здесь ПРОСТОЙ, без пула родного приёма.** Движок поверх этого держит
 *      свой `recvmsg`: он сперва спрашивает пул, куда порт завершений принимает сам,
 *      ещё до прихода потребителя, и лишь потом идёт к системе. Пул есть внутренность
 *      движка и ею остаётся. Оттого приём отсюда и приём движка - РАЗНЫЕ утверждения,
 *      и проверка, писанная поверх этого заголовка, доказывает «система донесла метку
 *      на датаграмме», а не «движок донёс её через свой пул»
 *
 *      **Код отказа выдаётся доводом, а не через `errno`.** Перенос кодов `WSA` в
 *      `errno` ведёт таблица движка, и тащить её сюда ради одного вызова незачем.
 *      Вызывающий получает код системы как есть и распоряжается им сам: движок
 *      переносит его в `errno` своей же таблицей, проверке довольно самого кода
 *
 *      **Типы остаются в общем пространстве имён, а функции - в своём.** Имена
 *      `msghdr`, `iovec` и `cmsghdr` движки пишут без всякого пространства, ровно как
 *      у POSIX, и уводить их значило бы разводить код там, где довод общий. Функции же
 *      зовутся с явным указанием - `win::message::receive`, - и с одноимёнными
 *      вызовами системы не путаются никогда
 *
 * @warning Обход служебных метаданных ведётся макросами `CMSG_*`, и прежние их
 *          определения снимаются ЯВНО, а не переопределяются поверх: у MS Windows
 *          имена эти заняты (`mswsock.h`), и работают там с `WSAMSG`, а не с `msghdr`.
 *          Подмена намеренная, и написана она так, чтобы это было видно
 *
 * \~english
 * @brief Header file of the POSIX message layout for MS Windows — the scattered buffers,
 *        the control metadata and the reception of a message together with them
 * @details MS Windows has this whole layout, but it is called by its own names: `WSAMSG`
 *          instead of `msghdr`, `WSABUF` instead of `iovec`, `WSA_CMSG_*` instead of `CMSG_*`,
 *          and the reception with the metadata itself — by the extended call `WSARecvMsg`,
 *          which is taken from a socket by a control request, and is not declared beforehand
 *          The composition of the fields at the twins differs from the POSIX one, and one
 *          redeclaration of a name is not enough here: the transfer of the composition is
 *          performed in the mediators themselves
 * @par Deliberate decisions
 *      **Taken out of the engine NOT for the beauty.** Formerly this layout lived inside the
 *      translation unit `iocp.cpp`, and there were two consequences of that, both of which came
 *      to light in one day, 03.09.2026. The first one: the test of the delivery of the congestion
 *      mark on a datagram (`SocketEcnDeliveryTest`) was being skipped under MS Windows with the
 *      argument «there is no `msghdr` available to the test» — the engine was delivering the mark,
 *      but there was nothing to make sure of it with. The second one: a probe of the reading of a
 *      directory could not be brought to the engine for the same reason, and what had to be compared
 *      was the literally taken out text of two editions instead of the work of the engine itself.
 *      Two independent consequences of one decision are a sign that the boundary of the module is
 *      drawn not where it should be
 *      **The reception here is a PLAIN one, without the pool of the native reception.** The engine
 *      keeps its own `recvmsg` on top of this: it first asks the pool, into which the completion port
 *      receives by itself, even before the arrival of the consumer, and only then goes to the system.
 *      The pool is an internal of the engine and remains one. Hence the reception from here and the
 *      reception of the engine are DIFFERENT statements, and a test written on top of this header
 *      proves «the system has delivered the mark on the datagram», and not «the engine has delivered
 *      it through its pool»
 *      **The code of a refusal is given out by an argument, and not through `errno`.** The transfer of
 *      the `WSA` codes into `errno` is performed by a table of the engine, and dragging it here for the
 *      sake of one call is pointless. The caller receives the code of the system as it is and disposes
 *      of it itself: the engine transfers it into `errno` by its own table, for a test the code itself
 *      is enough
 *      **The types remain in the common namespace, and the functions — in their own one.** The names
 *      `msghdr`, `iovec` and `cmsghdr` the engines write without any namespace, exactly as at POSIX,
 *      and taking them away would mean spreading the code apart where the argument is a common one.
 *      The functions, though, are called with an explicit indication — `win::message::receive`, — and
 *      are never confused with the calls of the system of the same name
 * @warning The walk of the control metadata is performed by the `CMSG_*` macros, and their former
 *          definitions are removed EXPLICITLY, and not redefined on top: at MS Windows these names are
 *          occupied (`mswsock.h`), and they work there with `WSAMSG`, and not with `msghdr`. The
 *          substitution is a deliberate one, and it is written so that this would be visible
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_NET_BACKEND_WIN_MESSAGE__
#define __AWH_NET_BACKEND_WIN_MESSAGE__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdint>
#include <cstddef>

/**
 * Подключаем единую точку подключения системных заголовков MS Windows
 */
#include <sys/win32.hpp>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/net.hpp>

/**
 * \~russian
 * @brief Описание буфера обмена в строе POSIX
 *
 * @note Лежит в общем пространстве имён намеренно: движки пишут имя это без всякого
 *       указания, ровно как у POSIX
 *
 * \~english
 * @brief Description of an exchange buffer in the POSIX layout
 * @note It lies in the common namespace deliberately: the engines write this name without any
 *       indication, exactly as at POSIX
 *
 * \~
 */
struct iovec {
	void * iov_base;                  // Начало буфера обмена
	size_t iov_len;                   // Размер буфера обмена
};

/**
 * \~russian
 * @brief Описание сообщения в строе POSIX
 *
 * \~english
 * @brief Description of a message in the POSIX layout
 *
 * \~
 */
struct msghdr {
	void * msg_name;                  // Адрес собеседника
	socklen_t msg_namelen;            // Размер адреса собеседника
	struct iovec * msg_iov;           // Набор буферов обмена
	size_t msg_iovlen;                // Число буферов обмена
	void * msg_control;               // Буфер служебных метаданных
	size_t msg_controllen;            // Размер буфера служебных метаданных
	int32_t msg_flags;                // Признаки сообщения
};

/**
 * \~russian
 * @brief Заголовок служебного метаданного
 *
 * @note Состав его у MS Windows совпадает с POSIX поимённо, оттого здесь довольно
 *       переобъявления имени
 *
 * \~english
 * @brief Header of a control metadatum
 * @note Its composition at MS Windows coincides with the POSIX one by the names, hence a
 *       redeclaration of the name is enough here
 *
 * \~
 */
typedef WSACMSGHDR cmsghdr;

/**
 * \~russian
 * Принудительное встраивание средствами GCC и Clang
 *
 * @details Обход метаданных идёт на каждой принятой датаграмме, и посредники обязаны
 *          разворачиваться на месте обращения целиком: смысл их в том, чтобы не стоить
 *          ничего сверх самого действия
 *
 * \~english
 * Forced inlining by the means of GCC and Clang
 * @details The walk of the metadata goes on every received datagram, and the mediators are
 *          obliged to unfold at the place of the address entirely: their point is in not costing
 *          anything beyond the action itself
 *
 * \~
 */
#define AWH_WIN_MESSAGE_INLINE inline __attribute__((always_inline))

/**
 * \~russian
 * @brief пространство имён библиотеки
 *
 * \~english
 * @brief namespace of the library
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief пространство имён средств MS Windows
	 *
	 * \~english
	 * @brief namespace of the means of MS Windows
	 *
	 * \~
	 */
	namespace win {
		/**
		 * \~russian
		 * @brief пространство имён строя сообщений
		 *
		 * \~english
		 * @brief namespace of the message layout
		 *
		 * \~
		 */
		namespace message {
			/**
			 * \~russian
			 * @brief Функция выравнивания размера служебного метаданного
			 *
			 * @param size выравниваемый размер
			 * @return     выровненный размер
			 *
			 * \~english
			 * @brief Function of the alignment of the size of a control metadatum
			 * @param size aligned size
			 * @return     aligned size
			 *
			 * \~
			 */
			AWH_WIN_MESSAGE_INLINE size_t align(const size_t size) noexcept {
				// Выравниваем размер по границе машинного слова
				return ((size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1));
			}
			/**
			 * \~russian
			 * @brief Функция получения первого служебного метаданного сообщения
			 *
			 * @param msg описание сообщения
			 * @return    первое служебное метаданное, либо пустое значение
			 *
			 * \~english
			 * @brief Function of the obtaining of the first control metadatum of a message
			 * @param msg description of the message
			 * @return    first control metadatum, or an empty value
			 *
			 * \~
			 */
			AWH_WIN_MESSAGE_INLINE cmsghdr * first(const struct msghdr * msg) noexcept {
				// Если буфера служебных метаданных нет либо он меньше заголовка
				if((msg == nullptr) || (msg->msg_control == nullptr) || (msg->msg_controllen < sizeof(cmsghdr)))
					// Выводим пустое значение
					return nullptr;
				// Выводим первое служебное метаданное
				return reinterpret_cast <cmsghdr *> (msg->msg_control);
			}
			/**
			 * \~russian
			 * @brief Функция получения следующего служебного метаданного сообщения
			 *
			 * @param msg  описание сообщения
			 * @param cmsg текущее служебное метаданное
			 * @return     следующее служебное метаданное, либо пустое значение
			 *
			 * \~english
			 * @brief Function of the obtaining of the next control metadatum of a message
			 * @param msg  description of the message
			 * @param cmsg current control metadatum
			 * @return     next control metadatum, or an empty value
			 *
			 * \~
			 */
			AWH_WIN_MESSAGE_INLINE cmsghdr * next(const struct msghdr * msg, cmsghdr * cmsg) noexcept {
				// Если обход окончен либо заголовок неполон
				if((msg == nullptr) || (cmsg == nullptr) || (cmsg->cmsg_len < sizeof(cmsghdr)))
					// Выводим пустое значение
					return nullptr;
				// Получаем начало буфера служебных метаданных
				uint8_t * const begin = reinterpret_cast <uint8_t *> (msg->msg_control);
				// Получаем следующее служебное метаданное
				uint8_t * const value = (reinterpret_cast <uint8_t *> (cmsg) + align(static_cast <size_t> (cmsg->cmsg_len)));
				// Если следующее метаданное за пределами буфера
				if(static_cast <size_t> ((value + sizeof(cmsghdr)) - begin) > msg->msg_controllen)
					// Выводим пустое значение
					return nullptr;
				// Выводим следующее служебное метаданное
				return reinterpret_cast <cmsghdr *> (value);
			}
			/**
			 * \~russian
			 * @brief Функция получения данных служебного метаданного
			 *
			 * @details Отвечает макросу `CMSG_DATA` и заведена оттого, что имя это у MS
			 *          Windows занято ДВАЖДЫ. Первый раз - сетевыми заголовками
			 *          (`WSA_CMSG_DATA` у `mswsock.h`), и с ним разобрано подменой ниже.
			 *          Второй раз - КРИПТОГРАФИЧЕСКИМИ: `wincrypt.h:2910` объявляет
			 *          `CMSG_DATA` числом 1, видом криптографического сообщения, и
			 *          приходит он через `windows.h` во всякую единицу трансляции, где
			 *          есть хоть что-то от криптографии
			 *
			 * @warning Подмена макроса от второго столкновения НЕ спасает: заголовок
			 *          криптографии приходит ПОЗЖЕ и переопределяет имя числом. Разбор
			 *          тогда отвечает «called object type 'int' is not a function», и
			 *          понять по нему причину нельзя никак. Установлено 03.09.2026 на
			 *          проверке доставки метки перегрузки, куда криптография приходит
			 *          окольным путём
			 *
			 * @note Оттого потребителям, кроме самого движка, следует звать ИМЕННО эту
			 *       функцию: имя её занять нечем, а макрос остаётся ради кода, писанного
			 *       на понятиях POSIX
			 *
			 * @param cmsg служебное метаданное
			 * @return     начало данных служебного метаданного
			 *
			 * \~english
			 * @brief Function of the obtaining of the data of a control metadatum
			 * @details It answers the `CMSG_DATA` macro and is started because this name at MS
			 *          Windows is occupied TWICE. The first time — by the network headers
			 *          (`WSA_CMSG_DATA` at `mswsock.h`), and that one is dealt with by the substitution
			 *          below. The second time — by the CRYPTOGRAPHIC ones: `wincrypt.h:2910` declares
			 *          `CMSG_DATA` as the number 1, a kind of a cryptographic message, and it comes
			 *          through `windows.h` into every translation unit where there is at least
			 *          something of the cryptography
			 * @warning The substitution of the macro does NOT save from the second collision: the header
			 *          of the cryptography comes LATER and redefines the name by a number. The parsing
			 *          then answers «called object type 'int' is not a function», and it is not possible
			 *          to understand the reason by it in any way. Established on 03.09.2026 at the test
			 *          of the delivery of the congestion mark, where the cryptography comes by a roundabout way
			 * @note Hence the consumers, apart from the engine itself, should call EXACTLY this
			 *       function: there is nothing to occupy its name with, and the macro remains for the sake
			 *       of the code written in the POSIX notions
			 * @param cmsg control metadatum
			 * @return     beginning of the data of the control metadatum
			 *
			 * \~
			 */
			AWH_WIN_MESSAGE_INLINE uint8_t * data(const cmsghdr * cmsg) noexcept {
				// Выводим начало данных служебного метаданного
				return (reinterpret_cast <uint8_t *> (const_cast <cmsghdr *> (cmsg)) + align(sizeof(cmsghdr)));
			}
			/**
			 * \~russian
			 * @brief Функция приёма сообщения со служебными метаданными
			 *
			 * @details Приём этот ПРОСТОЙ: он идёт прямо к системе расширенным вызовом
			 *          `WSARecvMsg` и о пуле родного приёма движка не знает ничего.
			 *          Движок держит поверх него свой `recvmsg`, и утверждения у них
			 *          разные - подробности при описании файла
			 *
			 * @warning Расширенный вызов берётся у самого сокета управляющим запросом и
			 *          запоминается: адрес его общий на всю библиотеку сокетов
			 *
			 * @param sock  сокет, из которого ведётся приём
			 * @param msg   описание принимаемого сообщения
			 * @param flags признаки приёма, системой не употребляемые
			 * @param error код отказа системы, если отказ случился
			 * @return      число принятых октетов, либо -1 при отказе
			 *
			 * \~english
			 * @brief Function of the reception of a message with the control metadata
			 * @details This reception is a PLAIN one: it goes straight to the system by the extended
			 *          call `WSARecvMsg` and knows nothing about the pool of the native reception of
			 *          the engine. The engine keeps its own `recvmsg` on top of it, and their
			 *          statements are different ones — the details are at the description of the file
			 * @warning The extended call is taken from the socket itself by a control request and is
			 *          remembered: its address is a common one for the whole library of the sockets
			 * @param sock  socket the reception is performed from
			 * @param msg   description of the received message
			 * @param flags signs of the reception, not used by the system
			 * @param error code of the refusal of the system, if a refusal has happened
			 * @return      number of the received octets, or -1 at a refusal
			 *
			 * \~
			 */
			int64_t receive(const net::socket_t sock, struct msghdr * msg, const int32_t flags = 0, int32_t * error = nullptr) noexcept;
			/**
			 * \~russian
			 * @brief Функция получения расширенного вызова приёма сообщения
			 *
			 * @details Вызов этот у MS Windows не объявлен наперёд: адрес его берётся у
			 *          самого сокета управляющим запросом. Берётся он однажды и
			 *          запоминается - адрес общий для всех сокетов библиотеки
			 *
			 * @note Наружу выведен ради движка: подача НАЛОЖЕННОГО приёма ведётся им
			 *       самим, порту завершений, и простым приёмом отсюда не выражается.
			 *       Прежде добыча эта была заведена в двух местах, и запомненных
			 *       адресов выходило два вместо одного
			 *
			 * @param sock сокет, у которого спрашивается вызов
			 * @return     адрес расширенного вызова, либо пустое значение
			 *
			 * \~english
			 * @brief Function of the obtaining of the extended call of the reception of a message
			 * @details This call at MS Windows is not declared beforehand: its address is taken from
			 *          the socket itself by a control request. It is taken once and is remembered —
			 *          its address is a common one for all the sockets of the library
			 * @note It is brought out for the sake of the engine: the submission of an OVERLAPPED
			 *       reception is performed by it itself, to the completion port, and is not expressed
			 *       by a plain reception from here. Formerly this obtaining was started in two places,
			 *       and there turned out to be two remembered addresses instead of one
			 * @param sock socket the call is asked from
			 * @return     address of the extended call, or an empty value
			 *
			 * \~
			 */
			LPFN_WSARECVMSG extended(const net::socket_t sock) noexcept;
		};
	};
};

/**
 * \~russian
 * Обход служебных метаданных сообщения
 *
 * @details Имена эти у MS Windows заняты макросами `WSA_CMSG_*` (`mswsock.h`), и
 *          работают те с `WSAMSG`, а не с `msghdr`. Оттого прежние определения
 *          снимаются ЯВНО, а не переопределяются поверх: подмена намеренная, и написана
 *          она так, чтобы это было видно
 *
 * @note Найдено разбором предупреждений 25.08.2026: переопределение поверх давало пять
 *       предупреждений `redefined`, и намерение в них не читалось никак
 *
 * \~english
 * The walk of the control metadata of a message
 * @details These names at MS Windows are occupied by the `WSA_CMSG_*` macros (`mswsock.h`), and
 *          they work with `WSAMSG`, and not with `msghdr`. Hence the former definitions are
 *          removed EXPLICITLY, and not redefined on top: the substitution is a deliberate one,
 *          and it is written so that this would be visible
 * @note Found by the analysis of the warnings on 25.08.2026: the redefinition on top gave five
 *       `redefined` warnings, and the intention was not read in them in any way
 *
 * \~
 */
#undef CMSG_FIRSTHDR
#undef CMSG_NXTHDR
#undef CMSG_DATA
#undef CMSG_LEN
#undef CMSG_SPACE
#define CMSG_FIRSTHDR(msg) ::awh::win::message::first(msg)
#define CMSG_NXTHDR(msg, cmsg) ::awh::win::message::next(msg, cmsg)
#define CMSG_DATA(cmsg) ::awh::win::message::data(cmsg)
#define CMSG_LEN(size) (::awh::win::message::align(sizeof(cmsghdr)) + (size))
#define CMSG_SPACE(size) (::awh::win::message::align(sizeof(cmsghdr)) + ::awh::win::message::align(size))

#endif // __AWH_NET_BACKEND_WIN_MESSAGE__
