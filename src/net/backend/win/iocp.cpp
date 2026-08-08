/**
 * @file: iocp.cpp
 * @date: 2026-08-05
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация асинхронного движка ввода-вывода для MS Windows —
 *        каркас бэкенда на портах завершения ввода-вывода (IOCP)
 *
 * @details Состояние каждого метода описано в файле README.md, лежащем рядом. Методы,
 *          телом не обзаведшиеся, отвечают отказом и заносят предупреждение в журнал —
 *          молча не отказывает ни один. Помечены они меткой `@todo IOCP`, и перечень
 *          их получается поиском по этой метке
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <deque>
#include <mutex>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <string>
#include <condition_variable>

/**
 * Подключаем единую точку подключения системных заголовков MS Windows
 */
#include <sys/win32.hpp>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Название бэкенда для записей в журнале
 *
 */
static constexpr const char * __AWH_IO_BACKEND__ = "MS Windows IO backend";

/**
 * @brief Инкапсулируем состояние движка в пространство имён
 *
 * @details Класс `awh::engine::IO` собственных полей под состояние не имеет: заголовок
 *          net/io.hpp един для всех систем, и заводить в нём поля, нужные лишь одному
 *          бэкенду, нельзя. Эталонный бэкенд bsd/kqueue.cpp держит состояние в файловых
 *          статиках, и здесь принят тот же порядок
 *
 * @note Ожидание событий ведётся портом завершения ввода-вывода. Соответствия ему у
 *       систем POSIX нет: там ожидание сообщает о **готовности** дескриптора к обмену,
 *       а обмен ведёт сама петля. Здесь наоборот - обмен начат заранее, а порт
 *       сообщает о его **завершении**
 *
 * @warning Обмен сокетами ещё не переведён на порт: сокеты движком пока не заводятся
 *          вовсе, и порт обслуживает межпроцессный обмен да пробуждения петли. Узлы
 *          прочих видов отвечают отказом с записью в журнал
 *
 */
namespace {
	/**
	 * @brief Узел события
	 *
	 */
	struct Node {
		// Идентификатор события
		awh::event::id_t id;
		// Вид узла
		awh::event::node_t node;
		// Семейство событий
		awh::event::family_t family;
		// Тип сокета
		awh::event::type_t type;
		// Протокол передачи данных
		awh::event::protocol_t protocol;
		// Состояние узла
		awh::event::status_t status;
		// Опции события
		uint16_t options;
		// Дескриптор операционной системы (у узла NOTIFY отсутствует)
		HANDLE handle;
		// Идентификатор узла-собеседника пары IPC (0 у узлов одиночных)
		awh::event::id_t peer;
		// Сокет узла, если за узлом стоит сокет
		SOCKET socket;
		// Название целевой машины
		std::string host;
		// Порт целевой машины
		uint16_t port;
		// Порт, каким узел представляется сети
		uint16_t source;
		// Число незавершённых обменов, начатых на узле
		uint32_t pending;
		// Признак того, что узел закрывается и новых обменов заводить не следует
		bool closing;
		/**
		 * Имя именованного канала, стоящего за узлом
		 *
		 * @details Соответствия socketpair у MS Windows нет, и пара обмена сообщениями
		 *          между процессами строится именованным каналом. Имя это узел несёт
		 *          затем, чтобы сторона, канал не заводившая, могла открыть свой конец
		 *          сама - порождённый процесс проходит main заново и дескриптора по
		 *          наследству не получает
		 *
		 */
		std::wstring name;
		// Признак того, что узел держит сторону канала, ожидающую подключения
		bool listener;
		// Поток чтения из именованного канала
		std::thread reader;
		// Признак того, что потоку чтения следует завершиться
		std::shared_ptr <std::atomic_bool> stopped;
		// Функция обратного вызова на чтение сообщений
		awh::engine::callback::read_t read;
		// Функция обратного вызова на запись сообщений
		awh::engine::callback::write_t write;
		// Функция обратного вызова на изменение состояния
		awh::engine::callback::status_t state;
		// Функция обратного вызова на получение ошибок
		awh::engine::callback::error_t error;
		// Функция обратного вызова на доступность очереди сообщений
		awh::engine::callback::available_t available;
		/**
		 * @brief Конструктор
		 *
		 */
		Node() noexcept :
		 id(0), node(awh::event::node_t::NONE), family(awh::event::family_t::NONE),
		 type(awh::event::type_t::NONE), protocol(awh::event::protocol_t::NONE),
		 status(awh::event::status_t::NONE), options(awh::event::options::NONE),
		 handle(INVALID_HANDLE_VALUE), peer(0), socket(INVALID_SOCKET),
		 host{""}, port(0), source(0), pending(0), closing(false),
		 name{L""}, listener(false),
		 stopped(std::make_shared <std::atomic_bool> (false)) {}
	};

	// Список заведённых узлов событий
	std::unordered_map <awh::event::id_t, std::unique_ptr <Node>> __awh_nodes__;
	// Счётчик выдачи идентификаторов событий (нулевой идентификатор означает отсутствие)
	awh::event::id_t __awh_last_id__ = 0;
	// Замок доступа к списку узлов
	std::recursive_mutex __awh_mutex__;
	// Замок ожидания петли событий
	std::mutex __awh_wait_mutex__;
	/**
	 * @brief Порт завершения ввода-вывода, вокруг какого построен движок
	 *
	 * @details Порт этот - средоточие движка: в него сходятся завершения всех
	 *          начатых обменов, и петля событий занята одним лишь снятием их с
	 *          порта. Соответствия ему у систем POSIX нет: там ожидание сообщает о
	 *          **готовности** дескриптора к обмену, а обмен ведёт сама петля.
	 *          Здесь наоборот - обмен начат заранее, а порт сообщает о его
	 *          **завершении**
	 *
	 * @warning Связь дескриптора с портом рвётся одним лишь закрытием дескриптора.
	 *          Отвязать его иначе нельзя вовсе, оттого закрытие обязано идти одной
	 *          обёрткой, где стоит и снятие всего, что за дескриптором числится
	 *
	 */
	HANDLE __awh_port__ = nullptr;
	/**
	 * @brief Ключ завершения, каким помечается пробуждение петли
	 *
	 * @note Ключом этим не может быть ни один идентификатор события: счёт их
	 *       начинается с единицы, а нулевой означает отсутствие
	 *
	 */
	constexpr ULONG_PTR __AWH_KEY_WAKE__ = 0;
	/**
	 * @brief Вид завершения, снятого с порта
	 *
	 */
	enum class op_t : uint8_t {
		WAKE    = 0x00, // Пробуждение петли без работы
		MESSAGE = 0x01, // Принятое сообщение, подлежащее раздаче
		RECV    = 0x02, // Завершение приёма из сокета
		SEND    = 0x03, // Завершение отправки в сокет
		ACCEPT  = 0x04, // Завершение принятия входящей связи
		CONNECT = 0x05  // Завершение установки исходящей связи
	};
	/**
	 * @brief Описание начатого обмена, отдаваемое порту завершения
	 *
	 * @details Описатель наложенного обмена обязан жить всё время, пока обмен не
	 *          завершён: система пишет в него итог и отдаёт его обратно портом.
	 *          Оттого он и заводится в куче, а не на стеке начавшего обмен
	 *
	 * @note Поле наложенного обмена стоит первым намеренно: система отдаёт
	 *       указатель именно на него, и приведение обратно к описанию опирается на
	 *       совпадение их начал
	 *
	 */
	struct Overlapped {
		OVERLAPPED overlapped;          // Описатель наложенного обмена для системы
		op_t operation;                 // Вид завершившегося обмена
		awh::event::id_t id;            // Идентификатор события, которому обмен принадлежит
		std::vector <uint8_t> buffer;   // Буфер обмена
		SOCKET socket;                  // Сокет принятой связи у принятия входящей
		/**
		 * @brief Конструктор
		 *
		 * @param operation вид обмена
		 * @param id        идентификатор события
		 *
		 */
		Overlapped(const op_t operation, const awh::event::id_t id) noexcept :
		 overlapped{}, operation(operation), id(id), socket(INVALID_SOCKET) {}
	};
	// Признак инициализации движка
	bool __awh_initialized__ = false;
	// Вид внутреннего таймера петли событий
	awh::event::timer_t __awh_timer__ = awh::event::timer_t::SIMPLE;

	/**
	 * @brief Функция поиска узла события по его идентификатору
	 *
	 * @param id идентификатор события
	 * @return   указатель на узел, либо nullptr если узел не найден
	 *
	 */
	Node * __awh_find__(const awh::event::id_t id) noexcept {
		// Выполняем поиск узла в списке заведённых
		auto i = __awh_nodes__.find(id);
		// Возвращаем найденный узел, либо признак отсутствия
		return ((i != __awh_nodes__.end()) ? i->second.get() : nullptr);
	}


