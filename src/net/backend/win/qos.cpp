/**
 * @file qos.cpp
 * @date 2026-08-08
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
 * @brief Реализация модуля отметки исходящих пакетов классом обслуживания
 *
 * @details Разбор устройства подсистемы и доводы к принятым решениям вынесены в
 *          заголовочный файл модуля, здесь же остаётся одно исполнение
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <mutex>
#include <unordered_map>

/**
 * Подключаем заголовочный файл модуля
 */
#include <net/backend/win/qos.hpp>

/**
 * @brief Средства подсистемы качества обслуживания
 *
 * @details Отсюда берутся объявления самой подсистемы. Сами вызовы связыванием не
 *          подключаются: библиотека их подключается по ходу работы, ибо машина вправе
 *          обойтись без неё вовсе
 *
 */
#include <qos2.h>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Название модуля для записей в журнале
 *
 */
static constexpr const char * __AWH_QOS_BACKEND__ = "MS Windows QoS backend";

/**
 * @brief Инкапсулируем состояние модуля в пространство имён
 *
 */
namespace {
	/**
	 * @brief Подписи вызовов подсистемы качества обслуживания
	 *
	 */
	typedef BOOL (WINAPI * qos_create_t)(PQOS_VERSION, PHANDLE);
	typedef BOOL (WINAPI * qos_close_t)(HANDLE);
	typedef BOOL (WINAPI * qos_add_t)(HANDLE, SOCKET, PSOCKADDR, QOS_TRAFFIC_TYPE, DWORD, PQOS_FLOWID);
	typedef BOOL (WINAPI * qos_remove_t)(HANDLE, SOCKET, QOS_FLOWID, DWORD);
	typedef BOOL (WINAPI * qos_set_t)(HANDLE, QOS_FLOWID, QOS_SET_FLOW, ULONG, PVOID, DWORD, LPOVERLAPPED);
	/**
	 * @brief Набор вызовов подсистемы качества обслуживания
	 *
	 */
	struct qwave_t {
		HMODULE dll;             // Описатель подключённой библиотеки
		HANDLE handle;           // Описатель работы с подсистемой
		qos_create_t create;     // Открытие работы с подсистемой
		qos_close_t close;       // Закрытие работы с подсистемой
		qos_add_t add;           // Заведение потока на сокете
		qos_remove_t remove;     // Снятие потока с сокета
		qos_set_t set;           // Правка настроек потока
		/**
		 * @brief Конструктор
		 *
		 */
		qwave_t() noexcept :
		 dll(nullptr), handle(nullptr), create(nullptr),
		 close(nullptr), add(nullptr), remove(nullptr), set(nullptr) {}
	};
	/**
	 * @brief Запись реестра отмеченных сокетов
	 *
	 * @details Запись эта заводится и намерением, и свершившейся отметкой: пустой
	 *          номер потока означает, что отметка запомнена, но ещё не применена -
	 *          назначение сокета к тому времени не было известно
	 *
	 */
	struct entry_t {
		awh::event::dscp_t dscp;   // Запрошенный класс обслуживания
		QOS_FLOWID flow;           // Номер заведённого потока
		/**
		 * @brief Конструктор
		 *
		 */
		entry_t() noexcept : dscp(awh::event::dscp_t::CS0), flow(0) {}
	};
	// Замок, оберегающий реестр отмеченных сокетов
	static std::mutex __awh_mutex__;
	// Реестр отмеченных сокетов
	static std::unordered_map <awh::net::socket_t, entry_t> __awh_registry__;
	/**
	 * @brief Функция открытия работы с подсистемой качества обслуживания
	 *
	 * @details Открывается она единожды за время работы и остаётся открытой: закрытие
	 *          её сняло бы разом все заведённые потоки
	 *
	 * @param log объект ведения журнала
	 * @return    набор вызовов подсистемы либо пустое значение при отказе
	 *
	 */
	static const qwave_t * __awh_qwave__(const awh::log_t * log) noexcept {
		// Набор вызовов подсистемы
		static qwave_t result;
		// Признак уже выполненной попытки открытия
		static bool attempted = false;
		// Если попытка открытия уже выполнялась
		if(attempted)
			// Выводим набор вызовов, если открытие удалось
			return (result.handle != nullptr ? &result : nullptr);
		// Запоминаем выполнение попытки открытия
		attempted = true;
		// Выполняем подключение библиотеки подсистемы
		result.dll = ::LoadLibraryExW(L"qwave.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		// Если подключить библиотеку подсистемы не удалось
		if(result.dll == nullptr){
			// Если объект ведения журнала передан
			if(log != nullptr)
				// Выводим в журнал сообщение о невозможности подключения
				log->print("%s: qwave.dll could not be loaded, error %lu", awh::log_t::flag_t::WARNING, ::__AWH_QOS_BACKEND__, ::GetLastError());
			// Выводим пустое значение
			return nullptr;
		}
		/**
		 * @brief Снятие вызова из подключённой библиотеки
		 *
		 */
		#define __AWH_BIND__(field, type, symbol) \
			result.field = reinterpret_cast <type> (reinterpret_cast <void *> (::GetProcAddress(result.dll, symbol)))
		// Выполняем снятие вызовов подсистемы
		__AWH_BIND__(create, qos_create_t, "QOSCreateHandle");
		__AWH_BIND__(close, qos_close_t, "QOSCloseHandle");
		__AWH_BIND__(add, qos_add_t, "QOSAddSocketToFlow");
		__AWH_BIND__(remove, qos_remove_t, "QOSRemoveSocketFromFlow");
		__AWH_BIND__(set, qos_set_t, "QOSSetFlow");
		// Снимаем объявление снятия вызовов
		#undef __AWH_BIND__
		// Если хотя бы один вызов подсистемы снять не удалось
		if((result.create == nullptr) || (result.close == nullptr) || (result.add == nullptr) ||
		   (result.remove == nullptr) || (result.set == nullptr)){
			// Если объект ведения журнала передан
			if(log != nullptr)
				// Выводим в журнал сообщение о несовпадении состава библиотеки
				log->print("%s: qwave.dll does not export the expected entry points", awh::log_t::flag_t::WARNING, ::__AWH_QOS_BACKEND__);
			// Выполняем отключение библиотеки подсистемы
			::FreeLibrary(result.dll);
			// Сбрасываем описатель подключённой библиотеки
			result.dll = nullptr;
			// Выводим пустое значение
			return nullptr;
		}
		// Издание подсистемы, с каким ведётся работа
		QOS_VERSION version{};
		// Устанавливаем старший номер издания
		version.MajorVersion = 1;
		// Устанавливаем младший номер издания
		version.MinorVersion = 0;
		// Если открыть работу с подсистемой не удалось
		if(!result.create(&version, &result.handle)){
			// Если объект ведения журнала передан
			if(log != nullptr)
				// Выводим в журнал сообщение о невозможности открытия
				log->print("%s: QoS subsystem could not be opened, error %lu", awh::log_t::flag_t::WARNING, ::__AWH_QOS_BACKEND__, ::GetLastError());
			// Сбрасываем описатель работы с подсистемой
			result.handle = nullptr;
			// Выводим пустое значение
			return nullptr;
		}
		// Выводим набор вызовов подсистемы
		return &result;
	}
	/**
	 * @brief Функция заведения потока на сокете с выставлением отметки
	 *
	 * @param sock отмечаемый сокет
	 * @param dscp устанавливаемый класс обслуживания
	 * @param flow номер заведённого потока
	 * @param log  объект ведения журнала
	 * @return     результат выполнения заведения
	 *
	 */
	static bool __awh_flow__(const awh::net::socket_t sock, const awh::event::dscp_t dscp, QOS_FLOWID & flow, const awh::log_t * log) noexcept {
		// Выполняем открытие работы с подсистемой
		const qwave_t * qwave = ::__awh_qwave__(log);
		// Если открыть работу с подсистемой не удалось
		if(qwave == nullptr)
			// Выводим отрицательный результат заведения
			return false;
		// Сбрасываем номер заводимого потока
		flow = 0;
		/**
		 * Заводим поток на сокете
		 *
		 * @note Назначение здесь не передаётся вовсе: сокет к этому времени обязан
		 *       быть соединён, и подсистема берёт назначение у него самого. Поток
		 *       заводится неприспосабливающимся - подсистема не вправе снижать
		 *       отметку по своему усмотрению, отметку задаёт вызывающий
		 *
		 */
		if(!qwave->add(qwave->handle, static_cast <SOCKET> (sock), nullptr, QOSTrafficTypeBestEffort, QOS_NON_ADAPTIVE_FLOW, &flow)){
			// Если объект ведения журнала передан
			if(log != nullptr)
				// Выводим в журнал сообщение о невозможности заведения потока
				log->print("%s: QoS flow could not be created, error %lu", awh::log_t::flag_t::WARNING, ::__AWH_QOS_BACKEND__, ::GetLastError());
			// Выводим отрицательный результат заведения
			return false;
		}
		// Устанавливаемое значение класса обслуживания
		DWORD value = static_cast <DWORD> (dscp);
		// Если выставить отметку потоку не удалось
		if(!qwave->set(qwave->handle, flow, QOSSetOutgoingDSCPValue, static_cast <ULONG> (sizeof(value)), &value, 0, nullptr)){
			// Если объект ведения журнала передан
			if(log != nullptr)
				// Выводим в журнал сообщение о невозможности выставления отметки
				log->print("%s: DSCP value could not be applied to the flow, error %lu", awh::log_t::flag_t::WARNING, ::__AWH_QOS_BACKEND__, ::GetLastError());
			// Выполняем снятие заведённого потока
			qwave->remove(qwave->handle, static_cast <SOCKET> (sock), flow, 0);
			// Сбрасываем номер заведённого потока
			flow = 0;
			// Выводим отрицательный результат заведения
			return false;
		}
		// Выводим положительный результат заведения
		return true;
	}
};

/**
 * @brief Функция установки класса обслуживания сокету
 *
 * @param sock отмечаемый сокет
 * @param dscp устанавливаемый класс обслуживания
 * @param log  объект ведения журнала
 * @return     результат выполнения установки
 *
 * @note Отказ заведения потока за отказ установки не считается: назначение сокета к
 *       этому времени может быть ещё неизвестно, и отметка тогда запоминается до
 *       применения. Отказ применения станет виден при нём самом
 *
 */
bool awh::win::qos::mark(const net::socket_t sock, const event::dscp_t dscp, const log_t * log) noexcept {
	// Если сокет не передан
	if(sock == net::invalid_socket_t)
		// Выводим отрицательный результат установки
		return false;
	// Заводимая запись реестра отмеченных сокетов
	entry_t entry;
	// Запоминаем запрошенный класс обслуживания
	entry.dscp = dscp;
	{
		// Выполняем блокировку реестра отмеченных сокетов
		const std::lock_guard <std::mutex> lock(::__awh_mutex__);
		// Выполняем поиск сокета в реестре
		auto i = ::__awh_registry__.find(sock);
		// Если сокет в реестре уже значится
		if(i != ::__awh_registry__.end()){
			// Запоминаем номер уже заведённого потока
			entry.flow = i->second.flow;
			// Если поток на сокете уже заведён
			if(entry.flow != 0){
				// Выполняем открытие работы с подсистемой
				const qwave_t * qwave = ::__awh_qwave__(log);
				// Если работа с подсистемой открыта
				if(qwave != nullptr){
					// Устанавливаемое значение класса обслуживания
					DWORD value = static_cast <DWORD> (dscp);
					// Выполняем правку отметки уже заведённого потока
					if(qwave->set(qwave->handle, entry.flow, QOSSetOutgoingDSCPValue, static_cast <ULONG> (sizeof(value)), &value, 0, nullptr)){
						// Запоминаем запрошенный класс обслуживания
						i->second.dscp = dscp;
						// Выводим положительный результат установки
						return true;
					}
				}
				// Выводим отрицательный результат установки
				return false;
			}
		}
	}
	// Номер заводимого потока
	QOS_FLOWID flow = 0;
	/**
	 * Заводим поток на сокете, не считая отказ за отказ установки
	 *
	 * @note Сокет к этому времени вправе быть ещё не соединён, и назначения у него
	 *       нет. Отметка тогда остаётся запомненной и будет применена применением
	 *
	 */
	::__awh_flow__(sock, dscp, flow, nullptr);
	// Запоминаем номер заведённого потока
	entry.flow = flow;
	// Выполняем блокировку реестра отмеченных сокетов
	const std::lock_guard <std::mutex> lock(::__awh_mutex__);
	// Выполняем занесение сокета в реестр
	::__awh_registry__[sock] = entry;
	// Выводим положительный результат установки
	return true;
}
/**
 * @brief Функция получения запрошенного класса обслуживания
 *
 * @param sock опрашиваемый сокет
 * @return     запрошенный класс обслуживания
 *
 * @warning Отдаётся здесь запрошенное, а не показание системы: обратного чтения
 *          отметки подсистема не даёт вовсе
 *
 */
awh::event::dscp_t awh::win::qos::mark(const net::socket_t sock) noexcept {
	// Выполняем блокировку реестра отмеченных сокетов
	const std::lock_guard <std::mutex> lock(::__awh_mutex__);
	// Выполняем поиск сокета в реестре
	auto i = ::__awh_registry__.find(sock);
	// Выводим запрошенный класс обслуживания
	return (i != ::__awh_registry__.end() ? i->second.dscp : event::dscp_t::CS0);
}
/**
 * @brief Функция применения запомненной отметки к соединённому сокету
 *
 * @param sock применяемый сокет
 * @param log  объект ведения журнала
 * @return     результат выполнения применения
 *
 */
bool awh::win::qos::apply(const net::socket_t sock, const log_t * log) noexcept {
	// Запрошенный класс обслуживания
	event::dscp_t dscp = event::dscp_t::CS0;
	{
		// Выполняем блокировку реестра отмеченных сокетов
		const std::lock_guard <std::mutex> lock(::__awh_mutex__);
		// Выполняем поиск сокета в реестре
		auto i = ::__awh_registry__.find(sock);
		// Если сокет в реестре не значится либо поток на нём уже заведён
		if((i == ::__awh_registry__.end()) || (i->second.flow != 0))
			// Выводим отрицательный результат применения
			return false;
		// Запоминаем запрошенный класс обслуживания
		dscp = i->second.dscp;
	}
	// Номер заводимого потока
	QOS_FLOWID flow = 0;
	// Если завести поток на сокете не удалось
	if(!::__awh_flow__(sock, dscp, flow, log))
		// Выводим отрицательный результат применения
		return false;
	// Выполняем блокировку реестра отмеченных сокетов
	const std::lock_guard <std::mutex> lock(::__awh_mutex__);
	// Выполняем поиск сокета в реестре
	auto i = ::__awh_registry__.find(sock);
	// Если сокет в реестре значится
	if(i != ::__awh_registry__.end())
		// Запоминаем номер заведённого потока
		i->second.flow = flow;
	// Выводим положительный результат применения
	return true;
}
/**
 * @brief Функция снятия потока с сокета
 *
 * @param sock освобождаемый сокет
 * @return     результат выполнения снятия
 *
 */
bool awh::win::qos::release(const net::socket_t sock) noexcept {
	// Номер снимаемого потока
	QOS_FLOWID flow = 0;
	{
		// Выполняем блокировку реестра отмеченных сокетов
		const std::lock_guard <std::mutex> lock(::__awh_mutex__);
		// Выполняем поиск сокета в реестре
		auto i = ::__awh_registry__.find(sock);
		// Если сокет в реестре не значится
		if(i == ::__awh_registry__.end())
			// Выводим отрицательный результат снятия
			return false;
		// Запоминаем номер снимаемого потока
		flow = i->second.flow;
		// Выполняем изъятие сокета из реестра
		::__awh_registry__.erase(i);
	}
	// Если поток на сокете заведён не был
	if(flow == 0)
		// Выводим положительный результат снятия
		return true;
	// Выполняем открытие работы с подсистемой
	const qwave_t * qwave = ::__awh_qwave__(nullptr);
	// Если работа с подсистемой не открыта
	if(qwave == nullptr)
		// Выводим отрицательный результат снятия
		return false;
	// Выводим результат снятия потока с сокета
	return static_cast <bool> (qwave->remove(qwave->handle, static_cast <SOCKET> (sock), flow, 0));
}