	/**
	 * @brief Расширенные вызовы Winsock, каких нет в самой библиотеке
	 *
	 * @details Вызовы эти в библиотеке не объявлены вовсе: их адреса спрашиваются у
	 *          самого поставщика услуг сокетов по неповторимому номеру. Сделано так
	 *          затем, что поставщики бывают разные, и вызов у каждого свой
	 *
	 * @note Спрашиваются они однажды и на любом сокете: набор их от сокета не
	 *       зависит вовсе
	 *
	 */
	struct extensions_t {
		LPFN_CONNECTEX connect;                 // Установка исходящей связи наложенным обменом
		LPFN_ACCEPTEX accept;                   // Принятие входящей связи наложенным обменом
		LPFN_GETACCEPTEXSOCKADDRS addresses;    // Разбор адресов принятой связи
		/**
		 * @brief Конструктор
		 *
		 */
		extensions_t() noexcept : connect(nullptr), accept(nullptr), addresses(nullptr) {}
	};
	// Расширенные вызовы Winsock
	extensions_t __awh_ext__;
	/**
	 * @brief Функция получения расширенных вызовов Winsock
	 *
	 * @param sock сокет, у поставщика которого спрашиваются вызовы
	 * @return     признак готовности набора вызовов
	 *
	 */
	bool __awh_extensions__(const SOCKET sock) noexcept {
		// Если набор вызовов уже получен
		if(__awh_ext__.connect != nullptr)
			// Выводим признак готовности набора
			return true;
		/**
		 * @brief Запрос одного расширенного вызова у поставщика услуг сокетов
		 *
		 */
		#define __AWH_EXT__(field, guid) do { \
			GUID id = guid; \
			DWORD size = 0; \
			if(::WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &id, sizeof(id), &__awh_ext__.field, sizeof(__awh_ext__.field), &size, nullptr, nullptr) != 0) \
				__awh_ext__.field = nullptr; \
		} while(0)
		// Выполняем запрос расширенных вызовов
		__AWH_EXT__(connect, WSAID_CONNECTEX);
		__AWH_EXT__(accept, WSAID_ACCEPTEX);
		__AWH_EXT__(addresses, WSAID_GETACCEPTEXSOCKADDRS);
		// Снимаем объявление запроса вызова
		#undef __AWH_EXT__
		// Выводим признак готовности набора вызовов
		return ((__awh_ext__.connect != nullptr) && (__awh_ext__.accept != nullptr) && (__awh_ext__.addresses != nullptr));
	}

	/**
	 * @brief Функция сборки адреса из названия машины и порта
	 *
	 * @param family семейство адресов
	 * @param host   название машины, пустое означает «любой адрес»
	 * @param port   порт машины
	 * @param out    собранный адрес
	 * @param size   размер собранного адреса
	 * @return       признак успешной сборки
	 *
	 */
	bool __awh_sockaddr__(const awh::event::family_t family, const std::string & host, const uint16_t port, sockaddr_storage & out, int32_t & size) noexcept {
		// Обнуляем собираемый адрес
		::memset(&out, 0, sizeof(out));
		// Если собирается адрес IPv6
		if(family == awh::event::family_t::IPV6){
			// Получаем собираемый адрес нужного вида
			sockaddr_in6 * value = reinterpret_cast <sockaddr_in6 *> (&out);
			// Устанавливаем семейство адреса
			value->sin6_family = AF_INET6;
			// Устанавливаем порт машины
			value->sin6_port = ::htons(port);
			// Если название машины не задано - берётся любой адрес
			if(host.empty())
				// Устанавливаем адрес, означающий любой
				value->sin6_addr = in6addr_any;
			// Если разобрать название машины не удалось
			else if(::inet_pton(AF_INET6, host.c_str(), &value->sin6_addr) != 1)
				// Выводим признак неуспешной сборки
				return false;
			// Запоминаем размер собранного адреса
			size = static_cast <int32_t> (sizeof(sockaddr_in6));
			// Выводим признак успешной сборки
			return true;
		}
		// Получаем собираемый адрес нужного вида
		sockaddr_in * value = reinterpret_cast <sockaddr_in *> (&out);
		// Устанавливаем семейство адреса
		value->sin_family = AF_INET;
		// Устанавливаем порт машины
		value->sin_port = ::htons(port);
		// Если название машины не задано - берётся любой адрес
		if(host.empty())
			// Устанавливаем адрес, означающий любой
			value->sin_addr.s_addr = INADDR_ANY;
		// Если разобрать название машины не удалось
		else if(::inet_pton(AF_INET, host.c_str(), &value->sin_addr) != 1)
			// Выводим признак неуспешной сборки
			return false;
		// Запоминаем размер собранного адреса
		size = static_cast <int32_t> (sizeof(sockaddr_in));
		// Выводим признак успешной сборки
		return true;
	}

	/**
	 * @brief Функция заведения приёма из сокета
	 *
	 * @details Приём заводится **вперёд**, не дожидаясь прихода данных: в этом и
	 *          состоит устройство порта завершения. Ожидания готовности здесь нет
	 *          вовсе - система сама сложит принятое в отданный ей буфер и сообщит,
	 *          когда закончит
	 *
	 * @note Буфер обмена принадлежит описанию обмена и живёт, пока обмен не
	 *       завершён: отдавать системе память, живущую на стеке заводящего, нельзя
	 *
	 * @param item узел, на котором заводится приём
	 * @return     признак успешного заведения
	 *
	 */
	bool __awh_recv__(Node * item) noexcept {
		// Если узел закрывается либо сокета за ним нет
		if((item == nullptr) || item->closing || (item->socket == INVALID_SOCKET))
			// Выводим признак неуспешного заведения
			return false;
		// Заводим описание приёма
		Overlapped * context = new Overlapped(op_t::RECV, item->id);
		// Отводим место под принимаемые данные
		context->buffer.assign(0x4000, 0);
		// Описание буфера приёма для библиотеки сокетов
		WSABUF buffer{};
		// Устанавливаем размер буфера приёма
		buffer.len = static_cast <ULONG> (context->buffer.size());
		// Устанавливаем сам буфер приёма
		buffer.buf = reinterpret_cast <char *> (context->buffer.data());
		// Число принятых байт
		DWORD received = 0;
		// Признаки приёма
		DWORD flags = 0;
		// Если завести приём не удалось
		if((::WSARecv(item->socket, &buffer, 1, &received, &flags, &context->overlapped, nullptr) != 0) && (::WSAGetLastError() != WSA_IO_PENDING)){
			// Выполняем освобождение описания приёма
			delete context;
			// Выводим признак неуспешного заведения
			return false;
		}
		// Отмечаем заведённый обмен незавершённым
		item->pending++;
		// Выводим признак успешного заведения
		return true;
	}

	/**
	 * @brief Функция заведения принятия входящей связи
	 *
	 * @details Сокет под принимаемую связь заводится **заранее**: система кладёт в
	 *          него принятую связь сама, не спрашивая. Тем принятие и отличается от
	 *          привычного accept, где сокет заводит система в миг принятия
	 *
	 * @param item узел, на котором заводится принятие
	 * @return     признак успешного заведения
	 *
	 */
	bool __awh_accept__(Node * item) noexcept {
		// Если узел закрывается либо сокета за ним нет
		if((item == nullptr) || item->closing || (item->socket == INVALID_SOCKET))
			// Выводим признак неуспешного заведения
			return false;
		// Определяем семейство заводимого сокета
		const int32_t family = (item->family == awh::event::family_t::IPV6 ? AF_INET6 : AF_INET);
		// Заводим сокет под принимаемую связь
		const SOCKET accepted = ::WSASocketW(family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		// Если завести сокет не удалось
		if(accepted == INVALID_SOCKET)
			// Выводим признак неуспешного заведения
			return false;
		// Заводим описание принятия
		Overlapped * context = new Overlapped(op_t::ACCEPT, item->id);
		// Запоминаем сокет принимаемой связи
		context->socket = accepted;
		/**
		 * Место под адреса отводится с запасом
		 *
		 * @note Система требует на каждый из двух адресов шестнадцать байт сверх
		 *       его собственного размера - таково её требование, а не наша
		 *       предосторожность
		 *
		 */
		context->buffer.assign((sizeof(sockaddr_storage) + 16) * 2, 0);
		// Число принятых вместе со связью байт
		DWORD received = 0;
		/**
		 * Заводим принятие входящей связи без ожидания данных
		 *
		 * @note Нулевая длина принимаемых данных задана намеренно: принятие с
		 *       ожиданием данных придержало бы связь до первой их порции, а
		 *       собеседник вправе молчать сколь угодно долго
		 *
		 */
		if(!__awh_ext__.accept(item->socket, accepted, context->buffer.data(), 0, sizeof(sockaddr_storage) + 16, sizeof(sockaddr_storage) + 16, &received, &context->overlapped) && (::WSAGetLastError() != WSA_IO_PENDING)){
			// Выполняем закрытие заведённого сокета
			::closesocket(accepted);
			// Выполняем освобождение описания принятия
			delete context;
			// Выводим признак неуспешного заведения
			return false;
		}
		// Отмечаем заведённый обмен незавершённым
		item->pending++;
		// Выводим признак успешного заведения
		return true;
	}

	/**
	 * @brief Функция составления имени именованного канала
	 *
	 * @details Имя обязано быть неповторимым в пределах всей машины: пространство имён
	 *          каналов у MS Windows общее, и совпадение имён свело бы вместе два
	 *          несвязанных кластера. Неповторимость даётся парой «номер процесса -
	 *          порядковый номер события»: первый разводит процессы между собой, второй
	 *          разводит пары внутри одного процесса
	 *
	 * @param id идентификатор события
	 * @return   полное имя именованного канала
	 *
	 */
	std::wstring __awh_pipe_name__(const awh::event::id_t id) noexcept {
		// Составляем имя именованного канала
		return (L"\\\\.\\pipe\\awh-" + std::to_wstring(static_cast <uint32_t> (::GetCurrentProcessId())) + L"-" + std::to_wstring(static_cast <uint64_t> (id)));
	}

	/**
	 * @brief Функция пробуждения петли событий
	 *
	 */
	void __awh_wake__() noexcept {
		// Выполняем блокировку замка ожидания
		const std::lock_guard <std::mutex> lock(__awh_wait_mutex__);
		// Если порт завершения заведён
		if(__awh_port__ != nullptr)
			// Отдаём порту завершение пробуждения без работы
			::PostQueuedCompletionStatus(__awh_port__, 0, __AWH_KEY_WAKE__, nullptr);
	}

	/**
	 * @brief Функция отдачи порту принятого сообщения
	 *
	 * @details Приняв сообщение, поток чтения не раздаёт его сам: раздача обязана
	 *          идти из потока петли событий, ибо обратные вызовы вправе заводить и
	 *          уничтожать события. Сообщение потому отдаётся порту, а петля снимает
	 *          его оттуда наравне с завершениями обменов
	 *
	 * @param id     идентификатор события
	 * @param buffer буфер принятого сообщения
	 * @param size   размер принятого сообщения
	 *
	 */
	void __awh_post__(const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept {
		// Выполняем блокировку замка ожидания
		const std::lock_guard <std::mutex> lock(__awh_wait_mutex__);
		// Если порт завершения не заведён
		if(__awh_port__ == nullptr)
			// Выходим из функции
			return;
		// Заводим описание отдаваемого сообщения
		Overlapped * context = new Overlapped(op_t::MESSAGE, id);
		// Наполняем буфер отдаваемого сообщения
		context->buffer.assign(buffer, buffer + size);
		// Если отдать сообщение порту не удалось
		if(!::PostQueuedCompletionStatus(__awh_port__, static_cast <DWORD> (size), static_cast <ULONG_PTR> (id), &context->overlapped))
			// Выполняем освобождение описания сообщения
			delete context;
	}

	/**
	 * @brief Функция запуска потока чтения из именованного канала
	 *
	 * @details Поток ведёт чтение вызовом ReadFile без наложения, оттого он и нужен:
	 *          иначе ожидание сообщения остановило бы петлю событий. С приходом порта
	 *          завершения ввода-вывода поток этот уходит - чтение станет наложенным, а
	 *          готовность его будет снимать сама петля
	 *
	 * @note Канал открыт в строе сообщений, и одно чтение даёт ровно одно сообщение
	 *       целиком. Отказ ERROR_MORE_DATA означает, что сообщение в буфер не влезло:
	 *       буфер тогда наращивается, а остаток дочитывается следующим вызовом
	 *
	 * @warning Поток обращается к узлу лишь под замком и по идентификатору, а не по
	 *          указателю: узел вправе исчезнуть между двумя чтениями
	 *
	 * @param id идентификатор события
	 *
	 */
	void __awh_reader__(const awh::event::id_t id) noexcept {
		// Дескриптор именованного канала, из которого ведётся чтение
		HANDLE handle = INVALID_HANDLE_VALUE;
		// Признак того, что узел держит сторону канала, ожидающую подключения
		bool listener = false;
		// Признак того, что потоку чтения следует завершиться
		std::shared_ptr <std::atomic_bool> stopped;
		{
			// Выполняем блокировку замка доступа к списку узлов
			const std::lock_guard <std::recursive_mutex> lock(__awh_mutex__);
			// Выполняем поиск узла события
			Node * item = __awh_find__(id);
			// Если узел не найден - читать неоткуда
			if(item == nullptr)
				// Завершаем работу потока чтения
				return;
			// Запоминаем дескриптор именованного канала
			handle = item->handle;
			// Запоминаем признак стороны, ожидающей подключения
			listener = item->listener;
			// Запоминаем признак завершения потока чтения
			stopped = item->stopped;
		}
		// Если дескриптор канала не получен либо признак завершения не заведён
		if((handle == INVALID_HANDLE_VALUE) || !stopped)
			// Завершаем работу потока чтения
			return;
		/**
		 * Сторона ожидания дожидается подключения встречной
		 *
		 * @details Читать со стороны, к какой никто не подключён, нельзя: система
		 *          отвечает отказом ERROR_PIPE_LISTENING. Отказ ERROR_PIPE_CONNECTED
		 *          означает, что встречная сторона подключилась прежде вызова, и
		 *          отказом по существу не является
		 *
		 */
		if(listener){
			// Событие завершения наложенного ожидания подключения
			HANDLE signal = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
			// Описание наложенной операции ожидания подключения
			OVERLAPPED overlapped{};
			// Привязываем событие завершения к наложенной операции
			overlapped.hEvent = signal;
			// Выполняем наложенное ожидание подключения встречной стороны
			if(!::ConnectNamedPipe(handle, &overlapped)){
				// Получаем код отказа ожидания подключения
				const DWORD code = ::GetLastError();
				/**
				 * Определяем исход ожидания подключения
				 */
				switch(code){
					// Ожидание принято системой - дожидаемся его завершения
					case ERROR_IO_PENDING: {
						// Количество переданных байт наложенной операции
						DWORD transferred = 0;
						// Дожидаемся завершения наложенного ожидания подключения
						if(!::GetOverlappedResult(handle, &overlapped, &transferred, TRUE)){
							// Закрываем событие завершения наложенной операции
							::CloseHandle(signal);
							// Завершаем работу потока чтения
							return;
						}
					} break;
					// Встречная сторона подключилась прежде вызова - отказом это не является
					case ERROR_PIPE_CONNECTED: break;
					// Всякий иной исход означает отказ
					default: {
						// Закрываем событие завершения наложенной операции
						::CloseHandle(signal);
						// Завершаем работу потока чтения
						return;
					}
				}
			}
			// Закрываем событие завершения наложенной операции
			::CloseHandle(signal);
		}
		// Событие завершения наложенных операций чтения
		HANDLE signal = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
		// Если событие завершения завести не удалось
		if(signal == nullptr)
			// Завершаем работу потока чтения
			return;
		// Буфер принимаемого сообщения
		std::vector <uint8_t> buffer(4096, 0);
		/**
		 * Выполняем чтение до тех пор, пока канал не закроется либо не поступит
		 * указание завершиться
		 */
		while(!stopped->load()){
			// Количество принятых байт
			DWORD received = 0;
			// Описание наложенной операции чтения
			OVERLAPPED overlapped{};
			// Привязываем событие завершения к наложенной операции
			overlapped.hEvent = signal;
			// Сбрасываем событие завершения наложенной операции
			::ResetEvent(signal);
			// Выполняем наложенное чтение сообщения из именованного канала
			BOOL result = ::ReadFile(handle, buffer.data(), static_cast <DWORD> (buffer.size()), &received, &overlapped);
			// Если чтение принято системой к исполнению - дожидаемся его завершения
			if(!result && (::GetLastError() == ERROR_IO_PENDING))
				// Дожидаемся завершения наложенного чтения
				result = ::GetOverlappedResult(handle, &overlapped, &received, TRUE);
			// Если чтение отказом завершилось
			if(!result){
				// Получаем код отказа чтения
				const DWORD code = ::GetLastError();
				// Если сообщение в буфер не поместилось - наращиваем буфер и дочитываем
				if(code == ERROR_MORE_DATA){
					// Наращиваем буфер вдвое
					buffer.resize(buffer.size() * 2, 0);
					// Переходим к следующему чтению
					continue;
				}
				// Завершаем работу потока чтения: канал закрыт другой стороной
				break;
			}
			// Если сообщение принято
			if(received > 0){
				{
					// Выполняем блокировку замка доступа к списку узлов
					const std::lock_guard <std::recursive_mutex> lock(__awh_mutex__);
					// Если узел исчез - завершаем работу потока чтения
					if(__awh_find__(id) == nullptr)
						// Завершаем работу потока чтения
						break;
				}
				// Отдаём принятое сообщение порту завершения
				__awh_post__(id, buffer.data(), static_cast <size_t> (received));
			// Если сообщения не принято - лишь пробуждаем петлю событий
			} else __awh_wake__();
		}
		// Закрываем событие завершения наложенных операций чтения
		::CloseHandle(signal);
	}
};

/**
 * @brief Метод фиксации настроек события
 *
 * @details Закрепляет всё, что было выставлено событию после `event()`:
 *          адреса, порты, опции сокета, размеры буферов, сроки. До этого
 *          вызова настройки лежат в самом событии и до сокета не доходят, а
 *          после - применены, и событие переходит из состояния «заведено» в
 *          «инициализировано»
 *
 * @details Состояния события образуют последовательность, и каждый шаг
 *          требует предыдущего:
 *
 *          | Вызов | Требует состояния | Оставляет состояние |
 *          |---|---|---|
 *          | `event()` | - | `NONE` |
 *          | `commit()` | `NONE` | `INITIAL` |
 *          | `connect()` | `INITIAL` | `SUCCESS` |
 *          | `listen()` | `INITIAL` | `SUCCESS` |
 *          | `launch()` | `INITIAL` или `SUCCESS` | `LAUNCHED` / `LISTENING` |
 *
 *          Отсюда следует, что `connect()` и `listen()` ставятся **между**
 *          фиксацией и запуском, а не до фиксации и не после запуска.
 *
 * @note Повторная фиксация уже инициализированного события ничего не делает
 *       и возвращает отрицательный результат: состояние `NONE` бывает у
 *       события лишь однажды. Настройки, изменённые после фиксации,
 *       применяются своими методами сразу, фиксации не требуя
 *
 * @param id идентификатор события
 * @return   результат выполнения фиксации
 *
 */
bool awh::engine::IO::commit([[maybe_unused]] const event::id_t id) noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Если узел не найден — фиксировать нечего
	if(item == nullptr)
		// Возвращаем отрицательный результат фиксации
		return false;
	/**
	 * Состояние NONE бывает у события лишь однажды: повторная фиксация уже
	 * инициализированного события ничего не делает и отвечает отказом
	 */
	if(item->status != event::status_t::NONE)
		// Возвращаем отрицательный результат фиксации
		return false;
	// Переводим узел в состояние инициализации
	item->status = event::status_t::INITIAL;
	// Возвращаем положительный результат фиксации
	return true;
}

/**
 * @brief Метод перестройки события: пересоздание нижележащего дескриптора с сохранением самого события
 *
 * @note Приложение работает с идентификатором события, а не с дескриптором,
 *       поэтому дескриптор пересоздаётся, а событие (его идентификатор,
 *       коллбэки, адрес/порт, опции и таймеры) сохраняется - подмена
 *       дескриптора приложению незаметна. Всё состояние, живущее на
 *       дескрипторе (регистрации (kqueue, epoll, ...), размеры буферов, DSCP/ECN/MTU,
 *       интерфейс), снимается до закрытия и переприменяется на новый
 *       дескриптор, а пройденные стадии подъёма (commit/listen/launch)
 *       переигрываются по исходному статусу события. Поддерживается для
 *       событий типа SERVER
 *
 * @param id идентификатор события
 * @return   результат выполнения перестройки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::rebuild([[maybe_unused]] const event::id_t id) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения сетевого интерфейса события
 *
 * @param id идентификатор события
 * @return   сетевой интерфейс события
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
string awh::engine::IO::getIface([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return string();
}

/**
 * @brief Метод установки сетевого интерфейса события
 *
 * @param id   идентификатор события
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setIface([[maybe_unused]] const event::id_t id, [[maybe_unused]] string_view name) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения локального порта события
 *
 * @param id идентификатор события
 * @return   локальный порт события
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
uint16_t awh::engine::IO::getSourcePort([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return uint16_t();
}

/**
 * @brief Метод установки локального порта события
 *
 * @param id   идентификатор события
 * @param port локальный порт события
 * @return     результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setSourcePort([[maybe_unused]] const event::id_t id, [[maybe_unused]] const uint16_t port) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения порта назначения события
 *
 * @param id идентификатор события
 * @return   порт назначения события
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
uint16_t awh::engine::IO::getTargetPort([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return uint16_t();
}

/**
 * @brief Метод установки порта назначения события
 *
 * @param id   идентификатор события
 * @param port порт назначения события
 * @return     результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setTargetPort([[maybe_unused]] const event::id_t id, [[maybe_unused]] const uint16_t port) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param id идентификатор события
 * @return   адрес хоста целевой машины
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
string awh::engine::IO::getTarget(const event::id_t id) const noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Если узел не найден - отдавать нечего
	if(item == nullptr)
		// Возвращаем пустой результат
		return string();
	/**
	 * Узел обмена сообщениями отдаёт имя своего именованного канала
	 *
	 * @details Именем этим сторона, канал не заводившая, открывает свой конец сама.
	 *          Кластеру оно нужно затем, чтобы передать его порождаемому процессу:
	 *          дескриптора тот по наследству не получает, а имя переносимо
	 *
	 */
	if(item->node == event::node_t::IPC)
		// Возвращаем имя именованного канала узла
		return this->_fmk->convert(item->name);
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return string();
}

/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param id     идентификатор события
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setTarget(const event::id_t id, string_view target) noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Если узел не найден - устанавливать некому
	if(item == nullptr)
		// Возвращаем отрицательный результат установки
		return false;
	/**
	 * Узлу обмена сообщениями задаётся имя именованного канала
	 *
	 * @details Само подключение к каналу ведёт connect: здесь имя лишь запоминается,
	 *          ровно как у сокета запоминается адрес целевой машины
	 *
	 */
	if(item->node == event::node_t::IPC){
		// Запоминаем имя именованного канала узла
		item->name = this->_fmk->convert(string(target));
		// Возвращаем положительный результат установки
		return true;
	}
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param id     идентификатор события
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::getTarget([[maybe_unused]] const event::id_t id, [[maybe_unused]] unique_ptr <net::addr_t> & target) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param id     идентификатор события
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setTarget([[maybe_unused]] const event::id_t id, [[maybe_unused]] const net::addr_t * target) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения адреса события
 *
 * @param id      идентификатор события
 * @param address тип адреса события
 * @return        значение адреса события
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
string awh::engine::IO::getAddress([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::address_t address) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return string();
}

/**
 * @brief Метод установки адреса события
 *
 * @param id      идентификатор события
 * @param address тип адреса события
 * @param value   значение адреса события
 * @return        результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setAddress([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::address_t address, [[maybe_unused]] string_view value) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения адреса события
 *
 * @param id      идентификатор события
 * @param address тип адреса события
 * @param value   объект для извлечения адреса события
 * @return        результат выполнения извлечения адреса события
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::getAddress([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::address_t address, [[maybe_unused]] unique_ptr <net::addr_t> & value) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод установки адреса события
 *
 * @param id      идентификатор события
 * @param address тип адреса события
 * @param value   значение адреса события
 * @return        результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setAddress([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::address_t address, [[maybe_unused]] const net::addr_t * value) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param id идентификатор события
 * @return   MTU сетевого интерфейса
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
uint16_t awh::engine::IO::getMaximumTransmissionUnit([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return uint16_t();
}

/**
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param id  идентификатор события
 * @param mtu размер MTU интерфейса
 * @return    результат установки MTU сетевого интерфейса
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setMaximumTransmissionUnit([[maybe_unused]] const event::id_t id, [[maybe_unused]] const uint32_t mtu) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения признака выдачи системой поля Explicit Congestion Notification (ECN) принятых пакетов
 *
 * @details Отметку перегрузки пути ставит маршрутизатор в заголовок пакета,
 *          а выдаёт её принимающему уже ядро - служебным сообщением при
 *          приёме дейтаграммы. Выдают её не все системы: NetBSD и OpenBSD
 *          по IPv4 не выдают вовсе, и опции запроса такой выдачи у них не
 *          заведено. По IPv6 выдают обе
 *
 *          Вызывающему это знать необходимо. Обмен, помечающий свои пакеты
 *          поддержкой отметок и не получающий отметок обратно, обязан по
 *          договору признать проверку несостоявшейся и отметки отключить -
 *          то есть проделать лишний круг там, где исход известен заранее
 *
 * @note Признак решается наличием средства запроса выдачи, а не перечнем
 *       систем поимённо: перечень устареет с первым же выпуском, который
 *       средство добавит
 *
 * @param family семейство протоколов (IPv4 или IPv6)
 * @return       признак выдачи системой отметок перегрузки пути
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::availableExplicitCongestionNotification([[maybe_unused]] const event::family_t family) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param id     идентификатор события
 * @param family семейство протоколов (IPv4 или IPv6)
 * @return       значение DSCP
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
awh::event::dscp_t awh::engine::IO::getDifferentiatedServicesCodePoint([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::family_t family) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return awh::event::dscp_t();
}

/**
 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param id     идентификатор события
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param dscp   значение DSCP
 * @return       результат работы функции
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setDifferentiatedServicesCodePoint([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::family_t family, [[maybe_unused]] const event::dscp_t dscp) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
 *
 * @note Выдаёт значение, устанавливаемое на исходящих пакетах. Признак
 *       перегрузки принятых пакетов приходит отдельно для каждой
 *       датаграммы и извлекается методом getTrafficInfo
 *
 * @param id     идентификатор события
 * @param family семейство протоколов (IPv4 или IPv6)
 * @return       значение ECN
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
awh::event::ecn_t awh::engine::IO::getExplicitCongestionNotification([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::family_t family) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return awh::event::ecn_t();
}

/**
 * @brief Метод установки значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
 *
 * @note Класс обслуживания (DSCP) сохраняется: оба поля занимают один
 *       октет заголовка, поэтому установка затрагивает только младшие
 *       два бита
 *
 * @param id     идентификатор события
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param ecn    значение ECN
 * @return       результат работы функции
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setExplicitCongestionNotification([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::family_t family, [[maybe_unused]] const event::ecn_t ecn) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения обнаружения максимального размера пакета (MTU)
 *
 * @param id     идентификатор события
 * @param family семейство протоколов (IPv4 или IPv6)
 * @return       режим обнаружения максимального размера пакета (MTU)
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
awh::event::mtu_discover_t awh::engine::IO::getMaximumTransmissionUnitDiscover([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::family_t family) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return awh::event::mtu_discover_t();
}

/**
 * @brief Метод установки обнаружения максимального размера пакета (MTU)
 *
 * @param id     идентификатор события
 * @param family семейство протоколов (IPv4 или IPv6)
 * @param mode   режим обнаружения максимального размера пакета (MTU)
 * @return       результат работы функции
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setMaximumTransmissionUnitDiscover([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::family_t family, [[maybe_unused]] const event::mtu_discover_t mode) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод активации/деактивации мультикаст группы события
 *
 * @param id     идентификатор события
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::membership([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::mode_t mode, [[maybe_unused]] string_view group, [[maybe_unused]] string_view source, [[maybe_unused]] const uint16_t port) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод активации/деактивации мультикаст группы события
 *
 * @param id     идентификатор события
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::membership([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::mode_t mode, [[maybe_unused]] const net::addr_t * group, [[maybe_unused]] const net::addr_t * source, [[maybe_unused]] const uint16_t port) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод привязки дополнительного ключа маршрутизации к сессии
 *
 * @note Одна сессия адресуется произвольным числом ключей: протоколы,
 *       меняющие идентификатор по ходу работы, обращаются к ней по
 *       любому из привязанных. Ключи снимаются автоматически при
 *       уничтожении сессии
 *
 * @param id  идентификатор события сессии
 * @param key привязываемый ключ сессии
 * @return    результат привязки (false - ключ занят другой сессией)
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::bind([[maybe_unused]] const event::id_t id, [[maybe_unused]] const net::origin_key_t & key) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод снятия ключа маршрутизации с сессии
 *
 * @param id  идентификатор события сессии
 * @param key снимаемый ключ сессии
 * @return    результат снятия (false - ключ сессии не принадлежит)
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::unbind([[maybe_unused]] const event::id_t id, [[maybe_unused]] const net::origin_key_t & key) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения предельного количества одновременных подключений события
 *
 * @param id идентификатор события
 * @return   предельное количество одновременных подключений
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
uint32_t awh::engine::IO::getMaxConnections([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return uint32_t();
}

/**
 * @brief Метод установки предельного количества одновременных подключений события
 *
 * @note Для потоковых событий ограничивает число принятых подключений,
 *       для дейтаграммных - число сессий. Достижение предела означает
 *       отказ в создании новой сессии, поэтому предел служит защитой
 *       от исчерпания памяти потоком датаграмм от чужих отправителей
 *
 * @param id  идентификатор события
 * @param max предельное количество одновременных подключений
 * @return    результат установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setMaxConnections([[maybe_unused]] const event::id_t id, [[maybe_unused]] const uint32_t max) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод удаления события
 *
 * @details Освобождает событие из любого состояния: настраиваемого,
 *          работающего, приостановленного. Снимает таймауты, закрывает
 *          дескриптор и убирает событие из опроса.
 *
 *          Освобождение выполняется **не сразу**, а откладывается на
 *          два оборота цикла событий. Отсрочка нужна по необходимости,
 *          а не для удобства: записи подписки уходят в ядро вместе с
 *          ожиданием следующего оборота, и закрой движок дескриптор
 *          раньше, его номер операционная система успела бы выдать
 *          другому объекту - записи легли бы на чужой дескриптор.
 *
 *          Для вызывающего отсрочка незаметна: обращения по этому
 *          идентификатору перестают действовать сразу, а функции
 *          обратного вызова по нему больше не приходят.
 *
 * @note    Вызывать изнутри функции обратного вызова этого же события
 *          **допустимо и безопасно** - ровно из-за отсрочки. Это
 *          обычный способ закрыть соединение по ошибке разбора или по
 *          завершении обмена.
 *
 * @note    Повторный вызов по тому же идентификатору отказывает, а не
 *          освобождает узел дважды: событие уже помечено, и найти его
 *          по идентификатору больше нельзя.
 *
 * @note    Освобождение события сервера не освобождает принятые им
 *          подключения - у каждого свой идентификатор и свой срок
 *          жизни. Закрывать их следует своими вызовами
 *
 * @par Пример: закрытие соединения из обратного вызова
 * @code{.cpp}
 * io.on(client, static_cast <awh::engine::callback::read_t> ([&io](const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept -> void {
 *     // Если разбор принятых данных не удался, закрываем соединение
 *     if(!parse(buffer, size))
 *         // Освобождение отложится на два оборота цикла и выполнится безопасно
 *         io.destroy(id);
 * }));
 * @endcode
 *
 * @param id идентификатор события
 * @return   результат выполнения удаления
 *
 */
bool awh::engine::IO::destroy([[maybe_unused]] const event::id_t id) noexcept {
	// Функция обратного вызова на изменение состояния
	engine::callback::status_t callback = nullptr;
	{
		// Выполняем блокировку замка доступа к списку узлов
		const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
		// Выполняем поиск узла события
		auto i = ::__awh_nodes__.find(id);
		// Если узел не найден — уничтожать нечего
		if(i == ::__awh_nodes__.end())
			// Возвращаем отрицательный результат уничтожения
			return false;
		// Получаем функцию обратного вызова на изменение состояния
		callback = i->second->state;
		/**
		 * Останавливаем поток чтения и закрываем именованный канал
		 *
		 * @details Закрытие дескриптора обрывает ожидающий ReadFile отказом, и поток
		 *          чтения выходит сам. Признак завершения выставляется прежде закрытия,
		 *          чтобы поток не ушёл на новое чтение по уже закрытому дескриптору
		 *
		 * @note Поток отвязывается, а не дожидается: уничтожение события вправе
		 *       случиться прямо из обратного вызова, отданного этим же потоком, и
		 *       ожидание его свело бы поток на себя самого
		 *
		 */
		if(i->second->handle != INVALID_HANDLE_VALUE){
			// Выставляем признак завершения потока чтения
			i->second->stopped->store(true);
			// Закрываем дескриптор именованного канала
			::CloseHandle(i->second->handle);
			// Помечаем дескриптор канала недействительным
			i->second->handle = INVALID_HANDLE_VALUE;
		}
		// Если поток чтения заведён
		if(i->second->reader.joinable())
			// Отвязываем поток чтения
			i->second->reader.detach();
		// Удаляем узел из списка заведённых
		::__awh_nodes__.erase(i);
	}
	// Если функция обратного вызова на изменение состояния установлена
	if(callback != nullptr)
		// Извещаем о уничтожении события
		callback(id, event::status_t::DESTROYED);
	// Возвращаем положительный результат уничтожения
	return true;
}

/**
 * @brief Метод получения пары событий для сокета
 *
 * @param family   семейство адресов
 * @param type     тип сокета
 * @param protocol протокол сокета
 * @return         пара идентификаторов созданных событий
 *
 * @details За парой стоит настоящий именованный канал в строе сообщений: строй этот
 *          сохраняет границы записей и потому отвечает SOCK_SEQPACKET, на который
 *          опирается обмен между процессами кластера. Заводится здесь лишь сторона
 *          ожидания; встречный конец открывается лениво, при пуске события, и
 *          открыть его вправе как свой процесс, так и порождённый - по имени канала,
 *          какое отдаёт getTarget
 *
 * @note Открывать встречный конец сразу нельзя: пару эту мастер заводит для
 *       порождаемого процесса и свой встречный конец тут же уничтожает, а закрытие
 *       подключённого конца обрывает сторону ожидания - подключиться к ней работник
 *       уже не смог бы
 *
 * @note Семейства UDS у MS Windows в том виде, в каком оно есть у POSIX, не
 *       существует, но договор метода семейством этим и не связан: вызывающая
 *       сторона просит канал обмена между процессами, а чем он устроен -
 *       забота бэкенда. Поэтому UDS и PIPE обрабатываются здесь одинаково
 *
 */
std::array <awh::event::id_t, 2> awh::engine::IO::events(const event::family_t family, const event::type_t type, const event::protocol_t protocol) noexcept {
	// Результат работы функции
	std::array <awh::event::id_t, 2> result = {0, 0};
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Если движок не инициализирован — пару узлов завести нельзя
	if(!::__awh_initialized__){
		// Заносим в журнал предупреждение о неинициализированном движке
		this->_log->print("%s: engine is not initialized", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__);
		// Возвращаем пустую пару идентификаторов событий
		return result;
	}
	/**
	 * Временное ядро несёт лишь пару обмена сообщениями между процессами.
	 * Пары сетевых событий появятся вместе с портом завершения ввода-вывода
	 */
	if((family != event::family_t::PIPE) && (family != event::family_t::UDS)){
		// Заносим в журнал предупреждение о неподдерживаемом семействе событий
		this->_log->print("%s: event pair for family %u is not supported yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, static_cast <uint16_t> (family));
		// Возвращаем пустую пару идентификаторов событий
		return result;
	}
	/**
	 * Заводим оба узла пары
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Создаём новый узел события
		std::unique_ptr <::Node> item = std::make_unique <::Node> ();
		// Устанавливаем идентификатор события, пропуская нулевой
		item->id = ++::__awh_last_id__;
		// Устанавливаем вид узла
		item->node = event::node_t::IPC;
		// Устанавливаем семейство событий
		item->family = family;
		// Устанавливаем тип сокета
		item->type = type;
		// Устанавливаем протокол передачи данных
		item->protocol = protocol;
		// Запоминаем идентификатор заведённого события
		result[i] = item->id;
		// Добавляем узел в список заведённых
		::__awh_nodes__.emplace(item->id, ::std::move(item));
	}
	// Выполняем поиск первого узла пары
	::Node * first = ::__awh_find__(result[0]);
	// Выполняем поиск второго узла пары
	::Node * second = ::__awh_find__(result[1]);
	// Если оба узла пары завести не удалось
	if((first == nullptr) || (second == nullptr))
		// Возвращаем пару идентификаторов заведённых событий
		return result;
	// Связываем первый узел пары со вторым
	first->peer = second->id;
	// Связываем второй узел пары с первым
	second->peer = first->id;
	/**
	 * Заводим за парой настоящий именованный канал
	 *
	 * @details Строй сообщений выбран не случайно: он сохраняет границы сообщений и
	 *          потому отвечает SOCK_SEQPACKET, на который опирается обмен между
	 *          процессами кластера. Строй поточный границы теряет, и разметку
	 *          сообщений пришлось бы заводить самим
	 *
	 * @note Отказ заведения канала пару не отменяет: узлы остаются связанными по
	 *       идентификаторам, и обмен внутри одного процесса работает как прежде. Тем
	 *       сохраняется поведение для случаев, где канал не нужен вовсе
	 *
	 */
	// Составляем имя именованного канала по первому узлу пары
	const std::wstring & name = ::__awh_pipe_name__(first->id);
	// Заводим сторону канала, ожидающую подключения
	HANDLE server = ::CreateNamedPipeW(
		name.c_str(),
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		2, 65536, 65536, 0, nullptr
	);
	// Если сторону канала завести не удалось
	if(server == INVALID_HANDLE_VALUE){
		// Заносим в журнал предупреждение об отказе заведения канала
		this->_log->print("%s: named pipe \"%ls\" could not be created, error %lu", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, name.c_str(), ::GetLastError());
		// Возвращаем пару идентификаторов заведённых событий
		return result;
	}
	/**
	 * Встречный конец канала здесь не открывается
	 *
	 * @details Открыть его сразу нельзя: пару эту мастер заводит для порождаемого
	 *          процесса и свой встречный конец тут же уничтожает, а закрытие
	 *          подключённого конца обрывает сторону ожидания - подключиться к ней
	 *          работник уже не смог бы. Оттого конец этот и открывается лениво, при
	 *          пуске события, и открывает его тот, кто событием пользуется: свой
	 *          процесс либо порождённый, безразлично
	 *
	 */
	// Запоминаем дескриптор стороны канала у первого узла пары
	first->handle = server;
	// Помечаем первый узел пары стороной, ожидающей подключения
	first->listener = true;
	// Запоминаем имя канала у первого узла пары
	first->name = name;
	// Запоминаем имя канала у второго узла пары
	second->name = name;
	// Возвращаем пару идентификаторов заведённых событий
	return result;
}

/**
 * @brief Метод создания нового события
 *
 * @details Первый шаг работы с событием. Заводит узел события,
 *          создаёт под него дескриптор операционной системы и
 *          переводит событие в состояние `INITIAL`, в котором оно
 *          принимает настройки, но ещё не работает.
 *
 *          Дескриптор создаётся **сразу**, а не при запуске, потому
 *          что настройка события - адреса, порты, опции - выполняется
 *          над готовым дескриптором. Отсюда следует, что созданное и
 *          брошенное событие удерживает дескриптор до `destroy()`.
 *
 *          Тип узла определяет, чем событие будет: `CLIENT` и
 *          `SERVER` - сокеты, `PEER` заводится движком сам при приёме
 *          подключения, `TIMEOUT` и `INTERVAL` - таймеры, `FILE` и
 *          `DIR` - наблюдение за файловой системой, `IPC` -
 *          межпроцессное взаимодействие, `NOTIFY` - пользовательское
 *          событие для передачи работы в поток цикла.
 *
 * @note    Для таймеров семейство адресов задаётся значением
 *          `event::family_t::TIMER`, а тип и протокол не нужны вовсе.
 *
 * @note    Событие `PEER` через этот метод не создаётся: принятые
 *          подключения движок заводит сам и отдаёт их идентификатор
 *          в функцию обратного вызова приёма подключения уже готовым
 *          и подписанным на чтение.
 *
 * @note    Нулевой идентификатор означает отказ создания. Проверять
 *          его следует до настройки: обращения по недействительному
 *          идентификатору молча ничего не делают, и без проверки
 *          отказ обнаружился бы только отсутствием событий
 *
 * @par Пример: таймер
 * @code{.cpp}
 * // Заводим событие таймера и задаём ему задержку в две секунды
 * const awh::event::id_t timer = io.event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
 * io.setTimeout(timer, awh::event::action_t::NONE, 2000);
 * io.commit(timer);
 * io.launch(timer);
 * @endcode
 *
 * @param node     узел события
 * @param family   семейство адресов
 * @param type     тип сокета
 * @param protocol протокол сокета
 * @return         идентификатор созданного события, нулевой при отказе
 *
 */
awh::event::id_t awh::engine::IO::event([[maybe_unused]] const event::node_t node, [[maybe_unused]] const event::family_t family, [[maybe_unused]] const event::type_t type, [[maybe_unused]] const event::protocol_t protocol) noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Если движок не инициализирован — узел завести нельзя
	if(!::__awh_initialized__){
		// Заносим в журнал предупреждение о неинициализированном движке
		this->_log->print("%s: engine is not initialized", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__);
		// Возвращаем признак отсутствия заведённого узла
		return 0;
	}
	/**
	 * Временное ядро несёт узел пробуждения и узел обмена сообщениями между
	 * процессами. Прочие виды узлов появятся вместе с портом завершения
	 * ввода-вывода
	 */
	if((node != event::node_t::NOTIFY) && (node != event::node_t::IPC)){
		// Заносим в журнал предупреждение о неподдерживаемом виде узла
		this->_log->print("%s: node type %u is not supported yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, static_cast <uint16_t> (node));
		// Возвращаем признак отсутствия заведённого узла
		return 0;
	}
	// Создаём новый узел события
	std::unique_ptr <::Node> item = std::make_unique <::Node> ();
	// Устанавливаем идентификатор события, пропуская нулевой
	item->id = ++::__awh_last_id__;
	// Устанавливаем вид узла
	item->node = node;
	// Устанавливаем семейство событий
	item->family = family;
	// Устанавливаем тип сокета
	item->type = type;
	// Устанавливаем протокол передачи данных
	item->protocol = protocol;
	// Получаем идентификатор заведённого события
	const event::id_t result = item->id;
	// Добавляем узел в список заведённых
	::__awh_nodes__.emplace(result, ::std::move(item));
	// Возвращаем идентификатор заведённого события
	return result;
}

/**
 * @brief Метод получения смещения в файле события
 *
 * @param id   идентификатор события
 * @param seek тип смещения в файле события
 * @return     смещение в файле события
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
size_t awh::engine::IO::getSeek([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::seek_t seek) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return size_t();
}

/**
 * @brief Метод установки смещения в файле события
 *
 * @param id     идентификатор события
 * @param seek   тип смещения в файле события
 * @param offset смещение в файле события
 * @return       результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setSeek([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::seek_t seek, [[maybe_unused]] const size_t offset) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения опций события
 *
 * @param id идентификатор события
 * @return   опции события
 *
 */
uint16_t awh::engine::IO::getOptions([[maybe_unused]] const event::id_t id) const noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Возвращаем опции события, либо признак отсутствия
	return ((item != nullptr) ? item->options : static_cast <uint16_t> (event::options::NONE));
}

/**
 * @brief Метод установки опций события
 *
 * @param id      идентификатор события
 * @param options опции события для установки
 * @return        результат выполнения установки
 *
 */
bool awh::engine::IO::setOptions([[maybe_unused]] const event::id_t id, [[maybe_unused]] const uint16_t options) noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Если узел не найден — устанавливать опции некому
	if(item == nullptr)
		// Возвращаем отрицательный результат установки
		return false;
	// Устанавливаем опции события
	item->options = options;
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод установки опции события
 *
 * @param id     идентификатор события
 * @param option опция события для установки
 * @param mode   режим установки опции события
 * @return       результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setOption([[maybe_unused]] const event::id_t id, [[maybe_unused]] const uint16_t option, [[maybe_unused]] const bool mode) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод объединения данных между событиями
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат выполнения объединения
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::splice([[maybe_unused]] const event::id_t eid, [[maybe_unused]] const event::id_t dest) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод запуска события
 *
 * @details Последний шаг подготовки: с него событие начинает участвовать в
 *          опросе. До запуска событие заведено, настроено и, возможно,
 *          подключено, но обратные вызовы ему не приходят.
 *
 *          Метод различает **два пути** по состоянию события, и оба
 *          законны. Из состояния `INITIAL` запускается событие, которому
 *          подключаться не нужно: таймер, наблюдение за файлом,
 *          дейтаграммный сокет. Из состояния `SUCCESS` - событие, прошедшее
 *          через `connect()` или `listen()`; для него запуск заодно
 *          применяет накопившиеся изменения к ядру.
 *
 * @note Дейтаграммный сервер можно запускать и без `listen()`: слушать
 *       очередь входящих соединений ему незачем. Потоковому `listen()`
 *       обязателен, иначе запуск откажет
 *
 * @note Повторный запуск уже запущенного события отказывает: требуемых
 *       состояний у него больше нет
 *
 * @param id идентификатор события
 * @return   результат выполнения запуска
 *
 */
bool awh::engine::IO::launch([[maybe_unused]] const event::id_t id) noexcept {
	// Функция обратного вызова на изменение состояния
	engine::callback::status_t callback = nullptr;
	{
		// Выполняем блокировку замка доступа к списку узлов
		const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
		// Выполняем поиск узла события
		::Node * item = ::__awh_find__(id);
		// Если узел не найден — запускать нечего
		if(item == nullptr)
			// Возвращаем отрицательный результат запуска
			return false;
		/**
		 * Запуск требует, чтобы событие было прежде зафиксировано: состояния
		 * INITIAL либо SUCCESS
		 */
		if((item->status != event::status_t::INITIAL) && (item->status != event::status_t::SUCCESS))
			// Возвращаем отрицательный результат запуска
			return false;
		// Переводим узел в состояние выполнения
		item->status = event::status_t::LAUNCHED;
		// Получаем функцию обратного вызова на изменение состояния
		callback = item->state;
		/**
		 * Запускаем поток чтения, если за узлом стоит настоящий именованный канал
		 *
		 * @note Поток заводится именно здесь, а не при заведении узла: до запуска
		 *       события подписки на чтение ещё может не быть, и принятое сообщение
		 *       ушло бы в никуда
		 *
		 */
		if((item->handle == INVALID_HANDLE_VALUE) && !item->name.empty() && (item->node == event::node_t::IPC)){
			// Открываем свой конец именованного канала
			HANDLE handle = ::CreateFileW(item->name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
			// Если свой конец канала открыт
			if(handle != INVALID_HANDLE_VALUE){
				// Строй чтения своего конца канала
				DWORD mode = PIPE_READMODE_MESSAGE;
				// Переводим свой конец канала в строй сообщений
				::SetNamedPipeHandleState(handle, &mode, nullptr, nullptr);
				// Запоминаем дескриптор именованного канала у узла
				item->handle = handle;
			// Если открыть свой конец канала не удалось
			} else this->_log->print("%s: named pipe \"%ls\" could not be opened, error %lu", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, item->name.c_str(), ::GetLastError());
		}
		// Если за узлом стоит настоящий именованный канал, а поток чтения ещё не заведён
		if((item->handle != INVALID_HANDLE_VALUE) && !item->reader.joinable())
			// Запускаем поток чтения из именованного канала
			item->reader = std::thread(&::__awh_reader__, id);
	}
	// Если функция обратного вызова на изменение состояния установлена
	if(callback != nullptr)
		// Извещаем о запуске работы события
		callback(id, event::status_t::LAUNCHED);
	// Возвращаем положительный результат запуска
	return true;
}

/**
 * @brief Метод отключения события
 *
 * @details Разрывает соединение: закрывает дескриптор и переводит событие в
 *          состояние отмены, после чего вызывается подписка `event_t` с
 *          действием `DISCONNECT`. Само событие при этом **остаётся
 *          живым** - его идентификатор действителен, подписки сохранены.
 *          Этим отключение и отличается от `destroy()`, который событие
 *          уничтожает.
 *
 * @note Дескриптор закрыт, поэтому просто запустить событие снова нельзя:
 *       вернуть его в работу можно через `rebirth()`, пересоздающий
 *       дескриптор с сохранением самого события
 *
 * @note Отключение должно быть событию разрешено соответствующим действием.
 *       Если оно запрещено, метод молча ничего не делает и возвращает
 *       отрицательный результат
 *
 * @note Событие, уже помеченное к уничтожению или отключённое, повторно не
 *       отключается
 *
 * @param id идентификатор события
 * @return   результат выполнения отключения
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::disconnect([[maybe_unused]] const event::id_t id) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод мультиподключения события к удалённым хостам
 *
 * @details Начинает подключение и **возвращается сразу**, не дожидаясь его
 *          исхода: соединение на неблокирующем сокете устанавливается за
 *          несколько оборотов цикла. Положительный результат означает лишь
 *          то, что попытка начата успешно. Об исходе сообщает подписка
 *          `connect_t`, и до её вызова отправлять данные некуда.
 *
 *          Список идентификаторов позволяет начать несколько подключений
 *          одним вызовом - они пойдут одновременно, а не по очереди, и
 *          каждое сообщит о себе своим вызовом `connect_t`.
 *
 * @note Ставится **между** `commit()` и `launch()`: до фиксации адрес ещё не
 *       применён, а запуск ожидает событие уже подключающимся
 *
 * @note Предел времени на установление соединения задаётся через
 *       `setTimeout()` с действием `CONNECT`. Без него неудачная попытка
 *       может висеть столько, сколько отведёт система
 *
 * @par Пример: клиент
 * @code{.cpp}
 * io.on(client, static_cast <awh::engine::callback::connect_t> ([&io](const awh::event::id_t id, const bool ok) noexcept -> void {
 *     // Отправлять можно только отсюда: раньше соединения ещё нет
 *     if(ok)
 *         io.send(id, request.data(), request.size());
 * }));
 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
 * if(io.commit(client) && io.connect(client) && io.launch(client))
 *     while(io.poll(100));
 * @endcode
 *
 * @param ids список идентификаторов событий для подключения
 * @return    результат выполнения подключения
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::connect(const vector <event::id_t> & ids) noexcept {
	// Результат выполнения подключения
	bool result = false;
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	/**
	 * Выполняем перебор всех переданных событий
	 */
	for(const auto & id : ids){
		// Выполняем поиск узла события
		::Node * item = ::__awh_find__(id);
		// Если узел не найден - подключать нечего
		if(item == nullptr)
			// Переходим к следующему событию
			continue;
		/**
		 * Временное ядро подключает лишь узлы обмена сообщениями: подключение
		 * сокетов появится вместе с портом завершения ввода-вывода
		 */
		if(item->node != event::node_t::IPC){
			// Заносим в журнал предупреждение об отсутствии реализации
			this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
			// Переходим к следующему событию
			continue;
		}
		// Если имя именованного канала узлу не задано
		if(item->name.empty()){
			// Заносим в журнал предупреждение об отсутствии имени канала
			this->_log->print("%s: named pipe for event %llu is not set", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, static_cast <uint64_t> (id));
			// Переходим к следующему событию
			continue;
		}
		// Если дескриптор канала у узла уже есть - подключать его повторно незачем
		if(item->handle != INVALID_HANDLE_VALUE){
			// Запоминаем положительный результат подключения
			result = true;
			// Переходим к следующему событию
			continue;
		}
		/**
		 * Открываем свой конец именованного канала
		 *
		 * @details Ожидания занятого канала здесь нет намеренно: заводит его мастер до
		 *          порождения процесса, и к мигу подключения канал уже стоит. Отказ
		 *          потому означает настоящую беду, а не гонку, и скрывать его ожиданием
		 *          значило бы прятать её
		 *
		 */
		HANDLE handle = ::CreateFileW(item->name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
		// Если открыть свой конец канала не удалось
		if(handle == INVALID_HANDLE_VALUE){
			// Заносим в журнал предупреждение об отказе подключения
			this->_log->print("%s: named pipe \"%ls\" could not be opened, error %lu", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, item->name.c_str(), ::GetLastError());
			// Переходим к следующему событию
			continue;
		}
		// Переводим свой конец канала в строй сообщений
		DWORD mode = PIPE_READMODE_MESSAGE;
		// Устанавливаем строй чтения своего конца канала
		::SetNamedPipeHandleState(handle, &mode, nullptr, nullptr);
		// Запоминаем дескриптор именованного канала у узла
		item->handle = handle;
		// Переводим узел в состояние состоявшегося подключения
		item->status = event::status_t::SUCCESS;
		// Запоминаем положительный результат подключения
		result = true;
	}
	// Возвращаем результат выполнения подключения
	return result;
}

/**
 * @brief Метод перевода события в режим прослушивания входящих соединений
 *
 * @details Открывает очередь входящих соединений: с этого момента ядро
 *          принимает их и складывает в очередь, а разбирать её событие
 *          начнёт с вызова `launch()`. Второй параметр задаёт предел
 *          одновременно ожидающих соединений - тот самый backlog.
 *
 * @note Требуется **только потоковым** серверам. Дейтаграммному серверу
 *       очередь соединений не нужна, и он обходится одним `launch()`
 *
 * @note Принятые соединения приходят подпиской `accept_t` уже заведёнными
 *       событиями, и заводить их своими вызовами не требуется
 *
 * @par Пример: потоковый сервер
 * @code{.cpp}
 * const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
 * io.setSourcePort(server, 8080);
 * io.setAddress(server, awh::event::address_t::IPV4, "0.0.0.0");
 * io.on(server, static_cast <awh::engine::callback::accept_t> (onAccept));
 * // Фиксация, очередь входящих, запуск - именно в этом порядке
 * if(io.commit(server) && io.listen(server, 1024) && io.launch(server))
 *     while(io.poll(100));
 * @endcode
 *
 * @param id  идентификатор события
 * @param max максимальное количество входящих соединений
 * @return    результат выполнения перевода в режим прослушивания
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::listen([[maybe_unused]] const event::id_t id, [[maybe_unused]] const uint32_t max) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения данных события
 *
 * @param id идентификатор события
 * @return   результат получения данных
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::recv([[maybe_unused]] const event::id_t id) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод отправки данных события
 *
 * @param id     идентификатор события
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт данных, отправленных событием
 *
 * @note Временное ядро принимает сообщение целиком либо не принимает вовсе:
 *       очередь его не ограничена, и частичной отправки, какая бывает у
 *       сокета с заполненным буфером, здесь не случается
 *
 */
size_t awh::engine::IO::send(const event::id_t id, const void * buffer, const size_t size) noexcept {
	// Если данные для отправки не переданы
	if((buffer == nullptr) || (size == 0))
		// Возвращаем количество отправленных байт
		return 0;
	// Количество отправленных байт
	size_t written = 0;
	// Код отказа отправки, нулевой при её успехе
	DWORD code = 0;
	// Функция обратного вызова на запись сообщений
	engine::callback::write_t callback = nullptr;
	// Функция обратного вызова на получение ошибок
	engine::callback::error_t failure = nullptr;
	{
		// Выполняем блокировку замка доступа к списку узлов
		const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
		// Выполняем поиск узла события
		::Node * item = ::__awh_find__(id);
		// Если узел не найден — отправлять некуда
		if(item == nullptr)
			// Возвращаем количество отправленных байт
			return 0;
		// Получаем функцию обратного вызова на запись сообщений
		callback = item->write;
		// Получаем функцию обратного вызова на получение ошибок
		failure = item->error;
		/**
		 * Отправляем сообщение в именованный канал, если тот за узлом стоит
		 *
		 * @details Путь этот и есть настоящий обмен между процессами: запись уходит в
		 *          канал, а принимает её поток чтения встречной стороны - в своём
		 *          процессе либо в чужом, безразлично. Строй сообщений сохраняет
		 *          границы, оттого одна запись даёт ровно одно чтение
		 *
		 * @note Очередь узла-собеседника ниже остаётся для узла пробуждения и для пар,
		 *       заведённых без канала: там обмен ведётся внутри одного процесса
		 *
		 */
		if((item->node == event::node_t::IPC) && (item->handle != INVALID_HANDLE_VALUE)){
			// Количество отправленных байт, снятое системой
			DWORD sent = 0;
			/**
			 * Отправка ведётся наложенной
			 *
			 * @details Дескриптор канала открыт с FILE_FLAG_OVERLAPPED, и иначе быть не
			 *          может: у дескриптора без наложения система выстраивает операции в
			 *          очередь, и запись дожидалась бы ожидающего чтения, какое поток
			 *          чтения держит всё время. Обнаружено опытом - обмен вставал намертво
			 *
			 */
			// Событие завершения наложенной операции отправки
			HANDLE signal = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
			// Описание наложенной операции отправки
			OVERLAPPED overlapped{};
			// Привязываем событие завершения к наложенной операции
			overlapped.hEvent = signal;
			// Выполняем наложенную отправку сообщения в именованный канал
			BOOL result = ::WriteFile(item->handle, buffer, static_cast <DWORD> (size), &sent, &overlapped);
			// Если отправка принята системой к исполнению - дожидаемся её завершения
			if(!result && (::GetLastError() == ERROR_IO_PENDING))
				// Дожидаемся завершения наложенной отправки
				result = ::GetOverlappedResult(item->handle, &overlapped, &sent, TRUE);
			// Если отправка состоялась
			if(result)
				// Запоминаем количество отправленных байт
				written = static_cast <size_t> (sent);
			// Если отправка завершилась отказом - запоминаем его код
			else code = ::GetLastError();
			// Если событие завершения наложенной операции заведено
			if(signal != nullptr)
				// Закрываем событие завершения наложенной операции
				::CloseHandle(signal);
		/**
		 * Если настоящего канала за узлом нет - сообщение ложится в очередь собеседника
		 */
		} else {
			// Узел, которому предназначено сообщение
			::Node * target = nullptr;
			/**
			 * Определяем вид узла-отправителя
			 */
			switch(static_cast <uint8_t> (item->node)){
				/**
				 * Узел пробуждения принимает сообщение к себе же: отправитель кладёт байты
				 * в очередь узла, а петля отдаёт их в обратный вызов чтения. Так устроено
				 * извещение о завершившихся процессах у модуля кластера
				 */
				case static_cast <uint8_t> (event::node_t::NOTIFY):
					// Сообщение предназначено самому узлу
					target = item;
				break;
				/**
				 * Узел обмена сообщениями отправляет собеседнику: сообщение ложится в
				 * очередь второго узла пары, откуда петля отдаёт его в обратный вызов
				 * чтения. Границы сообщения при этом сохраняются
				 */
				case static_cast <uint8_t> (event::node_t::IPC):
					// Сообщение предназначено собеседнику по паре
					target = ::__awh_find__(item->peer);
				break;
			}
			// Если узел-получатель определить не удалось
			if(target == nullptr){
				// Заносим в журнал предупреждение о неподдерживаемом виде узла
				this->_log->print("%s: sending to node type %u is not supported yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, static_cast <uint16_t> (item->node));
				// Возвращаем количество отправленных байт
				return 0;
			}
			// Отдаём сообщение порту завершения от имени узла-получателя
			::__awh_post__(target->id, reinterpret_cast <const uint8_t *> (buffer), size);
			// Запоминаем количество отправленных байт
			written = size;
		}
	}
	/**
	 * Извещения отдаются уже без замка: обратный вызов вправе завести или
	 * уничтожить событие, и удержание замка привело бы к его повторному захвату
	 */
	// Если отправка завершилась отказом
	if(code != 0){
		// Заносим в журнал предупреждение об отказе отправки
		this->_log->print("%s: message could not be sent, error %lu", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, code);
		// Если функция обратного вызова на получение ошибок установлена
		if(failure != nullptr)
			// Извещаем об отказе отправки
			failure(id, event::error_t::EVENT_FAIL, this->_fmk->format("named pipe write failed, error %lu", code));
		// Возвращаем количество отправленных байт
		return 0;
	}
	// Если функция обратного вызова на запись сообщений установлена
	if(callback != nullptr)
		// Извещаем отправителя об отданном сообщении
		callback(id, written);
	// Пробуждаем петлю событий, чтобы та разобрала накопленное
	::__awh_wake__();
	// Возвращаем количество отправленных байт
	return written;
}

/**
 * @brief Метод перенаправления объединённых данных в событие-приёмник (splice)
 *
 * @note Если на событии-приёмнике установлена функция инъекции (транспорт
 *       шифрует данные на уровне соединения, напр. QUIC), данные передаются
 *       ей для отправки собственным потоком; иначе выполняется обычная
 *       отправка байт в сокет
 *
 * @note Дейтаграммный приёмник отправляет каждую запись очереди
 *       отдельным сообщением, и запись, превышающую предел системы,
 *       отправить нельзя вовсе. Такие данные переносятся частями:
 *       порция источника устроена иначе, чем сообщение приёмника -
 *       файл, например, читается страницами, которые предельную
 *       дейтаграмму превышают. Делится только то, что иначе не
 *       прошло бы ни одним октетом, поэтому границы сообщений у
 *       проходящих целиком дейтаграмм сохраняются
 *
 * @param id     идентификатор события-приёмника
 * @param buffer буфер перенаправляемых данных
 * @param size   размер перенаправляемых данных
 * @return       количество принятых на перенаправление байт
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
size_t awh::engine::IO::relay([[maybe_unused]] const event::id_t id, [[maybe_unused]] const void * buffer, [[maybe_unused]] const size_t size) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return size_t();
}

/**
 * @brief Метод установки глубины очереди принятия входящих соединений события
 *
 * @param id       идентификатор события
 * @param depth    глубина очереди принятия входящих соединений
 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
void awh::engine::IO::backlog([[maybe_unused]] const event::id_t id, [[maybe_unused]] const uint16_t depth, [[maybe_unused]] const bool adaptive) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
}

/**
 * @brief Метод получения размера буфера события
 *
 * @param id     идентификатор события
 * @param action тип действия события
 * @return       размер буфера события
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
size_t awh::engine::IO::getBufferSize([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::action_t action) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return size_t();
}

/**
 * @brief Метод установки размера буфера события
 *
 * @param id     идентификатор события
 * @param action тип действия события
 * @param size   размер буфера события
 * @return       результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setBufferSize([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::action_t action, [[maybe_unused]] const size_t size) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод установки пропускной способности события
 *
 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
void awh::engine::IO::bandwidth([[maybe_unused]] const event::limiting_t limiting, [[maybe_unused]] string_view bandwidth) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
}

/**
 * @brief Метод установки пропускной способности события для события
 *
 * @param id        идентификатор события
 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
 * @return          результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::bandwidth([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::limiting_t limiting, [[maybe_unused]] string_view bandwidth) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения режима трансляции пакетов для события
 *
 * @param id идентификатор события
 * @return   режим трансляции пакетов (unicast, multicast, broadcast)
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
awh::event::delivery_mode_t awh::engine::IO::getDelivery([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return awh::event::delivery_mode_t();
}

/**
 * @brief Метод установки режима трансляции пакетов для события
 *
 * @param id       идентификатор события
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @return         результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setDelivery([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::delivery_mode_t delivery) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
 *
 * @param id идентификатор события
 * @return   метаданные последнего принятого дейтаграммного пакета
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
awh::net::dgram_info_t awh::engine::IO::getTrafficInfo([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return awh::net::dgram_info_t();
}

/**
 * @brief Метод получения количества хопов последнего принятого пакета
 *
 * @param id идентификатор события
 * @return   количество хопов последнего принятого пакета
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
uint8_t awh::engine::IO::getCountHops([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return uint8_t();
}

/**
 * @brief Метод установки количества хопов последнего принятого пакета
 *
 * @param id   идентификатор события
 * @param hops количество хопов последнего принятого пакета
 * @return     результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setCountHops([[maybe_unused]] const event::id_t id, [[maybe_unused]] const uint8_t hops) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
 *
 * @param id идентификатор события
 * @return   максимальное количество хопов
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
awh::event::hops_t awh::engine::IO::getHops([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return awh::event::hops_t();
}

/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param id   идентификатор события
 * @param hops максимальное количество хопов
 * @return     результат работы функции
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setHops([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::hops_t hops) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения режима использования таймаута для обработки события чтения
 *
 * @param id идентификатор события
 * @return   режим использования таймаута для обработки события чтения
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
awh::event::usage_t awh::engine::IO::getUsageReadTimeout([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return awh::event::usage_t();
}

/**
 * @brief Метод установки режима использования таймаута для обработки события чтения
 *
 * @details Определяет, что делать со сроком чтения, заданным через
 *          `setTimeout()` с действием `READ`, после того как данные пришли.
 *          Два режима отвечают двум разным по смыслу задачам.
 *
 *          `REUSABLE` - срок взводится заново после каждого чтения. Это
 *          постоянный страж простоя: соединение обязано подавать признаки
 *          жизни не реже заданного срока, иначе срабатывает таймаут.
 *          Подходит потокам данных и долгоживущим подпискам.
 *
 *          `DISPOSABLE` - срок снимается, как только данные пришли, и
 *          взводится заново при успешной отправке. То есть это не предел
 *          простоя, а **ожидание ответа**: отправили запрос - пошёл отсчёт,
 *          получили ответ - отсчёт снят. Подходит обмену «запрос-ответ», где
 *          молчание в паузе между запросами законно.
 *
 * @note Режимом по умолчанию является `DISPOSABLE`. Ожидающим постоянной
 *       активности соединениям режим следует менять явно, иначе простой
 *       между запросами замечен не будет
 *
 * @note Действует только на событиях с неблокирующим или частично
 *       блокирующим вводом-выводом: на блокирующих сроки держит сам сокет
 *
 * @param id    идентификатор события
 * @param usage режим использования таймаута для обработки события чтения (reusable или disposable)
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
void awh::engine::IO::setUsageReadTimeout([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::usage_t usage) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
}

/**
 * @brief Метод получения таймаута события
 *
 * @details Возвращает **заданный** срок, а не остаток до срабатывания:
 *          сколько времени таймеру осталось, отсюда узнать нельзя. Нулевое
 *          значение означает, что срок не выставлен или снят.
 *
 * @param id     идентификатор события
 * @param action тип действия события
 * @return       значение таймаута в миллисекундах
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
uint32_t awh::engine::IO::getTimeout([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::action_t action) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return uint32_t();
}

/**
 * @brief Метод установки таймаута события
 *
 * @details Один метод обслуживает два разных по смыслу случая, и различает
 *          их по типу узла.
 *
 *          **Узлы-таймеры** (`TIMEOUT`, `INTERVAL`) - здесь значение задаёт
 *          саму задержку срабатывания, а действие не участвует и передаётся
 *          как `action_t::NONE`. Разница между двумя типами узлов лишь в
 *          том, срабатывает таймер однажды или повторяется. О срабатывании
 *          сообщает подписка на `status_t` со статусом `SUCCESS` -
 *          отдельной функции у таймеров нет.
 *
 *          **Узлы соединений** (`CLIENT`, `PEER`, `ORIGIN`, `MEDIATOR`) -
 *          здесь значение задаёт предел простоя, а действие говорит, простой
 *          в чём считать. По истечении срока вызывается подписка на
 *          `timeout_t`, и если её нет, соединение уничтожается безусловно.
 *
 * @note Нулевое значение **снимает** срок: заведённый таймер разоружается,
 *       а событие возвращается в исходное состояние. Это и есть способ
 *       отменить ранее выставленный таймаут - отдельного метода для отмены
 *       нет
 *
 * @note Выставлять можно в любой момент, в том числе уже работающему
 *       событию: живой таймер перевзводится тут же, с новым сроком
 *
 * @note На **блокирующих** событиях сроки чтения и записи ставятся опциями
 *       сокета, а не таймерами движка. Наблюдаемое поведение то же, но
 *       подписка `timeout_t` в этом случае не работает - ждёт сам системный
 *       вызов
 *
 * @par Допустимые действия
 * | Действие | Для кого | Смысл |
 * |---|---|---|
 * | `NONE` | таймеры | задержка срабатывания |
 * | `READ` | соединения | сколько ждать входящих данных |
 * | `WRITE` | соединения | сколько ждать возможности отправить |
 * | `CONNECT` | только `CLIENT` | сколько ждать установления соединения |
 * | `RECONNECT` | только `CLIENT` | пауза перед повторной попыткой |
 *
 * Действие, узлу не подходящее, срок не выставляет: в лог уходит
 * предупреждение, а подписка на `status_t` получает статус `FAILURE`.
 *
 * @par Пример: таймер и предел простоя
 * @code{.cpp}
 * // Интервал, срабатывающий каждые пять секунд
 * const awh::event::id_t timer = io.event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
 * io.setTimeout(timer, awh::event::action_t::NONE, 5000);
 * // Клиенту - пять секунд на подключение и тридцать на молчание
 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
 * io.setTimeout(client, awh::event::action_t::READ, 30000);
 * // Передумали: снимаем предел простоя, оставив предел подключения
 * io.setTimeout(client, awh::event::action_t::READ, 0);
 * @endcode
 *
 * @param id      идентификатор события
 * @param action  тип действия события
 * @param timeout значение таймаута в миллисекундах
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
void awh::engine::IO::setTimeout([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::action_t action, [[maybe_unused]] const uint32_t timeout) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
}

/**
 * @brief Метод продолжения прерванного ожидания
 *
 * @details Одноразовый срок ожидания чтения (`usage_t::DISPOSABLE`) снимается
 *          приходом данных, и снимается **до** вызова отклика. Движку этого
 *          довольно: данные пришли, ожидание кончилось. Договору - не всегда:
 *          на дейтаграммном обмене прийти вправе что угодно и от кого угодно,
 *          и пришедшее бывает не ответом на заданный вопрос, а чужим ответом,
 *          ответом запоздалым либо шумом сети. Разобрать это способен лишь
 *          сам договор, и лишь он вправе решить, что ожидание не кончилось
 *
 *          Решив так, договор зовёт этот метод, и ожидание продолжается.
 *          Без него вопрос повисает навсегда: срок снят, взводить его заново
 *          нечем, а другого срабатывания не будет
 *
 *          Задержка нулевая означает продолжение **с того места, где ожидание
 *          прервано**: движок помнит остаток снятого срока и взводит ожидание
 *          ровно на него. Тем и отличается продолжение от нового ожидания:
 *          чужой ответ не дарит вопросу лишнего времени, и сколько бы их ни
 *          пришло, отказ наступит в свой черёд
 *
 *          Задержка ненулевая задаёт ожидание заново, на указанный срок.
 *          Нужна там, где договор знает больше движка: ответ пришёл частью,
 *          и остаток разумно ждать иначе, чем ждали целое
 *
 * @note Продолжать нечего, если ожидание не прерывалось, прервано у события
 *       иного, у срока иного либо успело истечь. Во всех этих случаях метод
 *       отвечает отказом, ничего не взводя
 *
 * @warning Продолжение нулевой задержкой осмысленно **только внутри отклика**,
 *          вызванного тем самым чтением, что прервало ожидание. Движок помнит
 *          один прерванный срок, а не все: обращение позднее застанет запись
 *          уже чужой и получит отказ. Обходить это, запоминая остаток у себя,
 *          не следует - лучше позвать метод там, где решение и принимается
 *
 * @par Пример: чужой ответ ожидания не прерывает
 * @code{.cpp}
 * // Разбираем пришедший ответ
 * if(header.id != this->_awaiting){
 *     // Ответ не на наш вопрос - продолжаем ожидание с прерванного места
 *     io.rearmTimeout(id, awh::event::action_t::READ);
 *     // Ответ чужой, разбирать его нечего
 *     return;
 * }
 * @endcode
 *
 * @param id     идентификатор события
 * @param action тип действия события
 * @param delay  задержка в миллисекундах, либо ноль для продолжения с остатка
 * @return       результат продолжения ожидания
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::rearmTimeout([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::action_t action, [[maybe_unused]] const uint32_t delay) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения действия события
 *
 * @param id     идентификатор события
 * @param action тип действия события
 * @return       режим действия события
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
awh::event::mode_t awh::engine::IO::getAction([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::action_t action) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return awh::event::mode_t();
}

/**
 * @brief Метод установки действия события
 *
 * @param id     идентификатор события
 * @param action тип действия события
 * @param mode   режим установки действия события
 * @return       результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::setAction([[maybe_unused]] const event::id_t id, [[maybe_unused]] const event::action_t action, [[maybe_unused]] const event::mode_t mode) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод установки параметров keep-alive для события
 *
 * @param id    идентификатор события
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::keepAlive([[maybe_unused]] const event::id_t id, [[maybe_unused]] const int32_t cnt, [[maybe_unused]] const int32_t idle, [[maybe_unused]] const int32_t intvl) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод приостановки события
 *
 * @details Отключает чтение, не разрывая соединения: данные остаются в
 *          приёмном буфере ядра, отправитель упирается в исчерпание окна и
 *          сам сбавляет темп. Это штатный способ придержать поток, когда
 *          принимающая сторона не успевает разбирать принятое, - в отличие
 *          от `disconnect()`, соединение при этом цело.
 *
 * @note Снятие чтения выполняется **немедленно**, не дожидаясь очередного
 *       оборота цикла: иначе успела бы прийти ещё порция данных. А вот
 *       возобновление откладывается до следующего оборота, и это
 *       расхождение намеренное
 *
 * @note Приостановить можно только запущенное событие, а возобновить -
 *       только приостановленное. Повторные вызовы отказывают
 *
 * @param id идентификатор события
 * @return   результат выполнения приостановки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::pause([[maybe_unused]] const event::id_t id) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод возобновления события
 *
 * @details Возвращает чтение приостановленному событию. Накопившееся в
 *          приёмном буфере ядра придёт обычными подписками на чтение,
 *          начиная с очередного оборота цикла/
 *
 * @param id идентификатор события
 * @return   результат выполнения возобновления
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::resume([[maybe_unused]] const event::id_t id) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод проверки состояния события
 *
 * @param id идентификатор события
 * @return   состояние события
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::isAlive([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод очистки сетевого движка
 *
 */
void awh::engine::IO::clear() noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	/**
	 * Перебираем все заведённые узлы, извещая о их уничтожении
	 */
	for(auto & item : ::__awh_nodes__){
		// Если функция обратного вызова на изменение состояния установлена
		if(item.second->state != nullptr)
			// Извещаем о уничтожении узла события
			item.second->state(item.first, event::status_t::DESTROYED);
	}
	// Очищаем список заведённых узлов
	::__awh_nodes__.clear();
}

/**
 * @brief Метод принудительного пинка базе событий
 *
 * @return результат выполнения операции
 *
 */
bool awh::engine::IO::kick() noexcept {
	// Пробуждаем ожидающую петлю событий
	::__awh_wake__();
	// Сообщаем об успешном пробуждении
	return true;
}

/**
 * @brief Метод инициализации сетевого движка
 *
 * @return результат выполнения инициализации
 *
 */
bool awh::engine::IO::initialize() noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Если движок уже инициализирован — повторная инициализация ничего не меняет
	if(::__awh_initialized__)
		// Сообщаем, что инициализация уже выполнена
		return true;
	/**
	 * Запускаем библиотеку сокетов Winsock
	 *
	 * @details Средства сокетов у MS Windows до запуска этого недоступны вовсе: всякий
	 *          вызов их отвечает отказом WSANOTINITIALISED. У систем POSIX
	 *          соответствия этому нет - там сокеты доступны всегда
	 *
	 * @note Запуск ведётся счётчиком: WSAStartup и WSACleanup вправе звать сколько
	 *       угодно раз, лишь бы поровну. Здесь пара эта привязана к жизни движка
	 *
	 */
	// Сведения о запущенной библиотеке сокетов
	WSADATA wsa{};
	// Выполняем запуск библиотеки сокетов
	const int32_t started = ::WSAStartup(MAKEWORD(2, 2), &wsa);
	// Если запустить библиотеку сокетов не удалось
	if(started != 0){
		// Заносим в журнал ошибку запуска библиотеки сокетов
		this->_log->print("%s: Winsock could not be started, error %d", log_t::flag_t::CRITICAL, ::__AWH_IO_BACKEND__, started);
		// Сообщаем об отказе инициализации
		return false;
	}
	{
		// Выполняем блокировку замка ожидания
		const std::lock_guard <std::mutex> lock(::__awh_wait_mutex__);
		/**
		 * Заводим порт завершения ввода-вывода
		 *
		 * @note Число потоков обслуживания задаётся единицей намеренно: договор
		 *       движка велит опрашивать его из одного и того же потока, и
		 *       позволять системе будить несколько разом значило бы договор этот
		 *       нарушить
		 *
		 */
		::__awh_port__ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
		// Если завести порт завершения не удалось
		if(::__awh_port__ == nullptr){
			// Заносим в журнал ошибку заведения порта завершения
			this->_log->print("%s: completion port could not be created, error %lu", log_t::flag_t::CRITICAL, ::__AWH_IO_BACKEND__, ::GetLastError());
			// Останавливаем библиотеку сокетов
			::WSACleanup();
			// Сообщаем об отказе инициализации
			return false;
		}
	}
	// Выставляем признак инициализации движка
	::__awh_initialized__ = true;
	// Сообщаем об успешной инициализации
	return true;
}

/**
 * @brief Метод реинициализации сетевого движка
 *
 * @return результат выполнения реинициализации
 *
 */
bool awh::engine::IO::reinitialize() noexcept {
	/**
	 * Переинициализация нужна дочернему процессу: узлы, унаследованные от родителя,
	 * ему не принадлежат, и работать с ними он не вправе. Счётчик идентификаторов
	 * при этом не сбрасывается — идентификаторы остаются несовпадающими между
	 * процессами, что облегчает чтение журналов
	 */
	{
		// Выполняем блокировку замка доступа к списку узлов
		const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
		// Очищаем список заведённых узлов без извещения о состоянии
		::__awh_nodes__.clear();
		// Выставляем признак инициализации движка
		::__awh_initialized__ = true;
	}
	{
		// Выполняем блокировку замка ожидания
		const std::lock_guard <std::mutex> lock(::__awh_wait_mutex__);
		/**
		 * Порт родителя дочернему процессу не принадлежит
		 *
		 * @note Порождённый процесс проходит main заново и описателя по наследству
		 *       не получает: порт, оставшийся в переменной, указывает в пустоту, и
		 *       заводить его следует наново
		 *
		 */
		if(::__awh_port__ != nullptr)
			// Выполняем закрытие прежнего порта завершения
			::CloseHandle(::__awh_port__);
		// Заводим порт завершения ввода-вывода
		::__awh_port__ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
	}
	// Сообщаем об успешной переинициализации
	return true;
}

/**
 * @brief Метод деинициализации сетевого движка
 *
 * @return результат выполнения деинициализации
 *
 */
bool awh::engine::IO::deinitialize() noexcept {
	// Выполняем уничтожение всех заведённых узлов событий
	this->clear();
	{
		// Выполняем блокировку замка доступа к списку узлов
		const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
		// Если движок был инициализирован
		if(::__awh_initialized__)
			/**
			 * Останавливаем библиотеку сокетов
			 *
			 * @note Останов ведётся парно к запуску: сколько было WSAStartup, столько
			 *       должно быть и WSACleanup, иначе сокеты остаются доступными и после
			 *       освобождения движка
			 *
			 */
			::WSACleanup();
		// Сбрасываем признак инициализации движка
		::__awh_initialized__ = false;
	}
	// Пробуждаем петлю событий, чтобы та вышла из ожидания
	::__awh_wake__();
	{
		// Выполняем блокировку замка ожидания
		const std::lock_guard <std::mutex> lock(::__awh_wait_mutex__);
		// Если порт завершения заведён
		if(::__awh_port__ != nullptr){
			// Выполняем закрытие порта завершения
			::CloseHandle(::__awh_port__);
			// Сбрасываем описатель порта завершения
			::__awh_port__ = nullptr;
		}
	}
	// Сообщаем об успешном освобождении
	return true;
}

/**
 * @brief Метод проверки состояния инициализации сетевого движка
 *
 * @return состояние инициализации
 *
 */
bool awh::engine::IO::isInitialized() const noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Возвращаем признак инициализации движка
	return ::__awh_initialized__;
}

/**
 * @brief Метод получения количества событий в сетевом движке
 *
 * @return количество событий
 *
 */
size_t awh::engine::IO::eventsCount() const noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Возвращаем количество заведённых узлов событий
	return ::__awh_nodes__.size();
}

/**
 * @brief Метод получения типа внутренних таймеров
 *
 * @return тип таймера для событий сетевого движка
 *
 */
awh::event::timer_t awh::engine::IO::getInternalTimer() const noexcept {
	// Возвращаем вид внутреннего таймера петли событий
	return ::__awh_timer__;
}

/**
 * @brief Метод установки типа внутренних таймеров
 *
 * @details Выбирает структуру, в которой движок держит сроки событий.
 *          `SIMPLE` - упорядоченное множество с хэш-таблицей положений:
 *          скромна по памяти, но каждая постановка срока выделяет по узлу в
 *          обеих. `DIFFICULT` - двоичная куча со страничной таблицей слотов:
 *          к аллокатору не обращается вовсе и на постановке быстрее в разы,
 *          зато таблица слотов выделяется чанками по тысяче событий.
 *
 *          Умолчанием служит `SIMPLE` - как и прочие умолчания, оно
 *          рассчитано на самую слабую машину и самый общий случай.
 *          Приложению, которое держит много сроков и ставит их часто,
 *          переключение выгодно, и выигрыш измеряется разами.
 *
 * @note Выбор общий для всего движка, а не для отдельного события, и менять
 *       его следует **до** заведения событий: переключение сбрасывает уже
 *       заведённые таймеры
 *
 * @par Пример: включить структуру для большого числа сроков
 * @code{.cpp}
 * awh::engine::io_t io(&fmk, &log);
 * // Переключаем до заведения событий и до initialize()
 * io.setInternalTimer(awh::event::timer_t::DIFFICULT);
 * io.initialize();
 * @endcode
 *
 * @param timer тип таймера для событий сетевого движка
 *
 */
void awh::engine::IO::setInternalTimer([[maybe_unused]] const event::timer_t timer) noexcept {
	// Устанавливаем вид внутреннего таймера петли событий
	::__awh_timer__ = timer;
}

/**
 * @brief Метод получения размера отслеживаемого файла
 *
 * @param id идентификатор события
 * @return   размер файла
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
size_t awh::engine::IO::size([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return size_t();
}

/**
 * @brief Метод получения количества байт, доступных для записи в очередь события
 *
 * @param id идентификатор события
 * @return   количество байт, доступных для записи
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
size_t awh::engine::IO::available([[maybe_unused]] const event::id_t id) const noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return size_t();
}

/**
 * @brief Метод получения типа события
 *
 * @param id идентификатор события
 * @return   тип события
 *
 */
awh::event::type_t awh::engine::IO::type([[maybe_unused]] const event::id_t id) const noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Возвращаем тип сокета, либо признак отсутствия
	return ((item != nullptr) ? item->type : event::type_t::NONE);
}

/**
 * @brief Метод получения типа узла события
 *
 * @param id идентификатор события
 * @return   тип узла события
 *
 */
awh::event::node_t awh::engine::IO::node([[maybe_unused]] const event::id_t id) const noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Возвращаем вид узла, либо признак отсутствия
	return ((item != nullptr) ? item->node : event::node_t::NONE);
}

/**
 * @brief Метод получения семейства события
 *
 * @param id идентификатор события
 * @return   семейство адресов
 *
 */
awh::event::family_t awh::engine::IO::family([[maybe_unused]] const event::id_t id) const noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Возвращаем семейство событий, либо признак отсутствия
	return ((item != nullptr) ? item->family : event::family_t::NONE);
}

/**
 * @brief Метод получения статуса события
 *
 * @param id идентификатор события
 * @return   статус события
 *
 */
awh::event::status_t awh::engine::IO::status([[maybe_unused]] const event::id_t id) const noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Возвращаем состояние узла, либо признак отсутствия
	return ((item != nullptr) ? item->status : event::status_t::NONE);
}

/**
 * @brief Метод получения протокола события
 *
 * @param id идентификатор события
 * @return   протокол события
 *
 */
awh::event::protocol_t awh::engine::IO::protocol([[maybe_unused]] const event::id_t id) const noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Возвращаем протокол передачи данных, либо признак отсутствия
	return ((item != nullptr) ? item->protocol : event::protocol_t::NONE);
}

/**
 * @brief Метод опроса событий
 *
 * @details Выполняет **один оборот** цикла событий и возвращает
 *          управление. Своего потока движок не создаёт и сам себя не
 *          крутит - цикл ведёт вызывающий:
 *
 *          @code{.cpp}
 *          while(io.poll(100));
 *          @endcode
 *
 *          За один оборот выполняется: освобождение узлов,
 *          отложенных позапрошлым оборотом; отправка накопленного
 *          пакета изменений подписки вместе с ожиданием - одним
 *          обращением к ядру, а не двумя; разбор полученных событий с
 *          вызовом функций обратного вызова; разбор истёкших
 *          дедлайнов таймеров.
 *
 *          Все функции обратного вызова вызываются **внутри** этого
 *          метода. Пока они выполняются, оборот не завершён, поэтому
 *          долгая работа в обратном вызове задерживает и остальные
 *          события, и срабатывание таймеров.
 *
 * @note    Время ожидания ограничивается не только переданным
 *          таймаутом, но и сроком ближайшего внутреннего таймера:
 *          движок обязан проснуться к дедлайну, даже если вызывающий
 *          просил ждать дольше или бесконечно.
 *
 * @note    Отрицательный таймаут означает ожидание без предела, но с
 *          учётом таймеров; нулевой - опрос без ожидания вовсе,
 *          пригодный для встраивания в чужой цикл событий.
 *
 * @note    Отрицательный результат означает отказ опроса, а не
 *          отсутствие событий: оборот без единого события - это
 *          обычный успех. Прерывание системного вызова сигналом
 *          отказом не считается, движок продолжает работу.
 *
 * @note    Метод обязан вызываться из одного и того же потока. Первый
 *          вызов запоминает поток опроса, и обращения к событиям из
 *          других потоков после этого недопустимы
 *
 * @par Встраивание в чужой цикл событий
 * @code{.cpp}
 * // Опрос без ожидания: управление возвращается сразу
 * while(running){
 *     io.poll(0);
 *     foreignLoopIteration();
 * }
 * @endcode
 *
 * @param timeout таймаут опроса в миллисекундах: отрицательный - без
 *                предела, нулевой - без ожидания
 * @return        результат выполнения опроса
 *
 */
bool awh::engine::IO::poll(const int32_t timeout) noexcept {
	// Описатель порта завершения, с какого снимаются завершения
	HANDLE port = nullptr;
	{
		// Выполняем блокировку замка ожидания
		const std::lock_guard <std::mutex> lock(::__awh_wait_mutex__);
		// Запоминаем описатель порта завершения
		port = ::__awh_port__;
	}
	// Если порт завершения не заведён
	if(port == nullptr){
		// Заносим в журнал предупреждение о неготовности движка
		this->_log->print("%s: engine is not initialized", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__);
		// Сообщаем об отказе опроса
		return false;
	}
	/**
	 * Завершения снимаются с порта пачкой, а не поодиночке
	 *
	 * @details Снятие поодиночке стоило бы перехода в ядро на каждое из них. Пачкой
	 *          же берётся всё, что накопилось, одним переходом - при потоке событий
	 *          разница эта заметна, а при их отсутствии не стоит ничего
	 *
	 */
	OVERLAPPED_ENTRY entries[64];
	// Число снятых с порта завершений
	ULONG removed = 0;
	// Предел ожидания завершений в миллисекундах
	const DWORD limit = (timeout < 0 ? INFINITE : static_cast <DWORD> (timeout));
	// Если снять завершения с порта не удалось
	if(!::GetQueuedCompletionStatusEx(port, entries, static_cast <ULONG> (sizeof(entries) / sizeof(entries[0])), &removed, limit, FALSE)){
		// Получаем причину отказа снятия
		const DWORD code = ::GetLastError();
		/**
		 * Истечение отведённого срока отказом не считается
		 *
		 * @note Оборот без единого завершения - это обычный успех, а не отказ:
		 *       вызывающий волен опрашивать движок вовсе без ожидания
		 *
		 */
		if(code == WAIT_TIMEOUT)
			// Сообщаем, что обход петли выполнен
			return true;
		// Заносим в журнал предупреждение об отказе снятия завершений
		this->_log->print("%s: completion port wait failed, error %lu", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, code);
		// Сообщаем об отказе опроса
		return false;
	}
	// Список принятых сообщений, подлежащих раздаче
	std::deque <std::pair <event::id_t, std::vector <uint8_t>>> messages;
	/**
	 * Разбираем снятые с порта завершения
	 */
	for(ULONG i = 0; i < removed; i++){
		// Если завершение пробуждения петли - работы за ним нет
		if(entries[i].lpOverlapped == nullptr)
			// Переходим к следующему завершению
			continue;
		/**
		 * Приводим описатель наложенного обмена обратно к его описанию
		 *
		 * @note Приведение опирается на то, что поле обмена стоит в описании первым
		 *
		 */
		::Overlapped * context = reinterpret_cast <::Overlapped *> (entries[i].lpOverlapped);
		/**
		 * Определяем вид завершившегося обмена
		 */
		switch(static_cast <uint8_t> (context->operation)){
			// Если завершением принесено сообщение
			case static_cast <uint8_t> (::op_t::MESSAGE):
				// Откладываем сообщение до раздачи
				messages.emplace_back(context->id, ::std::move(context->buffer));
			break;
		}
		// Выполняем освобождение описания завершившегося обмена
		delete context;
	}
	/**
	 * Раздаём принятые сообщения
	 *
	 * @note Раздача ведётся уже без замка: обратный вызов вправе завести или
	 *       уничтожить событие, и удержание замка привело бы к его повторному захвату
	 *
	 */
	for(auto & message : messages){
		// Функция обратного вызова на чтение сообщений
		engine::callback::read_t callback = nullptr;
		{
			// Выполняем блокировку замка доступа к списку узлов
			const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
			// Выполняем поиск узла события
			::Node * item = ::__awh_find__(message.first);
			// Если узел найден и запущен
			if((item != nullptr) && (item->status == event::status_t::LAUNCHED))
				// Получаем функцию обратного вызова на чтение сообщений
				callback = item->read;
		}
		// Если функция обратного вызова на чтение установлена
		if(callback != nullptr)
			// Передаём принятое сообщение
			callback(message.first, message.second.data(), message.second.size());
	}
	// Сообщаем, что обход петли выполнен
	return true;
}

/**
 * @brief Метод установки функции обратного вызова для обработки события чтения
 *
 * @details Подписка на приём данных. Буфер, приходящий в функцию,
 *          принадлежит движку и действителен **только на время вызова**:
 *          он переиспользуется под следующее чтение, поэтому данные,
 *          нужные позже, следует скопировать.
 *
 * @par Общие правила для всех перегрузок `on()`
 * Все перегрузки устроены одинаково, и сказанное здесь относится к
 * каждой из них.
 *
 * Подписка выполняется **присваиванием**: повторный вызов с тем же типом
 * функции заменяет прежнюю, не добавляя вторую. Двух обработчиков одного
 * события одного вида быть не может, а передача пустой функции подписку
 * снимает.
 *
 * Подписываться можно в любой момент, а не только до `commit()`. В
 * частности, принятое подключение приходит уже заведённым событием, и
 * подписки ему выставляются прямо в функции приёма - как в примере к
 * описанию класса.
 *
 * Неизвестный идентификатор и событие, помеченное к уничтожению,
 * **игнорируются молча**: ни исключения, ни возвращаемого признака здесь
 * нет. Если же тип функции узлу не подходит - скажем, чтение для узла
 * сервера, - в лог уходит предупреждение, а подписка не выставляется.
 * Поэтому список поддерживаемых типов узлов указан у каждой перегрузки
 * отдельно, и сверяться с ним стоит: опечатка в типе узла тихо оставит
 * событие без обработчика.
 *
 * Вызывать `on()` изнутри функции обратного вызова безопасно, включая
 * замену обработчика на самого себя.
 *
 * @note Приведение через `static_cast` требуется там, где по одной лямбде
 *       перегрузку не выбрать однозначно. Так происходит с парой
 *       `write_t` и `connect_t`: их второй параметр - `size_t` и `bool`, а
 *       они приводятся друг к другу неявно. Перегрузки, различающиеся
 *       типами перечислений или типом возврата, выбираются сами, и
 *       приведения не требуют
 *
 * @par Поддерживаемые типы узлов
 * `FILE`, `NOTIFY`, `IPC`, `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::read_t cb) noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Если узел не найден — подключать обратную связь некому
	if(item == nullptr){
		// Заносим в журнал предупреждение об отсутствии узла события
		this->_log->print("%s: event %u not found, callback \"read\" is not connected", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
		// Выходим из метода
		return;
	}
	// Устанавливаем функцию обратного вызова
	item->read = cb;
}

/**
 * @brief Метод установки функции обратного вызова для обработки события записи
 *
 * @details Сообщает, сколько байт ушло в сокет. Полезно там, где скорость
 *          отправки нужно согласовать с источником данных: размер
 *          записанного - это и есть освободившееся место в очереди.
 *
 * @note Требует приведения через `static_cast`, иначе перегрузка
 *       неотличима от `connect_t`
 *
 * @par Поддерживаемые типы узлов
 * `FILE`, `NOTIFY`, `IPC`, `PEER`, `ORIGIN`, `CLIENT`, `SERVER`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::write_t cb) noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Если узел не найден — подключать обратную связь некому
	if(item == nullptr){
		// Заносим в журнал предупреждение об отсутствии узла события
		this->_log->print("%s: event %u not found, callback \"write\" is not connected", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
		// Выходим из метода
		return;
	}
	// Устанавливаем функцию обратного вызова
	item->write = cb;
}

/**
 * @brief Метод установки функции обратного вызова для обработки возврата неотправленных данных
 *
 * @details Срабатывает, когда отправить данные не удалось, и возвращает их
 *          вызывающей стороне: движок их не сохраняет и после выхода из
 *          функции освобождает. Решение о судьбе байтов - повторить
 *          позже, отложить в свой буфер или отбросить - остаётся за
 *          вызывающей стороной. Второй параметр говорит, откуда данные
 *          вернулись: из самого события или из его очереди отправки.
 *
 * @note Без этой подписки неотправленные данные теряются без следа.
 *       Событиям, где потеря недопустима, подписку следует выставлять
 *       наравне с чтением
 *
 * @par Поддерживаемые типы узлов
 * `FILE`, `IPC`, `PEER`, `ORIGIN`, `CLIENT`, `SERVER`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::spool_t cb) noexcept {
	/**
	 * Обратная связь, к виду узла неприложимая, тихо не подключается: движок заносит
	 * в журнал предупреждение и продолжает работу. Виды узлов, какие несёт временное
	 * ядро, обратной связи "spool" не имеют
	 */
	this->_log->print("%s: callback \"spool\" is not applicable to event %u", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
}

/**
 * @brief Метод установки функции обратного вызова для обработки общего события
 *
 * @details Единая точка наблюдения за происходящим с событием: в функцию
 *          приходит тип действия, а не его последствия. Нужна там, где
 *          важен сам факт - для журналирования, счётчиков, отладки - а
 *          разбирать данные незачем.
 *
 * @par Поддерживаемые типы узлов
 * `NOTIFY`, `DIR`, `FILE`, `IPC`, `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`,
 * `SERVER`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::event_t cb) noexcept {
	/**
	 * Обратная связь, к виду узла неприложимая, тихо не подключается: движок заносит
	 * в журнал предупреждение и продолжает работу. Виды узлов, какие несёт временное
	 * ядро, обратной связи "event" не имеют
	 */
	this->_log->print("%s: callback \"event\" is not applicable to event %u", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
}

/**
 * @brief Метод установки функции обратного вызова для обработки ошибки события
 *
 * @details Получает код ошибки и её текстовое описание. Подписка эта
 *          заодно **подавляет вывод ошибок в лог**: пока она не
 *          выставлена, движок печатает ошибки сам, а с ней - передаёт их
 *          целиком на усмотрение вызывающей стороны.
 *
 * @note Поддерживается почти всеми типами узлов, включая таймеры, и
 *       выставлять её стоит всегда: без неё причина отказа события
 *       остаётся только в логе
 *
 * @par Поддерживаемые типы узлов
 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::error_t cb) noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Если узел не найден — подключать обратную связь некому
	if(item == nullptr){
		// Заносим в журнал предупреждение об отсутствии узла события
		this->_log->print("%s: event %u not found, callback \"error\" is not connected", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
		// Выходим из метода
		return;
	}
	// Устанавливаем функцию обратного вызова
	item->error = cb;
}

/**
 * @brief Метод установки функции обратного вызова для обработки изменений события
 *
 * @details Наблюдение за файловой системой: сообщает, что именно
 *          произошло с файлом или каталогом, и с каким именно.
 *
 * @note Имя, приходящее в функцию, действительно ТОЛЬКО на время вызова
 *
 * @par Поддерживаемые типы узлов
 * `DIR`, `FILE`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::vnode_t cb) noexcept {
	/**
	 * Обратная связь, к виду узла неприложимая, тихо не подключается: движок заносит
	 * в журнал предупреждение и продолжает работу. Виды узлов, какие несёт временное
	 * ядро, обратной связи "vnode" не имеют
	 */
	this->_log->print("%s: callback \"vnode\" is not applicable to event %u", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
}

/**
 * @brief Метод установки функции обратного вызова инъекции объединённых данных (splice)
 *
 * @details Позволяет транспорту, шифрующему данные на уровне соединения,
 *          принять перенаправленные из события-источника байты и отправить
 *          их собственным потоком, а не записывать сырьём в сокет.
 *          Отрицательный результат означает отказ принять данные.
 *
 * @par Поддерживаемые типы узлов
 * `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::inject_t cb) noexcept {
	/**
	 * Обратная связь, к виду узла неприложимая, тихо не подключается: движок заносит
	 * в журнал предупреждение и продолжает работу. Виды узлов, какие несёт временное
	 * ядро, обратной связи "inject" не имеют
	 */
	this->_log->print("%s: callback \"inject\" is not applicable to event %u", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
}

/**
 * @brief Метод установки функции обратного вызова для обновления статуса события
 *
 * @details Сообщает о смене состояния события - подключено, отключено,
 *          отказ и так далее.
 *
 * @note Через эту же подписку сообщают о срабатывании **узлы-таймеры**:
 *       отдельной функции у них нет, и сработавший `TIMEOUT` или
 *       `INTERVAL` приходит сюда со статусом `event::status_t::SUCCESS`.
 *       Это единственный способ узнать о срабатывании таймера, и в
 *       функции статус следует проверять: приходят и остальные состояния
 *
 * @par Пример: срабатывание интервала
 * @code{.cpp}
 * const awh::event::id_t timer = io.event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
 * io.setTimeout(timer, awh::event::action_t::NONE, 5000);
 * io.on(timer, static_cast <awh::engine::callback::status_t> ([](const awh::event::id_t id, const awh::event::status_t status) noexcept -> void {
 *     // Интервал сработал, и сработает снова через те же пять секунд
 *     if(status == awh::event::status_t::SUCCESS)
 *         tick();
 * }));
 * io.commit(timer);
 * io.launch(timer);
 * @endcode
 *
 * @par Поддерживаемые типы узлов
 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::status_t cb) noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Если узел не найден — подключать обратную связь некому
	if(item == nullptr){
		// Заносим в журнал предупреждение об отсутствии узла события
		this->_log->print("%s: event %u not found, callback \"status\" is not connected", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
		// Выходим из метода
		return;
	}
	// Устанавливаем функцию обратного вызова
	item->state = cb;
}

/**
 * @brief Метод установки функции обратного вызова для приёма входящего подключения
 *
 * @details Принятое подключение приходит **уже заведённым событием**: его
 *          идентификатор передаётся вторым параметром, и заводить его
 *          через `event()`, настраивать и запускать не требуется. Всё, что
 *          нужно сделать в этой функции - выставить принятому событию
 *          подписки, иначе принимаемые им данные обрабатывать будет некому.
 *
 * @note Время жизни принятого события движку не принадлежит: закрывать его
 *       следует своим вызовом `destroy()`
 *
 * @par Поддерживаемые типы узлов
 * `SERVER`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::accept_t cb) noexcept {
	/**
	 * Обратная связь, к виду узла неприложимая, тихо не подключается: движок заносит
	 * в журнал предупреждение и продолжает работу. Виды узлов, какие несёт временное
	 * ядро, обратной связи "accept" не имеют
	 */
	this->_log->print("%s: callback \"accept\" is not applicable to event %u", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
}

/**
 * @brief Метод установки функции обратного вызова для определения сессии дейтаграммного пакета
 *
 * @note Поддерживается только серверными узлами. Установка функции
 *       переводит событие на маршрутизацию датаграмм по ключу
 *       приложения вместо адреса отправителя
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::origin_t cb) noexcept {
	/**
	 * Обратная связь, к виду узла неприложимая, тихо не подключается: движок заносит
	 * в журнал предупреждение и продолжает работу. Виды узлов, какие несёт временное
	 * ядро, обратной связи "origin" не имеют
	 */
	this->_log->print("%s: callback \"origin\" is not applicable to event %u", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
}

/**
 * @brief Метод установки функции обратного вызова на получение информационных метаданных о дейтаграммном пакете
 *
 * @details Сопутствующие сведения о датаграмме - откуда пришла, каким
 *          интерфейсом принята, что несёт в заголовках. Сами данные
 *          приходят обычной подпиской на чтение, а сюда попадает то, что в
 *          них не содержится.
 *
 * @par Поддерживаемые типы узлов
 * `CLIENT`, `SERVER`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::traffic_t cb) noexcept {
	/**
	 * Обратная связь, к виду узла неприложимая, тихо не подключается: движок заносит
	 * в журнал предупреждение и продолжает работу. Виды узлов, какие несёт временное
	 * ядро, обратной связи "traffic" не имеют
	 */
	this->_log->print("%s: callback \"traffic\" is not applicable to event %u", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
}

/**
 * @brief Метод установки функции обратного вызова для обработки подключения
 *
 * @details Сообщает об исходе попытки подключения: признак говорит,
 *          состоялось соединение или нет. До этого вызова отправлять данные
 *          некуда, поэтому первая отправка клиента обычно делается именно
 *          отсюда.
 *
 * @note Требует приведения через `static_cast`, иначе перегрузка
 *       неотличима от `write_t`
 *
 * @par Поддерживаемые типы узлов
 * `CLIENT`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::connect_t cb) noexcept {
	/**
	 * Обратная связь, к виду узла неприложимая, тихо не подключается: движок заносит
	 * в журнал предупреждение и продолжает работу. Виды узлов, какие несёт временное
	 * ядро, обратной связи "connect" не имеют
	 */
	this->_log->print("%s: callback \"connect\" is not applicable to event %u", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
}

/**
 * @brief Метод установки функции обратного вызова на получение информации о пакетах в туннельном интерфейсе
 *
 * @details Сведения о пакетах, прошедших через туннельный интерфейс, вместе
 *          с идентификатором удалённого узла, которому они принадлежат.
 *
 * @par Поддерживаемые типы узлов
 * `TUNNEL`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::tuninfo_t cb) noexcept {
	/**
	 * Обратная связь, к виду узла неприложимая, тихо не подключается: движок заносит
	 * в журнал предупреждение и продолжает работу. Виды узлов, какие несёт временное
	 * ядро, обратной связи "tuninfo" не имеют
	 */
	this->_log->print("%s: callback \"tuninfo\" is not applicable to event %u", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
}

/**
 * @brief Метод установки функции обратного вызова для обработки таймаута события
 *
 * @details Срабатывает, когда истёк срок, заданный через `setTimeout()`:
 *          соединение не приняло данных (`READ`), не смогло их отправить
 *          (`WRITE`), не установилось (`CONNECT`) или подошла пора повторить
 *          попытку (`RECONNECT`).
 *
 * @warning Смысл возвращаемого признака **зависит от действия**. Для
 *          `READ`, `WRITE` и `CONNECT` положительный признак узел
 *          уничтожает, а отрицательный оставляет жить. Для `RECONNECT` всё
 *          наоборот: положительный означает «переподключаться», а прервать
 *          попытку нужно отрицательным. Одна функция обслуживает все
 *          действия сразу, и различать эти случаи обязана она
 *
 * @note Если подписка не выставлена, узел по истечении срока `READ`,
 *       `WRITE` или `CONNECT` уничтожается **безусловно**. То есть она
 *       нужна ровно затем, чтобы обрыв предотвратить или обставить своими
 *       действиями
 *
 * @note По истечении срока `CONNECT` дополнительно вызывается подписка
 *       `connect_t` с отрицательным исходом, и происходит это **до** вызова
 *       этой функции
 *
 * @par Пример: разные действия - разный смысл ответа
 * @code{.cpp}
 * io.setTimeout(client, awh::event::action_t::READ, 30000);
 * io.setTimeout(client, awh::event::action_t::RECONNECT, 5000);
 * io.on(client, static_cast <awh::engine::callback::timeout_t> ([&attempts](const awh::event::id_t id, const awh::event::action_t action, const uint32_t delay) noexcept -> bool {
 *     // Переподключение: положительный ответ означает «пробовать снова»
 *     if(action == awh::event::action_t::RECONNECT)
 *         return (attempts++ < 3);
 *     // Простой: положительный ответ означает «рвать соединение»
 *     return true;
 * }));
 * @endcode
 *
 * @par Поддерживаемые типы узлов
 * `PEER`, `ORIGIN`, `CLIENT`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::timeout_t cb) noexcept {
	/**
	 * Обратная связь, к виду узла неприложимая, тихо не подключается: движок заносит
	 * в журнал предупреждение и продолжает работу. Виды узлов, какие несёт временное
	 * ядро, обратной связи "timeout" не имеют
	 */
	this->_log->print("%s: callback \"timeout\" is not applicable to event %u", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
}

/**
 * @brief Метод установки функции обратного вызова для обработки доступности очереди события
 *
 * @details Сообщает, что в очереди отправки освободилось место, и передаёт
 *          доступный размер. Это обратная связь для источника данных:
 *          отправлять следующую порцию имеет смысл отсюда, а не вслепую -
 *          так очередь не растёт без предела, а отправка идёт со скоростью,
 *          которую держит соединение.
 *
 * @par Поддерживаемые типы узлов
 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
 *
 * @param id идентификатор события
 * @param cb функция обратного вызова
 *
 */
void awh::engine::IO::on([[maybe_unused]] const event::id_t id, [[maybe_unused]] engine::callback::available_t cb) noexcept {
	// Выполняем блокировку замка доступа к списку узлов
	const std::lock_guard <std::recursive_mutex> lock(::__awh_mutex__);
	// Выполняем поиск узла события
	::Node * item = ::__awh_find__(id);
	// Если узел не найден — подключать обратную связь некому
	if(item == nullptr){
		// Заносим в журнал предупреждение об отсутствии узла события
		this->_log->print("%s: event %u not found, callback \"available\" is not connected", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, id);
		// Выходим из метода
		return;
	}
	// Устанавливаем функцию обратного вызова
	item->available = cb;
}

/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект работы с логами
 *
 */
awh::engine::IO::IO(const fmk_t * fmk, const log_t * log) noexcept :
 engine_t(fmk, log),
 whitelist(awh::event::control_list_t::WHITE, fmk, log),
 blacklist(awh::event::control_list_t::BLACK, fmk, log) {}

/**
 * @brief Деструктор
 *
 */
awh::engine::IO::~IO() noexcept {
	/**
	 * @todo IOCP: освобождение порта завершения ввода-вывода и всех заведённых событий
	 */
}

/**
 * @brief Метод очистки контрольного списка события
 *
 * @param id идентификатор события
 * @return   результат выполнения очистки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::Control_List::clear([[maybe_unused]] const event::id_t id) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод добавления адреса в контрольный список события
 *
 * @param id    идентификатор события
 * @param value значение адреса события
 * @return      результат выполнения установки
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::Control_List::add([[maybe_unused]] const event::id_t id, [[maybe_unused]] string_view value) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод удаления адреса из контрольного списка события
 *
 * @param id    идентификатор события
 * @param value адрес для удаления из контрольного списка
 * @return      результат выполнения удаления
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает отказом
 *
 */
bool awh::engine::IO::Control_List::remove([[maybe_unused]] const event::id_t id, [[maybe_unused]] string_view value) noexcept {
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return bool();
}

/**
 * @brief Метод получения контрольного списка события
 *
 * @param id идентификатор события
 * @return   контрольный список события
 *
 * @note Отвечает ссылкой на пустой список, живущий всё время работы процесса:
 *       договор метода требует ссылки, и вернуть ссылку на местную переменную
 *       нельзя
 *
 *
 * @todo IOCP: тела у метода ещё нет — отвечает пустым списком
 *
 */
const unordered_map <string, awh::event::address_t> & awh::engine::IO::Control_List::get([[maybe_unused]] const event::id_t id) const noexcept {
	// Пустой контрольный список, отдаваемый при отсутствии реализации
	static const unordered_map <string, event::address_t> result;
	// Заносим в журнал предупреждение об отсутствии реализации
	this->_log->print("%s: method \"%s\" is not implemented yet", log_t::flag_t::WARNING, ::__AWH_IO_BACKEND__, __FUNCTION__);
	// Возвращаем пустой результат
	return result;
}

/**
 * @brief Конструктор
 *
 * @param type тип контрольного списка
 * @param fmk  объект фреймворка
 * @param log  объект работы с логами
 *
 */
awh::engine::IO::Control_List::Control_List(const event::control_list_t type, const fmk_t * fmk, const log_t * log) noexcept :
 _addr(fmk, log), _type(type), _fmk(fmk), _log(log) {}

/**
 * @brief Деструктор
 *
 */
awh::engine::IO::Control_List::~Control_List() noexcept {}
